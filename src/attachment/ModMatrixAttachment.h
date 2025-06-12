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
            startTimerHz(120);
        }

        ~ModMatrixAttachment() override {
            modMatrix.removeListener(this);
        }

        void OnMatrixUpdated() override {
            sourcesValueInitialized = false;
            targetsValueInitialized = false;

            const auto& matrix = modMatrix.getSerializedMatrix();
            for (const auto& entry : matrix) {
                matrixFifo.enqueue(entry);
            }

            matrixUpdatedFlag = true;
        }

        void OnTargetValueUpdated(const TargetID &targetID) override {
            targetValuesUpdatedFlag = true;

            targetValuesFifo.try_enqueue({
                targetID, modMatrix.getTargetValues()[targetID]
            });
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
        // TODO: this is not realtime safe!
        // currently copying unordered maps and vectors lol
        void processCallback() override {
            sourceValuesUpdatedFlag = true;
            const auto& sourceValues = modMatrix.getSourceValues();
            for (const auto& [sourceID, source]: sourceValues) {
                sourceValuesFifo.try_enqueue({sourceID, source});
            }

            lastAudioUpdate = juce::Time::getMillisecondCounterHiRes();
        }

        void timerCallback() override {
            const auto mostRecentVoiceIndex = modMatrix.getMostRecentVoiceIndex();

            if (matrixUpdatedFlag) {
                matrixMessageThread.clearQuick();
                SerializedMatrixEntry entry;

                while (matrixFifo.try_dequeue(entry)) {
                    matrixMessageThread.addIfNotAlreadyThere(entry);
                }

                auto matrixValue = matrixMessageThread.getState();
                connection.eval("window.ui.modMatrixUpdated", {matrixValue});
                matrixUpdatedFlag = false;
            }

            sourceChocValues = choc::value::createObject("SourceValues");
            targetChocValues = choc::value::createObject("TargetValues");

            if (sourceValuesUpdatedFlag) {
                std::pair<SourceID, std::shared_ptr<ModMatrix::SourceValue> > updatedSource;
                while (sourceValuesFifo.try_dequeue(updatedSource)) {
                    if (!sourceValues.contains(updatedSource.first)) {
                        sourceValues.insert({updatedSource});
                    } else {
                        sourceValues[updatedSource.first] = updatedSource.second;
                    }

                    const auto val = updatedSource.second->value.getGlobalValue()
                                     + updatedSource.second->value.getVoiceValue(mostRecentVoiceIndex);
                    sourceChocValues.setMember(updatedSource.first, val);
                }

                connection.eval("window.ui.sourceValuesUpdated", {
                                    sourceChocValues, choc::value::Value(lastUIUpdate.load())
                                });
            }

            if (targetValuesUpdatedFlag) {
                std::pair<TargetID, std::shared_ptr<ModMatrix::TargetValue> > updatedTarget;
                while (targetValuesFifo.try_dequeue(updatedTarget)) {
                    if (!targetValues.contains(updatedTarget.first)) {
                        targetValues.insert({updatedTarget});
                    } else {
                        targetValues[updatedTarget.first] = updatedTarget.second;
                    }

                    const auto val = updatedTarget.second->value.getGlobalValue()
                                     + updatedTarget.second->value.getVoiceValue(mostRecentVoiceIndex);
                    targetChocValues.setMember(updatedTarget.first, val);
                }

                connection.eval("window.ui.targetValuesUpdated", {
                                    targetChocValues, choc::value::Value(lastAudioUpdate.load())
                                });
            }

            lastUIUpdate = lastAudioUpdate.load();
        }

        void OnRecentVoiceUpdated(size_t voiceIndex) override {
            connection.eval("window.ui.onRecentVoiceUpdated", {choc::value::Value((int)voiceIndex)});
        }

    private:
        std::atomic<double> lastAudioUpdate;
        std::atomic<double> lastUIUpdate;

        ModMatrix& modMatrix;
        // ModMatrix::MatrixType matrixMessageThread;

        SerializedMatrix matrixMessageThread {};
        moodycamel::ReaderWriterQueue<SerializedMatrixEntry> matrixFifo {128};

        std::atomic<bool> matrixUpdatedFlag {false};

        choc::value::Value sourceChocValues;
        choc::value::Value targetChocValues;

        std::atomic<bool> sourceValuesUpdatedFlag {false};
        std::unordered_map<SourceID, std::shared_ptr<ModMatrix::SourceValue>> sourceValues;
        moodycamel::ReaderWriterQueue<std::pair<SourceID, std::shared_ptr<ModMatrix::SourceValue>>> sourceValuesFifo {128};

        std::atomic<bool> targetValuesUpdatedFlag {false};
        std::unordered_map<TargetID, std::shared_ptr<ModMatrix::TargetValue>> targetValues;
        moodycamel::ReaderWriterQueue<std::pair<TargetID, std::shared_ptr<ModMatrix::TargetValue>>> targetValuesFifo {MAX_MOD_TARGETS};

        juce::VBlankAttachment vBlank;

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