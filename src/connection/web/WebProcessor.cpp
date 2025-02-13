#include "WebProcessor.h"
#include "WebUIPluginEditor.h"

#include "imagiro_webview/src/attachment/ParameterAttachment.h"
#include "imagiro_webview/src/attachment/PresetAttachment.h"
#include "imagiro_webview/src/attachment/PluginInfoAttachment.h"
#include "imagiro_webview/src/attachment/FileIOAttachment.h"
#include "imagiro_webview/src/attachment/UtilAttachment.h"
#include "imagiro_webview/src/attachment/ModMatrixAttachment.h"
#include "imagiro_webview/src/attachment/util/UIData.h"

namespace imagiro {

    WebProcessor::WebProcessor(AssetServer& server, const juce::String& parametersYAMLString, const ParameterLoader& loader,
                               const juce::AudioProcessor::BusesProperties& layout)
            : ConnectedProcessor(std::make_unique<WebUIConnection>(server), parametersYAMLString, loader, layout),
              webUIAttachment(getWebUIConnection()),
              devicesAttachment(getWebUIConnection())
//              backgroundTaskRunner()
    {
        addUIAttachment(webUIAttachment);
        addUIAttachment(devicesAttachment);
    }

    void WebProcessor::wrapEditor(WebUIPluginEditor* e) {
#if JUCE_DEBUG
        e->getBrowser().getWebUIConnection().navigate("http://localhost:4342");
#endif

        auto defaultSize = getDefaultWindowSize();

#if JUCE_DEBUG
        // add space for standalone debug bar
        if (wrapperType == wrapperType_Standalone) defaultSize.y += 30;
#endif

        auto size = defaultSize;

        auto& configFile = resources->getConfigFile();
        if (configFile->containsKey("defaultWidth")) size.x = configFile->getIntValue("defaultWidth");
        if (configFile->containsKey("defaultHeight")) size.y = configFile->getIntValue("defaultHeight");

        size *= e->getWindowsTextScaleFactor();

        e->setSize(size.x, size.y);
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
        auto e = new WebUIPluginEditor(*this);
        wrapEditor(e);
        return e;
    }

    Preset WebProcessor::createPreset(const juce::String &name, bool isDAWSaveState) {
        auto p = ConnectedProcessor::createPreset(name, isDAWSaveState);
        p.getData().addMember("uiData", getWebViewData().getState(!isDAWSaveState));
        return p;
    }

    void WebProcessor::loadPreset(Preset preset) {
        ConnectedProcessor::loadPreset(preset);
        if (preset.getData().hasObjectMember("uiData")) {
            uiData = UIData::fromState(preset.getData()["uiData"],
                                       !preset.isDAWSaveState());
        } else if (preset.getData().hasObjectMember("webviewData")) { // compatibility
            uiData = UIData::fromState(preset.getData()["webviewData"],
                                       !preset.isDAWSaveState());
        }


        for (auto& [key, val] : uiData.getValues()) {
            uiConnection->eval("window.ui.processorValueUpdated", {
                    choc::value::Value(key),
                    choc::value::Value(val.value)
            });
        }
    }
}
