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
    viewManager.bind(
            "juce_updatePluginParameter",
            [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                auto uid = getStdString(args[0]);
                auto value01 = args[1].ToNumber();

                auto param = processor.getParameter(uid);
                if (param) {
                    juce::ScopedValueSetter<Parameter*> svs (ignoreCallbackParam, param);
                    param->setValueNotifyingHost(static_cast<float>(value01));
                }

                return param ? param->getValue() : 0;
            });

    viewManager.bind(
            "juce_startPluginParameterGesture",
            [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                auto param = processor.getParameter(getStdString(args[0]));
                if (param) param->beginChangeGesture();
                return {};
            });
    viewManager.bind(
            "juce_endPluginParameterGesture",
            [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                auto param = processor.getParameter(getStdString(args[0]));
                if (param) param->endChangeGesture();
                return {};
            });

    viewManager.bind(
            "juce_getPluginParameters",
            [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                return {getAllParameterSpecValue()};
            });

    viewManager.bind(
            "juce_getDisplayValue",
            [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                auto param = processor.getParameter(getStdString(args[0]));
                if (!param) return {};

                auto v = param->getValue();
                if (args.size() > 1) {
                    v = static_cast<float>(args[1].ToNumber());
                }

                auto userVal = param->convertFrom0to1(v);

                auto displayVal = param->getDisplayValueForUserValue(userVal);

                JSObject val;
                val["value"] = displayVal.value.toRawUTF8();
                val["suffix"] = displayVal.suffix.toRawUTF8();
                return {val};
            });

    viewManager.bind(
            "juce_textToValue",
            [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                auto param = processor.getParameter(getStdString(args[0]));
                auto displayValue = getStdString(args[1]);
                if (!param) return {};

                auto val = param->getConfig()->valueFunction(*param, displayValue);
                return {val};
            });

    viewManager.bind(
            "juce_setDisplayValue",
            [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                auto param = processor.getParameter(getStdString(args[0]));
                if (!param) return {};

                auto displayValue = getStdString(args[1]);
                auto val = param->getConfig()->valueFunction(*param, displayValue);
                param->setUserValueAsUserAction(val);
                sendStateToBrowser(param);

                return {};
            }
    );

}

void imagiro::ParameterAttachment::parameterChangedSync(imagiro::Parameter *param) {
    if (ignoreCallbackParam != param)
        sendStateToBrowser(param);
}

void imagiro::ParameterAttachment::sendStateToBrowser(imagiro::Parameter *param) {
    auto uid = param->getUID().toRawUTF8();
    auto value = param->getValue();
    juce::MessageManager::callAsync([&, uid, value]() {
        JSArgs args {uid, value};
        this->viewManager.evaluateWindowFunction("updateParameterState", args);
    });
}

JSArray imagiro::ParameterAttachment::getAllParameterSpecValue() {
    JSArray params;
    for (auto param: processor.getPluginParameters()) {
        params.push({getParameterSpecValue(param)});
    }
    return params;
}

void imagiro::ParameterAttachment::configChanged(imagiro::Parameter *param) {
    auto specValue = getParameterSpecValue(param);

    JSArgs args {{getParameterSpecValue(param)}};
    this->viewManager.evaluateWindowFunction("onParameterConfigChanged", args);
}

JSObject imagiro::ParameterAttachment::getParameterSpecValue(imagiro::Parameter *param) {
    JSObject paramSpec;
    paramSpec["uid"] = param->getUID().toRawUTF8();
    paramSpec["name"] = param->getName(100).toRawUTF8();
    paramSpec["value01"] = param->getValue();
    paramSpec["defaultVal01"] = param->getDefaultValue();

    JSObject range;
    range["min"] = param->getUserRange().start;
    range["max"] = param->getUserRange().end;
    range["step"] = param->getUserRange().interval;
    range["skew"] = param->getUserRange().skew;

    paramSpec["range"] = (JSObjectRef) range;
    return paramSpec;
}
