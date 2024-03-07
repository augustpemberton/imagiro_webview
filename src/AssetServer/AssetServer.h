//
// Created by August Pemberton on 19/02/2024.
//

#pragma once

class AssetServer {
public:
    virtual std::optional<choc::ui::WebView::Options::Resource> getResource(
            const choc::ui::WebView::Options::Path& p,
            const choc::ui::WebView::Options::Method& m,
            const choc::ui::WebView::Options::Headers& h) = 0;
};
