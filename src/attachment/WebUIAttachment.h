//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include "../UltralightUtil.h"

using namespace UltralightUtil;
namespace imagiro {
    class WebProcessor;

    class WebUIAttachment {
    public:
        WebUIAttachment(WebProcessor& p)
        : processor(p), viewManager(p.getViewManager()) { }

        virtual ~WebUIAttachment() = default;

        virtual void addBindings() = 0;
        virtual void addListeners() {}

    protected:
        WebProcessor& processor;
        UltralightViewManager& viewManager;

        JUCE_LEAK_DETECTOR(WebUIAttachment)
    };
}
