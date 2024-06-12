//
// Created by August Pemberton on 19/03/2024.
//

#pragma once
#include "WebUIAttachment.h"

namespace imagiro {
    class UtilAttachment : public WebUIAttachment {
    public:
        UtilAttachment(WebProcessor& p, WebViewManager& w)
                : WebUIAttachment(p, w) {

        }

        void addBindings() override {
            webViewManager.bind(
                    "juce_compressString",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto string = std::string(args[0].getWithDefault(""));

                        auto compressedPresetString = compress_string(string);

                        juce::MemoryOutputStream encodedStream;
                        juce::Base64::convertToBase64(encodedStream, compressedPresetString.data(),
                                                      compressedPresetString.size());
                        return choc::value::Value {encodedStream.toString().toStdString()};
                    });

            webViewManager.bind(
                    "juce_decompressString",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto encodedString = args[0].getWithDefault("");

                        juce::MemoryOutputStream decodedStream;
                        if (!juce::Base64::convertFromBase64(decodedStream, encodedString)) {
                            throw new std::runtime_error("Failed to decode string");
                        }

                        std::string compressedString (static_cast<const char*>(decodedStream.getData()),
                                                      decodedStream.getDataSize());

                        auto decompressedString = decompress_string(compressedString);
                        return choc::value::Value (decompressedString);
                    });


            webViewManager.bind(
                    "juce_saveInProcessor", [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto key = std::string(args[0].getWithDefault(""));
                        if (key.empty()) return {};
                        auto value = std::string(args[1].getWithDefault(""));

                        processor.getWebViewData().setMember(key, value);

                        std::string js = "window.ui.processorValueUpdated(";
                        js += "'"; js += key; js += "',";
                        js += "'"; js += value; js += "');";
                        webViewManager.evaluateJavascript(js);
                        return {};
                    });

            webViewManager.bind(
                    "juce_loadFromProcessor", [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto key = std::string(args[0].getWithDefault(""));
                        if (key.empty()) return {};

                        if (!processor.getWebViewData().hasObjectMember(key)) return {};
                        return choc::value::Value(processor.getWebViewData()[key]);
                    });

            webViewManager.bind(
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

                        std::string js = "window.ui.configValueUpdated(";
                        js += "'"; js += key; js += "',";
                        js += "'"; js += configFile->getValue(key).toStdString(); js += "');";
                        webViewManager.evaluateJavascript(js);
                        return {};
                    }
            );
            webViewManager.bind(
                    "juce_loadFromConfig",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto key = juce::String(args[0].toString());
                        auto& configFile = Resources::getInstance()->getConfigFile();
                        if (!configFile->containsKey(key)) return {};

                        auto val = configFile->getValue(key);

                        return choc::value::Value(val.toStdString());
                    }
            );

            webViewManager.bind(
                    "juce_Log",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto string = args[0].getWithDefault("");
                        juce::Logger::outputDebugString(string);
                        return {};
                    });

            webViewManager.bind("juce_copyToClipboard",
                                [&](const choc::value::ValueView& args) -> choc::value::Value {
                                    juce::SystemClipboard::copyTextToClipboard(args[0].toString());
                                    return {};
                                }
            );

            webViewManager.bind("juce_getTextFromClipboard",
                                [&](const choc::value::ValueView& args) -> choc::value::Value {
                                    return choc::value::Value(
                                            juce::SystemClipboard::getTextFromClipboard().toStdString()
                                    );
                                }
            );

            webViewManager.bind("juce_setDefaultBPM", [&](const choc::value::ValueView& args) -> choc::value::Value {
                const auto defaultBPM = args[0].getWithDefault(120);
                processor.setDefaultBPM(defaultBPM);
                return {};
            });
            webViewManager.bind("juce_getDefaultBPM", [&](const choc::value::ValueView& args) -> choc::value::Value {
                return choc::value::Value(processor.getDefaultBPM());
            });
        }

    };
}