//
// Created by August Pemberton on 21/05/2023.
//

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <imagiro_processor/imagiro_processor.h>
#include "WebViewManager.h"
#include "imagiro_webview/src/AssetServer/BinaryDataAssetServer.h"
#include <memory>

namespace imagiro {
class WebUIPluginEditor;
class WebUIAttachment;

class WebProcessor : public Processor {
public:
    WebProcessor(AssetServer& server, juce::String parametersYAMLString,
                 juce::String currentVersion = "1.0.0", juce::String productSlug = "");
    WebProcessor(const juce::AudioProcessor::BusesProperties& ioLayouts,
                 AssetServer& server, juce::String parametersYAMLString,
                 juce::String currentVersion = "1.0.0", juce::String productSlug = "");

    WebViewManager& getWebViewManager() { return webViewManager; }
    juce::AudioProcessorEditor* createEditor() override;

    virtual juce::Point<int> getDefaultWindowSize() { return {400, 300}; }

    virtual juce::Point<int> getResizeMin() { return {200, 200}; }
    virtual juce::Point<int> getResizeMax() { return {1500, 1500}; }

    enum class ResizeType {
        Resizable,
        FixedAspect,
        NonResizable
    };
    virtual ResizeType isResizable() { return ResizeType::NonResizable; }

    void addUIAttachment(WebUIAttachment& attachment);

    choc::value::Value& getWebViewData() { return webViewCustomData; }

    Preset createPreset(const juce::String &name, bool isDAWSaveState) override;
    void loadPreset(Preset preset) override;

protected:
    void init();
    bool hasInitialized {false};
    AssetServer& server;
    WebViewManager webViewManager { server };
    std::vector<std::unique_ptr<WebUIAttachment>> uiAttachments;

    choc::value::Value webViewCustomData {choc::value::createObject("CustomData")};

private:
    void addUIAttachment(std::unique_ptr<WebUIAttachment> attachment);
};

} // namespace imagiro
