//
// Created by August Pemberton on 11/05/2023.
//

#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "WebViewManager.h"


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
        ChocBrowserComponent(juce::AudioProcessorEditor& editor, WebViewManager &w)
                : webViewManager(w) {
            webView = webViewManager.getWebView();
#if JUCE_MAC
            setView(webView->getViewHandle());
#elif JUCE_WINDOWS
            setHWND(webView.getViewHandle());
#endif
            setOpaque(true);

            webViewManager.setupWebview(&editor, webView.get());
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


    private:
        WebViewManager &webViewManager;
        std::unique_ptr<choc::ui::WebView> webView;
    };
}