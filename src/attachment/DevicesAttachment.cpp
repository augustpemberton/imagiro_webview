//
// Created by August Pemberton on 04/04/2025.
//
#include "DevicesAttachment.h"

#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include "juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h"

namespace imagiro {
    DevicesAttachment::DevicesAttachment(UIConnection &c)
            : UIAttachment(c)
    {
        startTimer(100);
    }

    DevicesAttachment::~DevicesAttachment() {
        stopTimer();
        auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
        if (standaloneInstance) {
            standaloneInstance->deviceManager.removeChangeListener(this);
        }
    }

    void DevicesAttachment::addBindings() {
        connection.bind(
                "juce_showStandaloneAudioSettings",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                    if (!standaloneInstance) {
                        return {};
                    }
                    standaloneInstance->showAudioSettingsDialog();
                    return {};
                }
        );

        connection.bind(
                "juce_setAudioDeviceType",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                    if (!standaloneInstance) {
                        return {};
                    }
                    auto &deviceManager = standaloneInstance->deviceManager;
                    auto typeName = args[0].getWithDefault("");
                    deviceManager.setCurrentAudioDeviceType(typeName, true);
                    return {};
                });

        connection.bind(
                "juce_getAudioDeviceType",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                    if (!standaloneInstance) {
                        return {};
                    }
                    auto &deviceManager = standaloneInstance->deviceManager;
                    if (!deviceManager.getCurrentAudioDevice()) return choc::value::Value{std::string("")};
                    return choc::value::Value{deviceManager.getCurrentAudioDeviceType().toStdString()};
                });

        connection.bind(
                "juce_getAudioDeviceTypes",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                    if (!standaloneInstance) {
                        return choc::value::Value{"no standalone instance"};
                    }
                    auto &deviceManager = standaloneInstance->deviceManager;

                    auto types = choc::value::createEmptyArray();
                    for (auto &t: deviceManager.getAvailableDeviceTypes()) {
                        types.addArrayElement(t->getTypeName().toStdString());
                    }

                    return types;
                });

        connection.bind(
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

        connection.bind(
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

        connection.bind(
                "juce_getAvailableSampleRates",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                    if (!standaloneInstance) {
                        return {};
                    }

                    const auto &deviceManager = standaloneInstance->deviceManager;
                    if (!deviceManager.getCurrentAudioDevice()) return choc::value::createEmptyArray();
                    auto sampleRates = deviceManager.getCurrentAudioDevice()->getAvailableSampleRates();

                    auto rates = choc::value::createEmptyArray();
                    for (const auto &rate: sampleRates) {
                        rates.addArrayElement(rate);
                    }
                    return rates;
                });

        connection.bind(
                "juce_getAvailableBufferSizes",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                    if (!standaloneInstance) {
                        return {};
                    }
                    const auto &deviceManager = standaloneInstance->deviceManager;
                    if (!deviceManager.getCurrentAudioDevice()) return choc::value::createEmptyArray();
                    auto bufferSizes = deviceManager.getCurrentAudioDevice()->getAvailableBufferSizes();

                    auto sizes = choc::value::createEmptyArray();
                    for (const auto &size: bufferSizes) {
                        sizes.addArrayElement(size);
                    }
                    return sizes;
                });

        connection.bind(
                "juce_getAvailableOutputDevices",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                    if (!standaloneInstance) {
                        return {};
                    }

                    const auto &deviceManager = standaloneInstance->deviceManager;
                    const auto deviceType = deviceManager.getCurrentDeviceTypeObject();

                    auto outputs = choc::value::createEmptyArray();
                    for (const auto &output: deviceType->getDeviceNames(false)) {
                        outputs.addArrayElement(output.toStdString());
                    }
                    return outputs;
                });

        connection.bind(
                "juce_getAvailableInputDevices",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                    if (!standaloneInstance) {
                        return {};
                    }
                    auto &deviceManager = standaloneInstance->deviceManager;
                    auto deviceType = deviceManager.getCurrentDeviceTypeObject();

                    auto inputs = choc::value::createEmptyArray();
                    for (auto &input: deviceType->getDeviceNames(true)) {
                        inputs.addArrayElement(input.toStdString());
                    }
                    return inputs;
                });

        connection.bind(
                "juce_getAvailableMidiInputDevices",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                    if (!standaloneInstance) {
                        return {};
                    }
                    auto &deviceManager = standaloneInstance->deviceManager;
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

        connection.bind(
                "juce_setMidiInputDevicesEnabled",
                [&](const choc::value::ValueView &args) -> choc::value::Value {

                    auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
                    if (!standaloneInstance) {
                        return {};
                    }
                    auto &deviceManager = standaloneInstance->deviceManager;
                    auto devicesValue = args[0];
                    for (auto d: devicesValue) {
                        auto id = std::string(d["id"].getWithDefault(""));
                        const auto enabled = d["enabled"].getWithDefault(false);

                        deviceManager.setMidiInputDeviceEnabled(id, enabled);
                    }
                    return {};
                });

        connection.bind(
                "juce_getMillisecondTime",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    return choc::value::Value((juce::int64) juce::Time::getMillisecondCounter());
                });
    }

    void DevicesAttachment::changeListenerCallback(juce::ChangeBroadcaster *source) {
        connection.eval("window.ui.onDevicesChanged");
    }

    void DevicesAttachment::timerCallback() {
        // we have to do this after a delay because getInstance() is null at first
        // kinda hacky but WHATVAAAA
        auto standaloneInstance = juce::StandalonePluginHolder::getInstance();
        if (standaloneInstance) {
            standaloneInstance->deviceManager.addChangeListener(this);
        }

        stopTimer();
    }

}
