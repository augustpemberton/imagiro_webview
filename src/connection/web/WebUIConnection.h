#pragma once

#include <choc/gui/choc_WebView.h>
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "imagiro_webview/src/AssetServer/BinaryDataAssetServer.h"
#include <imagiro_util/imagiro_util.h>
#include "../UIConnection.h"

namespace imagiro {
    class WebUIPluginEditor;

    class WebUIConnection : public UIConnection, juce::Timer {
    public:
        struct Listener {
            virtual void fileOpenerRequested(const juce::String& patternsAllowed) {}
        };

        void addListener(Listener* l);
        void removeListener(Listener* l);

        explicit WebUIConnection(AssetServer& server);
        ~WebUIConnection() override = default;

        std::shared_ptr<choc::ui::WebView> getWebView(WebUIPluginEditor* editor);
        std::shared_ptr<choc::ui::WebView> createWebView();

        static void bindEditorSpecificFunctions(choc::ui::WebView& view, WebUIPluginEditor* editor);

        void eval(const std::string &functionName, const std::vector<choc::value::Value> &args = {}) override;
        void bind(const std::string &functionName, CallbackFn&& callback) override;

        void requestFileChooser(juce::String patternsAllowed = "*.wav");

        void navigate(const std::string &url);
        void setHTML(const std::string &html);
        void reload();
        std::string getCurrentURL();

        void timerCallback() override;
        bool isShowing();
        void removeWebView(choc::ui::WebView* v);
        void setupWebview(choc::ui::WebView& wv);

    private:
        static choc::ui::WebView::CallbackFn wrapFn(choc::ui::WebView::CallbackFn func);
        void evaluateJavascript(const std::string& js);

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