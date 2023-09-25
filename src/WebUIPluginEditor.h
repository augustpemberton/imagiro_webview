//
// Created by August Pemberton on 19/05/2023.
//

#pragma once
#include <choc/gui/choc_WebView.h>
#include "WebProcessor.h"
#include "ChocBrowserComponent.h"
#include "WebUIDebugToolbar.h"

namespace imagiro {
class WebUIPluginEditor : public juce::AudioProcessorEditor, WebViewManager::Listener,
                              public juce::FileDragAndDropTarget {
public:
    WebUIPluginEditor(WebProcessor& p)
            : juce::AudioProcessorEditor(p),
              browser(p.getWebViewManager()),
              debugToolbar(browser)
    {
        setSize(400, 300);
        addAndMakeVisible(browser);
        addAndMakeVisible(debugToolbar);
        browser.getWebViewManager().addListener(this);
    }

    ~WebUIPluginEditor() override {
        browser.getWebViewManager().removeListener(this);
    }

    void fileOpenerRequested(juce::String patternsAllowed) override {
        fileChooser = std::make_unique<juce::FileChooser>(
                "please choose a file",
                juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                patternsAllowed
                );

        auto folderChooserFlags = juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::canSelectMultipleItems;

        fileChooser->launchAsync (folderChooserFlags, [this] (const juce::FileChooser& chooser) {
            auto results = chooser.getResults();
            if (results.isEmpty()) return;
            auto resultsView = choc::value::createEmptyArray();
            for (const auto& r : results) {
                resultsView.addArrayElement(r.getFullPathName().toStdString());
            }
            juce::String js = "window.juceFileOpened(" + choc::json::toString(resultsView) + ")";
            browser.getWebViewManager().evaluateJavascript(js.toStdString());
        });
    }

    static WebUIPluginEditor* createFromHTMLString(WebProcessor& p, const juce::String& html) {
        auto editor = new WebUIPluginEditor(p);
        editor->browser.getWebViewManager().setHTML(html.toStdString());
        return editor;
    }

    static WebUIPluginEditor* createFromURL(WebProcessor& p, const juce::String& url) {
        auto editor = new WebUIPluginEditor(p);
        editor->browser.getWebViewManager().navigate(url.toStdString());
        return editor;
    }

    void resized() override {
        auto b = getLocalBounds();
#if JUCE_DEBUG
        debugToolbar.setBounds(b.removeFromBottom(20));
#endif
        browser.setBounds(b);
    }

    void filesDropped(const juce::StringArray &files, int x, int y) override {
        DBG("files dropped");
    }

    bool isInterestedInFileDrag(const juce::StringArray &files) override {
        return true;
    }

private:
    ChocBrowserComponent browser;
    WebUIDebugToolbar debugToolbar;

    std::unique_ptr<juce::FileChooser> fileChooser;
};

}

