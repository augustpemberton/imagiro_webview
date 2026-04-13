//
// Created by August Pemberton on 28/10/2023.
//

#include "ParameterAttachment.h"

namespace imagiro {

ParameterAttachment::ParameterAttachment(UIConnection &w, Processor& p)
    : UIAttachment(w), processor(p) {
}

void ParameterAttachment::addListeners() {
    processor.params().forEach([this](Handle h, const ParamConfig& config) {
        paramConnections_.push_back(
            processor.params().uiSignal(h).connect_scoped([this, h](float) {
                sendStateToBrowser(h);
            })
        );
    });
}

void ParameterAttachment::addBindings() {
    connection.bind(
        "juce_incrementPluginParameter",
        [&](const choc::value::ValueView &args) -> choc::value::Value {
            auto paramID = args[0].getWithDefault("");
            auto numSteps = args[1].getWithDefault(1.f);
            auto loop = args.size() > 2 ? args[2].getWithDefault(false) : false;

            if (!processor.params().has(paramID)) return {};
            auto h = processor.params().handle(paramID);
            const auto& config = processor.params().config(h);

            auto v = processor.params().getValue(h);
            if (v >= config.range.max_ && loop) {
                v = config.range.min_;
            } else {
                auto positive = numSteps > 0;
                auto interval = config.range.step_;

                // For choice parameters, use step size of 1
                if (config.range.isDiscrete() && config.range.numSteps() > 1) {
                    interval = 1.f;
                }

                if (interval <= 0.f) {
                    // For continuous parameters, use a small percentage
                    interval = config.range.length() * 0.01f;
                }

                auto currentVal01 = processor.params().getValue01(h);
                auto minIncrementPercent = 0.01;
                auto minVal01 = currentVal01 + (positive ? minIncrementPercent : -minIncrementPercent);
                minVal01 = std::clamp((float)minVal01, 0.f, 1.f);
                auto minVal = config.range.denormalize(minVal01);

                auto val = v + (interval * numSteps);
                v = (positive ? std::max(val, minVal) : std::min(val, minVal));
            }

            processor.juceAdapter()->setValueAsUserAction(h, v);
            // Return the new normalized value, not the stale one from getValue01
            auto newVal01 = config.range.normalize(v);
            return choc::value::Value(newVal01);
        });

    connection.bind(
        "juce_setPluginParameterValue01",
        [&](const choc::value::ValueView &args) -> choc::value::Value {
            auto paramID = args[0].getWithDefault("");
            auto newValue01 = args[1].getWithDefault(0.);

            if (!processor.params().has(paramID)) return {};
            auto h = processor.params().handle(paramID);

            auto clampedVal01 = std::clamp((float)newValue01, 0.f, 1.f);
            processor.params().setValue01(h, clampedVal01);
            processor.juceAdapter()->pushToHost(h);
            // Return the new normalized value, not the stale one from getValue01
            return choc::value::Value(clampedVal01);
        });

    connection.bind(
        "juce_setPluginParameterUserValue",
        [&](const choc::value::ValueView &args) -> choc::value::Value {
            auto paramID = args[0].getWithDefault("");
            auto newUserValue = args[1].getWithDefault(0.);

            if (!processor.params().has(paramID)) return {};
            auto h = processor.params().handle(paramID);
            const auto& config = processor.params().config(h);

            processor.juceAdapter()->setValueAsUserAction(h, static_cast<float>(newUserValue));
            // Return the new normalized value, not the stale one from getValue01
            auto newVal01 = config.range.normalize(static_cast<float>(newUserValue));
            return choc::value::Value(newVal01);
        });

    connection.bind(
        "juce_startPluginParameterGesture",
        [&](const choc::value::ValueView &args) -> choc::value::Value {
            auto paramID = args[0].toString();
            if (!processor.params().has(paramID)) return {};
            auto h = processor.params().handle(paramID);
            processor.juceAdapter()->beginGesture(h);
            return {};
        });

    connection.bind(
        "juce_endPluginParameterGesture",
        [&](const choc::value::ValueView &args) -> choc::value::Value {
            auto paramID = args[0].toString();
            if (!processor.params().has(paramID)) return {};
            auto h = processor.params().handle(paramID);
            processor.juceAdapter()->endGesture(h);
            return {};
        });

    connection.bind(
        "juce_getPluginParameters",
        [&](const choc::value::ValueView &args) -> choc::value::Value {
            return getAllParameterSpecValue();
        });

    connection.bind(
        "juce_getPluginParameter",
        [&](const choc::value::ValueView &args) -> choc::value::Value {
            auto paramID = args[0].toString();
            if (!processor.params().has(paramID)) return {};
            auto h = processor.params().handle(paramID);
            return getParameterSpecValue(h);
        });

    connection.bind(
        "juce_getUserValue",
        [&](const choc::value::ValueView &args) -> choc::value::Value {
            auto paramID = args[0].toString();
            if (!processor.params().has(paramID)) return {};
            auto h = processor.params().handle(paramID);
            const auto& config = processor.params().config(h);

            auto v01 = processor.params().getValue01(h);
            if (args.size() >= 2) v01 = args[1].getWithDefault(0.f);
            auto uv = config.range.denormalize(v01);

            return choc::value::Value(uv);
        });

    connection.bind(
        "juce_getDisplayValue",
        [&](const choc::value::ValueView &args) -> choc::value::Value {
            auto paramID = args[0].toString();
            if (!processor.params().has(paramID)) return {};
            auto h = processor.params().handle(paramID);
            const auto& config = processor.params().config(h);

            auto v01 = processor.params().getValue01(h);
            if (args.size() > 1) {
                v01 = args[1].getWithDefault(0.f);
            }

            auto userVal = config.range.denormalize(v01);
            auto displayStr = config.format.toString(userVal);

            auto val = choc::value::createObject("displayValue");
            val.setMember("value", displayStr);
            val.setMember("suffix", ""); // Suffix is now part of the formatted string
            return val;
        });

    connection.bind(
        "juce_textToValue01",
        [&](const choc::value::ValueView &args) -> choc::value::Value {
            auto paramID = std::string(args[0].toString());
            auto displayValue = std::string(args[1].toString());

            if (!processor.params().has(paramID)) return {};
            auto h = processor.params().handle(paramID);
            const auto& config = processor.params().config(h);

            auto parsed = config.format.fromString(displayValue);
            if (!parsed) return {};

            auto val01 = config.range.normalize(*parsed);
            return choc::value::Value(val01);
        });

    connection.bind(
        "juce_setDisplayValue",
        [&](const choc::value::ValueView &args) -> choc::value::Value {
            auto paramID = std::string(args[0].toString());
            auto displayValue = std::string(args[1].toString());

            if (!processor.params().has(paramID)) return {};
            auto h = processor.params().handle(paramID);
            const auto& config = processor.params().config(h);

            auto parsed = config.format.fromString(displayValue);
            if (!parsed) return {};

            processor.juceAdapter()->setValueAsUserAction(h, *parsed);
            return {};
        });

    connection.bind(
        "juce_setPluginParameterLocked",
        [&](const choc::value::ValueView &args) -> choc::value::Value {
            auto paramID = args[0].getWithDefault("");
            auto locked = args[1].getWithDefault(false);

            if (!processor.params().has(paramID)) return {};
            auto h = processor.params().handle(paramID);
            processor.params().setLocked(h, locked);
            return {};
        });
}

void ParameterAttachment::sendStateToBrowser(Handle h) {
    const auto& config = processor.params().config(h);
    auto uid = choc::value::Value(config.uid);
    auto value = choc::value::Value(processor.params().getValue01(h));
    connection.eval("window.ui.updateParameterState", {uid, value});
}

choc::value::Value ParameterAttachment::getAllParameterSpecValue() {
    auto params = choc::value::createEmptyArray();
    processor.params().forEach([&](Handle h, const ParamConfig& config) {
        params.addArrayElement(getParameterSpecValue(h));
    });
    return params;
}

choc::value::Value ParameterAttachment::getParameterSpecValue(Handle h) {
    const auto& config = processor.params().config(h);

    auto paramSpec = choc::value::createObject("param");
    paramSpec.setMember("uid", config.uid);
    paramSpec.setMember("name", config.name);
    paramSpec.setMember("value01", processor.params().getValue01(h));
    paramSpec.setMember("defaultVal01", config.range.normalize(config.defaultValue));
    paramSpec.setMember("locked", processor.params().isLocked(h));

    return paramSpec;
}

} // namespace imagiro
