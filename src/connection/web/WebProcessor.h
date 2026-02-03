//
// Created by August Pemberton on 21/05/2023.
//

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <imagiro_processor/processor/Processor.h>
#include "../../attachment/DevicesAttachment.h"

#include "WebUIConnection.h"
#include "../../ConnectedProcessor.h"
#include "../../AssetServer/AssetServer.h"


namespace imagiro {
    class WebUIPluginEditor;
    class WebUIAttachment;

    class WebProcessor : public ConnectedProcessor {
    public:
        WebProcessor(AssetServer& assetServer,
                     const juce::AudioProcessor::BusesProperties& layout = getDefaultProperties());
        ~WebProcessor() override;

        // Call this after adding parameters to set up all attachments
        void initializeWebAttachments();

        WebUIConnection& getWebUIConnection() const { return dynamic_cast<WebUIConnection&>(*uiConnection); }
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

    protected:
        void wrapEditor(WebUIPluginEditor* e);

        std::unique_ptr<WebUIAttachment> webUIAttachment;
        DevicesAttachment devicesAttachment;
    };

} // namespace imagiro
