//
// Created by August Pemberton on 21/05/2023.
//

#pragma once
#include "WebProcessorParameterAttachment.h"
#include "WebViewManager.h"


namespace imagiro {

class WebProcessor : public Processor, public Parameter::Listener {
public:
    using Processor::Processor;

    WebViewManager& getWebViewManager() { return webView; }
    juce::AudioProcessorEditor* createEditor() override;

    virtual std::string getHTMLString() = 0;

    virtual juce::Point<int> getDefaultWindowSize() { return {400, 300}; }

    virtual juce::Point<int> getResizeMin() { return {200, 200}; }
    virtual juce::Point<int> getResizeMax() { return {1500, 1500}; }

    virtual bool isResizable() { return false; }

protected:
    WebViewManager webView;
    WebProcessorParameterAttachment parameterAttachment {*this, webView};
};

} // namespace imagiro