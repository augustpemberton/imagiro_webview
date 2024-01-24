//
// Created by August Pemberton on 20/01/2024.
//

#pragma once
#include "LoadListenerWrappedView.h"

namespace imagiro {
    class InspectorWindow : public juce::DocumentWindow {
    public:
        InspectorWindow(RefPtr<View> view, RefPtr<Renderer> renderer,
                        std::function<void()> onCloseFn = [](){});

        void closeButtonPressed() override {
            onClose();
        }

    private:
        LoadListenerWrappedView loadWrappedView;
        std::function<void()> onClose;
    };
}
