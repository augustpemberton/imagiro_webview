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
            webViewManager.bind("juce_doesFileExist",
                                [&](const choc::value::ValueView& args) -> choc::value::Value {
                const auto path = args[0].getWithDefault("");
                return choc::value::Value(juce::File(path).exists());
            });

            webViewManager.bind("juce_doesFileHaveWriteAccess",
                                [&](const choc::value::ValueView& args) -> choc::value::Value {
                                    const auto path = args[0].getWithDefault("");
                                    return choc::value::Value(juce::File(path).hasWriteAccess());
                                });

            webViewManager.bind("juce_requestFileChooser",
            [&](const choc::value::ValueView& args) -> choc::value::Value {
                webViewManager.requestFileChooser(args[0].toString());
                return {};
            });

            webViewManager.bind("juce_revealPath",
            [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto path = args[0].toString();
                auto file = juce::File(path);
                if (!file.exists()) return {};

                file.revealToUser();
                return {};
            });

            webViewManager.bind("juce_copyToClipboard",
                [&](const choc::value::ValueView& args) -> choc::value::Value {
                    juce::SystemClipboard::copyTextToClipboard(args[0].toString());
                    return {};
                }
            );

            webViewManager.bind("juce_getTextFromClipboard",
                [&](const choc::value::ValueView& args) -> choc::value::Value {
                    return choc::value::Value(
                            juce::SystemClipboard::getTextFromClipboard().toStdString()
                    );
                }
            );
        }
    };

}
