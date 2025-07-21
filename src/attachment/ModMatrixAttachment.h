//
// Created by August Pemberton on 03/02/2025.
//

#pragma once
#include "UIAttachment.h"

namespace imagiro {

    class ModMatrixAttachment : public UIAttachment, ModMatrix::Listener, juce::Timer {

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

        void OnConnectionAdded(const SourceID& source, const TargetID& target) override {
            const auto connection = modMatrix.getMatrix()[{source, target}];
            matrixCommands.try_enqueue({
                ChangeCommandType::Added,
                {
                    source,
                    target,
                    connection.getSettings().depth,
                    connection.getSettings().attackMS,
                    connection.getSettings().releaseMS,
                }
            });
        }

        void OnConnectionUpdated(const SourceID& source, const TargetID& target) override {
            const auto connection = modMatrix.getMatrix()[{source, target}];
            matrixCommands.try_enqueue({
                ChangeCommandType::Updated,
                {
                    source,
                    target,
                    connection.getSettings().depth,
                    connection.getSettings().attackMS,
                    connection.getSettings().releaseMS,
                }
            });
        }

        void OnConnectionRemoved(const SourceID& source, const TargetID& target) override {
            matrixCommands.try_enqueue({ChangeCommandType::Removed, { source, target }});
        }

        void OnSourceValueAdded(const SourceID& sourceID) override {
            const auto source = modMatrix.getSourceValues()[sourceID];
            sourceCommands.try_enqueue({
                ChangeCommandType::Added,
                sourceID,
                0, 0,
                source->name,
                source->bipolar
            });
        }

        void OnSourceValueUpdated(const SourceID& sourceID, const int voiceIndex) override {
            const auto source = modMatrix.getSourceValues()[sourceID];
            sourceCommands.try_enqueue({
                ChangeCommandType::Added,
                sourceID,
                voiceIndex < 0 ? source->value.getGlobalValue() : source->value.getVoiceValue(voiceIndex),
                voiceIndex,
                source->name,
                source->bipolar
            });
        }

        void OnSourceValueRemoved(const SourceID& sourceID) override {
            sourceCommands.try_enqueue({ ChangeCommandType::Removed, sourceID });
        }

        void OnTargetValueAdded(const TargetID& targetID) override {
            targetCommands.try_enqueue({ ChangeCommandType::Added, targetID });
        }

        void OnTargetValueUpdated(const TargetID& targetID, const int voiceIndex) override {
            const auto target = modMatrix.getTargetValues()[targetID];
            targetCommands.try_enqueue({
                ChangeCommandType::Updated,
                targetID,
                voiceIndex < 0 ? target->value.getGlobalValue() : target->value.getVoiceValue(voiceIndex),
                voiceIndex
            });
        }

        void OnTargetValueReset(const TargetID& targetID) override {
            targetCommands.try_enqueue({ ChangeCommandType::Reset, targetID });
        }

        void OnTargetValueRemoved(const TargetID& targetID) override {
            targetCommands.try_enqueue({ ChangeCommandType::Removed, targetID });
        }

        void addBindings() override {
            connection.bind("juce_updateModulation", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto sourceID = args[0].getWithDefault(0);
                auto targetID = args[1].getWithDefault(0);
                auto depth = args[2].getWithDefault(0.f);

                auto attackMS = args.size() > 3 ? args[3].getWithDefault(0.f) : 0.f;
                auto releaseMS = args.size() > 4 ? args[4].getWithDefault(0.f) : 0.f;

                modMatrix.queueConnection(sourceID, targetID, {depth, attackMS, releaseMS});

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
                return getSourceDefs();
            });

            connection.bind("juce_getTargetValues", [&](const choc::value::ValueView& args) -> choc::value::Value {
                return getTargetDefs();
            });
        }

        void timerCallback() override {
            processMatrixCommands();
            processSourceCommands();
            processTargetCommands();
        }

        void processMatrixCommands() {
            MatrixChangeCommand command {};
            bool updated {false};
            while (matrixCommands.try_dequeue(command)) {
                updated = true;
                if (command.type == ChangeCommandType::Added) {
                    matrixMessageThread.addIfNotAlreadyThere(command.entry);
                } else if (command.type == ChangeCommandType::Removed) {
                    matrixMessageThread.removeFirstMatchingValue(command.entry);
                } else if (command.type == ChangeCommandType::Updated) {
                    for (auto& entry : matrixMessageThread) {
                        if (entry.sourceID == command.entry.sourceID &&
                            entry.targetID == command.entry.targetID) {
                            entry = command.entry;
                        }
                    }
                }
            }

            if (updated) {
                connection.eval("window.ui.modMatrixUpdated", {
                    matrixMessageThread.getState()
                });
            }
        }

