//
// Created by August Pemberton on 08/02/2025.
//

#pragma once


namespace imagiro {
    class WebUIAttachment : public UIAttachment {
    public:
        explicit WebUIAttachment(WebUIConnection& connection)
            : UIAttachment(connection), connection(connection)
        {

        }

        void addBindings() override {
            connection.bind("juce_requestFileChooser",
                [&](const choc::value::ValueView& args) -> choc::value::Value {
                    connection.requestFileChooser(args[0].toString());
                    return {};
                });
        }

    private:
        WebUIConnection& connection;

    };
}