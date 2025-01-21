#pragma once

#include <choc/gui/choc_WebView.h>
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "imagiro_webview/src/AssetServer/BinaryDataAssetServer.h"
#include <imagiro_util/imagiro_util.h>

namespace imagiro {
    class WebUIPluginEditor;
    class WebViewManager : juce::Timer {
    public:
        struct Listener {
            virtual void fileOpenerRequested(juce::String patternsAllowed) {}
        };

        WebViewManager(AssetServer& server);

        void addListener(Listener* l);
        void removeListener(Listener* l);

        std::shared_ptr<choc::ui::WebView> getWebView(WebUIPluginEditor* editor);
        std::shared_ptr<choc::ui::WebView> createWebView();

        static void bindEditorSpecificFunctions(choc::ui::WebView& view, WebUIPluginEditor* editor);

        void navigate(const std::string &url);
        void reload();
        std::string getCurrentURL();
        void setHTML(const std::string &html);
        void evaluateJavascript(const std::string& js);
        void timerCallback() override;
        void bind(const std::string &functionName, choc::ui::WebView::CallbackFn &&fn);
        static choc::ui::WebView::CallbackFn wrapFn(choc::ui::WebView::CallbackFn func);
        void requestFileChooser(juce::String patternsAllowed = "*.wav");
        void removeWebView(choc::ui::WebView* v);
        bool isShowing();
        void setupWebview(choc::ui::WebView& wv);

    private:
        juce::ListenerList<Listener> listeners;
        std::shared_ptr<choc::ui::WebView> preparedWebview;
        juce::Array<choc::ui::WebView*, juce::CriticalSection> activeWebViews;
        std::vector<std::pair<std::string, choc::ui::WebView::CallbackFn>> fnsToBind;
        std::optional<std::string> htmlToSet;
        std::optional<std::string> currentURL;
        AssetServer& server;
        moodycamel::ConcurrentQueue<std::string> jsEvalQueue;

        juce::SharedResourcePointer<Resources> resources;
    };
}