//
// Created by August Pemberton on 11/05/2023.
//

#pragma once

#include "WebViewManager.h"
#include "juce_gui_extra/embedding/juce_NSViewComponent.h"
#include "juce_gui_extra/embedding/juce_HWNDComponent.h"
#if JUCE_WINDOWS
#include "WinKeypressWorkaround.h"
#endif

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

            webView->bind( "juce_setUILoaded",
                           ([&](const choc::value::ValueView &args) -> choc::value::Value {
                               auto fade = args[0].getWithDefault(false);
                               if (fade) startFadeIn();
                               else {
                                   setAlpha(1.f);
                                   repaint();
                               }
                               return {};
                           }));
#if JUCE_MAC
            setView(webView->getViewHandle());
#elif JUCE_WINDOWS
            setHWND(webView->getViewHandle());
            wKW = std::make_unique<WinKeypressWorkaround>(*webView, *this);
#endif

            setOpaque(false);
            setAlpha(0.f);
            webView->evaluateJavascript("window.ui.onUIOpened();");
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

        void startFadeIn() {
            fadeStartTime = juce::Time::getMillisecondCounter();
            fading = true;
        }

        void update() {
            if (fading) {
                auto timeIntoFade = (juce::Time::getMillisecondCounter() - fadeStartTime);
                auto fadePercent = timeIntoFade / (float) fadeMS;
                if (fadePercent >= 1.f) fading = false;
                setAlpha(juce::jlimit(0.f, 1.f, fadePercent));
                repaint();
            }
        }

    private:
        WebViewManager &webViewManager;
        std::shared_ptr<choc::ui::WebView> webView;

        juce::VBlankAttachment vBlankAttachment { this, [this] { update(); } };

        juce::int64 fadeStartTime;
        bool fading;
        const int fadeMS = 70;

#if JUCE_WINDOWS
        std::unique_ptr<WinKeypressWorkaround> wKW;
#endif
    };
}