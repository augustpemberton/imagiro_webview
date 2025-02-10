//
// Created by August Pemberton on 10/02/2025.
//

#pragma once
#include "UIConnection.h"

namespace imagiro {
    class DummyUIConnection : public UIConnection {
    public:
        void bind(const std::string &functionName, CallbackFn&& callback) override {
            //
        };
        void eval(const std::string &functionName, const std::vector<choc::value::ValueView>& args = {}) {
            //
        }
    };
}
