#include "WebProcessor.h"
#include <web_ui/web_ui.h>

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
}
