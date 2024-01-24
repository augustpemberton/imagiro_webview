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
        WebUIPluginEditor(WebProcessor &p, int defaultWidth = 400, int defaultHeight = 300, bool allowInspector = false)
                : AudioProcessorEditor(p),
                  processor(p),
                  view(p.getViewManager().createView(defaultWidth, defaultHeight)),
                  browser(view, WebProcessor::RENDERER, allowInspector)
        {
            setSize(defaultWidth, defaultHeight);
            addAndMakeVisible(browser);

#if JUCE_DEBUG
            browser.loadURL("http://localhost:4342");
#else
            browser.loadURL("file:///index.html");
#endif
        }

        ~WebUIPluginEditor() override {
            processor.getViewManager().removeView(view);
        }

        void resized() override {
            const auto b = getLocalBounds();
            browser.setBounds(b);
        }

    private:
        WebProcessor& processor;
        LoadListenerWrappedView& view;

        UltralightViewComponent browser;
    };
}
