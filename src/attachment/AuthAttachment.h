//
// Created by August Pemberton on 30/10/2023.
//

#pragma once
#include <imagiro_processor/imagiro_processor.h>
#include "WebUIAttachment.h"

namespace imagiro {
    class AuthAttachment : public WebUIAttachment {
        using WebUIAttachment::WebUIAttachment;

        void addBindings() override {
            webViewManager.bind( "juce_getIsAuthorized",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     return choc::value::Value(processor.auth.isAuthorized());
                                 }
            );

            webViewManager.bind( "juce_getDemoStarted",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     return choc::value::Value(processor.auth.hasDemoStarted());
                                 }
            );

            webViewManager.bind( "juce_getDemoFinished",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     return choc::value::Value(processor.auth.hasDemoFinished());
                                 }
            );

            webViewManager.bind( "juce_getDemoTimeLeftSeconds",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     return choc::value::Value(processor.auth.getDemoTimeLeft().inSeconds());
                                 }
            );

            webViewManager.bind( "juce_startDemo",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     processor.auth.startDemo();
                                     return {};
                                 }
            );

            webViewManager.bind( "juce_tryAuthorize",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     auto serial = args[0].toString();
                                     auto success = processor.auth.tryAuth(serial);
                                     return choc::value::createBool(success);
                                 }
            );
        }
    };
}
