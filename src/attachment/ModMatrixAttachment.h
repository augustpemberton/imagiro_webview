//
// Created by August Pemberton on 03/02/2025.
//

#pragma once
#include "WebUIAttachment.h"
#include "../WebProcessor.h"

namespace imagiro {

    class ModMatrixAttachment : public WebUIAttachment, ModMatrix::Listener, juce::Timer {
    private:

    public:
        ModMatrixAttachment(WebProcessor& p, ModMatrix& matrix)
                : WebUIAttachment(p, p.getWebViewManager()), modMatrix(matrix)
        {
            modMatrix.addListener(this);
            startTimerHz(30);
        }

        ~ModMatrixAttachment() override {
            modMatrix.removeListener(this);
        }

        void OnMatrixUpdated() override {
            sendMatrixUpdateFlagAfterNextDataLoad = true;
        }

        void addBindings() override {
            webViewManager.bind("juce_updateModulation", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto sourceID = args[0].getWithDefault(0u);
                auto targetID = args[1].getWithDefault(0u);
                auto depth = args[2].getWithDefault(0.f);

                auto attackMS = args.size() > 3 ? args[3].getWithDefault(0.f) : 0.f;
                auto releaseMS = args.size() > 4 ? args[4].getWithDefault(0.f) : 0.f;

                modMatrix.setConnection(sourceID, targetID, {depth, attackMS, releaseMS});
                return {};
            });

            webViewManager.bind("juce_removeModulation", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto sourceID = args[0].getWithDefault(0u);
                auto targetID = args[1].getWithDefault(0u);

                modMatrix.removeConnection(sourceID, targetID);
                return {};
            });

            webViewManager.bind("juce_getModMatrix", [&](const choc::value::ValueView& args) -> choc::value::Value {
                return matrixMessageThread.getState();
            });

            webViewManager.bind("juce_getSourceNames", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto namesValue = choc::value::createObject("SourceNames");
                for (const auto& [sourceID, name] : modMatrix.getSourceNames()) {
                    namesValue.setMember(std::to_string(sourceID), name);
                }
                return namesValue;
            });

            webViewManager.bind("juce_getTargetNames", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto namesValue = choc::value::createObject("TargetNames");
                for (const auto& [targetID, name] : modMatrix.getTargetNames()) {
                    namesValue.setMember(std::to_string(targetID), name);
                }
                return namesValue;
            });

            webViewManager.bind("juce_getSourceValues", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto sourcesValue = choc::value::createObject("");
                for (const auto& [sourceID, source] : sourceValues) {
                    sourcesValue.addMember(std::to_string(sourceID), source.getValue());
                }
                return sourcesValue;
            });

            webViewManager.bind("juce_getTargetValues", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto targetsValue = choc::value::createObject("");
                for (const auto& [targetID, target] : targetValues) {
                    const auto v = target.getValue();
                    targetsValue.addMember(std::to_string(targetID), v);
                }
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
        }

        void timerCallback() override {
            while (matrixFifo.try_dequeue(matrixMessageThread)) { /**/ }
            while (sourceValuesFifo.try_dequeue(sourceValues)) { /**/ }
            while (targetValuesFifo.try_dequeue(targetValues)) { /**/ }

            if (sendMatrixUpdateFlag) {
                auto matrixValue = matrixMessageThread.getState();
                webViewManager.evaluateJavascript("window.ui.modMatrixUpdated("+choc::json::toString(matrixValue)+")");
                sendMatrixUpdateFlag = false;
            }
        }

        void OnRecentVoiceUpdated(size_t voiceIndex) override {
            webViewManager.evaluateJavascript("window.ui.onRecentVoiceUpdated("+std::to_string(voiceIndex)+")");
        }

    private:
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