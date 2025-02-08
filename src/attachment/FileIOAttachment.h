//
// Created by August Pemberton on 31/10/2023.
//

#pragma once
#include <imagiro_processor/imagiro_processor.h>
#include "UIAttachment.h"

namespace imagiro {
    class FileIOAttachment : public UIAttachment {
        using UIAttachment::UIAttachment;

        void addBindings() override {
            connection.bind("juce_doesFileExist",
                                [&](const choc::value::ValueView& args) -> choc::value::Value {
                const auto path = args[0].getWithDefault("");
                return choc::value::Value(juce::File(path).exists());
            });

            connection.bind("juce_doesFileHaveWriteAccess",
                                [&](const choc::value::ValueView& args) -> choc::value::Value {
                                    const auto path = args[0].getWithDefault("");
                                    return choc::value::Value(juce::File(path).hasWriteAccess());
                                });

            connection.bind("juce_revealPath",
            [&](const choc::value::ValueView& args) -> choc::value::Value {
                auto path = args[0].toString();
                auto file = juce::File(path);
                if (!file.exists()) return {};

                file.revealToUser();
                return {};
            });

        }
    };

}
