//
// Created by August Pemberton on 08/02/2025.
//

#pragma once

#include <utility>

#include <imagiro_processor/processor/Processor.h>
#include "imagiro_webview/src/attachment/ParameterAttachment.h"
#include "imagiro_webview/src/attachment/PluginInfoAttachment.h"
#include "imagiro_webview/src/attachment/FileIOAttachment.h"
#include "imagiro_webview/src/attachment/UtilAttachment.h"

namespace imagiro {
    class ConnectedProcessor : public Processor {
    public:
        ConnectedProcessor(std::unique_ptr<UIConnection> c,
                           const juce::AudioProcessor::BusesProperties& layout = getDefaultProperties())
                : Processor(layout),
                  uiConnection(std::move(c)),
                  parameterAttachment(*uiConnection, *this),
                  pluginInfoAttachment(*uiConnection, *this),
                  fileIOAttachment(*uiConnection),
                  utilAttachment(*uiConnection, *this)
        {
        }

        // Call this after parameters are added to set up attachments
        void initializeAttachments() {
            initParameters();  // Initialize JUCE parameter adapter

            addUIAttachment(parameterAttachment);
            addUIAttachment(pluginInfoAttachment);
            addUIAttachment(fileIOAttachment);
            addUIAttachment(utilAttachment);
        }

        void addUIAttachment(UIAttachment& attachment) {
            attachment.addListeners();
            attachment.addBindings();
            attachments.push_back(&attachment);
        }

        void afterProcess() override {
            for (auto attachment : attachments) {
                attachment->processCallback();
            }
        }

    protected:
        std::unique_ptr<UIConnection> uiConnection;
        UtilAttachment utilAttachment;
        ParameterAttachment parameterAttachment;
        PluginInfoAttachment pluginInfoAttachment;
        FileIOAttachment fileIOAttachment;

    private:

        std::vector<UIAttachment*> attachments;
    };
}
