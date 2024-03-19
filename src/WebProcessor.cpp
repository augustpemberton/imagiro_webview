#include "WebProcessor.h"
#include "WebUIPluginEditor.h"

#include "imagiro_webview/src/attachment/ParameterAttachment.h"
#include "imagiro_webview/src/attachment/PresetAttachment.h"
#include "imagiro_webview/src/attachment/PluginInfoAttachment.h"
#include "imagiro_webview/src/attachment/AuthAttachment.h"
#include "imagiro_webview/src/attachment/FileIOAttachment.h"
#include "imagiro_webview/src/attachment/UtilAttachment.h"

namespace imagiro {

    WebProcessor::WebProcessor(AssetServer& server, juce::String parametersYAMLString, juce::String currentVersion, juce::String productSlug)
            : Processor(parametersYAMLString, currentVersion, productSlug),
              server(server)
    {
        init();
    }

    WebProcessor::WebProcessor(const juce::AudioProcessor::BusesProperties &ioLayouts,
                               AssetServer& server,
                               juce::String parametersYAMLString,
                               juce::String currentVersion, juce::String productSlug)
            : Processor(ioLayouts, parametersYAMLString, currentVersion, productSlug),
              server(server)
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

        auto defaultSize = getDefaultWindowSize();
        e->setSize(defaultSize.x, defaultSize.y);
        if (isResizable() != ResizeType::NonResizable) {
            e->setResizable(true, true);
            e->setResizeLimits(getResizeMin().x, getResizeMin().y,
                               getResizeMax().x, getResizeMax().y);
            if (isResizable() == ResizeType::FixedAspect) {
                e->getConstrainer()->setFixedAspectRatio(defaultSize.x / (float) defaultSize.y);
            }
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
        addUIAttachment(std::make_unique<ParameterAttachment>(*this, webViewManager));
        addUIAttachment(std::make_unique<PresetAttachment>(*this, webViewManager));
        addUIAttachment(std::make_unique<PluginInfoAttachment>(*this, webViewManager));
        addUIAttachment(std::make_unique<AuthAttachment>(*this, webViewManager));
        addUIAttachment(std::make_unique<FileIOAttachment>(*this, webViewManager));
        addUIAttachment(std::make_unique<UtilAttachment>(*this, webViewManager));
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

    Preset WebProcessor::createPreset(const juce::String &name, bool isDAWSaveState) {
        auto p = Processor::createPreset(name, isDAWSaveState);
        if (isDAWSaveState) {
            p.getData().addMember("webviewData", getWebViewData());
        }
        return p;
    }

    void WebProcessor::loadPreset(Preset preset) {
        Processor::loadPreset(preset);
        if (preset.isDAWSaveState() && preset.getData().hasObjectMember("webviewData")) {
            webViewCustomData = preset.getData()["webviewData"];
        }
    }
}
