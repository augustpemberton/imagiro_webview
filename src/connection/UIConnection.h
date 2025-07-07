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

        void bind(const std::string &functionName, CallbackFn&& callback) {
            auto callbackCopy = callback;
            boundFunctions[functionName] = callbackCopy;
            bindFunction(functionName, std::move(callback));
        }

        void eval(const std::string &functionName, const std::vector<choc::value::Value>& args = {}) {
            evalFunction(functionName, args);
        }

        const std::unordered_map<std::string, CallbackFn>& getBoundFunctions() { return boundFunctions; }

    protected:
        virtual void bindFunction(const std::string &functionName, CallbackFn&& callback) = 0;
        virtual void evalFunction(const std::string &functionName, const std::vector<choc::value::Value>& args = {}) = 0;
        std::unordered_map<std::string, CallbackFn> boundFunctions;
    };
}