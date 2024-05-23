//
// Created by August Pemberton on 19/05/2023.
//

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <choc/gui/choc_WebView.h>
#include "WebProcessor.h"
#include "ChocBrowserComponent.h"

namespace imagiro {
class WebUIPluginEditor : public juce::AudioProcessorEditor, WebViewManager::Listener {
public:
    WebUIPluginEditor(WebProcessor& p)
            : juce::AudioProcessorEditor(p),
              browser(*this, p.getWebViewManager())
    {
        setSize(400, 300);
        addAndMakeVisible(browser);
        browser.getWebViewManager().addListener(this);
    }

    ~WebUIPluginEditor() override {
        browser.getWebViewManager().removeListener(this);
    }

    void fileOpenerRequested(juce::String patternsAllowed, bool createNewFile = false) override {
        fileChooser = std::make_unique<juce::FileChooser>(
                "please choose a file",
                juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                patternsAllowed
                );

        auto folderChooserFlags = juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::canSelectMultipleItems;

        if (createNewFile) folderChooserFlags = juce::FileBrowserComponent::FileChooserFlags::saveMode;

        fileChooser->launchAsync (folderChooserFlags, [this] (const juce::FileChooser& chooser) {
            auto results = chooser.getResults();
            if (results.isEmpty()) return;
            auto resultsView = choc::value::createEmptyArray();
            for (const auto& r : results) {
                resultsView.addArrayElement(r.getFullPathName().toStdString());
            }
            juce::String js = "window.ui.juceFileOpened(" + choc::json::toString(resultsView) + ")";
            browser.getWebViewManager().evaluateJavascript(js.toStdString());
        });
    }

    static WebUIPluginEditor* createFromHTMLString(WebProcessor& p, const juce::String& html) {
        auto editor = new WebUIPluginEditor(p);
        editor->browser.getWebViewManager().setHTML(html.toStdString());
        return editor;
    }

    static WebUIPluginEditor* create(WebProcessor& p) {
        auto editor = new WebUIPluginEditor(p);
        return editor;
    }

    static WebUIPluginEditor* createFromURL(WebProcessor& p, const juce::String& url) {
        auto editor = new WebUIPluginEditor(p);
        editor->browser.getWebViewManager().navigate(url.toStdString());
        return editor;
    }

    void resized() override {
        auto b = getLocalBounds();
//#if JUCE_DEBUG
//        debugToolbar.setBounds(b.removeFromBottom(20));
//#endif
        browser.setBounds(b);
    }

private:
    ChocBrowserComponent browser;
    std::unique_ptr<juce::FileChooser> fileChooser;
};

}
