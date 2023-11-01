#include "WebProcessor.h"
#include "WebUIPluginEditor.h"

#include "imagiro_webview/src/attachment/ParameterAttachment.h"
#include "imagiro_webview/src/attachment/PresetAttachment.h"
#include "imagiro_webview/src/attachment/PluginInfoAttachment.h"
#include "imagiro_webview/src/attachment/AuthAttachment.h"
#include "imagiro_webview/src/attachment/FileIOAttachment.h"

namespace imagiro {

    juce::AudioProcessorEditor *WebProcessor::createEditor() {
        juce::AudioProcessorEditor* e;
#if JUCE_DEBUG
        e = WebUIPluginEditor::createFromURL(*this, "http://localhost:4342");
#else
        e = WebUIPluginEditor::createFromHTMLString(*this, getHTMLString());
#endif
        e->setSize(getDefaultWindowSize().x, getDefaultWindowSize().y);
        if (isResizable()) {
            e->setResizable(true, true);
            e->setResizeLimits(getResizeMin().x, getResizeMin().y,
                               getResizeMax().x, getResizeMax().y);
        }

        return e;
    }

    void WebProcessor::init() {
        uiAttachments.emplace_back(std::make_unique<ParameterAttachment>(*this, webView));
        uiAttachments.emplace_back(std::make_unique<PresetAttachment>(*this, webView));
        uiAttachments.emplace_back(std::make_unique<PluginInfoAttachment>(*this, webView));
        uiAttachments.emplace_back(std::make_unique<AuthAttachment>(*this, webView));
        uiAttachments.emplace_back(std::make_unique<FileIOAttachment>(*this, webView));

        for (auto& attachment : uiAttachments) {
            attachment->addListeners();
            attachment->addBindings();
        }
    }


}
