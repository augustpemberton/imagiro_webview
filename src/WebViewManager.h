//
// Created by August Pemberton on 30/05/2023.
//

#include <choc/gui/choc_WebView.h>
#include <juce_core/juce_core.h>

#pragma once

namespace imagiro {
    class WebViewManager {
    public:
        struct Listener {
            virtual void fileOpenerRequested(juce::String patternsAllowed) {}
        };

        void addListener(Listener* l) {listeners.add(l);}
        void removeListener(Listener* l) {listeners.remove(l);}

        void navigate(const std::string &url) {
            for (auto wv : activeWebViews) wv->navigate(url);
            currentURL = url;
            htmlToSet.reset();
        }

        void reload() {
            if (!currentURL) return;
            for (auto wv : activeWebViews) wv->navigate(*currentURL);
        }

        std::string getCurrentURL() { return currentURL ? *currentURL : ""; }

        // base functions
        void setHTML(const std::string &html) {
            for (auto wv : activeWebViews) wv->setHTML(html);
            htmlToSet = html;
            currentURL = "";
        }

        void evaluateJavascript(const std::string& js) {
            for (auto wv : activeWebViews) wv->evaluateJavascript(js);
        }

        void bind(const std::string &functionName, choc::ui::WebView::CallbackFn &&func) {
            for (auto wv : activeWebViews) wv->bind(functionName, std::move(func));
            fnsToBind.emplace_back(functionName, func);
        }

        void requestFileChooser(juce::String patternsAllowed = "*.wav") {
            listeners.call(&Listener::fileOpenerRequested, patternsAllowed);
        }

        void removeWebView(choc::ui::WebView* v) {
            activeWebViews.removeFirstMatchingValue(v);
        }

        bool isShowing() { return !activeWebViews.isEmpty();}

        void setupWebview(choc::ui::WebView* wv) {
            activeWebViews.add(wv);

            for (auto& func : fnsToBind) {
                auto funcCopy = func.second;
                wv->bind(func.first, std::move(funcCopy));
            }

            if (htmlToSet) wv->setHTML(*htmlToSet);
            if (currentURL) wv->navigate(*currentURL);
        }

    private:
        juce::ListenerList<Listener> listeners;

        juce::Array<choc::ui::WebView*, juce::CriticalSection> activeWebViews;

        std::vector<std::pair<std::string, choc::ui::WebView::CallbackFn>> fnsToBind;
        std::optional<std::string> htmlToSet;
        std::optional<std::string> currentURL;
    };
}