//
// Created by August Pemberton on 11/05/2023.
//

#pragma once

#include "WebViewManager.h"
#include "juce_gui_extra/embedding/juce_NSViewComponent.h"
#include "juce_gui_extra/embedding/juce_HWNDComponent.h"


namespace imagiro {

    class ChocBrowserComponent
            : public juce::Timer,
#if JUCE_MAC
              public juce::NSViewComponent
#elif JUCE_WINDOWS
        public juce::HWNDComponent
#endif
    {
    public:
        ChocBrowserComponent(juce::AudioProcessorEditor& editor, WebViewManager &w,
                             bool sendMousePosition = false)
                : webViewManager(w), sendMousePos(sendMousePosition) {
            webView = webViewManager.getWebView(&editor);
#if JUCE_MAC
            setView(webView->getViewHandle());
#elif JUCE_WINDOWS
            setHWND(webView->getViewHandle());
#endif

            if (sendMousePos) startTimerHz(20);
            setOpaque(true);
        }

        void focusGained(juce::Component::FocusChangeType cause) override {
            stopTimer();
        }

        void focusLost(juce::Component::FocusChangeType cause) override {
            if (sendMousePos) startTimerHz(20);
        }

        ~ChocBrowserComponent() override {
            webViewManager.removeWebView(webView.get());
#if JUCE_MAC
            setView ({});
#elif JUCE_WINDOWS
            setHWND ({});
#endif
        }

        void timerCallback() override {
            auto source = juce::Desktop::getInstance().getMainMouseSource();
            auto mousePos = source.getScreenPosition();
            mousePos -= getScreenPosition().toFloat();
            lastMousePos = mousePos;

            auto js = "if (window.ui.juceMouseMove !== undefined) window.ui.juceMouseMove(" + juce::String(mousePos.x) + ", " + juce::String(mousePos.y) + ")";
            webView->evaluateJavascript(js.toStdString());
        }

        WebViewManager &getWebViewManager() { return webViewManager; }

    private:
        WebViewManager &webViewManager;
        std::shared_ptr<choc::ui::WebView> webView;
        juce::Point<float> lastMousePos;

        bool sendMousePos;
    };
}