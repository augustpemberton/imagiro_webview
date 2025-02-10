#include "WebUIConnection.h"
#include "WebUIPluginEditor.h"

namespace imagiro {
    WebUIConnection::WebUIConnection(AssetServer &server)
        : server(server), jsEvalQueue(256)
    {
        startTimerHz(60);
    }

    void WebUIConnection::addListener(Listener* l) {
        listeners.add(l);
    }

    void WebUIConnection::removeListener(Listener* l) {
        listeners.remove(l);
    }

    std::shared_ptr<choc::ui::WebView> WebUIConnection::getWebView(WebUIPluginEditor* editor) {
        if (!preparedWebview) {
            preparedWebview = createWebView();
            setupWebview(*preparedWebview);
        }

        auto activeView = std::move(preparedWebview);
        bindEditorSpecificFunctions(*activeView, editor);
        preparedWebview = createWebView();
        setupWebview(*preparedWebview);
        return activeView;
    }

    std::shared_ptr<choc::ui::WebView> WebUIConnection::createWebView() {
#if JUCE_DEBUG || defined(BETA)
        auto debugMode = true;
#else
        auto debugMode = false;
#endif
        try {
            auto view = std::make_shared<choc::ui::WebView>(
                    choc::ui::WebView::Options{
                            debugMode, true, true, "",
                            [&](auto& path) {
                                std::optional<choc::ui::WebView::Options::Resource> r2 {};
                                auto r = server.getResource(path);
                                if (r) {
                                    r2 = choc::ui::WebView::Options::Resource();
                                    r2->data = r->data;
                                    r2->mimeType = r->mimeType;
                                }

                                return r2;
                            }
                    }
            );

            if (!view || !view->loadedOK()) {
                throw std::runtime_error("Failed to load WebView");
            }

            return view;
        } catch (const std::exception& e) {
            resources->getErrorLogger().logMessage("webview creation failed");
            resources->getErrorLogger().logMessage(e.what());
            throw e;
        }
    }

    void WebUIConnection::bindEditorSpecificFunctions(choc::ui::WebView& view, WebUIPluginEditor* editor) {
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
    }

    void WebUIConnection::navigate(const std::string &url) {
        for (auto wv : activeWebViews) wv->navigate(url);
        currentURL = url;
        htmlToSet.reset();
    }

    void WebUIConnection::reload() {
        if (!currentURL.has_value()) return;
        for (auto wv : activeWebViews) wv->navigate(currentURL.value());
    }

    std::string WebUIConnection::getCurrentURL() {
        return currentURL ? currentURL.value() : "";
    }

    void WebUIConnection::setHTML(const std::string &html) {
        for (auto wv : activeWebViews) wv->setHTML(html);
        htmlToSet = html;
        currentURL = "";
    }

    void WebUIConnection::evaluateJavascript(const std::string& js) {
        jsEvalQueue.enqueue(choc::json::toString(choc::value::Value(js)));
    }

    void WebUIConnection::eval(const std::string &functionName, const std::vector<choc::value::ValueView>& args) {
        auto evalString = functionName + "(";
        for (auto i=0u; i<args.size(); i++) {
            evalString += choc::json::toString(args[i]);
            if (i != args.size()-1) evalString += ",";
        }
        evalString += ");";

        evaluateJavascript(evalString);
    }

    void WebUIConnection::timerCallback() {
        std::string js;
        while (jsEvalQueue.try_dequeue(js)) {
            auto evalString = "window.ui.evaluate(" + js + ");";
            for (auto wv: activeWebViews) wv->evaluateJavascript(evalString);
        }
    }

    void WebUIConnection::bind(const std::string &functionName, CallbackFn &&fn) {
        choc::ui::WebView::CallbackFn func = wrapFn(fn);
        for (auto wv : activeWebViews) {
            auto funcCopy = func;
            wv->bind(functionName, std::move(funcCopy));
        }
        fnsToBind.emplace_back(functionName, std::move(func));
    }

    choc::ui::WebView::CallbackFn WebUIConnection::wrapFn(choc::ui::WebView::CallbackFn func) {
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

    void WebUIConnection::requestFileChooser(juce::String patternsAllowed) {
        listeners.call(&Listener::fileOpenerRequested, patternsAllowed);
    }

    void WebUIConnection::removeWebView(choc::ui::WebView* v) {
        activeWebViews.removeFirstMatchingValue(v);
    }

    bool WebUIConnection::isShowing() {
        return !activeWebViews.isEmpty();
    }

    void WebUIConnection::setupWebview(choc::ui::WebView& wv) {
        activeWebViews.add(&wv);
        for (auto& func : fnsToBind) {
            auto funcCopy = func.second;
            wv.bind(func.first, std::move(funcCopy));
        }
        if (htmlToSet) wv.setHTML(htmlToSet.value());
        if (currentURL) wv.navigate(currentURL.value());
    }

} // namespace imagiro