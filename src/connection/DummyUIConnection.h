//
// Created by August Pemberton on 10/02/2025.
//

#pragma once
#include "UIConnection.h"

namespace imagiro {
    class DummyUIConnection : public UIConnection {
    public:
        void bindFunction(const std::string &functionName, CallbackFn&& callback) override {
            //
        };
        void evalFunction(const std::string &functionName, const std::vector<choc::value::ValueView>& args) {
            //
        }
    };
}
