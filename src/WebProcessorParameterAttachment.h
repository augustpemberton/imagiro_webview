//
// Created by August Pemberton on 21/05/2023.
//

#pragma once
#include "WebViewManager.h"
#include "imagiro-processor/src/parameter/Parameter.h"

namespace imagiro {
    class WebProcessor;

    class WebProcessorParameterAttachment : public Parameter::Listener {
    public:
        WebProcessorParameterAttachment(WebProcessor& p, WebViewManager& w);
        ~WebProcessorParameterAttachment() override;

        void addWebBindings();
        void parameterChangedSync(imagiro::Parameter *param) override;

        void sendStateToBrowser(Parameter* param);

    private:
        WebProcessor& processor;
        WebViewManager& webView;

        choc::value::Value getParameterSpec();

        bool ignoreCallbacks {false};
    };
}
