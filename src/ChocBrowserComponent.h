//
// Created by August Pemberton on 11/05/2023.
//

#pragma once

#include "WebViewManager.h"
#include "juce_gui_extra/embedding/juce_NSViewComponent.h"
#include "juce_gui_extra/embedding/juce_HWNDComponent.h"


namespace imagiro {

    class ChocBrowserComponent
            :
#if JUCE_MAC
              public juce::NSViewComponent
#elif JUCE_WINDOWS
        public juce::HWNDComponent
#endif
    {
    public:
        ChocBrowserComponent(juce::AudioProcessorEditor& editor, WebViewManager &w) : webViewManager(w) {
            webView = webViewManager.getWebView(&editor);
#if JUCE_MAC
            setView(webView->getViewHandle());
#elif JUCE_WINDOWS
            setHWND(webView->getViewHandle());
#endif

            setOpaque(true);
        }

        ~ChocBrowserComponent() override {
            webViewManager.removeWebView(webView.get());
#if JUCE_MAC
            setView ({});
#elif JUCE_WINDOWS
            setHWND ({});
#endif
        }

        WebViewManager &getWebViewManager() { return webViewManager; }

        bool keyPressed(const juce::KeyPress &key) override {
            return true;
        }

    private:
        WebViewManager &webViewManager;
        std::shared_ptr<choc::ui::WebView> webView;
    };
}