//
// Created by August Pemberton on 08/02/2025.
//

#pragma once
#include <choc/network/choc_HTTPServer.h>
#include <choc/gui/choc_WebView.h>
#include <juce_core/juce_core.h>

#include "UIConnection.h"
#include "../AssetServer/AssetServer.h"


namespace imagiro {
    struct ClientInstance: public choc::network::HTTPServer::ClientInstance {
        ClientInstance(AssetServer& s) : server(s) {

        }

        choc::network::HTTPContent getHTTPContent(std::string_view requestedPath) override {
            choc::network::HTTPContent content;
            auto resource = server.getResource(requestedPath);
            if (!resource) return content;

            content.mimeType = resource->mimeType;
            content.content = std::string(resource->data.begin(), resource->data.end());
            return content;
        }

        void handleWebSocketMessage(std::string_view message) override {
            DBG("message received ");
            DBG(std::string(message));
        }

        void upgradedToWebSocket(std::string_view path) override {
            DBG("opened websocket for path " + std::string(path));
        }

    private:
        AssetServer& server;
    };


    class SocketUIConnection : public UIConnection {
    public:
        SocketUIConnection(AssetServer& s, std::string address = "127.0.0.1", uint16_t port = 4350)
                : assetServer(s)
        {
            auto serverPointer = &this->assetServer;
            bool openedOk = server.open(address, port, 0,
                                        [serverPointer] {
                                            return std::make_unique<ClientInstance>(*serverPointer);
                                        },
                                        [](const std::string& error) {
                                            DBG(error);
                                        });

            if (!openedOk) {
                DBG("unable to open web server");
            }
        }

        void bind(const std::string &functionName, CallbackFn&& callback) override {
            //
        }

        void eval(const std::string &functionName, const std::vector<choc::value::ValueView> &args) override {
            //
        }

    private:
        choc::network::HTTPServer server;
        AssetServer& assetServer;
    };
}