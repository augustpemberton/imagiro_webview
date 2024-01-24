//
// Created by August Pemberton on 22/01/2024.
//

#include "UltralightViewManager.h"

namespace imagiro {
    RefPtr<View> UltralightViewManager::createView() {
        auto& renderer = WebProcessor::RENDERER;
        activeViews.push_back(UltralightUtil::CreateView(renderer));
        auto& view = activeViews.back();
        view->set_load_listener(this);
        return view;
    }

    void UltralightViewManager::removeView(RefPtr<ultralight::View> v) {
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
            bindFnToView(*view, name, fn);
        }
    }

    void UltralightViewManager::evaluateJS(const std::string& js) {
        juce::MessageManager::callAsync([&]() {
            for (auto &view: activeViews) {
                view->EvaluateScript(js.c_str());
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
            return UltralightUtil::evaluateWindowFunctionInContext(*view->LockJSContext(), functionName, args);
        }
    }
}