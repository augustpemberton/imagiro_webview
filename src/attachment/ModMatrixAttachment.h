//
// Created by August Pemberton on 03/02/2025.
//

#pragma once
#include "UIAttachment.h"

namespace imagiro {

    class ModMatrixAttachment : public UIAttachment, ModMatrix::Listener, juce::Timer {
    private:
        // Cached choc::value objects for reuse
        choc::value::Value cachedSourceDefs;
        choc::value::Value cachedTargetsValue;
        bool sourceDefNeedsRefresh = true;
        bool sourceDefRefreshReady = false;
        bool targetDefNeedsRefresh = true;
        bool targetDefRefreshReady = false;

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
            sourceDefNeedsRefresh = true;
            targetDefNeedsRefresh = true;

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

        void OnSourceValueUpdated(const SourceID& sourceID) override {
            const auto& sourceValue = modMatrix.getSourceValues().at(sourceID);
            sourceValuesFifo.try_enqueue({sourceID, sourceValue});
            sourceValuesUpdatedFlag = true;
        }

        void addBindings() override {
            connection.bind("juce_updateModulation", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto sourceID = args[0].getWithDefault(0);
                auto targetID = args[1].getWithDefault(0);
                auto depth = args[2].getWithDefault(0.f);

                auto attackMS = args.size() > 3 ? args[3].getWithDefault(0.f) : 0.f;
                auto releaseMS = args.size() > 4 ? args[4].getWithDefault(0.f) : 0.f;

                auto bipolar = args.size() > 5 ? args[5].getWithDefault(false) : false;

                modMatrix.setConnection(sourceID, targetID, {depth, attackMS, releaseMS, bipolar});

                return {};
            });

            connection.bind("juce_removeModulation", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto sourceID = args[0].getWithDefault(0);
                auto targetID = args[1].getWithDefault(0);

                modMatrix.removeConnection(sourceID, targetID);

                return {};
            });

            connection.bind("juce_getModMatrix", [&](const choc::value::ValueView& args) -> choc::value::Value {
                return matrixMessageThread.getState();
            });

            connection.bind("juce_getSourceValues", [&](const choc::value::ValueView& args) -> choc::value::Value {
                return getCachedSourceDefs();
            });

            connection.bind("juce_getTargetValues", [&](const choc::value::ValueView& args) -> choc::value::Value {
                return getCachedTargetValues();
            });
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
                sourceValuesUpdatedFlag = false;
                std::pair<SourceID, std::shared_ptr<ModMatrix::SourceValue>> updatedSource;
                while (sourceValuesFifo.try_dequeue(updatedSource)) {
                    if (!sourceValues.contains(updatedSource.first)) {
                        sourceValues.insert({updatedSource});
                    } else {
                        sourceValues[updatedSource.first] = updatedSource.second;
                    }

                    const auto val = updatedSource.second->value.getGlobalValue()
                                     + updatedSource.second->value.getVoiceValue(mostRecentVoiceIndex);
                    sourceChocValues.setMember(std::to_string(updatedSource.first), val);
                }

                connection.eval("window.ui.sourceValuesUpdated", {
                    sourceChocValues
                });

                if (sourceDefNeedsRefresh) {
                    sourceDefNeedsRefresh = false;
                    sourceDefRefreshReady = true;
                }
            }

            if (targetValuesUpdatedFlag) {
                targetValuesUpdatedFlag = false;
                std::pair<TargetID, std::shared_ptr<ModMatrix::TargetValue> > updatedTarget;
                while (targetValuesFifo.try_dequeue(updatedTarget)) {
                    if (!targetValues.contains(updatedTarget.first)) {
                        targetValues.insert({updatedTarget});
                    } else {
                        targetValues[updatedTarget.first] = updatedTarget.second;
                    }

                    const auto val = updatedTarget.second->value.getGlobalValue()
                                     + updatedTarget.second->value.getVoiceValue(mostRecentVoiceIndex);
                    targetChocValues.setMember(std::to_string(updatedTarget.first), val);
                }

                connection.eval("window.ui.targetValuesUpdated", { targetChocValues });

                if (targetDefNeedsRefresh) {
                    targetDefNeedsRefresh = false;
                    targetDefRefreshReady = true;
                }
            }
        }

        void OnRecentVoiceUpdated(size_t voiceIndex) override {
            connection.eval("window.ui.onRecentVoiceUpdated", {choc::value::Value((int)voiceIndex)});
        }

    private:

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

        choc::value::Value& getCachedSourceDefs() {
            if (sourceDefRefreshReady) {
                sourceDefRefreshReady = false;
                cachedSourceDefs = choc::value::createObject("");

                for (const auto& [sourceID, source] : sourceValues) {
                    cachedSourceDefs.addMember(std::to_string(sourceID), source->getState());
                }
            }
            return cachedSourceDefs;
        }

        choc::value::Value& getCachedTargetValues() {
            if (targetDefRefreshReady) {
                targetDefRefreshReady = false;
                cachedTargetsValue = choc::value::createObject("");

                for (const auto& [targetID, target] : targetValues) {
                    const auto v = target->getState();
                    cachedTargetsValue.addMember(std::to_string(targetID), v);
                }
            }
            return cachedTargetsValue;
        }
    };
}