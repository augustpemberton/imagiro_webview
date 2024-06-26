//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include <timestamp.h>
#include <imagiro_processor/imagiro_processor.h>
#include "WebUIAttachment.h"

#include "juce_audio_devices/juce_audio_devices.h"
#include "juce_audio_plugin_client/juce_audio_plugin_client.h"
#include "juce_audio_utils/juce_audio_utils.h"
#include "juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h"

namespace imagiro {
    class PluginInfoAttachment : public WebUIAttachment, juce::ChangeListener,
                                 juce::Timer {
    public:
        PluginInfoAttachment(WebProcessor& p, WebViewManager& w)
                : WebUIAttachment(p, w)
        {
            startTimer(100);
        }

        void timerCallback() override {
            // we have to do this after a delay because getInstance() is null at first
            // kinda hacky but WHATVAAAA
            auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
            if (standaloneInstance) {
                standaloneInstance->deviceManager.addChangeListener(this);
            }

            stopTimer();
        }

        ~PluginInfoAttachment() override {
            auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
            if (standaloneInstance) {
                standaloneInstance->deviceManager.removeChangeListener(this);
            }
        }

        void changeListenerCallback(juce::ChangeBroadcaster *source) override {
            webViewManager.evaluateJavascript("window.ui.onDevicesChanged()");
        }

        void addBindings() override {
            webViewManager.bind(
                    "juce_getCurrentVersion",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        return choc::value::Value(processor.getCurrentVersion().toStdString());
                    }
            );

            webViewManager.bind(
                    "juce_showStandaloneAudioSettings",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        if (!juce::StandalonePluginHolder::getInstance()) return {};
                        juce::StandalonePluginHolder::getInstance()->showAudioSettingsDialog();
                        return {};
                    }
            );

