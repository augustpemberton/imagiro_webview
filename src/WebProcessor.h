//
// Created by August Pemberton on 21/05/2023.
//

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <imagiro_processor/imagiro_processor.h>
#include "WebViewManager.h"
#include "imagiro_webview/src/ChocAssetsServer.h"
#include <memory>

namespace imagiro {
class WebUIPluginEditor;
class WebUIAttachment;

class WebProcessor : public Processor {
public:
    WebProcessor(std::function<juce::String()> getParametersYAMLString, juce::String currentVersion = "1.0.0", juce::String productSlug = "");
    WebProcessor(const juce::AudioProcessor::BusesProperties& ioLayouts,
              std::function<juce::String()> getParametersYAMLString, juce::String currentVersion = "1.0.0", juce::String productSlug = "");

    WebViewManager& getWebViewManager() { return webViewManager; }
    juce::AudioProcessorEditor* createEditor() override;

    virtual juce::Point<int> getDefaultWindowSize() { return {400, 300}; }

    virtual juce::Point<int> getResizeMin() { return {200, 200}; }
    virtual juce::Point<int> getResizeMax() { return {1500, 1500}; }

    virtual bool isResizable() { return false; }

    void addUIAttachment(WebUIAttachment& attachment);

    choc::value::Value& getWebViewData() { return webViewCustomData; }

    Preset createPreset(const juce::String &name, bool isDAWSaveState) override;
    void loadPreset(Preset preset) override;

    // Override these in your subclass to provide the binary resources that the web view will serve
    virtual const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes) = 0;
    virtual const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8) = 0;
protected:
    ChocAssetsServer server {
        [this](const char *resourceNameUTF8, int &dataSizeInBytes) {
            return this->getNamedResource(resourceNameUTF8, dataSizeInBytes);
        },
        [this](const char *resourceNameUTF8) {
            return this->getNamedResourceOriginalFilename(resourceNameUTF8);
        }
    };
    WebViewManager webViewManager { server };
    std::vector<std::unique_ptr<WebUIAttachment>> uiAttachments;

    choc::value::Value webViewCustomData {choc::value::createObject("CustomData")};

private:
    void addUIAttachment(std::unique_ptr<WebUIAttachment> attachment);
    void init();
};

} // namespace imagiro
