//
// Created by August Pemberton on 21/05/2023.
//

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <imagiro_processor/imagiro_processor.h>

#include "WebViewManager.h"

#include <memory>

namespace imagiro {
class WebUIPluginEditor;
class WebUIAttachment;

class WebProcessor : public Processor {
public:
    WebProcessor(juce::String currentVersion = "1.0.0", juce::String productSlug = "");
    WebProcessor(const juce::AudioProcessor::BusesProperties& ioLayouts,
              juce::String currentVersion = "1.0.0", juce::String productSlug = "");

    WebViewManager& getWebViewManager() { return webView; }
    juce::AudioProcessorEditor* createEditor() override;

    virtual juce::Point<int> getDefaultWindowSize() { return {400, 300}; }

    virtual juce::Point<int> getResizeMin() { return {200, 200}; }
    virtual juce::Point<int> getResizeMax() { return {1500, 1500}; }

    virtual bool isResizable() { return false; }

    void addUIAttachment(WebUIAttachment& attachment);

    choc::value::Value& getWebViewData() { return webViewCustomData; }

    Preset createPreset(const juce::String &name, bool isDAWSaveState) override;
    void loadPreset(Preset preset) override;

protected:
    WebViewManager webView;
    std::vector<std::unique_ptr<WebUIAttachment>> uiAttachments;

    choc::value::Value webViewCustomData {choc::value::createObject("CustomData")};

private:
    void addUIAttachment(std::unique_ptr<WebUIAttachment> attachment);
    void init();
};

} // namespace imagiro