            webViewManager.bind(
                    "juce_setAudioDeviceType",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                        if (!standaloneInstance) {
                            return {};
                        }
                        auto &deviceManager = juce::StandalonePluginHolder::getInstance()->deviceManager;
                        auto typeName = args[0].getWithDefault("");
                        deviceManager.setCurrentAudioDeviceType(typeName, true);
                        return {};
                    });

            webViewManager.bind(
                    "juce_getAudioDeviceType",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                        if (!standaloneInstance) {
                            return {};
                        }
                        auto &deviceManager = juce::StandalonePluginHolder::getInstance()->deviceManager;
                        if (!deviceManager.getCurrentAudioDevice()) return choc::value::Value{std::string("")};
                        return choc::value::Value{deviceManager.getCurrentAudioDeviceType().toStdString()};
                    });

            webViewManager.bind(
                    "juce_getAudioDeviceTypes",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                        if (!standaloneInstance) {
                            return {};
                        }
                        auto &deviceManager = juce::StandalonePluginHolder::getInstance()->deviceManager;

                        auto types = choc::value::createEmptyArray();
                        for (auto &t: deviceManager.getAvailableDeviceTypes()) {
                            types.addArrayElement(t->getTypeName().toStdString());
                        }

                        return types;
                    });

            webViewManager.bind(
                    "juce_getAudioDeviceSetup",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                        if (!standaloneInstance) {
                            return {};
                        }

                        const auto &deviceManager = standaloneInstance->deviceManager;
                        const auto currentSetup = deviceManager.getAudioDeviceSetup();

                        auto setupValue = choc::value::createObject("AudioDeviceSetup");
                        setupValue.setMember("outputDeviceName",
                                             choc::value::Value(currentSetup.outputDeviceName.toStdString()));
                        setupValue.setMember("inputDeviceName",
                                             choc::value::Value(currentSetup.inputDeviceName.toStdString()));
                        setupValue.setMember("bufferSize", choc::value::Value(currentSetup.bufferSize));
                        setupValue.setMember("sampleRate", choc::value::Value(currentSetup.sampleRate));

                        return setupValue;
                    });

            webViewManager.bind(
                    "juce_setAudioDeviceSetup",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                        if (!standaloneInstance) {
                            return {};
                        }

                        const auto setupObj = args[0];
                        auto &deviceManager = standaloneInstance->deviceManager;
                        const auto currentSetup = deviceManager.getAudioDeviceSetup();

                        auto newOutputDevice = std::string(setupObj["outputDeviceName"].getString());
                        auto newInputDevice = std::string(setupObj["inputDeviceName"].getString());

                        const juce::AudioDeviceManager::AudioDeviceSetup setup{
                                juce::String(newOutputDevice),
                                juce::String(newInputDevice),
                                setupObj["sampleRate"].getWithDefault(48000.0),
                                setupObj["bufferSize"].getWithDefault(512),
                                currentSetup.inputChannels,
                                currentSetup.useDefaultInputChannels,
                                currentSetup.outputChannels,
                                currentSetup.useDefaultOutputChannels
                        };

                        const auto error = deviceManager.setAudioDeviceSetup(setup, true);
                        if (error.isNotEmpty()) {
                            DBG(error);
                        }
                        return {};
                    });

            webViewManager.bind(
                    "juce_getAvailableSampleRates",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                        if (!standaloneInstance) {
                            return {};
                        }

                        const auto &deviceManager = juce::StandalonePluginHolder::getInstance()->deviceManager;
                        if (!deviceManager.getCurrentAudioDevice()) return choc::value::createEmptyArray();
                        auto sampleRates = deviceManager.getCurrentAudioDevice()->getAvailableSampleRates();

                        auto rates = choc::value::createEmptyArray();
                        for (const auto &rate: sampleRates) {
                            rates.addArrayElement(rate);
                        }
                        return rates;
                    });

            webViewManager.bind(
                    "juce_getAvailableBufferSizes",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                        if (!standaloneInstance) {
                            return {};
                        }
                        const auto &deviceManager = juce::StandalonePluginHolder::getInstance()->deviceManager;
                        if (!deviceManager.getCurrentAudioDevice()) return choc::value::createEmptyArray();
                        auto bufferSizes = deviceManager.getCurrentAudioDevice()->getAvailableBufferSizes();

                        auto sizes = choc::value::createEmptyArray();
                        for (const auto &size: bufferSizes) {
                            sizes.addArrayElement(size);
                        }
                        return sizes;
                    });

            webViewManager.bind(
                    "juce_getAvailableOutputDevices",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                        if (!standaloneInstance) {
                            return {};
                        }

                        const auto &deviceManager = juce::StandalonePluginHolder::getInstance()->deviceManager;
                        const auto deviceType = deviceManager.getCurrentDeviceTypeObject();

                        auto outputs = choc::value::createEmptyArray();
                        for (const auto &output: deviceType->getDeviceNames(false)) {
                            outputs.addArrayElement(output.toStdString());
                        }
                        return outputs;
                    });

            webViewManager.bind(
                    "juce_getAvailableInputDevices",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                        if (!standaloneInstance) {
                            return {};
                        }
                        auto &deviceManager = juce::StandalonePluginHolder::getInstance()->deviceManager;
                        auto deviceType = deviceManager.getCurrentDeviceTypeObject();

                        auto inputs = choc::value::createEmptyArray();
                        for (auto &input: deviceType->getDeviceNames(true)) {
                            inputs.addArrayElement(input.toStdString());
                        }
                        return inputs;
                    });

            webViewManager.bind(
                    "juce_getAvailableMidiInputDevices",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                        if (!standaloneInstance) {
                            return {};
                        }
                        auto &deviceManager = juce::StandalonePluginHolder::getInstance()->deviceManager;
                        auto midiInputs = juce::MidiInput::getAvailableDevices();

                        auto inputs = choc::value::createEmptyArray();
                        for (auto &input: midiInputs) {
                            auto val = choc::value::createObject("MidiInput");
                            val.addMember("name", input.name.toStdString());
                            val.addMember("id", input.identifier.toStdString());
                            val.addMember("enabled", deviceManager.isMidiInputDeviceEnabled(input.identifier));
                            inputs.addArrayElement(val);
                        }
                        return inputs;
                    });

            webViewManager.bind(
                    "juce_setMidiInputDevicesEnabled",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {

                        auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                        if (!standaloneInstance) {
                            return {};
                        }
                        auto &deviceManager = juce::StandalonePluginHolder::getInstance()->deviceManager;
                        auto devicesValue = args[0];
                        for (auto d: devicesValue) {
                            auto id = std::string(d["id"].getWithDefault(""));
                            const auto enabled = d["enabled"].getWithDefault(false);

                            deviceManager.setMidiInputDeviceEnabled(id, enabled);
                        }
                        return {};
                    });

            webViewManager.bind(
                    "juce_getMillisecondTime",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        return choc::value::Value((juce::int64) juce::Time::getMillisecondCounter());
                    });

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
                        return choc::value::Value(JucePlugin_Name);
                    }
            );

            webViewManager.bind("juce_getIsDebug",
                                [&](const choc::value::ValueView &args) -> choc::value::Value {
#if JUCE_DEBUG
                                    return choc::value::Value(true);
#else
                                    return choc::value::Value(false);
#endif
                                });

            webViewManager.bind("juce_getPlatform",
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

            webViewManager.bind("juce_getDebugVersionString",
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

            webViewManager.bind(
                    "juce_getCpuLoad",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        return choc::value::Value(processor.getCpuLoad());
                    }
            );

            webViewManager.bind(
                    "juce_getUUID",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        return choc::value::Value(uuid.toString().toStdString());
                    });


            webViewManager.bind("juce_getIsBeta",
                                [&](const choc::value::ValueView &args) -> choc::value::Value {
#ifdef BETA
                                    return choc::value::Value(true);
#else
                                    return choc::value::Value(false);
#endif
                                });

            webViewManager.bind("juce_getDefaultWindowSize",
                                [&](const choc::value::ValueView &args) -> choc::value::Value {
                                    const auto size = processor.getDefaultWindowSize();
                                    auto state = choc::value::createObject("WindowSize");
                                    state.setMember("x", size.x);
                                    state.setMember("y", size.y);
                                    return state;
                                });

            webViewManager.bind("juce_openURL",
                                [&](const choc::value::ValueView &args) -> choc::value::Value {
                                    juce::URL(args[0].getWithDefault("https://imagi.ro")).launchInDefaultBrowser();
                                    return {};
                                });

            webViewManager.bind("juce_getTimeOpened",
                                [&](const choc::value::ValueView &args) -> choc::value::Value {
                                    const auto time = juce::SharedResourcePointer<Resources>()->getConfigFile()->getIntValue("timeOpened", 0);
                                    return choc::value::Value(time);
                                });

            webViewManager.bind("juce_getProductSlug",
                                [&](const choc::value::ValueView &args) -> choc::value::Value {
                                    std::string productSlug;
#ifdef PRODUCT_SLUG
                                    productSlug = std::string(PRODUCT_SLUG);
#endif
                                    return choc::value::Value(productSlug);
                                });
        }



    private:
        juce::Uuid uuid;
    };
}
