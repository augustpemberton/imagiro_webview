//
// Created by August Pemberton on 19/03/2024.
//

#pragma once
#include "UIAttachment.h"
#include "choc/text/choc_JSON.h"
#include "imagiro_processor/imagiro_processor.h"
#include "imagiro_util/src/BackgroundTaskRunner.h"

namespace imagiro {
    class UtilAttachment : public UIAttachment, ValueData::Listener, BackgroundTaskRunner::Listener {
    public:
        UtilAttachment(UIConnection& c, Processor& p)
                : UIAttachment(c), processor(p) {
            backgroundTaskRunner.addListener(this);
            processor.getValueData().addListener(this);
        }

        ~UtilAttachment() override {
            backgroundTaskRunner.removeListener(this);
            processor.getValueData().removeListener(this);
        }

        void OnValueDataUpdated(ValueData& data, const std::string &key, const choc::value::ValueView &newValue) override {
            connection.eval("window.ui.processorValueUpdated", {
                choc::value::Value{key},
                choc::value::Value{newValue}
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
                        const auto key = std::string(args[0].getWithDefault(""));
                        if (key.empty()) return {};
                        const auto value = args[1];
                        const auto saveInPreset = args[2].getWithDefault(false);

                        processor.getValueData().set(key, value, saveInPreset);

                        return {};
                    });

            connection.bind(
                    "juce_loadFromProcessor", [&](const choc::value::ValueView &args) -> choc::value::Value {
                        const auto key = std::string(args[0].getWithDefault(""));
                        if (key.empty()) return {};

                        auto val = processor.getValueData().get(key);
                        if (!val) return {};
                        return *val;
                    });

            connection.bind(
                    "juce_saveInConfig",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto key = args[0].toString();

                        std::string valString;

                        auto& configFile = Resources::getInstance()->getConfigFile();
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
                        auto& configFile = Resources::getInstance()->getConfigFile();
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
                const auto defaultBPM = args[0].getWithDefault(120);
                processor.setDefaultBPM(defaultBPM);
                return {};
            });
            connection.bind("juce_getDefaultBPM", [&](const choc::value::ValueView&) -> choc::value::Value {
                return choc::value::Value(processor.getDefaultBPM());
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
    };
}
