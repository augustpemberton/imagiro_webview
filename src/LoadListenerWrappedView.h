//
// Created by August Pemberton on 24/01/2024.
//

#pragma once
#include <Ultralight/Ultralight.h>

#include <utility>

using namespace ultralight;
class LoadListenerWrappedView : LoadListener {
public:
    LoadListenerWrappedView(RefPtr<View> v) : view(std::move(v)) {
        view->set_load_listener(this);
    }

    void addLoadListener(LoadListener* v) { loadListeners.add(v); }
    void removeLoadListener(LoadListener* v) { loadListeners.remove(v); }

    bool operator==(const LoadListenerWrappedView& other) const {
        return other.view == view;
    }


    void OnWindowObjectReady(ultralight::View *caller, uint64_t frame_id, bool is_main_frame,
                             const ultralight::String &url) override {
        loadListeners.call(&LoadListener::OnWindowObjectReady, caller, frame_id, is_main_frame, url);
    }

    void OnDOMReady(ultralight::View *caller, uint64_t frame_id,
                    bool is_main_frame, const ultralight::String &url) override {
        loadListeners.call(&LoadListener::OnDOMReady, caller, frame_id, is_main_frame, url);
    }

    RefPtr<View> getView() { return view; }

private:
    RefPtr<View> view;
    juce::ListenerList<LoadListener> loadListeners;
};
