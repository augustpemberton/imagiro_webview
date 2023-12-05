#include "WebProcessor.h"
#include "WebUIPluginEditor.h"

#include "imagiro_webview/src/attachment/ParameterAttachment.h"
#include "imagiro_webview/src/attachment/PresetAttachment.h"
#include "imagiro_webview/src/attachment/PluginInfoAttachment.h"
#include "imagiro_webview/src/attachment/AuthAttachment.h"
#include "imagiro_webview/src/attachment/FileIOAttachment.h"

namespace imagiro {

    WebProcessor::WebProcessor(juce::String currentVersion, juce::String productSlug)
            : Processor(currentVersion, productSlug)
    {
        init();
    }

    WebProcessor::WebProcessor(const juce::AudioProcessor::BusesProperties &ioLayouts,
                               juce::String currentVersion, juce::String productSlug)
            : Processor(ioLayouts, currentVersion, productSlug)
    {
        init();
    }

    juce::AudioProcessorEditor *WebProcessor::createEditor() {
        juce::AudioProcessorEditor* e;
#if JUCE_DEBUG
        e = WebUIPluginEditor::createFromURL(*this, "http://localhost:4342");
#else
        e = WebUIPluginEditor::create(*this);
#endif
        e->setSize(getDefaultWindowSize().x, getDefaultWindowSize().y);
        if (isResizable()) {
            e->setResizable(true, true);
            e->setResizeLimits(getResizeMin().x, getResizeMin().y,
                               getResizeMax().x, getResizeMax().y);
        }

        if(wrapperType == wrapperType_Standalone)
        {
            if(juce::TopLevelWindow::getNumTopLevelWindows() == 1)
            {
                juce::TopLevelWindow* w = juce::TopLevelWindow::getTopLevelWindow(0);
                w->setUsingNativeTitleBar(true);
            }
        }

        return e;
    }

    void WebProcessor::init() {
        addUIAttachment(std::make_unique<ParameterAttachment>(*this, webView));
        addUIAttachment(std::make_unique<PresetAttachment>(*this, webView));
        addUIAttachment(std::make_unique<PluginInfoAttachment>(*this, webView));
        addUIAttachment(std::make_unique<AuthAttachment>(*this, webView));
        addUIAttachment(std::make_unique<FileIOAttachment>(*this, webView));
    }

    void WebProcessor::addUIAttachment(WebUIAttachment& attachment) {
        attachment.addListeners();
        attachment.addBindings();
    }

    void WebProcessor::addUIAttachment(std::unique_ptr<WebUIAttachment> attachment) {
        uiAttachments.emplace_back(std::move(attachment));
        uiAttachments.back()->addListeners();
        uiAttachments.back()->addBindings();
    }


}
