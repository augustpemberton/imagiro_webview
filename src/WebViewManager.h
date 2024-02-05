//
// Created by August Pemberton on 30/05/2023.
//

#include <choc/gui/choc_WebView.h>
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ChocServer.h"
#include "farbot/fifo.hpp"

#pragma once

namespace imagiro {
    class WebViewManager : juce::Timer {
    public:
        struct Listener {
            virtual void fileOpenerRequested(juce::String patternsAllowed) {}
        };

        void addListener(Listener* l) {listeners.add(l);}
        void removeListener(Listener* l) {listeners.remove(l);}

        WebViewManager() {
            preparedWebview = std::make_unique<choc::ui::WebView>(
                    choc::ui::WebView::Options
                            {
                                    true, true, "",
                                    [&](auto& path) {
                                        return server.getResource(path);
                                    }
                            }
            );
            setupWebview(preparedWebview.get());

            startTimerHz(60);
        }

        std::shared_ptr<choc::ui::WebView> getWebView(juce::AudioProcessorEditor* editor) {
            auto s = std::move(preparedWebview);

            preparedWebview = std::make_unique<choc::ui::WebView>(
                    choc::ui::WebView::Options
                    {
                            true, true, "",
                            [&](auto& path) {
                                return server.getResource(path);
                            }
                    }
            );

            s->bind( "juce_setWindowSize",
                      [editor](const choc::value::ValueView &args) -> choc::value::Value {
                          auto x = args[0].getWithDefault(500);
                          auto y = args[1].getWithDefault(400);
                          editor->setSize(x, y);
                          return {};
                      }
            );

            setupWebview(preparedWebview.get());

            return s;
        }

        void navigate(const std::string &url) {
            for (auto wv : activeWebViews) wv->navigate(url);
            currentURL = url;
            htmlToSet.reset();
        }

        void reload() {
            if (!currentURL.has_value()) return;
            for (auto wv : activeWebViews) wv->navigate(currentURL.value());
        }

        std::string getCurrentURL() { return currentURL ? currentURL.value() : ""; }

        // base functions
        void setHTML(const std::string &html) {
            for (auto wv : activeWebViews) wv->setHTML(html);
            htmlToSet = html;
            currentURL = "";
        }

        void evaluateJavascript(const std::string& js) {
            jsEvalFifo.push(choc::json::toString(choc::value::Value(js)));
        }

        void timerCallback() override {
            std::string temp;
            while (jsEvalFifo.pop(temp)) {
                auto evalString = "window.ui.evaluate(" + temp + ");";
                for (auto wv: activeWebViews) wv->evaluateJavascript(evalString);
            }
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

            if (htmlToSet) wv->setHTML(htmlToSet.value());
            if (currentURL) wv->navigate(currentURL.value());
        }

    private:
        juce::ListenerList<Listener> listeners;

        std::shared_ptr<choc::ui::WebView> preparedWebview;
        juce::Array<choc::ui::WebView*, juce::CriticalSection> activeWebViews;

        std::vector<std::pair<std::string, choc::ui::WebView::CallbackFn>> fnsToBind;
        std::optional<std::string> htmlToSet;
        std::optional<std::string> currentURL;

        ChocServer server;

        farbot::fifo<std::string,
                farbot::fifo_options::concurrency::single,
                farbot::fifo_options::concurrency::single,
                farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
                farbot::fifo_options::full_empty_failure_mode::overwrite_or_return_default> jsEvalFifo {2048};
    };
}