//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include <version.h>
#include <timestamp.h>
#include <imagiro_processor/imagiro_processor.h>
#include "WebUIAttachment.h"

#include "../WebProcessor.h"
#include "juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h"
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>

namespace imagiro {
    class PluginInfoAttachment : public WebUIAttachment, public VersionManager::Listener {
    public:
        using WebUIAttachment::WebUIAttachment;
        void addListeners() override {
            processor.getVersionManager().addListener(this);
        }

        ~PluginInfoAttachment() override {
            processor.getVersionManager().removeListener(this);
        }

        void OnUpdateDiscovered() override {
//            processor.getViewManager().evaluateJavascript("window.ui.updateDiscovered()");
        }

        void addBindings() override {
             viewManager.bind( "juce_getCurrentVersion", [&](const JSObject& obj, const JSArgs& args) -> JSValue {
                    return processor.getVersionManager().getCurrentVersion().toRawUTF8();
             });

            viewManager.bind(
                    "juce_showStandaloneAudioSettings",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        juce::StandalonePluginHolder::getInstance()->showAudioSettingsDialog();
                        return {};
                    }
            );

            viewManager.bind(
                    "juce_getMillisecondTime",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                            return {(juce::int64)juce::Time::getMillisecondCounter()};
                    });

            viewManager.bind(
                    "juce_getWrapperType",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        std::string wrapperTypeString;

                        if (processor.wrapperType == juce::AudioProcessor::wrapperType_AudioUnit)
                            wrapperTypeString = "AU";
                        else if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
                            wrapperTypeString = "Standalone";
                        else if (processor.wrapperType == juce::AudioProcessor::wrapperType_VST3)
                            wrapperTypeString = "VST3";
                        else if (processor.wrapperType == juce::AudioProcessor::wrapperType_VST)
                            wrapperTypeString = "VST2";

                        return {wrapperTypeString.c_str()};
                    }
            );

            viewManager.bind(
                    "juce_getPluginName",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        return {PLUGIN_NAME};
                    }
            );

//            viewManager.bind(
//                    "juce_saveInProcessor", [&](const JSObject& obj, const JSArgs &args) -> JSValue {
//                        auto key = getStdString(args[0]);
//                        if (key.empty()) return {};
//                        auto value = getStdString(args[1]);
//
//                        processor.getWebViewData().setMember(key, value);
//                        return {};
//                    });
//
//            viewManager.bind(
//                    "juce_loadFromProcessor", [&](const JSObject& obj, const JSArgs &args) -> JSValue {
//                        auto key = std::string(args[0].getWithDefault(""));
//                        if (key.empty()) return {};
//
//                        if (!processor.getWebViewData().hasObjectMember(key)) return {};
//                        return processor.getWebViewData()[key];
//                    });

            viewManager.bind(
                    "juce_setConfig",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        auto key = getStdString(args[0]);
                        auto value = getStdString(args[1]);
                        Resources::getConfigFile()->setValue(juce::String(key), juce::String(value));
                        return {};
                    }
            );
            viewManager.bind(
                    "juce_getConfig",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        auto key = getStdString(args[0]);
                        auto configFile = Resources::getConfigFile();
                        if (!configFile->containsKey(key)) return {};

                        auto val = configFile->getValue(key);
                        return {val.toRawUTF8()};
                    }
            );
            viewManager.bind(
                    "juce_getIsUpdateAvailable",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        auto newVersion = processor.getVersionManager().isUpdateAvailable();
                        if (!newVersion) return {};

                        return {newVersion->toRawUTF8()};
                    }
            );
            viewManager.bind( "juce_revealUpdate",
                                 [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                                     juce::URL(processor.getVersionManager().getUpdateURL()).launchInDefaultBrowser();
                                     return {};
                                 } );

            viewManager.bind("juce_getIsDebug",
                                [&](const JSObject& obj, const JSArgs& args) -> JSValue {
#if JUCE_DEBUG
                                    return true;
#else
                                    return false;
#endif
                                });

            viewManager.bind("juce_getPlatform",
                                [&](const JSObject& obj, const JSArgs& args) -> JSValue {
                                    if ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::Windows) != 0) {
                                        return {"windows"};
                                    } else if ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::MacOSX) != 0) {
                                        return {"macOS"};
                                    } else {
                                        return {juce::SystemStats::getOperatingSystemName().toRawUTF8()};
                                    }
                                });

            viewManager.bind("juce_getDebugVersionString",
                                [&](const JSObject& obj, const JSArgs& args) -> JSValue {
                                    juce::String statusString = " ";

                                    if (juce::String(git_branch_str) != "master" && juce::String(git_branch_str) != "main") {
                                        statusString += juce::String(git_branch_str);
                                    }
                                    statusString += "#" + juce::String(git_short_hash_str);
                                    if (git_dirty_str) statusString += "!";
                                    statusString += " (" + juce::String(build_date_str) + ")";

                                    return statusString.toRawUTF8();
                                }
            );

            viewManager.bind(
                    "juce_getCpuLoad",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        return processor.getCpuLoad();
                    }
            );
            viewManager.bind(
                    "juce_Log",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        auto string = toStdString(args[0]);
                        juce::Logger::outputDebugString(string);
                        return {};
                    });
        }
    };
}
