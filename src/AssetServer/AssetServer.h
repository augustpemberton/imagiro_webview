//
// Created by August Pemberton on 19/02/2024.
//

#pragma once

class AssetServer {
public:

    struct Resource
    {
        Resource() = default;
        Resource (std::string_view content, std::string mimeType);

        std::vector<uint8_t> data;
        std::string mimeType;
    };
    virtual std::optional<Resource> getResource(
            std::string_view p) = 0;
};
