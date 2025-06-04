//
// Created by August Pemberton on 08/02/2025.
//

#pragma once

#include <utility>

#include "imagiro_webview/src/attachment/ParameterAttachment.h"
#include "imagiro_webview/src/attachment/PresetAttachment.h"
#include "imagiro_webview/src/attachment/PluginInfoAttachment.h"
#include "imagiro_webview/src/attachment/FileIOAttachment.h"
#include "imagiro_webview/src/attachment/UtilAttachment.h"
#include "imagiro_webview/src/attachment/ModMatrixAttachment.h"

namespace imagiro {
    class ConnectedProcessor : public Processor {
    public:
        ConnectedProcessor(std::unique_ptr<UIConnection> c, juce::String parametersYAMLString, const ParameterLoader& loader = ParameterLoader(),
                           const juce::AudioProcessor::BusesProperties& layout = getDefaultProperties())
                : Processor(std::move(parametersYAMLString), loader, layout),
                  uiConnection(std::move(c))
        {
            addUIAttachment(parameterAttachment);
            addUIAttachment(presetAttachment);
            addUIAttachment(pluginInfoAttachment);
            addUIAttachment(fileIOAttachment);
            addUIAttachment(utilAttachment);
            addUIAttachment(modMatrixAttachment);
        }

        void addUIAttachment(UIAttachment& attachment) {
            attachment.addListeners();
            attachment.addBindings();
            attachments.push_back(&attachment);
        }

        void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) override {
            Processor::processBlock(buffer, midiMessages);

            for (auto attachment : attachments) {
                attachment->processCallback();
            }
        }


    protected:
        std::unique_ptr<UIConnection> uiConnection;

    private:
        ParameterAttachment parameterAttachment {*uiConnection, *this};
        PresetAttachment presetAttachment {*uiConnection, *this};
        PluginInfoAttachment pluginInfoAttachment {*uiConnection, *this};
        FileIOAttachment fileIOAttachment {*uiConnection};
        UtilAttachment utilAttachment {*uiConnection, stringData, *this};
        ModMatrixAttachment modMatrixAttachment {*uiConnection, modMatrix};

        std::vector<UIAttachment*> attachments;
    };
}
