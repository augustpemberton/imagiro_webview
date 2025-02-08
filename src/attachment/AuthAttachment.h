//
// Created by August Pemberton on 30/10/2023.
//

#pragma once
#include "UIAttachment.h"
#include <imagiro_processor/imagiro_processor.h>

namespace imagiro {
    class AuthAttachment : public UIAttachment, AuthorizationManager::Listener {
    public:

        AuthAttachment(UIConnection& c, AuthorizationManager& a)
                : UIAttachment(c), authManager(a) {
            authManager.addListener(this);
        }

        ~AuthAttachment() override {
            authManager.removeListener(this);
        }

        void addBindings() override {
            connection.bind( "juce_getIsAuthorized",
                                 [&](const choc::value::ValueView &) -> choc::value::Value {
                                     return choc::value::Value(authManager.isAuthorized());
                                 }
            );

            connection.bind( "juce_getIsSerialValid",
                                 [&](const choc::value::ValueView &) -> choc::value::Value {
                                     return choc::value::Value(authManager.isAuthorized(true));
                                 }
            );

            connection.bind( "juce_getDemoStarted",
                                 [&](const choc::value::ValueView &) -> choc::value::Value {
                                     return choc::value::Value(authManager.hasDemoStarted());
                                 }
            );

            connection.bind( "juce_getDemoFinished",
                                 [&](const choc::value::ValueView &) -> choc::value::Value {
                                     return choc::value::Value(authManager.hasDemoFinished());
                                 }
            );

            connection.bind( "juce_getDemoTimeLeftSeconds",
                                 [&](const choc::value::ValueView &) -> choc::value::Value {
                                     return choc::value::Value(authManager.getDemoTimeLeft().inSeconds());
                                 }
            );

            connection.bind( "juce_startDemo",
                                 [&](const choc::value::ValueView &) -> choc::value::Value {
                                     authManager.startDemo();
                                     return {};
                                 }
            );

            connection.bind( "juce_tryAuthorize",
                                 [&](const choc::value::ValueView &args) -> choc::value::Value {
                                     auto serial = args[0].toString();
                                     auto success = authManager.tryAuth(serial);
                                     return choc::value::createBool(success);
                                 }
            );

            connection.bind( "juce_getSerial",
                                 [&](const choc::value::ValueView &) -> choc::value::Value {
                                     return choc::value::Value {authManager.getSerial().toStdString()};
                                 });

            connection.bind( "juce_logOut",
                                 [&](const choc::value::ValueView &) -> choc::value::Value {
                                     authManager.logout();
                                     return {};
                                 });

            connection.bind( "juce_getNumBeats",
                                 [&](const choc::value::ValueView &) -> choc::value::Value {
                auto b = 2;
#ifdef SERIAL_REMAINDER
                b = SERIAL_REMAINDER;
#endif
                                     return choc::value::Value(b);
                                 });

            connection.bind( "juce_getBeatClock",
                                 [&](const choc::value::ValueView &) -> choc::value::Value {
                authManager.cancelAuth();
                return {};
            });
        }

        void onAuthStateChanged(bool authorized) override {
             if (authorized) {
                 connection.eval("window.ui.onAuthStateChanged", {choc::value::Value(true)});
             } else {
                 connection.eval("window.ui.onAuthStateChanged", {choc::value::Value(false)});
             }
        }

        AuthorizationManager& authManager;
    };
}
