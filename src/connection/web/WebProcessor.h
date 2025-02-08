//
// Created by August Pemberton on 21/05/2023.
//

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <imagiro_processor/imagiro_processor.h>
#include <imagiro_util/imagiro_util.h>

#include "WebUIConnection.h"
#include "../../ConnectedProcessor.h"
#include "../../AssetServer/AssetServer.h"
#include "../../attachment/WebUIAttachment.h"

namespace imagiro {
    class WebUIPluginEditor;

    class WebProcessor : public ConnectedProcessor {
    public:
        WebProcessor(AssetServer& assetServer, const juce::String& parametersYAMLString, const ParameterLoader& loader = ParameterLoader(),
                     const juce::AudioProcessor::BusesProperties& layout = getDefaultProperties());

        WebUIConnection& getWebUIConnection() { return static_cast<WebUIConnection&>(*uiConnection); }
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

        Preset createPreset(const juce::String &name, bool isDAWSaveState) override;
        void loadPreset(Preset preset) override;

//    BackgroundTaskRunner& getBackgroundTaskRunner() { return backgroundTaskRunner; }

    protected:
        void wrapEditor(WebUIPluginEditor* e);

        WebUIAttachment webUIAttachment;

//     BackgroundTaskRunner backgroundTaskRunner;
    };

} // namespace imagiro