        void processSourceCommands() {
            SourceChangeCommand command {};
            bool updated = false;
            while (sourceCommands.try_dequeue(command)) {
                updated = true;
                if (command.type == ChangeCommandType::Added) {
                    sourceValues.insert({ command.id, { command.name, command.bipolar } });
                } else if (command.type == ChangeCommandType::Updated) {
                    jassert(sourceValues.contains(command.id));
                    sourceValues[command.id].bipolar = command.bipolar;
                    if (command.voiceIndex < 0) {
                        sourceValues[command.id].value.setGlobalValue(command.value);
                    } else {
                        sourceValues[command.id].value.setVoiceValue(command.value, command.voiceIndex);
                    }

                    auto mostRecentVoiceValue = sourceValues[command.id].value.getGlobalValue() +
                        sourceValues[command.id].value.getVoiceValue(mostRecentVoice);

                    connection.eval("window.ui.sourceValueUpdated", {
                        choc::value::Value(command.id),
                        choc::value::Value(static_cast<int>(mostRecentVoice)),
                        choc::value::Value(mostRecentVoiceValue)
                    });

                } else if (command.type == ChangeCommandType::Removed) {
                    sourceValues.erase(command.id);
                }
            }

            if (updated) {
            }
        }

        void processTargetCommands() {
            TargetChangeCommand command {};
            while (targetCommands.try_dequeue(command)) {
                if (command.type == ChangeCommandType::Added) {
                    targetValues.insert({command.id, {}});
                } else if (command.type == ChangeCommandType::Updated) {
                    if (!targetValues.contains(command.id)) targetValues.insert({command.id, {}});
                    if (command.voiceIndex < 0) {
                        targetValues[command.id].value.setGlobalValue(command.value);
                    } else {
                        targetValues[command.id].value.setVoiceValue(command.value, command.voiceIndex);
                    }

                    auto mostRecentVoiceValue = targetValues[command.id].value.getGlobalValue() +
                        targetValues[command.id].value.getVoiceValue(mostRecentVoice);

                    connection.eval("window.ui.targetValueUpdated", {
                        choc::value::Value(command.id),
                        choc::value::Value(static_cast<int>(mostRecentVoice)),
                        choc::value::Value(mostRecentVoiceValue)
                    });
                } else if (command.type == ChangeCommandType::Reset) {
                    jassert(targetValues.contains(command.id));
                    targetValues[command.id].value.resetValue();

                    connection.eval("window.ui.targetValueUpdated", {
                        choc::value::Value(command.id),
                        choc::value::Value(static_cast<int>(mostRecentVoice)),
                        choc::value::Value(0)
                    });

                } else if (command.type == ChangeCommandType::Removed) {
                    targetValues.erase(command.id);
                }
            }
        }

        void OnRecentVoiceUpdated(size_t voiceIndex) override {
            mostRecentVoice = voiceIndex;
            connection.eval("window.ui.onRecentVoiceUpdated", {choc::value::Value((int)voiceIndex)});
        }

    private:
        ModMatrix& modMatrix;

        size_t mostRecentVoice;

        SerializedMatrix matrixMessageThread {};
        std::unordered_map<SourceID, ModMatrix::SourceValue> sourceValues;
        std::unordered_map<TargetID, ModMatrix::TargetValue> targetValues;

        enum class ChangeCommandType {
            Added, Removed, Updated, Reset
        };

        struct MatrixChangeCommand {
            ChangeCommandType type;
            SerializedMatrixEntry entry;
        };

        struct SourceChangeCommand {
            ChangeCommandType type;
            SourceID id;
            float value;
            int voiceIndex;
            std::string name;
            bool bipolar;
        };

        struct TargetChangeCommand {
            ChangeCommandType type;
            TargetID id;
            float value;
            int voiceIndex;
            std::string name;
        };

        moodycamel::ReaderWriterQueue<MatrixChangeCommand> matrixCommands {128};
        moodycamel::ReaderWriterQueue<SourceChangeCommand> sourceCommands {128};
        moodycamel::ReaderWriterQueue<TargetChangeCommand> targetCommands {128};

        choc::value::Value getSourceDefs() {
            auto defs = choc::value::createObject("SourceDefs");
            for (const auto& [sourceID, source] : sourceValues) {
                defs.addMember(std::to_string(sourceID), source.getState());
            }
            return defs;
        }

        choc::value::Value getTargetDefs() {
            auto defs = choc::value::createObject("TargetDefs");
            for (const auto& [sourceID, source] : sourceValues) {
                defs.addMember(std::to_string(sourceID), source.getState());
            }
            return defs;
        }
    };
}