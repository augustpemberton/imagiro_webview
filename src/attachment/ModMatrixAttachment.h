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

        void timerCallback() override {
            while (matrixFifo.try_dequeue(matrixMessageThread)) { /**/ }

            if (!sendMatrixUpdateFlag) return;
            auto matrixValue = matrixMessageThread.getState();
            webViewManager.evaluateJavascript("window.ui.modMatrixUpdated("+choc::json::toString(matrixValue)+")");
            sendMatrixUpdateFlag = false;
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

            webViewManager.bind("juce_getSources", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto namesValue = choc::value::createEmptyArray();
                for (const auto& [id, name]: modMatrix.getSourceNames()) {
                    auto nameObj = choc::value::createObject("");
                    nameObj.addMember("id", (int)id);
                    nameObj.addMember("name", name);
                    namesValue.addArrayElement(nameObj);
                }
                return namesValue;
            });

            webViewManager.bind("juce_getTargets", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto namesValue = choc::value::createEmptyArray();
                for (const auto& [id, name]: modMatrix.getTargetNames()) {
                    auto nameObj = choc::value::createObject("");
                    nameObj.addMember("id", (int)id);
                    nameObj.addMember("name", name);
                    namesValue.addArrayElement(nameObj);
                }
                return namesValue;
            });
        }

        // call each processBlock tick
        void processCallback() override {
            matrixFifo.enqueue(modMatrix.getSerializedMatrix());
            if (sendMatrixUpdateFlagAfterNextDataLoad) {
                sendMatrixUpdateFlagAfterNextDataLoad = false;
                sendMatrixUpdateFlag = true;
            }
        }

    private:
        ModMatrix& modMatrix;

        SerializedMatrix matrixMessageThread {};
        moodycamel::ReaderWriterQueue<SerializedMatrix> matrixFifo {128};

        std::atomic<bool> sendMatrixUpdateFlag {false};
        std::atomic<bool> sendMatrixUpdateFlagAfterNextDataLoad {false};

        std::unordered_map<SourceID, float> latestSourceValues;
        moodycamel::ReaderWriterQueue<std::pair<SourceID, float>> sourceValueFifo {512};
    };
}