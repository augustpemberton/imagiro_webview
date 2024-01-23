//
// Created by August Pemberton on 21/05/2023.
//

#pragma once
#include <version.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <imagiro_processor/imagiro_processor.h>

#include <Ultralight/Ultralight.h>
#include "filesystems/BinaryDataFileSystem.h"
#include "UltralightViewManager.h"


namespace imagiro {
    class WebUIPluginEditor;
    class WebUIAttachment;

    class WebProcessor : public Processor {
    public:
        WebProcessor(juce::String currentVersion = "1.0.0", juce::String productSlug = "");

        WebProcessor(const BusesProperties &ioLayouts,
                     juce::String currentVersion = "1.0.0", juce::String productSlug = "");

        juce::AudioProcessorEditor *createEditor() override;

        virtual juce::Point<int> getDefaultWindowSize() { return {400, 300}; }
        virtual juce::Point<int> getResizeMin() { return {200, 200}; }
        virtual juce::Point<int> getResizeMax() { return {1500, 1500}; }

        virtual bool isResizable() { return false; }
        virtual bool allowInspector() { return JUCE_DEBUG; }

        // Location of your HTML/JS/CSS resources. Can be anywhere on your machine.
        inline static std::string JS_RESOURCES_PATH = std::string(SRCPATH) + "/ui/dist/";
        inline static std::string ULTRALIGHT_SDK_PATH = std::string(SRCPATH) + "/../modules/ultralight/";

        // Location of the Ultralight SDK resources.
        inline static ultralight::String ULTRALIGHT_RESOURCES_PREFIX = "../../../modules/ultralight/resources/";

        inline static ultralight::RefPtr<ultralight::Renderer> RENDERER = nullptr;
        inline static BinaryDataFileSystem fileSystem;

        UltralightViewManager& getViewManager() { return viewManager; }

    private:
        void init();
        void addUIAttachment(std::unique_ptr<WebUIAttachment> attachment);

        std::vector<std::unique_ptr<WebUIAttachment>> baseAttachments;

        UltralightViewManager viewManager;
    };
} // namespace imagiro
