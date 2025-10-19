//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include <timestamp.h>
#include <imagiro_processor/imagiro_processor.h>
#include "UIAttachment.h"

#include "juce_audio_devices/juce_audio_devices.h"
#include "juce_audio_plugin_client/juce_audio_plugin_client.h"
// #include "juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h"

namespace imagiro {
    class PluginInfoAttachment : public UIAttachment, juce::ChangeListener {
    public:
        PluginInfoAttachment(UIConnection& connection, Processor& p)
                : UIAttachment(connection), processor(p)
        {
        }

        void changeListenerCallback(juce::ChangeBroadcaster *source) override {
            connection.eval("window.ui.onDevicesChanged");
        }

        void addBindings() override {
            connection.bind(
                    "juce_getCurrentVersion",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        std::string version;
#ifdef PROJECT_VERSION
                        version = std::string(PROJECT_VERSION);
#endif
                        return choc::value::Value(version);
                    }
            );

            connection.bind(
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
                        else if (processor.wrapperType == juce::AudioProcessor::wrapperType_AAX)
                            wrapperTypeString = "AAX";

                        return choc::value::Value(wrapperTypeString);
                    }
            );

            connection.bind(
                    "juce_getPluginName",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        return choc::value::Value(JucePlugin_Name);
                    }
            );

            connection.bind(
                    "juce_getPluginHostType",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        juce::PluginHostType hostType;
                        return choc::value::Value(hostType.getHostDescription());
                    }
            );

            connection.bind("juce_getIsDebug",
                                [&](const choc::value::ValueView &args) -> choc::value::Value {
#if JUCE_DEBUG
                                    return choc::value::Value(true);
#else
                                    return choc::value::Value(false);
#endif
                                });

            connection.bind("juce_getPlatform",
                                [&](const choc::value::ValueView &args) -> choc::value::Value {
                                    if ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::Windows) !=
                                        0) {
                                        return choc::value::Value("windows");
                                    } else if ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::MacOSX)
                                               != 0) {
                                        return choc::value::Value("macOS");
                                    } else {
                                        return choc::value::Value(
                                                juce::SystemStats::getOperatingSystemName().toStdString());
                                    }
                                });

            connection.bind("juce_getDebugVersionString",
                                [&](const choc::value::ValueView &args) -> choc::value::Value {
                                    juce::String statusString = " ";

                                    if (juce::String(git_branch_str) != "master" && juce::String(git_branch_str) !=
                                                                                    "main") {
                                        statusString += juce::String(git_branch_str);
                                    }
                                    statusString += "#" + juce::String(git_short_hash_str);
                                    if (git_dirty_str) statusString += "!";
                                    statusString += " (" + juce::String(build_date_str) + ")";

                                    return choc::value::Value(statusString.toStdString());
                                }
            );

            connection.bind(
                    "juce_getCpuLoad",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        return choc::value::Value(processor.getCpuLoad());
                    }
            );

            connection.bind(
                    "juce_getUUID",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        return choc::value::Value(uuid.toString().toStdString());
                    });


            connection.bind("juce_getIsBeta",
                                [&](const choc::value::ValueView &args) -> choc::value::Value {
#ifdef BETA
                                    return choc::value::Value(true);
#else
                                    return choc::value::Value(false);
#endif
                                });

            connection.bind("juce_openURL",
                                [&](const choc::value::ValueView &args) -> choc::value::Value {
                                    juce::URL(args[0].getWithDefault("https://imagi.ro")).launchInDefaultBrowser();
                                    return {};
                                });

            connection.bind("juce_getTimeOpened",
                                [&](const choc::value::ValueView &args) -> choc::value::Value {
                                    const auto time = juce::SharedResourcePointer<Resources>()->getConfigFile()->getIntValue("timeOpened", 0);
                                    return choc::value::Value(time);
                                });

            connection.bind("juce_getProductSlug",
                                [&](const choc::value::ValueView &args) -> choc::value::Value {
                                    std::string productSlug;
#ifdef PRODUCT_SLUG
                                    productSlug = std::string(PRODUCT_SLUG);
#endif
                                    return choc::value::Value(productSlug);
                                });

            connection.bind("juce_getConfigFilePath",
                                [&](const choc::value::ValueView &args) -> choc::value::Value {
                                    auto resources = juce::SharedResourcePointer<Resources>();

                                    return choc::value::Value(resources->getConfigFile()->getFile().getFullPathName().toStdString());
                                });
        }



    private:
        Processor& processor;
        juce::Uuid uuid;
    };
}
