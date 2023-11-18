//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include "WebUIAttachment.h"
#include <imagiro_processor/imagiro_processor.h>

namespace imagiro {
    class ParameterAttachment : public WebUIAttachment, public Parameter::Listener {
    public:
        using WebUIAttachment::WebUIAttachment;
        ~ParameterAttachment() override;

        void addListeners() override;
        void addBindings() override;

        void parameterChangedSync(Parameter *param) override;

    private:
        void sendStateToBrowser(Parameter* param);
        choc::value::Value getParameterSpec();

        bool ignoreCallbacks {false};
    };
} // namespace imagiro
