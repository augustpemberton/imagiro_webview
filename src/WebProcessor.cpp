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
              server(server),
              backgroundTaskRunner(webViewManager)
    {
//        init();
    }

    WebProcessor::WebProcessor(const juce::AudioProcessor::BusesProperties &ioLayouts,
                               AssetServer& server,
                               juce::String parametersYAMLString,
                               juce::String currentVersion, juce::String productSlug)
            : Processor(ioLayouts, parametersYAMLString, currentVersion, productSlug),
              server(server),
              backgroundTaskRunner(webViewManager)
    {
//        init();
    }

    void WebProcessor::wrapEditor(WebUIPluginEditor* e) {
#if JUCE_DEBUG
        e->getBrowser().getWebViewManager().navigate("http://localhost:4342");
#endif

        auto defaultSize = getDefaultWindowSize();

#if JUCE_DEBUG
        // add space for standalone debug bar
        if (wrapperType == wrapperType_Standalone) defaultSize.y += 30;
#endif

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
    }

    juce::AudioProcessorEditor *WebProcessor::createEditor() {
        jassert(hasInitialized); // make sure to call init() from the subclass constructor!

        auto e = new WebUIPluginEditor(*this);
        wrapEditor(e);
        return e;
    }

    void WebProcessor::init() {
        addUIAttachment(std::make_unique<ParameterAttachment>(*this, webViewManager));
        addUIAttachment(std::make_unique<PresetAttachment>(*this, webViewManager));
        addUIAttachment(std::make_unique<PluginInfoAttachment>(*this, webViewManager));
        addUIAttachment(std::make_unique<AuthAttachment>(*this, webViewManager));
        addUIAttachment(std::make_unique<FileIOAttachment>(*this, webViewManager));
        addUIAttachment(std::make_unique<UtilAttachment>(*this, webViewManager));
        hasInitialized = true;
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
        p.getData().addMember("webviewData", getWebViewData().getState(!isDAWSaveState));
        return p;
    }

    void WebProcessor::loadPreset(Preset preset) {
        Processor::loadPreset(preset);
        if (preset.getData().hasObjectMember("webviewData")) {
            webViewCustomData = WebviewData::fromState(preset.getData()["webviewData"],
                                                       !preset.isDAWSaveState());

            for (auto& [key, val] : webViewCustomData.getValues()) {
                std::string js = "window.ui.processorValueUpdated(";
                js += "'"; js += key; js += "',";
                js += "'"; js += val.value; js += "');";
                webViewManager.evaluateJavascript(js);
            }
        }
    }
}
