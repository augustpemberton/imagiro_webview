//
// Created by August Pemberton on 22/01/2024.
//

#include "UltralightViewManager.h"

namespace imagiro {
    LoadListenerWrappedView& UltralightViewManager::createView(int width, int height) {
        auto& renderer = WebProcessor::RENDERER;
        activeViews.emplace_back(UltralightUtil::CreateView(renderer, width, height));
        auto& view = activeViews.back();
        view.addLoadListener(this);
        return view;
    }

    void UltralightViewManager::removeView(LoadListenerWrappedView& v) {
        v.removeLoadListener(this);
        activeViews.remove(v);
    }

    void UltralightViewManager::OnWindowObjectReady(ultralight::View *caller, uint64_t frame_id,
                                                    bool is_main_frame, const String &url) {
        for (auto& [name, fn] : bindFns) {
            bindFnToView(*caller, name, fn);
        }
    }

    void UltralightViewManager::bind(const std::string& name, CallbackFn &&fn) {
        bindFns.emplace_back(name, fn);
        for (auto& view : activeViews) {
            bindFnToView(*view.getView(), name, fn);
        }
    }

    void UltralightViewManager::evaluateJS(const std::string& js) {
        juce::MessageManager::callAsync([&]() {
            for (auto &view: activeViews) {
                view.getView()->EvaluateScript(js.c_str());
            }
        });
    }

    void UltralightViewManager::bindFnToView(ultralight::View &view, const std::string& name, CallbackFn& fn) {
        RefPtr<JSContext> context = view.LockJSContext();
        SetJSContext(context->ctx());

        JSObject global = JSGlobalObject();
        global[name.c_str()] = (JSCallbackWithRetval)fn;
    }
    std::optional<JSValueRef> UltralightViewManager::evaluateWindowFunction(const std::string& functionName, const JSArgs& args) {
        for (auto& view : activeViews) {
            return UltralightUtil::evaluateWindowFunctionInContext(*view.getView()->LockJSContext(), functionName, args);
        }
    }
}