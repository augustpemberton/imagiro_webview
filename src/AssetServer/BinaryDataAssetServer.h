//
// Created by August Pemberton on 05/12/2023.
//

#pragma once
#include "choc/gui/choc_WebView.h"
#include <utility>
#include "AssetServer.h"

using choc::ui::WebView;
using GetResource = std::function<const char*(const char*, int&)>;
using GetResourceOriginalFilename = std::function<const char*(const char*)>;

namespace imagiro {
    class BinaryDataAssetServer : public AssetServer {
    public:
        explicit BinaryDataAssetServer(GetResource getNamedResourceLambda,
                                       GetResourceOriginalFilename getNamedResourceOriginalFilenameLambda)
                : getNamedResource(getNamedResourceLambda),
                  getNamedResourceOriginalFilename(getNamedResourceOriginalFilenameLambda) {}

        std::optional<WebView::Options::Resource> getResource(const choc::ui::WebView::Options::Path& p,
                                                              const choc::ui::WebView::Options::Method & m,
                                                              const choc::ui::WebView::Options::Headers& h) override {
//            if (p.starts_with("http")) {
//                return getWebResource(juce::URL(p));
//            }

            std::string path = p;
            if (path == "/") {
                path = "index.html";
            }

            auto resourceName = juce::String(path)
                    .replace(".", "_")
                    .replace("-", "").toStdString();

            resourceName = resourceName.substr(resourceName.find_last_of("/") + 1);
            if (isdigit(resourceName[0])) resourceName = "_" + resourceName;

            int resourceSize = 0;
            auto resource = getNamedResource(resourceName.c_str(), resourceSize);
            if (!resource) {
                jassert(resourceName == "favicon_ico" || resourceName == "index_html"); // probably an error if theres something that isnt the favicon we cant find
                return {};
            }

            auto filePath = std::string(getNamedResourceOriginalFilename(
                    resourceName.c_str()));

            auto fileExtension = filePath.substr(filePath.find_last_of(".") + 1);

            for (const auto& [k, v] : h) {
                DBG(p << "=====================");
                DBG(m);
                DBG(k << " => " << v);
            }

            juce::Range<int> byteRange = {-1, -1};
            if (h.contains("Range")) {
                auto rangeString = juce::String(h.at("Range"));
                auto rangeParts = juce::StringArray::fromTokens(rangeString, "=", "");
                jassert(rangeParts.size() == 2);
                jassert(rangeParts[0] == "bytes");
                auto byteRangeStrings = juce::StringArray::fromTokens(rangeParts[1], "-", "");
                jassert(byteRangeStrings.size() == 2);

                byteRange.setEnd(byteRangeStrings[1].getIntValue());
                byteRange.setStart(byteRangeStrings[0].getIntValue());
            }

            return toResource(resource, resourceSize,
                              getMimeType(fileExtension), m, byteRange);
        }
//
//        std::optional<WebView::Options::Resource> getWebResource(juce::URL url) {
//            juce::URL::InputStreamOptions options (juce::URL::ParameterHandling::inAddress);
//
//            juce::StringPairArray responseHeaders;
//            std::unique_ptr<juce::InputStream> stream (url.createInputStream(options
//                                                                                     .withResponseHeaders(&responseHeaders)));
//
//            if (stream == nullptr) {
//                jassertfalse;
//                return {};
//            }
//
//            auto path = url.toString(false).toStdString();
//            auto fileExtension = path.substr(path.find_last_of(".") + 1);
//            if (fileExtension.empty() || fileExtension == path) fileExtension = "html";
//
//            auto responseString = stream->readEntireStreamAsString();
//            return toResource(responseString,
//                              responseHeaders.getValue("Content-Type", "text/html")
//                                      .toStdString());
//        }

        std::string getMimeType (std::string_view extension)
        {
            if (extension == "css")  return "text/css";
            if (extension == "html") return "text/html";
            if (extension == "js")   return "text/javascript";
            if (extension == "mjs")  return "text/javascript";
            if (extension == "svg")  return "image/svg+xml";
            if (extension == "png")  return "image/png";
            if (extension == "wasm") return "application/wasm";
            if (extension == "woff2") return "font/woff2";
            if (extension == "webm") return "video/webm";

            return "application/octet-stream";
        }

        WebView::Options::Resource toResource(const char* data, int size, const std::string& mimeType,
                                              const std::string& methodType,
                                              juce::Range<int> byteRange = {-1, -1}) {
            std::vector<uint8_t> d;
            std::string contentRange;

            if (byteRange.getStart() == -1 || byteRange.getEnd() > size) {
                d.assign(reinterpret_cast<const uint8_t*>(&data[0]), reinterpret_cast<const uint8_t*>(&data[size]));
                return {d, mimeType};
            } else {
                // Validate and adjust the range to ensure it's within the data size.
                int start = std::max(0, byteRange.getStart());
                int end = std::min(size, byteRange.getEnd());

                d.assign(reinterpret_cast<const uint8_t*>(&data[start]), reinterpret_cast<const uint8_t*>(&data[end+1]));
                contentRange = "bytes " + std::to_string(start) + "-" + std::to_string(std::max(0, end)) + "/" + std::to_string(size);
                return {d, mimeType, contentRange};
            }
        }
//
//        WebView::Options::Resource toResource(juce::String data, const std::string& mimeType,
//                                              juce::Range<int> byteRange = {-1, -1}) {
//            auto p = data.toRawUTF8();
//
//            if (byteRange.getStart() == -1) {
//                byteRange = juce::Range<int>(0, strlen(p));
//            }
//
//            auto d = std::vector<uint8_t>(p + byteRange.getStart(),
//                                          p + byteRange.getEnd());
//
//            return {d, mimeType};
//        }

    private:

        GetResource getNamedResource;
        GetResourceOriginalFilename getNamedResourceOriginalFilename;
    };
} // namespace imagiro
