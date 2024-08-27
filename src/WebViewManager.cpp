#include "WebViewManager.h"
#include "WebUIPluginEditor.h"

namespace imagiro {

    WebViewManager::WebViewManager(AssetServer& server) : server(server), jsEvalQueue(256) {
        preparedWebview = createWebView();
        setupWebview(*preparedWebview);
        startTimerHz(60);
    }

    void WebViewManager::addListener(Listener* l) {
        listeners.add(l);
    }

    void WebViewManager::removeListener(Listener* l) {
        listeners.remove(l);
    }

    std::shared_ptr<choc::ui::WebView> WebViewManager::getWebView(WebUIPluginEditor* editor) {
        auto activeView = std::move(preparedWebview);
        bindEditorSpecificFunctions(*activeView, editor);
        preparedWebview = createWebView();
        setupWebview(*preparedWebview);
        return activeView;
    }

    std::shared_ptr<choc::ui::WebView> WebViewManager::createWebView() {
#if JUCE_DEBUG || defined(BETA)
        auto debugMode = true;
#else
        auto debugMode = false;
#endif
        return std::make_shared<choc::ui::WebView>(
                choc::ui::WebView::Options{
                        debugMode, true, "",
                        [&](auto& path) {
                            return server.getResource(path);
                        }
                }
        );
    }

    void WebViewManager::bindEditorSpecificFunctions(choc::ui::WebView& view, WebUIPluginEditor* editor) {
        view.bind("juce_getWindowSize",
                  wrapFn([editor](const choc::value::ValueView &args) -> choc::value::Value {
                      auto dims = choc::value::createObject("WindowSize");
                      dims.setMember("width", editor->getWidth());
                      dims.setMember("height", editor->getHeight());
                      return dims;
                  })
        );
        view.bind("juce_setWindowSize",
                  wrapFn([editor](const choc::value::ValueView &args) -> choc::value::Value {
                      auto x = args[0].getWithDefault(500);
                      auto y = args[1].getWithDefault(400);
                      editor->setSizeScaled(x, y);
                      return {};
                  })
        );
        view.bind("juce_openURLInDefaultBrowser",
                  wrapFn([editor](const choc::value::ValueView &args) -> choc::value::Value {
                      auto url = args[0].getWithDefault("https://imagi.ro");
                      return choc::value::Value(juce::URL(url).launchInDefaultBrowser());
                  })
        );
    }

    void WebViewManager::navigate(const std::string &url) {
        for (auto wv : activeWebViews) wv->navigate(url);
        currentURL = url;
        htmlToSet.reset();
    }

    void WebViewManager::reload() {
        if (!currentURL.has_value()) return;
        for (auto wv : activeWebViews) wv->navigate(currentURL.value());
    }

    std::string WebViewManager::getCurrentURL() {
        return currentURL ? currentURL.value() : "";
    }

    void WebViewManager::setHTML(const std::string &html) {
        for (auto wv : activeWebViews) wv->setHTML(html);
        htmlToSet = html;
        currentURL = "";
    }

    void WebViewManager::evaluateJavascript(const std::string& js) {
        jsEvalQueue.enqueue(choc::json::toString(choc::value::Value(js)));
    }

    void WebViewManager::timerCallback() {
        std::string js;
        while (jsEvalQueue.try_dequeue(js)) {
            auto evalString = "window.ui.evaluate(" + js + ");";
            for (auto wv: activeWebViews) wv->evaluateJavascript(evalString);
        }
    }

    void WebViewManager::bind(const std::string &functionName, choc::ui::WebView::CallbackFn &&fn) {
        choc::ui::WebView::CallbackFn func = wrapFn(fn);
        for (auto wv : activeWebViews) {
            auto funcCopy = func;
            wv->bind(functionName, std::move(funcCopy));
        }
        fnsToBind.emplace_back(functionName, std::move(func));
    }

    choc::ui::WebView::CallbackFn WebViewManager::wrapFn(choc::ui::WebView::CallbackFn func) {
        return [func](const choc::value::ValueView& args) -> choc::value::Value {
            auto responseState = choc::value::createObject("Response");
            try {
                auto response = func(args);
                responseState.setMember("status", "ok");
                responseState.setMember("data", response);
                return responseState;
            } catch (std::exception& e) {
                responseState.setMember("status", "error");
                responseState.setMember("error", e.what());
                return responseState;
            }
        };
    }

    void WebViewManager::requestFileChooser(juce::String patternsAllowed) {
        listeners.call(&Listener::fileOpenerRequested, patternsAllowed);
    }

    void WebViewManager::removeWebView(choc::ui::WebView* v) {
        activeWebViews.removeFirstMatchingValue(v);
    }

    bool WebViewManager::isShowing() {
        return !activeWebViews.isEmpty();
    }

    void WebViewManager::setupWebview(choc::ui::WebView& wv) {
        activeWebViews.add(&wv);
        for (auto& func : fnsToBind) {
            auto funcCopy = func.second;
            wv.bind(func.first, std::move(funcCopy));
        }
        if (htmlToSet) wv.setHTML(htmlToSet.value());
        if (currentURL) wv.navigate(currentURL.value());
    }

} // namespace imagiro
