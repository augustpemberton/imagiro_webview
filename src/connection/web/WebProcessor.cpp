#include "WebProcessor.h"
#include "WebUIPluginEditor.h"

#include "imagiro_webview/src/attachment/PluginInfoAttachment.h"
#include "imagiro_webview/src/attachment/UtilAttachment.h"
#include "imagiro_webview/src/attachment/WebUIAttachment.h"

namespace imagiro {
    WebProcessor::WebProcessor(AssetServer &server,
                               const juce::AudioProcessor::BusesProperties &layout)
        : ConnectedProcessor(std::make_unique<WebUIConnection>(server), layout),
          devicesAttachment(getWebUIConnection())
    {
    }

    void WebProcessor::initializeWebAttachments() {
        initializeAttachments();  // Initialize base attachments

        webUIAttachment = std::make_unique<WebUIAttachment>(getWebUIConnection(), *this);
        addUIAttachment(*webUIAttachment);
        addUIAttachment(devicesAttachment);
    }

    WebProcessor::~WebProcessor() = default;

    void WebProcessor::wrapEditor(WebUIPluginEditor *e) {
#if JUCE_DEBUG
        e->getBrowser().getWebUIConnection().navigate("http://localhost:4342");
#endif

        auto defaultSize = getDefaultWindowSize();

#if JUCE_DEBUG
        // add space for standalone debug bar
        if (wrapperType == wrapperType_Standalone) defaultSize.y += 30;
#endif

        auto size = defaultSize;

        auto &configFile = juce::SharedResourcePointer<Resources>()->getConfigFile();
        if (configFile->containsKey("defaultWidth")) size.x = configFile->getIntValue("defaultWidth");
        if (configFile->containsKey("defaultHeight")) size.y = configFile->getIntValue("defaultHeight");

        size *= e->getWindowsTextScaleFactor();

        e->setSize(size.x, size.y);
        if (isResizable() != ResizeType::NonResizable) {
            e->setResizable(true, false);
            e->setResizeLimits(getResizeMin().x, getResizeMin().y,
                               getResizeMax().x, getResizeMax().y);
            if (isResizable() == ResizeType::FixedAspect) {
                e->getConstrainer()->setFixedAspectRatio(defaultSize.x / (float) defaultSize.y);
            }
        }

        if (wrapperType == wrapperType_Standalone) {
            if (juce::TopLevelWindow::getNumTopLevelWindows() == 1) {
                juce::TopLevelWindow *w = juce::TopLevelWindow::getTopLevelWindow(0);
                w->setUsingNativeTitleBar(true);
            }
        }
    }

    juce::AudioProcessorEditor *WebProcessor::createEditor() {
        auto e = new WebUIPluginEditor(*this);
        wrapEditor(e);
        return e;
    }
}
