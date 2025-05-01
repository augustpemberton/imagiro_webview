//
// Created by August Pemberton on 08/02/2025.
//

#pragma once
#include "../connection/web/WebProcessor.h"


namespace imagiro {
    class WebUIAttachment : public UIAttachment {
    public:
        explicit WebUIAttachment(WebUIConnection& connection, WebProcessor& p)
            : UIAttachment(connection), connection(connection), processor(p)
        {

        }

        void addBindings() override {
            connection.bind("juce_getDefaultWindowSize", [&](const choc::value::ValueView& args) {
                  auto dims = choc::value::createObject("WindowSize");
                  dims.setMember("width", processor.getDefaultWindowSize().x);
                  dims.setMember("height", processor.getDefaultWindowSize().y);
                  return dims;
            });

            connection.bind("juce_requestFileChooser",
                [&](const choc::value::ValueView& args) -> choc::value::Value {
                    connection.requestFileChooser(args[0].toString());
                    return {};
                });
        }

    ~WebUIAttachment() override = default;

    private:
        WebUIConnection& connection;
        WebProcessor& processor;

    };
}