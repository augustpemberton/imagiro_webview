//
// Created by August Pemberton on 11/05/2023.
//

#pragma once

#include "WebViewManager.h"
//#include <juce_gui_extra/juce_gui_extra.h> // this causes some compilation issues on windows, not needed

#if JUCE_WINDOWS
#include "WinKeypressWorkaround.h"
#endif

namespace imagiro {

    class WebviewBrowserComponent : public juce::Component {

    public:
        WebviewBrowserComponent() {

        }

        void initialize(bool debug, void* window) {
            webview = std::make_unique<juce::WebBrowserComponent>();
            webview->goToURL("http://localhost:4342");
            addAndMakeVisible(*webview);
        }

        void resized() override {
            webview->setBounds(getLocalBounds());
        }

        ~WebviewBrowserComponent() override {

        }

        std::unique_ptr<juce::WebBrowserComponent>& getWebview() { return webview; }

    private:
        std::unique_ptr<juce::WebBrowserComponent> webview;
    };
}