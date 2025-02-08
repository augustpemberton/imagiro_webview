//
// Created by August Pemberton on 08/02/2025.
//

#pragma once
#include <functional>
#include <choc/containers/choc_Value.h>

namespace imagiro {
    class UIConnection {
    public:
        virtual ~UIConnection() = default;
        typedef std::function<choc::value::Value(const choc::value::ValueView &args)> CallbackFn;

        virtual void bind(const std::string &functionName, CallbackFn&& callback) = 0;
        virtual void eval(const std::string &functionName, const std::vector<choc::value::ValueView>& args = {}) = 0;
    };
}