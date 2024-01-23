//
// Created by August Pemberton on 22/01/2024.
//

#pragma once

#include <AppCore/JSHelpers.h>

using CallbackFn = std::function<JSValue(const JSObject&, const JSArgs&)>;
namespace imagiro {
    class UltralightViewManager : public LoadListener {
    public:
        RefPtr<View> createView();
        void bind(const std::string& name, CallbackFn&& fn);
        void evaluateJS(const std::string& js);

        void removeView(RefPtr<View> v);

    private:
        std::vector<std::pair<std::string, CallbackFn>> bindFns;
        std::list<RefPtr<View>> activeViews;

        void OnWindowObjectReady(ultralight::View *caller, uint64_t frame_id, bool is_main_frame, const ultralight::String &url) override;
        static void bindFnToView(View& view, const std::string& name, CallbackFn& fn);
    };
}
