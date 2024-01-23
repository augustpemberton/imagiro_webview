#include "WebProcessor.h"
#include <AppCore/Platform.h>
#include "WebUIPluginEditor.h"

#include "attachment/PluginInfoAttachment.h"
#include "attachment/ParameterAttachment.h"

namespace imagiro {
    WebProcessor::WebProcessor(juce::String currentVersion, juce::String productSlug)
        : Processor(currentVersion, productSlug) {
        init();
    }

    WebProcessor::WebProcessor(const juce::AudioProcessor::BusesProperties &ioLayouts,
                               juce::String currentVersion, juce::String productSlug)
        : Processor(ioLayouts, currentVersion, productSlug) {
        init();
    }

    juce::AudioProcessorEditor *WebProcessor::createEditor() {
        juce::AudioProcessorEditor *e = new WebUIPluginEditor(*this, allowInspector());

        e->setSize(getDefaultWindowSize().x, getDefaultWindowSize().y);
        if (isResizable()) {
            e->setResizable(true, true);
            e->setResizeLimits(getResizeMin().x, getResizeMin().y,
                               getResizeMax().x, getResizeMax().y);
        }

        if (wrapperType == wrapperType_Standalone) {
            for (auto i=0; i<juce::TopLevelWindow::getNumTopLevelWindows(); i++) {
                juce::TopLevelWindow::getTopLevelWindow(i)->setUsingNativeTitleBar(true);
            }
        }

        return e;
    }

    void WebProcessor::init() {
        if (RENDERER.get() == nullptr) {
            Config config;
            config.resource_path_prefix = ULTRALIGHT_RESOURCES_PREFIX;
            config.user_stylesheet = "";

            Platform::instance().set_config(config);
            Platform::instance().set_font_loader(GetPlatformFontLoader());
            Platform::instance().set_file_system(&fileSystem);
            Platform::instance().set_logger(GetDefaultLogger("ultralight.log"));

            RENDERER = Renderer::Create();
        }

         addUIAttachment(std::make_unique<ParameterAttachment>(*this, webView));
        // addUIAttachment(std::make_unique<PresetAttachment>(*this, webView));
         addUIAttachment(std::make_unique<PluginInfoAttachment>(*this));
        // addUIAttachment(std::make_unique<AuthAttachment>(*this, webView));
        // addUIAttachment(std::make_unique<FileIOAttachment>(*this, webView));
    }

    void WebProcessor::addUIAttachment(std::unique_ptr<WebUIAttachment> attachment) {
        baseAttachments.push_back(std::move(attachment));
        baseAttachments.back()->addBindings();
        baseAttachments.back()->addListeners();
    }
}
