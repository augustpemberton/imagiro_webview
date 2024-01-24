//
// Created by August Pemberton on 22/01/2024.
//

#include "InspectorWindow.h"
#include "UltralightViewComponent.h"

namespace imagiro {
    InspectorWindow::InspectorWindow(RefPtr <View> view, RefPtr <Renderer> renderer,
                                     std::function<void()> onCloseFn)
            : DocumentWindow("Inspector", juce::Colours::white, allButtons, true),
              loadWrappedView(view),
              onClose(onCloseFn)
    {
        setUsingNativeTitleBar(true);
        setOpaque(true);

        setSize(1000, 1000);

        auto component = new UltralightViewComponent(loadWrappedView, renderer);
        component->setSize(1000, 1000);

        setContentOwned(component, true);
        setResizable(true, true);
    }
}