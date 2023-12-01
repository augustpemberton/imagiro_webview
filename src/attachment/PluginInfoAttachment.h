//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include <version.h>
#include <timestamp.h>
#include <imagiro_processor/imagiro_processor.h>
#include "WebUIAttachment.h"

#include "juce_audio_devices/juce_audio_devices.h"
#include "juce_audio_plugin_client/juce_audio_plugin_client.h"
#include "juce_audio_utils/juce_audio_utils.h"
#include "juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h"

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
            webViewManager.evaluateJavascript("window.ui.updateDiscovered()");
        }

        void addBindings() override {
            webViewManager.bind(
                    "juce_getCurrentVersion",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        return choc::value::Value(processor.getVersionManager().getCurrentVersion().toStdString());
                    }
            );
            webViewManager.bind(
                    "juce_showStandaloneAudioSettings",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        juce::StandalonePluginHolder::getInstance()->showAudioSettingsDialog();
                        return {};
                    }
            );

            webViewManager.bind(
                    "juce_getWrapperType",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        std::string wrapperTypeString;

                        if (processor.wrapperType == juce::AudioProcessor::wrapperType_AudioUnit)
                            wrapperTypeString = "AU";
                        else if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
                            wrapperTypeString = "Standalone";
                        else if (processor.wrapperType == juce::AudioProcessor::wrapperType_VST3)
                            wrapperTypeString = "VST3";
                        else if (processor.wrapperType == juce::AudioProcessor::wrapperType_VST)
                            wrapperTypeString = "VST2";

                        return choc::value::Value(wrapperTypeString);
                    }
            );

            webViewManager.bind(
                    "juce_getPluginName",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        return choc::value::Value(PLUGIN_NAME);
                    }
            );

            webViewManager.bind(
                    "juce_saveInProcessor", [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto key = std::string(args[0].getWithDefault(""));
                        if (key.empty()) return {};
                        auto value = std::string(args[1].getWithDefault(""));

                        processor.getWebViewData().setMember(key, value);
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
                    "juce_setConfig",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto key = args[0].toString();
                        auto value = args[1].toString();
                        Resources::getConfigFile()->setValue(juce::String(key), juce::String(value));
                        return {};
                    }
            );
            webViewManager.bind(
                    "juce_getConfig",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto key = juce::String(args[0].toString());
                        auto configFile = Resources::getConfigFile();
                        if (!configFile->containsKey(key)) return {};

                        auto val = configFile->getValue(key);
                        return choc::value::Value(val.toStdString());
                    }
            );
            webViewManager.bind(
                    "juce_getIsUpdateAvailable",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto newVersion = processor.getVersionManager().isUpdateAvailable();
                        if (!newVersion) return {};

                        return choc::value::Value(newVersion->toStdString());
                    }
            );
            webViewManager.bind( "juce_revealUpdate",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     juce::URL(processor.getVersionManager().getUpdateURL()).launchInDefaultBrowser();
                                     return {};
                                 } );

            webViewManager.bind("juce_getIsDebug",
                                [&](const choc::value::ValueView& args) -> choc::value::Value {
#if JUCE_DEBUG
                                    return choc::value::Value(true);
#else
                                    return choc::value::Value(false);
#endif
                                });

            webViewManager.bind("juce_getPlatform",
                                [&](const choc::value::ValueView& args) -> choc::value::Value {
                                    if ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::Windows) != 0) {
                                        return choc::value::Value("windows");
                                    } else if ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::MacOSX) != 0) {
                                        return choc::value::Value("macOS");
                                    } else {
                                        return choc::value::Value(juce::SystemStats::getOperatingSystemName().toStdString());
                                    }
                                });

            webViewManager.bind("juce_getDebugVersionString",
                                [&](const choc::value::ValueView& args) -> choc::value::Value {
                                    juce::String statusString = " ";

                                    if (juce::String(git_branch_str) != "master" && juce::String(git_branch_str) != "main") {
                                        statusString += juce::String(git_branch_str);
                                    }
                                    statusString += "#" + juce::String(git_short_hash_str);
                                    if (git_dirty_str) statusString += "!";
                                    statusString += " (" + juce::String(build_date_str) + ")";

                                    return choc::value::Value(statusString.toStdString());
                                }
            );

            webViewManager.bind(
                    "juce_getCpuLoad",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        return choc::value::Value(processor.getCpuLoad());
                    }
            );
            webViewManager.bind(
                    "juce_Log",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto string = args[0].getWithDefault("");
                        juce::Logger::outputDebugString(string);
                        return {};
                    });
        }
    };
}
