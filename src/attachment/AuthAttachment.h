//
// Created by August Pemberton on 30/10/2023.
//

#pragma once
#include <imagiro_processor/imagiro_processor.h>
#include "WebUIAttachment.h"

namespace imagiro {
    class AuthAttachment : public WebUIAttachment, AuthorizationManager::Listener {
    public:

        AuthAttachment(WebProcessor& p, AuthorizationManager& a)
                : WebUIAttachment(p, p.getWebViewManager()), authManager(a) {
            authManager.addListener(this);
        }

        ~AuthAttachment() override {
            authManager.removeListener(this);
        }

        void addBindings() override {
            webViewManager.bind( "juce_getIsAuthorized",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     return choc::value::Value(authManager.isAuthorized());
                                 }
            );

            webViewManager.bind( "juce_getIsSerialValid",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     return choc::value::Value(authManager.isAuthorized(true));
                                 }
            );

            webViewManager.bind( "juce_getDemoStarted",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     return choc::value::Value(authManager.hasDemoStarted());
                                 }
            );

            webViewManager.bind( "juce_getDemoFinished",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     return choc::value::Value(authManager.hasDemoFinished());
                                 }
            );

            webViewManager.bind( "juce_getDemoTimeLeftSeconds",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     return choc::value::Value(authManager.getDemoTimeLeft().inSeconds());
                                 }
            );

            webViewManager.bind( "juce_startDemo",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     authManager.startDemo();
                                     return {};
                                 }
            );

            webViewManager.bind( "juce_tryAuthorize",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     auto serial = args[0].toString();
                                     auto success = authManager.tryAuth(serial);
                                     return choc::value::createBool(success);
                                 }
            );

            webViewManager.bind( "juce_getSerial",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     return choc::value::Value {authManager.getSerial().toStdString()};
                                 });

            webViewManager.bind( "juce_logOut",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     authManager.logout();
                                     return {};
                                 });

            webViewManager.bind( "juce_getNumBeats",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                auto b = 2;
#ifdef SERIAL_REMAINDER
                b = SERIAL_REMAINDER;
#endif
                                     return choc::value::Value(b);
                                 });

            webViewManager.bind( "juce_getBeatClock",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                authManager.cancelAuth();
                return {};
            });
        }

        void onAuthStateChanged(bool authorized) override {
             if (authorized) {
                 webViewManager.evaluateJavascript("window.ui.onAuthStateChanged(true)");
             } else {
                 webViewManager.evaluateJavascript("window.ui.onAuthStateChanged(false)");
             }
        }

        AuthorizationManager& authManager;
    };
}
