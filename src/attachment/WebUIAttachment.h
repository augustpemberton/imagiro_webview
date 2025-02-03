//
// Created by August Pemberton on 28/10/2023.
//

#pragma once

namespace imagiro {
    class WebProcessor;

    class WebUIAttachment {
    public:
        WebUIAttachment(WebProcessor& p, WebViewManager& w)
        : processor(p), webViewManager(w) { }

        virtual ~WebUIAttachment() = default;

        virtual void addBindings() = 0;
        virtual void addListeners() {}

        virtual void processCallback() {}

    protected:
        WebProcessor& processor;
        WebViewManager& webViewManager;

        JUCE_LEAK_DETECTOR(WebUIAttachment)
    };
}
