//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include<imagiro_processor/imagiro_processor.h>
#include "UIAttachment.h"
#include "imagiro_processor/src/Processor.h"

namespace imagiro {
    class ParameterAttachment : public UIAttachment, public Parameter::Listener, Processor::ParameterListener {
    public:
        ParameterAttachment(UIConnection& connection, Processor& p);
        ~ParameterAttachment() override;

        void addListeners() override;
        void addBindings() override;

        void onParameterAdded(Parameter &p) override;

        void parameterChanged(Parameter *param) override;
        void configChanged(imagiro::Parameter *param) override;

    private:
        Processor& processor;
        void sendStateToBrowser(Parameter* param);
        choc::value::Value getAllParameterSpecValue();

        static choc::value::Value getParameterSpecValue(Parameter* param);
    };
} // namespace imagiro
