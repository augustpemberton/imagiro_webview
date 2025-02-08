//
// Created by August Pemberton on 19/02/2024.
//

#pragma once

class AssetServer {
public:
    virtual std::optional<choc::ui::WebView::Options::Resource> getResource(
            std::string_view p) = 0;
};
