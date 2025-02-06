//
// Created by August Pemberton on 28/10/2023.
//

#include "ParameterAttachment.h"
#include <imagiro_processor/imagiro_processor.h>

imagiro::ParameterAttachment::ParameterAttachment(imagiro::WebProcessor &p, imagiro::WebViewManager &w)
        : WebUIAttachment(p, w) {
}

void imagiro::ParameterAttachment::addListeners() {
    for (auto param: processor.getPluginParameters()) {
        param->addListener(this);
    }
}

imagiro::ParameterAttachment::~ParameterAttachment() {
    for (auto param: processor.getPluginParameters()) {
        param->removeListener(this);
    }
}

void imagiro::ParameterAttachment::addBindings() {
    webViewManager.bind(
            "juce_incrementPluginParameter",
            [&](const choc::value::ValueView &args) -> choc::value::Value {
                auto paramID = args[0].getWithDefault("");
                auto numSteps = args[1].getWithDefault(1.f);

                auto loop = false;
                if (args.size() > 2) loop = args[2].getWithDefault(false);

                auto param = processor.getParameter(paramID);

                if (param) {
                    auto v = param->getUserValue();
                    if (v >= param->getNormalisableRange().getRange().getEnd() && loop) {
                        v = param->getNormalisableRange().getRange().getStart();
                    } else {
                        auto positive = numSteps > 0;
                        auto interval = param->getNormalisableRange().interval;

                        auto currentVal01 = param->getValue();

                        auto minIncrementPercent = 0.01;

                        auto minVal01 = currentVal01 + (positive ? minIncrementPercent : -minIncrementPercent);
                        minVal01 = std::clamp((float)minVal01, 0.f, 1.f);
                        auto minVal = param->getNormalisableRange().convertFrom0to1(minVal01);

                        auto val = v + (interval * numSteps);

                        v = (positive ? std::max(val, minVal) : std::min(val, minVal));
                    }
                    param->setUserValueAndNotifyHost(v);
                }

                return choc::value::Value(param->getValue());
            });
    webViewManager.bind(
            "juce_updatePluginParameter",
            [&](const choc::value::ValueView &args) -> choc::value::Value {
                auto paramID = args[0].getWithDefault("");
                auto newValue01 = args[1].getWithDefault(0.);

                auto param = processor.getParameter(paramID);
                if (param) {
                    param->setValueAndNotifyHost(juce::jlimit(0.f, 1.f, (float)newValue01));

                }

                return choc::value::Value(param->getValue());
            });
    webViewManager.bind(
            "juce_setPluginParameterLocked",
            [&](const choc::value::ValueView &args) -> choc::value::Value {
                auto uid = args[0].getWithDefault("");
                auto locked = args[1].getWithDefault(false);

                auto param = processor.getParameter(uid);
                if (!param) return {};

                param->setLocked(locked);
                return {};
            });

    webViewManager.bind(
            "juce_startPluginParameterGesture",
            [&](const choc::value::ValueView &args) -> choc::value::Value {
                auto param = processor.getParameter(args[0].toString());
                if (param) param->beginUserAction();
                return {};
            });
    webViewManager.bind(
            "juce_endPluginParameterGesture",
            [&](const choc::value::ValueView &args) -> choc::value::Value {
                auto param = processor.getParameter(args[0].toString());
                if (param) param->endUserAction();
                return {};
            });

    webViewManager.bind(
            "juce_getPluginParameters",
            [&](const choc::value::ValueView &args) -> choc::value::Value {
                return getAllParameterSpecValue();
            });

    webViewManager.bind(
            "juce_getUserValue",
            [&](const choc::value::ValueView &args) -> choc::value::Value {
                auto param = processor.getParameter(args[0].toString());
                if (!param) return {};

                auto uv = param->getUserValue();
                return choc::value::Value(uv);
            });

    webViewManager.bind(
            "juce_getDisplayValue",
            [&](const choc::value::ValueView &args) -> choc::value::Value {
                auto param = processor.getParameter(args[0].toString());
                if (!param) return {};

                auto v = param->getValue();
                if (args.size() > 1) {
                    v = args[1].getWithDefault(0.f);
                }

                auto userVal = param->convertFrom0to1(v);
                auto val = choc::value::createObject("displayValue");
                auto displayVal = param->getDisplayValueForUserValue(userVal);
                val.setMember("value", displayVal.value.toStdString());
                val.setMember("suffix", displayVal.suffix.toStdString());

                return val;
            });

    webViewManager.bind(
            "juce_textToValue01",
            [&](const choc::value::ValueView &args) -> choc::value::Value {
                auto paramID = juce::String(args[0].toString());
                auto displayValue = juce::String(args[1].toString());
                auto param = processor.getParameter(paramID);
                if (!param) return {};

                auto val = param->getConfig()->valueFunction(displayValue);
                auto val01 = param->convertTo0to1(val);
                return choc::value::Value(val01);
            });

    webViewManager.bind(
            "juce_setDisplayValue",
            [&](const choc::value::ValueView &args) -> choc::value::Value {
                auto paramID = juce::String(args[0].toString());
                auto displayValue = juce::String(args[1].toString());
                auto param = processor.getParameter(paramID);
                if (!param) return {};

                auto val = param->getConfig()->valueFunction(displayValue);
                param->setUserValueAsUserAction(val);

                return {};
            }
    );

    webViewManager.bind(
            "juce_setConfig",
            [&](const choc::value::ValueView &args) -> choc::value::Value {
                auto paramID = juce::String(args[0].toString());
                auto configIndex = args[1].getWithDefault(0);

                auto param = processor.getParameter(paramID);
                if (!param) return {};

                param->setConfig(configIndex);

                return {};
            }
    );


    webViewManager.bind("juce_setParameterJitter", [&](const choc::value::ValueView &args) -> choc::value::Value {
        auto paramID = juce::String(args[0].toString());
        auto jitterAmount = args[1].getWithDefault(0.f);

        auto param = processor.getParameter(paramID);
        if (!param) return {};

        param->setJitterAmount(jitterAmount);

        return {};
    });

}

