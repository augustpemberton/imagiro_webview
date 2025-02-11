//
// Created by August Pemberton on 08/02/2025.
//

#pragma once
#include <choc/network/choc_HTTPServer.h>
// #include <choc/gui/choc_WebView.h>
#include <juce_core/juce_core.h>

#include "UIConnection.h"
#include "../AssetServer/AssetServer.h"


namespace imagiro
{
    struct ClientInstance : public choc::network::HTTPServer::ClientInstance
    {
        ClientInstance(AssetServer& s, std::unordered_map<std::string, UIConnection::CallbackFn>& fns)
            : functions(fns), assetServer(s)
        {
        }

        ~ClientInstance() override { }

        choc::network::HTTPContent getHTTPContent(std::string_view requestedPath) override {
            choc::network::HTTPContent content;
            auto resource = assetServer.getResource(requestedPath);
            if (!resource) return content;

            content.mimeType = resource->mimeType;
            content.content = std::string(resource->data.begin(), resource->data.end());
            return content;
        }

        void handleWebSocketMessage(const std::string_view messageString) override
        {
            try
            {
                const auto message = choc::json::parse(messageString);
                auto id = message["id"].getWithDefault("");
                auto functionName = message["functionName"].getWithDefault("");
                auto args = choc::value::Value(message["args"]);

                // Capture necessary data and dispatch to message thread
                juce::MessageManager::callAsync([this, id, functionName, args]()
                {
                    auto resultMessage = choc::value::createObject("Message");
                    resultMessage.addMember("type", 1);
                    resultMessage.addMember("id", id);

                    try
                    {
                        const auto function = functions.find(functionName);
                        if (function == functions.end())
                        {
                            resultMessage.setMember("type", 2);
                            resultMessage.addMember("result", "unable to find function " + std::string(functionName));
                        }
                        else
                        {
                            auto result = function->second(args);
                            resultMessage.addMember("result", result);
                        }
                    }
                    catch (const std::exception& e)
                    {
                        resultMessage.setMember("type", 2);
                        resultMessage.addMember("result", e.what());
                    }

                    sendWebSocketMessage(choc::json::toString(resultMessage));
                });
            }
            catch (const std::exception& e)
            {
                DBG("Error handling WebSocket message: " << e.what());
            }
        }

        void upgradedToWebSocket(std::string_view path) override
        {
            DBG("opened websocket for path " << std::string(path));
        }

    private:
        std::unordered_map<std::string, UIConnection::CallbackFn>& functions;
        AssetServer& assetServer;
    };

    class SocketUIConnection : public UIConnection, juce::Timer
    {
    public:
        explicit SocketUIConnection(AssetServer& s, const std::string& address = "0.0.0.0", const uint16_t port = 4350)
            : assetServer(s)
        {
            auto serverPointer = &this->assetServer;
            auto fnsPointer = &this->boundFunctions;
            bool openedOk = server.open(address, port, 0,
                                        [this, serverPointer, fnsPointer]
                                        {
                                            auto client = std::make_shared<ClientInstance>(*serverPointer, *fnsPointer);
                                            {
                                                std::lock_guard l(activeClientsLock);
                                                activeClients.push_back({client});
                                            }
                                            return client;
                                        },
                                        [](const std::string& error)
                                        {
                                            DBG(error);
                                        });
            if (openedOk) {
                DBG(server.getWebSocketAddress());
            }
            else {
                DBG("unable to open web server");
            }

            startTimerHz(20);
        }

        void bind(const std::string& functionName, CallbackFn&& callback) override
        {
            boundFunctions.insert({functionName, callback});
        }

        void timerCallback() override
        {
            std::string js;
            while (jsEvalQueue.try_dequeue(js))
            {
                auto evalMessage = choc::value::createObject("Message");
                evalMessage.addMember("type", 3); // 3 = Evaluate
                evalMessage.addMember("js", js);

                const auto evalString = choc::json::toString(evalMessage);

                std::lock_guard l(activeClientsLock);

                std::erase_if(activeClients, [](const std::weak_ptr<ClientInstance>& client) {
                    return client.expired();
                });

                for (const auto& client : activeClients)
                {
                    if (auto c = client.lock()) c->sendWebSocketMessage(evalString);
                }
            }
        }

        void eval(const std::string& functionName, const std::vector<choc::value::ValueView>& args) override
        {
            auto js = functionName + "(";
            for (auto i = 0u; i < args.size(); i++)
            {
                js += choc::json::toString(args[i]);
                if (i != args.size() - 1) js += ",";
            }
            js += ");";

            jsEvalQueue.enqueue(choc::json::toString(choc::value::Value(js)));
        }

    private:
        choc::network::HTTPServer server;
        AssetServer& assetServer;

        std::unordered_map<std::string, CallbackFn> boundFunctions;

        moodycamel::ConcurrentQueue<std::string> jsEvalQueue{512};

        std::mutex activeClientsLock;
        std::vector<std::weak_ptr<ClientInstance>> activeClients {};
    };
}
