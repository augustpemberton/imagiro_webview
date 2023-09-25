//
// Created by August Pemberton on 30/05/2023.
//

#pragma once
#include "ChocBrowserComponent.h"

namespace imagiro {
    class WebUIDebugToolbar : public juce::Component {
    public:
        WebUIDebugToolbar(ChocBrowserComponent &b)
                : browser(b) {
            addAndMakeVisible(reloadButton);

            reloadButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);

            reloadButton.onClick = [&]() {
                browser.getWebViewManager().reload();
            };
        }

        void resized() override {
            auto b = getLocalBounds();
            reloadButton.setBounds(b.removeFromLeft(50));
        }

    private:
        ChocBrowserComponent &browser;
        juce::TextButton reloadButton {"reload"};
    };
}