void imagiro::ParameterAttachment::parameterChanged(imagiro::Parameter *param) {
    sendStateToBrowser(param);
}

void imagiro::ParameterAttachment::sendStateToBrowser(imagiro::Parameter *param) {
    auto uid = param->getUID();
    auto value = param->getValue();

    std::string s = "window.ui.updateParameterState(";
    s += "\""  + uid + "\"";
    s += ", ";
    s += std::to_string(value);
    s += ")";

    this->webViewManager.evaluateJavascript(s);
}

choc::value::Value imagiro::ParameterAttachment::getAllParameterSpecValue() {
    auto params = choc::value::createEmptyArray();
    for (auto param: processor.getPluginParameters()) {
        params.addArrayElement(getParameterSpecValue(param));
    }
    return params;
}

void imagiro::ParameterAttachment::configChanged(imagiro::Parameter *param) {
    this->webViewManager.evaluateJavascript("window.ui.onParameterConfigChanged("
                                            + choc::json::toString(getParameterSpecValue(param)) + ")");
}

choc::value::Value imagiro::ParameterAttachment::getParameterSpecValue(imagiro::Parameter *param) {
    auto paramSpec = choc::value::createObject("param");
    paramSpec.setMember("uid", param->getUID());
    paramSpec.setMember("name", param->getName(100).toStdString());
    paramSpec.setMember("value01", param->getValue());
    paramSpec.setMember("defaultVal01", param->getDefaultValue());
    paramSpec.setMember("locked", param->isLocked());
    paramSpec.setMember("configIndex", param->getConfigIndex());
    paramSpec.setMember("modTargetID", (int)param->getModTarget().getID());
    paramSpec.setMember("jitter", param->getJitterAmount());

    auto configsArray = choc::value::createEmptyArray();
    for (const auto& config : param->configs) {
        auto configObject = choc::value::createObject("ParamConfig");
        configObject.setMember("name", config.name);

        // Convert std::vector<std::string> to choc::value::Value
        auto choicesArray = choc::value::createEmptyArray();
        for (const auto& choice : param->getConfig()->choices) {
            choicesArray.addArrayElement(choc::value::Value(choice));
        }
        configObject.setMember("choices", choicesArray);

        configsArray.addArrayElement(configObject);
    }

    paramSpec.setMember("configs", configsArray);

    return paramSpec;
}
