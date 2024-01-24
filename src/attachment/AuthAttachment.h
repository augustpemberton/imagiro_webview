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
            viewManager.bind( "juce_getIsAuthorized",
                                 [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                                     return {processor.getAuthManager().isAuthorized()};
                                 }
            );

            viewManager.bind( "juce_getDemoStarted",
                                 [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                                     return {processor.getAuthManager().hasDemoStarted()};
                                 }
            );

            viewManager.bind( "juce_getDemoFinished",
                                 [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                                     return {processor.getAuthManager().hasDemoFinished()};
                                 }
            );

            viewManager.bind( "juce_getDemoTimeLeftSeconds",
                                 [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                                     return {processor.getAuthManager().getDemoTimeLeft().inSeconds()};
                                 }
            );

            viewManager.bind( "juce_startDemo",
                                 [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                                     processor.getAuthManager().startDemo();
                                     return {};
                                 }
            );

            viewManager.bind( "juce_tryAuthorize",
                                 [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                                     auto serial = getStdString(args[0]);
                                     auto success = processor.getAuthManager().tryAuth(serial);
                                     return {success};
                                 }
            );
        }
    };
}
