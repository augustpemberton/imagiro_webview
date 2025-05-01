//
// Created by August Pemberton on 03/02/2025.
//

#pragma once
#include "UIAttachment.h"

namespace imagiro {

    class ModMatrixAttachment : public UIAttachment, ModMatrix::Listener, juce::Timer {
    private:

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
                auto sourcesValue = choc::value::createObject("");
                for (const auto& [sourceID, source] : sourceValues) {
                    sourcesValue.addMember(sourceID, source.getValue());
                }
                sourcesValue.addMember("time", lastUIUpdate.load());
                return sourcesValue;
            });

            connection.bind("juce_getTargetValues", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto targetsValue = choc::value::createObject("");
                for (const auto& [targetID, target] : targetValues) {
                    const auto v = target.getValue();
                    targetsValue.addMember(targetID, v);
                }
                targetsValue.addMember("time", lastUIUpdate.load());
                return targetsValue;
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
            while (matrixFifo.try_dequeue(matrixMessageThread)) { /**/ }
            while (sourceValuesFifo.try_dequeue(sourceValues)) { /**/ }
            while (targetValuesFifo.try_dequeue(targetValues)) { /**/ }

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
    };
}