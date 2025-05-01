//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include <timestamp.h>
#include <imagiro_processor/imagiro_processor.h>
#include "UIAttachment.h"

#include "juce_audio_devices/juce_audio_devices.h"

namespace imagiro {
    class DevicesAttachment : public UIAttachment, juce::ChangeListener,
                              juce::Timer {
    public:
        DevicesAttachment(UIConnection& c);
        ~DevicesAttachment() override;

        void addBindings() override;

        void changeListenerCallback(juce::ChangeBroadcaster *source) override;
        void timerCallback() override;

    private:
        juce::Uuid uuid;
    };
}
