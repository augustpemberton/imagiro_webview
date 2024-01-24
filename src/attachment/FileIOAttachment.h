//
// Created by August Pemberton on 31/10/2023.
//

#pragma once
#include <imagiro_processor/imagiro_processor.h>
#include "WebUIAttachment.h"

namespace imagiro {
    class FileIOAttachment : public WebUIAttachment {
        using WebUIAttachment::WebUIAttachment;

        void addBindings() override {
            viewManager.bind("juce_requestFileChooser",
            [&](const JSObject& obj, const JSArgs& args) -> JSValue {
                // TODO
//                viewManager.requestFileChooser(args[0].toString());
                return {};
            });

            viewManager.bind("juce_revealPath",
            [&](const JSObject& obj, const JSArgs& args) -> JSValue {
                auto path = getStdString(args[0]);
                auto file = juce::File(path);
                if (!file.exists()) return {};

                file.revealToUser();
                return {};
            });

            viewManager.bind("juce_copyToClipboard",
                [&](const JSObject& obj, const JSArgs& args) -> JSValue {
                    juce::SystemClipboard::copyTextToClipboard(getStdString(args[0]));
                    return {};
                }
            );

            viewManager.bind("juce_getTextFromClipboard",
                [&](const JSObject& obj, const JSArgs& args) -> JSValue {
                    return {juce::SystemClipboard::getTextFromClipboard().toRawUTF8()};
                }
            );
        }
    };

}
