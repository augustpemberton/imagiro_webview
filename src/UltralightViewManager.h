//
// Created by August Pemberton on 22/01/2024.
//

#pragma once

#include <AppCore/JSHelpers.h>
#include "LoadListenerWrappedView.h"

using CallbackFn = std::function<JSValue(const JSObject&, const JSArgs&)>;
namespace imagiro {
    class UltralightViewManager : public LoadListener {
    public:
        LoadListenerWrappedView& createView(int width = 0, int height = 0);
        void bind(const std::string& name, CallbackFn&& fn);
        void evaluateJS(const std::string& js);

        void removeView(LoadListenerWrappedView& v);

        std::optional<JSValueRef> evaluateWindowFunction(const std::string& functionName, const JSArgs& args = {});

    private:
        std::vector<std::pair<std::string, CallbackFn>> bindFns;
        std::list<LoadListenerWrappedView> activeViews;

        void OnWindowObjectReady(ultralight::View *caller, uint64_t frame_id, bool is_main_frame, const ultralight::String &url) override;
        static void bindFnToView(View& view, const std::string& name, CallbackFn& fn);
    };
}
