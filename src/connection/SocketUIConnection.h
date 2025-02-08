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
    struct ClientInstance: public choc::network::HTTPServer::ClientInstance, juce::Timer {
        ClientInstance(AssetServer& s, std::unordered_map<std::string, UIConnection::CallbackFn>& fns)
        : functions(fns), assetServer(s)
        {
            startTimerHz(60);
        }
        ~ClientInstance() {
            stopTimer();
        }

        choc::network::HTTPContent getHTTPContent(std::string_view requestedPath) override {
            choc::network::HTTPContent content;
            auto resource = assetServer.getResource(requestedPath);
            if (!resource) return content;

            content.mimeType = resource->mimeType;
            content.content = std::string(resource->data.begin(), resource->data.end());
            return content;
        }

        void handleWebSocketMessage(std::string_view messageString) override {
            auto message = choc::json::parse(messageString);
            auto id = message["id"].getWithDefault("");
            auto functionName = message["functionName"].getWithDefault("");
            auto args = choc::value::Value(message["args"]);

            functionRequestFifo.enqueue({
                id,
                functionName,
                args
            });
        }

        void timerCallback() override {
            FunctionRequest request;
            while (functionRequestFifo.try_dequeue(request)) {
                auto function = functions.find(request.functionName);

                auto resultMessage = choc::value::createObject("Message");
                resultMessage.addMember("type", 1); // 1 for FunctionResponse
                resultMessage.addMember("id", request.requestID);

                if (function == functions.end()) {
                    resultMessage.setMember("type", 2); // 2 for FunctionResponseError
                    resultMessage.addMember("result", "unable to find function " + request.functionName);
                } else {
                    try {
                        auto result = function->second(request.args);
                        resultMessage.addMember("result", result);
                    } catch (std::exception& e) {
                        resultMessage.setMember("type", 2); // 2 for FunctionResponseError
                        resultMessage.addMember("result", e.what());
                    }
                }

                sendWebSocketMessage(choc::json::toString(resultMessage));
            }
        }

        void upgradedToWebSocket(std::string_view path) override {
            DBG("opened websocket for path " + std::string(path));
        }

    private:
        std::unordered_map<std::string, UIConnection::CallbackFn>& functions;
        AssetServer& assetServer;

        struct FunctionRequest {
            std::string requestID;
            std::string functionName;
            choc::value::Value args;
        };
        moodycamel::ReaderWriterQueue<FunctionRequest> functionRequestFifo {1024};
    };

    class SocketUIConnection : public UIConnection {
    public:
        SocketUIConnection(AssetServer& s, std::string address = "127.0.0.1", uint16_t port = 4350)
                : assetServer(s)
        {
            auto serverPointer = &this->assetServer;
            auto fnsPointer = &this->boundFunctions;
            bool openedOk = server.open(address, port, 0,
                                        [serverPointer, fnsPointer] {
                                            return std::make_unique<ClientInstance>(*serverPointer, *fnsPointer);
                                        },
                                        [](const std::string& error) {
                                            DBG(error);
                                        });

            if (!openedOk) {
                DBG("unable to open web server");
            } else {
                DBG(server.getWebSocketAddress());
            }
        }

        void bind(const std::string &functionName, CallbackFn&& callback) override {
            boundFunctions.insert({functionName, callback});
        }

        void eval(const std::string &functionName, const std::vector<choc::value::ValueView> &args) override {
            //
        }

    private:
        choc::network::HTTPServer server;
        AssetServer& assetServer;

        std::unordered_map<std::string, CallbackFn> boundFunctions;
    };
}