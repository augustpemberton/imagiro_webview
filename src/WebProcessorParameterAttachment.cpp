#include "WebProcessorParameterAttachment.h"
#include "WebProcessor.h"

namespace imagiro {
    WebProcessorParameterAttachment::WebProcessorParameterAttachment(WebProcessor &p, WebViewManager &w)
            : processor(p), webView(w) {
        addWebBindings();

        for (auto param: processor.getPluginParameters()) {
            param->addListener(this);
        }
    }

    WebProcessorParameterAttachment::~WebProcessorParameterAttachment() {
        for (auto param: processor.getPluginParameters()) {
            param->removeListener(this);
        }
    }

    void WebProcessorParameterAttachment::addWebBindings() {
        webView.bind("requestFileChooser",
                     [&](const choc::value::ValueView& args) -> choc::value::Value {
                        webView.requestFileChooser(args[0].toString());
                        return {};
                     });

        webView.bind(
                "updatePluginParameter",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto payload = args[0];

                    auto paramID = payload["parameter"].toString();
                    auto newValue = juce::String(payload["value"].toString()).getFloatValue();

                    auto param = processor.getParameter(paramID);
                    if (param) {
                        juce::ScopedValueSetter<bool> svs(ignoreCallbacks, true);
                        param->setUserValueNotifingHost(newValue);
                    }
                    return {};
                });
        webView.bind(
                "startPluginParameterGesture",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto param = processor.getParameter(args[0].toString());
                    if (param) param->beginChangeGesture();
                    return {};
                });
        webView.bind(
                "endPluginParameterGesture",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto param = processor.getParameter(args[0].toString());
                    if (param) param->endChangeGesture();
                    return {};
                });

        webView.bind(
                "reloadPluginParameters",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    return getParameterSpec();
                });

        webView.bind(
                "getDisplayValue",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto param = processor.getParameter(args[0].toString());
                    if (!param) return {};

                    auto userVal = param->getUserValue();
                    if (args.size() > 1) {
                        auto valToUse = args[1].getFloat64();
                        userVal = valToUse;
                    }

                    auto val = choc::value::createObject("displayValue");
                    auto displayVal = param->getDisplayValueForUserValue(userVal);
                    val.setMember("value", displayVal.value.toStdString());
                    val.setMember("suffix", displayVal.suffix.toStdString());

                    return val;
                });

        webView.bind(
                "setDisplayValue",
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

    choc::value::Value WebProcessorParameterAttachment::getParameterSpec() {
        auto params = choc::value::createEmptyArray();
        for (auto param: processor.getPluginParameters()) {
            auto paramSpec = choc::value::createObject("param");
            paramSpec.setMember("uid", param->getUID().toStdString());
            paramSpec.setMember("name", param->getName(100).toStdString());
            paramSpec.setMember("value", param->getUserValue());
            paramSpec.setMember("defaultVal", param->getUserDefaultValue());

            auto range = choc::value::createObject("range");
            range.setMember("min", param->getUserRange().start);
            range.setMember("max", param->getUserRange().end);
            range.setMember("step", param->getUserRange().interval);

            paramSpec.setMember("range", range);
            params.addArrayElement(paramSpec);
        }
        return params;
    }

    void WebProcessorParameterAttachment::parameterChangedSync(Parameter *param) {
        if (!ignoreCallbacks)
            sendStateToBrowser(param);
    }

    void WebProcessorParameterAttachment::sendStateToBrowser(Parameter *param) {
        auto *obj = new juce::DynamicObject();
        obj->setProperty("parameter", param->getUID());
        obj->setProperty("value", param->getUserValue());

        juce::var json(obj);
        auto jsonString = juce::JSON::toString(json);
        webView.evaluateJavascript("updateParameterState(" + jsonString.toStdString() + ")");
    }

}