//
// Created by August Pemberton on 03/02/2025.
//

#pragma once
#include "WebUIAttachment.h"
#include "../WebProcessor.h"

namespace imagiro {
    using MatrixType = std::unordered_map<std::pair<SourceID, TargetID>, ModMatrix::ConnectionInfo>;

class ModMatrixAttachment : public WebUIAttachment, ModMatrix::Listener {
    public:
        ModMatrixAttachment(WebProcessor& p, ModMatrix& matrix)
            : WebUIAttachment(p, p.getWebViewManager()), modMatrix(matrix)
        {
            modMatrix.addListener(this);
        }

        ~ModMatrixAttachment() override {
            modMatrix.removeListener(this);
        }

        void OnMatrixUpdated() override {
            auto matrix = getValueFromMatrix(matrixMessageThread);
            webViewManager.evaluateJavascript("window.ui.modMatrixUpdated("+choc::json::toString(matrix)+")");
        }

        void addBindings() override {
            webViewManager.bind("juce_updateModulation", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto sourceID = args[0].getWithDefault(0u);
                auto targetID = args[1].getWithDefault(0u);
                auto depth = args[2].getWithDefault(0.f);

                modMatrix.setConnectionInfo(sourceID, targetID, {depth});
                return {};
            });

            webViewManager.bind("juce_removeModulation", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto sourceID = args[0].getWithDefault(0u);
                auto targetID = args[1].getWithDefault(0u);

                modMatrix.removeConnectionInfo(sourceID, targetID);
                return {};
            });

            webViewManager.bind("juce_getModMatrix", [&](const choc::value::ValueView& args) -> choc::value::Value {
                while (matrixFifo.try_dequeue(matrixMessageThread)) { /**/ }
                return getValueFromMatrix(matrixMessageThread);
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
            matrixFifo.enqueue(modMatrix.getMatrix());
        }

    private:
        ModMatrix& modMatrix;

        MatrixType matrixMessageThread {};
        moodycamel::ReaderWriterQueue<MatrixType> matrixFifo {128};

        std::unordered_map<SourceID, float> latestSourceValues;
        moodycamel::ReaderWriterQueue<std::pair<SourceID, float>> sourceValueFifo {512};

        choc::value::Value getValueFromMatrix(MatrixType m) {
            auto val = choc::value::createEmptyArray();
            for(const auto& [pair, connection] : m) {
                auto connectionValue = choc::value::createObject("connection");
                connectionValue.addMember("sourceID", choc::value::Value((int)pair.first));
                connectionValue.addMember("targetID", choc::value::Value((int)pair.second));
                connectionValue.addMember("depth", choc::value::Value(connection.depth));
                val.addArrayElement(connectionValue);
            }
            return val;
        }
    };
}