//
// Created by August Pemberton on 03/02/2025.
//

#pragma once
#include "UIAttachment.h"

namespace imagiro {

    class ModMatrixAttachment : public UIAttachment, ModMatrix::Listener, juce::Timer {
    private:
        // Cached choc::value objects for reuse
        mutable choc::value::Value cachedSourcesValue;
        mutable choc::value::Value cachedTargetsValue;
        mutable choc::value::Value cachedEmptyValue;
        mutable bool sourcesValueInitialized = false;
        mutable bool targetsValueInitialized = false;
        mutable bool emptyValueInitialized = false;

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
            sendMatrixUpdateFlagAfterNextDataLoad = true;
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

                // Reuse cached empty value
                return getCachedEmptyValue();
            });

            connection.bind("juce_removeModulation", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto sourceID = args[0].getWithDefault("");
                auto targetID = args[1].getWithDefault("");

                modMatrix.removeConnection(sourceID, targetID);

                // Reuse cached empty value
                return getCachedEmptyValue();
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
            bool sourceValuesUpdated = false;
            bool targetValuesUpdated = false;

            while (matrixFifo.try_dequeue(matrixMessageThread)) { /**/ }
            while (sourceValuesFifo.try_dequeue(sourceValues)) {
                sourceValuesUpdated = true;
            }
            while (targetValuesFifo.try_dequeue(targetValues)) {
                targetValuesUpdated = true;
            }

            // Update cached values in-place if data changed
            if (sourceValuesUpdated && sourcesValueInitialized) {
                updateCachedSourceValues();
            }
            if (targetValuesUpdated && targetsValueInitialized) {
                updateCachedTargetValues();
            }

            lastUIUpdate = lastAudioUpdate.load();

            if (sendMatrixUpdateFlag) {
                auto matrixValue = matrixMessageThread.getState();
                connection.eval("window.ui.modMatrixUpdated", {matrixValue});
                sendMatrixUpdateFlag = false;
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

        std::unordered_map<SourceID, ModMatrix::SourceValue> sourceValues;
        moodycamel::ReaderWriterQueue<std::unordered_map<SourceID, ModMatrix::SourceValue>> sourceValuesFifo {128};

        std::unordered_map<TargetID, ModMatrix::TargetValue> targetValues;
        moodycamel::ReaderWriterQueue<std::unordered_map<TargetID, ModMatrix::TargetValue>> targetValuesFifo {128};

        // Helper methods for cached value management
        choc::value::Value& getCachedEmptyValue() const {
            if (!emptyValueInitialized) {
                cachedEmptyValue = choc::value::Value();
                emptyValueInitialized = true;
            }
            return cachedEmptyValue;
        }

        choc::value::Value& getCachedSourceValues() const {
            if (!sourcesValueInitialized) {
                cachedSourcesValue = choc::value::createObject("");

                for (const auto& [sourceID, source] : sourceValues) {
                    cachedSourcesValue.addMember(sourceID, source.getState());
                }
                cachedSourcesValue.addMember("time", lastUIUpdate.load());
                sourcesValueInitialized = true;
            }
            return cachedSourcesValue;
        }

        choc::value::Value& getCachedTargetValues() const {
            if (!targetsValueInitialized) {
                cachedTargetsValue = choc::value::createObject("");

                for (const auto& [targetID, target] : targetValues) {
                    const auto v = target.getState();
                    cachedTargetsValue.addMember(targetID, v);
                }
                cachedTargetsValue.addMember("time", lastUIUpdate.load());
                targetsValueInitialized = true;
            }
            return cachedTargetsValue;
        }

        void updateCachedSourceValues() {
            // Update existing members in-place
            for (const auto& [sourceID, source] : sourceValues) {
                if (cachedSourcesValue.hasObjectMember(sourceID)) {
                    cachedSourcesValue.setMember(sourceID, source.getState());
                } else {
                    cachedSourcesValue.addMember(sourceID, source.getState());
                }
            }
            cachedSourcesValue.setMember("time", lastUIUpdate.load());
        }

        void updateCachedTargetValues() {
            // Update existing members in-place
            for (const auto& [targetID, target] : targetValues) {
                const auto v = target.getState();
                if (cachedTargetsValue.hasObjectMember(targetID)) {
                    cachedTargetsValue.setMember(targetID, v);
                } else {
                    cachedTargetsValue.addMember(targetID, v);
                }
            }
            cachedTargetsValue.setMember("time", lastUIUpdate.load());
        }
    };
}