//
// Created by August Pemberton on 30/10/2023.
//

#pragma once
#include <imagiro_processor/imagiro_processor.h>
#include "WebUIAttachment.h"

namespace imagiro {
    class AuthAttachment : public WebUIAttachment, AuthorizationManager::Listener {
    public:

        AuthAttachment(WebProcessor& p, WebViewManager& w)
                : WebUIAttachment(p, w) {
            processor.getAuthManager().addListener(this);
        }

        ~AuthAttachment() override {
            processor.getAuthManager().removeListener(this);
        }

        void addBindings() override {
            webViewManager.bind( "juce_getIsAuthorized",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     return choc::value::Value(processor.getAuthManager().isAuthorized());
                                 }
            );

            webViewManager.bind( "juce_getIsSerialValid",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     return choc::value::Value(processor.getAuthManager().isAuthorized(true));
                                 }
            );

            webViewManager.bind( "juce_getDemoStarted",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     return choc::value::Value(processor.getAuthManager().hasDemoStarted());
                                 }
            );

            webViewManager.bind( "juce_getDemoFinished",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     return choc::value::Value(processor.getAuthManager().hasDemoFinished());
                                 }
            );

            webViewManager.bind( "juce_getDemoTimeLeftSeconds",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     return choc::value::Value(processor.getAuthManager().getDemoTimeLeft().inSeconds());
                                 }
            );

            webViewManager.bind( "juce_startDemo",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     processor.getAuthManager().startDemo();
                                     return {};
                                 }
            );

            webViewManager.bind( "juce_tryAuthorize",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     auto serial = args[0].toString();
                                     auto success = processor.getAuthManager().tryAuth(serial);
                                     return choc::value::createBool(success);
                                 }
            );

            webViewManager.bind( "juce_getSerial",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     return choc::value::Value {processor.getAuthManager().getSerial().toStdString()};
                                 });

            webViewManager.bind( "juce_logOut",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     processor.getAuthManager().logout();
                                     return {};
                                 });
        }

        void onAuthSuccess() override {
            webViewManager.evaluateJavascript("window.ui.onAuthSuccess()");
        }
    };
}
