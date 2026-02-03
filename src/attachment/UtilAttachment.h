//
// Created by August Pemberton on 19/03/2024.
//

#pragma once
#include "UIAttachment.h"
#include "choc/text/choc_JSON.h"
#include "imagiro_util/BackgroundTaskRunner.h"
#include "imagiro_util/miniz/compress_string.h"
#include <unordered_map>
#include <unordered_set>
#include <string_view>

namespace imagiro {
    class UtilAttachment : public UIAttachment, BackgroundTaskRunner::Listener {
    public:
        UtilAttachment(UIConnection& c, Processor& p)
                : UIAttachment(c), processor(p) {
            backgroundTaskRunner.addListener(this);
        }

        ~UtilAttachment() override {
            backgroundTaskRunner.removeListener(this);
        }

        // Serialize processor data that should be saved in presets
        choc::value::Value getProcessorDataForPreset() const {
            auto obj = choc::value::createObject({});
            for (const auto& key : presetKeys_) {
                if (auto it = processorData_.find(key); it != processorData_.end()) {
                    obj.addMember(key, it->second);
                }
            }
            return obj;
        }

        // Restore processor data from preset
        void loadProcessorDataFromPreset(const choc::value::ValueView& data) {
            if (!data.isObject()) return;
            data.visitObjectMembers([this](std::string_view key, const choc::value::ValueView& value) {
                processorData_[std::string(key)] = choc::value::Value(value);
                presetKeys_.insert(std::string(key));
            });
        }

        void OnTaskFinished(int taskID, const choc::value::ValueView &result) override {
            connection.eval("window.ui.onBackgroundTaskFinished", {
                choc::value::Value{taskID},
                choc::value::Value{result}
            });
        }

        void addBindings() override {
            connection.bind(
                    "juce_compressString",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto string = std::string(args[0].getWithDefault(""));

                        auto compressedPresetString = compress_string(string);

                        juce::MemoryOutputStream encodedStream;
                        juce::Base64::convertToBase64(encodedStream, compressedPresetString.data(),
                                                      compressedPresetString.size());
                        return choc::value::Value {encodedStream.toString().toStdString()};
                    });

            connection.bind(
                    "juce_decompressString",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto encodedString = args[0].getWithDefault("");

                        juce::MemoryOutputStream decodedStream;
                        if (!juce::Base64::convertFromBase64(decodedStream, encodedString)) {
                            throw std::runtime_error("Failed to decode string");
                        }

                        std::string compressedString (static_cast<const char*>(decodedStream.getData()),
                                                      decodedStream.getDataSize());

                        auto decompressedString = decompress_string(compressedString);
                        return choc::value::Value (decompressedString);
                    });


            connection.bind(
                    "juce_saveInProcessor", [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto key = std::string(args[0].toString());
                        auto value = choc::value::Value(args[1]);

                        // Optional third argument: save in preset (default false)
                        bool saveInPreset = args.size() > 2 && args[2].getWithDefault(false);

                        processorData_[key] = value;
                        if (saveInPreset) {
                            presetKeys_.insert(key);
                        } else {
                            presetKeys_.erase(key);
                        }

                        return {};
                    });

            connection.bind(
                    "juce_loadFromProcessor", [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto key = std::string(args[0].toString());

                        if (auto it = processorData_.find(key); it != processorData_.end()) {
                            return choc::value::Value(it->second);
                        }

                        return {};
                    });

            connection.bind(
                    "juce_saveInConfig",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto key = args[0].toString();

                        std::string valString;

                        auto& configFile = resources->getConfigFile();
                        if (args[1].isFloat32()) {
                            configFile->setValue(juce::String(key), args[1].getFloat32());
                        } else if (args[1].isFloat64()) {
                            configFile->setValue(juce::String(key), args[1].getFloat64());
                        } else if (args[1].isInt()) {
                            configFile->setValue(juce::String(key), args[1].getWithDefault(0));
                        } else if (args[1].isBool()) {
                            configFile->setValue(juce::String(key), args[1].getBool());
                        } else if (args[1].isString()) {
                            configFile->setValue(juce::String(key), juce::String(args[1].toString()));
                        } else {
                            configFile->setValue(juce::String(key), juce::String(choc::json::toString(args[1])));
                        }

                        configFile->save();
                        connection.eval("window.ui.configValueUpdated", {
                            choc::value::Value(args[0]),
                            choc::value::Value(args[1])
                        });
                        return {};
                    }
            );
            connection.bind(
                    "juce_loadFromConfig",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto key = juce::String(args[0].toString());
                        auto& configFile = resources->getConfigFile();
                        if (!configFile->containsKey(key)) return {};

                        auto val = configFile->getValue(key);

                        return choc::value::Value(val.toStdString());
                    }
            );

            connection.bind(
                    "juce_Log",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto string = args[0].getWithDefault("");
                        juce::Logger::outputDebugString(string);
                        return {};
                    });

            connection.bind("juce_copyToClipboard",
                                [&](const choc::value::ValueView& args) -> choc::value::Value {
                                    juce::SystemClipboard::copyTextToClipboard(args[0].toString());
                                    return {};
                                }
            );

            connection.bind("juce_getTextFromClipboard",
                                [&](const choc::value::ValueView& ) -> choc::value::Value {
                                    return choc::value::Value(
                                            juce::SystemClipboard::getTextFromClipboard().toStdString()
                                    );
                                }
            );

            connection.bind("juce_setDefaultBPM", [&](const choc::value::ValueView& args) -> choc::value::Value {
                double bpm = args[0].getWithDefault(120.0);
                processor.transport().setDefaultBpm(bpm);
                return {};
            });
            connection.bind("juce_getDefaultBPM", [&](const choc::value::ValueView&) -> choc::value::Value {
                return choc::value::Value(processor.transport().defaultBpm());
            });

            connection.bind("juce_emitSignal", [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto signalName = std::string(args[0].getWithDefault(""));

                std::vector<choc::value::Value> v;
                v.emplace_back(signalName);
                if (args.size() > 1) v.emplace_back(choc::value::Value(args[1]));
                connection.eval("window.ui.onSignal", v);
                return {};
            });

            connection.bind("juce_Test", [&](const choc::value::ValueView& args) -> choc::value::Value {
                return choc::value::Value("hello world");
            });

            connection.bind("juce_onBackgroundThread", [&](const choc::value::ValueView& args) -> choc::value::Value {
                const auto fnName = std::string(args[0].getWithDefault(""));
                const auto fnArgs = choc::value::Value(args[1]);

                if (!connection.getBoundFunctions().contains(fnName)) return {};
                const auto underlyingFn = connection.getBoundFunctions().at(fnName);

                const auto jobID = backgroundTaskRunner.queueTask({
                    [underlyingFn, fnArgs] {
                        return underlyingFn(fnArgs);
                    }
                });

                return choc::value::Value{jobID};
            });
        }

    private:
        Processor& processor;
        BackgroundTaskRunner backgroundTaskRunner;
        juce::SharedResourcePointer<Resources> resources;

        // Storage for arbitrary UI data
        std::unordered_map<std::string, choc::value::Value> processorData_;
        std::unordered_set<std::string> presetKeys_;  // Keys that should be saved in preset
    };
}
