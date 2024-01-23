//
// Created by August Pemberton on 19/05/2023.
//

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "WebProcessor.h"
#include "UltralightViewComponent.h"
#include "InspectorWindow.h"
#include "UltralightViewManager.h"

namespace imagiro {
    class WebUIPluginEditor : public juce::AudioProcessorEditor, ViewListener {
    public:
        WebUIPluginEditor(WebProcessor &p, bool allowInspector = false)
                : AudioProcessorEditor(p),
                  processor(p),
                  view(p.getViewManager().createView()),
                  browser(view, WebProcessor::RENDERER, allowInspector)
        {
            setSize(400, 300);
            addAndMakeVisible(browser);

//            browser.loadURL("file:///index.html");
            browser.loadURL("http://localhost:4342");
        }

        ~WebUIPluginEditor() {
            processor.getViewManager().removeView(view);
        }

        void resized() override {
            const auto b = getLocalBounds();
            browser.setBounds(b);
        }

    private:
        WebProcessor& processor;
        RefPtr<View> view;

        UltralightViewComponent browser;
    };
}
