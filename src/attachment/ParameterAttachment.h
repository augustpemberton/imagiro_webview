//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include "WebUIAttachment.h"
#include <imagiro_processor/imagiro_processor.h>

namespace imagiro {
    class ParameterAttachment : public WebUIAttachment, public Parameter::Listener {
    public:
        ParameterAttachment(WebProcessor& p, WebViewManager& w);
        ~ParameterAttachment() override;

        void addListeners() override;
        void addBindings() override;

        void parameterChanged(Parameter *param) override;
        void configChanged(imagiro::Parameter *param) override;

    private:
        void sendStateToBrowser(Parameter* param);
        choc::value::Value getAllParameterSpecValue();

        static choc::value::Value getParameterSpecValue(Parameter* param);
    };
} // namespace imagiro
