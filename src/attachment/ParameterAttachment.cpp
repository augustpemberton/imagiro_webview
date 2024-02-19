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
            "juce_updatePluginParameter",
            [&](const choc::value::ValueView &args) -> choc::value::Value {
                auto paramID = args[0].getWithDefault("");
                auto newValue01 = args[1].getWithDefault(0.);

                auto param = processor.getParameter(paramID);
                if (param) {
                    param->setValueNotifyingHost(newValue01);
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
                if (param) param->beginChangeGesture();
                return {};
            });
    webViewManager.bind(
            "juce_endPluginParameterGesture",
            [&](const choc::value::ValueView &args) -> choc::value::Value {
                auto param = processor.getParameter(args[0].toString());
                if (param) param->endChangeGesture();
                return {};
            });

    webViewManager.bind(
            "juce_getPluginParameters",
            [&](const choc::value::ValueView &args) -> choc::value::Value {
                return getAllParameterSpecValue();
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
            "juce_textToValue",
            [&](const choc::value::ValueView &args) -> choc::value::Value {
                auto paramID = juce::String(args[0].toString());
                auto displayValue = juce::String(args[1].toString());
                auto param = processor.getParameter(paramID);
                if (!param) return {};

                auto val = param->getConfig()->valueFunction(*param, displayValue);
                return choc::value::Value(val);
            });

    webViewManager.bind(
            "juce_setDisplayValue",
            [&](const choc::value::ValueView &args) -> choc::value::Value {
                auto paramID = juce::String(args[0].toString());
                auto displayValue = juce::String(args[1].toString());
                auto param = processor.getParameter(paramID);
                if (!param) return {};

                auto val = param->getConfig()->valueFunction(*param, displayValue);
                param->setUserValueAsUserAction(val);
                sendStateToBrowser(param);

                return {};
            }
    );

}

void imagiro::ParameterAttachment::parameterChangedSync(imagiro::Parameter *param) {
    sendStateToBrowser(param);
}

void imagiro::ParameterAttachment::sendStateToBrowser(imagiro::Parameter *param) {
    auto uid = param->getUID();
    auto value = param->getValue();

    juce::String s = "window.ui.updateParameterState(";
    s += "\""  + uid + "\"";
    s += ", ";
    s += juce::String(value);
    s += ")";

    this->webViewManager.evaluateJavascript(s.toStdString());
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
    paramSpec.setMember("uid", param->getUID().toStdString());
    paramSpec.setMember("name", param->getName(100).toStdString());
    paramSpec.setMember("value01", param->getValue());
    paramSpec.setMember("defaultVal01", param->getDefaultValue());
    paramSpec.setMember("locked", param->isLocked());

    auto range = choc::value::createObject("range");
    range.setMember("min", param->getUserRange().start);
    range.setMember("max", param->getUserRange().end);
    range.setMember("step", param->getUserRange().interval);
    range.setMember("skew", param->getUserRange().skew);

    paramSpec.setMember("range", range);
    return paramSpec;
}

