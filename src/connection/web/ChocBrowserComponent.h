//
// Created by August Pemberton on 11/05/2023.
//

#pragma once

#include "WebUIConnection.h"

#if JUCE_WINDOWS
#include "../../WinKeypressWorkaround.h"
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
        ChocBrowserComponent(WebUIPluginEditor& editor, WebUIConnection &w) : webViewManager(w) {
            webView = webViewManager.getWebView(&editor);

            webView->bind( "juce_setUILoaded",
                           ([&](const choc::value::ValueView &args) -> choc::value::Value {
                               if (fading || getAlpha() > 0.02f) return {};
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
            setAlpha(0.01f);
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

        WebUIConnection &getWebUIConnection() { return webViewManager; }

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
        WebUIConnection &webViewManager;
        std::shared_ptr<choc::ui::WebView> webView;

        juce::VBlankAttachment vBlankAttachment { this, [&] { update(); } };

        juce::int64 fadeStartTime{};
        bool fading{};
        const int fadeMS = 120;

#if JUCE_WINDOWS
        std::unique_ptr<WinKeypressWorkaround> wKW;
#endif
    };
}