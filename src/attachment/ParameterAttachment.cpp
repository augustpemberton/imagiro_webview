//
// Created by August Pemberton on 28/10/2023.
//

#include "ParameterAttachment.h"
#include <imagiro_processor/imagiro_processor.h>

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
                auto payload = args[0];

                auto paramID = payload["uid"].toString();
                auto newValue01 = juce::String(payload["value01"].toString()).getFloatValue();

                auto param = processor.getParameter(paramID);
                if (param) {
                    juce::ScopedValueSetter<bool> svs(ignoreCallbacks, true);
                    param->setValueNotifyingHost(newValue01);
                }
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
                return getParameterSpec();
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
    if (!ignoreCallbacks)
        sendStateToBrowser(param);
}

void imagiro::ParameterAttachment::sendStateToBrowser(imagiro::Parameter *param) {
    auto uid = param->getUID();
    auto value = param->getValue();
    juce::MessageManager::callAsync([&, uid, value]() {
        auto *obj = new juce::DynamicObject();
        obj->setProperty("uid", uid);
        obj->setProperty("value01", value);

        juce::var json(obj);
        auto jsonString = juce::JSON::toString(json);
        this->webViewManager.evaluateJavascript("window.ui.updateParameterState(" + jsonString.toStdString() + ")");
    });
}

choc::value::Value imagiro::ParameterAttachment::getParameterSpec() {
    auto params = choc::value::createEmptyArray();
    for (auto param: processor.getPluginParameters()) {
        auto paramSpec = choc::value::createObject("param");
        paramSpec.setMember("uid", param->getUID().toStdString());
        paramSpec.setMember("name", param->getName(100).toStdString());
        paramSpec.setMember("value01", param->getValue());
        paramSpec.setMember("defaultVal01", param->getDefaultValue());

        auto range = choc::value::createObject("range");
        range.setMember("min", param->getUserRange().start);
        range.setMember("max", param->getUserRange().end);
        range.setMember("step", param->getUserRange().interval);

        paramSpec.setMember("range", range);
        params.addArrayElement(paramSpec);
    }
    return params;
}
