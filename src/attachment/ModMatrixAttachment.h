//
// Created by August Pemberton on 03/02/2025.
//

#pragma once
#include "UIAttachment.h"

namespace imagiro {

    class ModMatrixAttachment : public UIAttachment, ModMatrix::Listener, juce::Timer {
    private:
        // Cached choc::value objects for reuse
        choc::value::Value cachedSourcesValue;
        choc::value::Value cachedTargetsValue;
        bool sourcesValueInitialized = false;
        bool targetsValueInitialized = false;
        bool emptyValueInitialized = false;

    public:
        ModMatrixAttachment(UIConnection& connection, ModMatrix& matrix)
                : UIAttachment(connection), modMatrix(matrix)
        {
            modMatrix.addListener(this);
            startTimerHz(60);
        }

        ~ModMatrixAttachment() override {
            modMatrix.removeListener(this);
        }

        void OnMatrixUpdated() override {
            sendMatrixUpdateFlagAfterNextDataLoad = true;
            sourcesValueInitialized = false;
            targetsValueInitialized = false;
        }

        void addBindings() override {
            connection.bind("juce_updateModulation", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto sourceID = args[0].getWithDefault("");
                auto targetID = args[1].getWithDefault("");
                auto depth = args[2].getWithDefault(0.f);

                auto attackMS = args.size() > 3 ? args[3].getWithDefault(0.f) : 0.f;
                auto releaseMS = args.size() > 4 ? args[4].getWithDefault(0.f) : 0.f;

                auto bipolar = args.size() > 5 ? args[5].getWithDefault(false) : false;

                modMatrix.setConnection(sourceID, targetID, {depth, attackMS, releaseMS, bipolar});

                return {};
            });

            connection.bind("juce_removeModulation", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto sourceID = args[0].getWithDefault("");
                auto targetID = args[1].getWithDefault("");

                modMatrix.removeConnection(sourceID, targetID);

                return {};
            });

            connection.bind("juce_getModMatrix", [&](const choc::value::ValueView& args) -> choc::value::Value {
                return matrixMessageThread.getState();
            });

            connection.bind("juce_getSourceValues", [&](const choc::value::ValueView& args) -> choc::value::Value {
                return getCachedSourceValues();
            });

            connection.bind("juce_getTargetValues", [&](const choc::value::ValueView& args) -> choc::value::Value {
                return getCachedTargetValues();
            });
        }

        // call each processBlock tick
        void processCallback() override {
            matrixFifo.enqueue(modMatrix.getSerializedMatrix());
            if (sendMatrixUpdateFlagAfterNextDataLoad) {
                sendMatrixUpdateFlagAfterNextDataLoad = false;
                sendMatrixUpdateFlag = true;
            }

            sourceValuesFifo.enqueue(modMatrix.getSourceValues());
            targetValuesFifo.enqueue(modMatrix.getTargetValues());

            lastAudioUpdate = juce::Time::getMillisecondCounterHiRes();
        }

        void timerCallback() override {
            const auto mostRecentVoiceIndex = modMatrix.getMostRecentVoiceIndex();

            while (matrixFifo.try_dequeue(matrixMessageThread)) { /**/ }

            sourceChocValues = choc::value::createObject("SourceValues");
            targetChocValues = choc::value::createObject("TargetValues");

            bool sourcesUpdated = false;
            bool targetsUpdated = false;

            while (sourceValuesFifo.try_dequeue(sourceValues)) { sourcesUpdated = true; }
            if (sourcesUpdated) {
                for (auto &[id, sourceValue]: sourceValues) {
                    const auto val = sourceValue->value.getGlobalValue() + sourceValue->value.getVoiceValue(mostRecentVoiceIndex);
                    sourceChocValues.setMember(id, val);
                }
            }

            while (targetValuesFifo.try_dequeue(targetValues)) { targetsUpdated = true; }
            if (targetsUpdated) {
                for (auto &[id, targetValue]: targetValues) {
                    const auto val = targetValue->value.getGlobalValue() + targetValue->value.getVoiceValue(mostRecentVoiceIndex);
                    targetChocValues.setMember(id, val);
                }
            }

            lastUIUpdate = lastAudioUpdate.load();

            if (sendMatrixUpdateFlag) {
                auto matrixValue = matrixMessageThread.getState();
                connection.eval("window.ui.modMatrixUpdated", {matrixValue});
                sendMatrixUpdateFlag = false;
            }

            if (sourcesUpdated) {
               connection.eval("window.ui.sourceValuesUpdated", {sourceChocValues, choc::value::Value(lastUIUpdate.load())});
            }

            if (targetsUpdated) {
               connection.eval("window.ui.targetValuesUpdated", {targetChocValues, choc::value::Value(lastAudioUpdate.load())});
            }
        }

        void OnRecentVoiceUpdated(size_t voiceIndex) override {
            connection.eval("window.ui.onRecentVoiceUpdated", {choc::value::Value((int)voiceIndex)});
        }

    private:
        std::atomic<double> lastAudioUpdate;
        std::atomic<double> lastUIUpdate;

        ModMatrix& modMatrix;

        SerializedMatrix matrixMessageThread {};
        moodycamel::ReaderWriterQueue<SerializedMatrix> matrixFifo {128};

        std::atomic<bool> sendMatrixUpdateFlag {false};
        std::atomic<bool> sendMatrixUpdateFlagAfterNextDataLoad {false};

        choc::value::Value sourceChocValues;
        choc::value::Value targetChocValues;

        std::unordered_map<SourceID, std::shared_ptr<ModMatrix::SourceValue>> sourceValues;
        moodycamel::ReaderWriterQueue<std::unordered_map<SourceID, std::shared_ptr<ModMatrix::SourceValue>>> sourceValuesFifo {128};

        std::unordered_map<TargetID, std::shared_ptr<ModMatrix::TargetValue>> targetValues;
        moodycamel::ReaderWriterQueue<std::unordered_map<TargetID, std::shared_ptr<ModMatrix::TargetValue>>> targetValuesFifo {128};

        choc::value::Value& getCachedSourceValues() {
            if (!sourcesValueInitialized) {
                cachedSourcesValue = choc::value::createObject("");

                for (const auto& [sourceID, source] : sourceValues) {
                    cachedSourcesValue.addMember(sourceID, source->getState());
                }
                cachedSourcesValue.addMember("time", lastUIUpdate.load());
                sourcesValueInitialized = true;
            }
            return cachedSourcesValue;
        }

        choc::value::Value& getCachedTargetValues() {
            if (!targetsValueInitialized) {
                cachedTargetsValue = choc::value::createObject("");

                for (const auto& [targetID, target] : targetValues) {
                    const auto v = target->getState();
                    cachedTargetsValue.addMember(targetID, v);
                }
                cachedTargetsValue.addMember("time", lastUIUpdate.load());
                targetsValueInitialized = true;
            }
            return cachedTargetsValue;
        }
    };
}