//
// Created by August Pemberton on 19/03/2024.
//

#pragma once
#include "UIAttachment.h"
#include "util/UIData.h"

namespace imagiro {
    class UtilAttachment : public UIAttachment {
    public:
        UtilAttachment(UIConnection& c, UIData& u, Processor& p)
                : UIAttachment(c), uiData(u), processor(p) {

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
                        auto key = std::string(args[0].getWithDefault(""));
                        if (key.empty()) return {};
                        auto value = std::string(args[1].getWithDefault(""));
                        auto saveInPreset = args[2].getWithDefault(false);

                        uiData.set(key, value, saveInPreset);

                        connection.eval("window.ui.processorValueUpdated", {args[0], args[1]});
                        return {};
                    });

            connection.bind(
                    "juce_loadFromProcessor", [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto key = std::string(args[0].getWithDefault(""));
                        if (key.empty()) return {};

                        auto val = uiData.get(key);
                        if (!val) return {};
                        return choc::value::Value(*val);
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
                        connection.eval("window.ui.configValueUpdated", {args[0], args[1]});
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

                std::vector<choc::value::ValueView> v;
                if (args.size() > 1) v.push_back(args[1]);
                connection.eval("window.ui.onSignal", v);
                return {};
            });
        }

    private:
        UIData& uiData;
        Processor& processor;

    };
}