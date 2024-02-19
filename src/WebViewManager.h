//
// Created by August Pemberton on 30/05/2023.
//

#include <choc/gui/choc_WebView.h>
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "imagiro_webview/src/AssetServer/BinaryDataAssetServer.h"
#include <imagiro_util/imagiro_util.h>

#pragma once

namespace imagiro {
    class WebViewManager : juce::Timer {
    public:
        struct Listener {
            virtual void fileOpenerRequested(juce::String patternsAllowed) {}
        };

        void addListener(Listener* l) {listeners.add(l);}
        void removeListener(Listener* l) {listeners.remove(l);}

        WebViewManager(AssetServer& server) : server(server) {

            preparedWebview = createWebView();
            setupWebview(*preparedWebview);

            startTimerHz(60);
        }

        std::shared_ptr<choc::ui::WebView> getWebView(juce::AudioProcessorEditor* editor) {
            // move prepared to active
            auto activeView = std::move(preparedWebview);
            bindEditorSpecificFunctions(*activeView, editor);

            // recreate prepared webview for next open
            preparedWebview = createWebView();
            setupWebview(*preparedWebview);

            return activeView;
        }

        std::shared_ptr<choc::ui::WebView> createWebView() {
            return std::make_shared<choc::ui::WebView>(
                    choc::ui::WebView::Options
                            {
                                    true, true, "",
                                    [&](auto& path) {
                                        return server.getResource(path);
                                    }
                            }
            );
        }

        static void bindEditorSpecificFunctions(choc::ui::WebView& view, juce::AudioProcessorEditor* editor) {
            view.bind( "juce_setWindowSize",
                      [editor](const choc::value::ValueView &args) -> choc::value::Value {
                          auto x = args[0].getWithDefault(500);
                          auto y = args[1].getWithDefault(400);
                          editor->setSize(x, y);
                          return {};
                      }
            );
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
            jsEvalQueue.enqueue(choc::json::toString(choc::value::Value(js)));
        }

        void timerCallback() override {
            std::string js;
            while (jsEvalQueue.try_dequeue(js)) {
                auto evalString = "window.ui.evaluate(" + js + ");";
                for (auto wv: activeWebViews) wv->evaluateJavascript(evalString);
            }
        }

        void bind(const std::string &functionName, choc::ui::WebView::CallbackFn &&fn) {
            choc::ui::WebView::CallbackFn func = fn;
            for (auto wv : activeWebViews) {
                auto funcCopy = func;
                wv->bind(functionName, std::move(funcCopy));
            }
            fnsToBind.emplace_back(functionName, std::move(func));
        }

        void requestFileChooser(juce::String patternsAllowed = "*.wav") {
            listeners.call(&Listener::fileOpenerRequested, patternsAllowed);
        }

        void removeWebView(choc::ui::WebView* v) {
            activeWebViews.removeFirstMatchingValue(v);
        }

        bool isShowing() { return !activeWebViews.isEmpty();}

        void setupWebview(choc::ui::WebView& wv) {
            activeWebViews.add(&wv);

            for (auto& func : fnsToBind) {
                auto funcCopy = func.second;
                wv.bind(func.first, std::move(funcCopy));
            }

            if (htmlToSet) wv.setHTML(htmlToSet.value());
            if (currentURL) wv.navigate(currentURL.value());
        }

    private:
        juce::ListenerList<Listener> listeners;

        std::shared_ptr<choc::ui::WebView> preparedWebview;
        juce::Array<choc::ui::WebView*, juce::CriticalSection> activeWebViews;

        std::vector<std::pair<std::string, choc::ui::WebView::CallbackFn>> fnsToBind;
        std::optional<std::string> htmlToSet;
        std::optional<std::string> currentURL;

        AssetServer& server;

        moodycamel::ReaderWriterQueue<std::string> jsEvalQueue {2048};
    };
}
