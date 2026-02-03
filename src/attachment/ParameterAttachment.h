//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include <imagiro_processor/processor/Processor.h>
#include <imagiro_processor/parameter/ParamController.h>
#include <sigslot/sigslot.h>
#include "UIAttachment.h"

namespace imagiro {
    class ParameterAttachment : public UIAttachment {
    public:
        ParameterAttachment(UIConnection& connection, Processor& p);
        ~ParameterAttachment() override = default;

        void addListeners() override;
        void addBindings() override;

    private:
        Processor& processor;
        std::vector<sigslot::scoped_connection> paramConnections_;

        void sendStateToBrowser(Handle h);
        choc::value::Value getAllParameterSpecValue();
        choc::value::Value getParameterSpecValue(Handle h);
    };
} // namespace imagiro
