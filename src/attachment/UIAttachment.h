//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include "../connection/UIConnection.h"

namespace imagiro {
    class UIAttachment {
    public:
        explicit UIAttachment(UIConnection& c)
        : connection(c) { }

        virtual ~UIAttachment() = default;

        virtual void addBindings() = 0;
        virtual void addListeners() {}

        virtual void processCallback() {}

    protected:
        UIConnection& connection;
    };
}
