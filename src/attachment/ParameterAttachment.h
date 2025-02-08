//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include<imagiro_processor/imagiro_processor.h>
#include "UIAttachment.h"

namespace imagiro {
    class ParameterAttachment : public UIAttachment, public Parameter::Listener {
    public:
        ParameterAttachment(UIConnection& connection, Processor& p);
        ~ParameterAttachment() override;

        void addListeners() override;
        void addBindings() override;

        void parameterChanged(Parameter *param) override;
        void configChanged(imagiro::Parameter *param) override;

    private:
        Processor& processor;
        void sendStateToBrowser(Parameter* param);
        choc::value::Value getAllParameterSpecValue();

        static choc::value::Value getParameterSpecValue(Parameter* param);
    };
} // namespace imagiro
