//
// Created by August Pemberton on 19/05/2023.
//

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <choc/gui/choc_WebView.h>
#include "WebProcessor.h"
#include "ChocBrowserComponent.h"

#if JUCE_WINDOWS
#include <windows.h>
#include <winrt/Windows.UI.ViewManagement.h>
#endif

namespace imagiro {
class WebUIPluginEditor : public juce::AudioProcessorEditor, WebUIConnection::Listener {
public:
    WebUIPluginEditor(WebProcessor& p, const juce::String& url = "")
            : juce::AudioProcessorEditor(p),
              browser(*this, p.getWebUIConnection())
    {
        addAndMakeVisible(browser);
        browser.getWebUIConnection().addListener(this);
    }

    ~WebUIPluginEditor() override {
        browser.getWebUIConnection().removeListener(this);
    }

    void fileOpenerRequested(const juce::String& patternsAllowed, const bool isNewFile, juce::File openTo) override {
        fileChooser = std::make_unique<juce::FileChooser>(
                "please choose a file",
                openTo.exists() ? openTo : juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                patternsAllowed
                );

        juce::FileBrowserComponent::FileChooserFlags fileChooserFlags;
        if (isNewFile) {
                fileChooserFlags = juce::FileBrowserComponent::saveMode;
        } else {
            fileChooserFlags = static_cast<juce::FileBrowserComponent::FileChooserFlags>(
                juce::FileBrowserComponent::openMode
                | juce::FileBrowserComponent::canSelectFiles
                | juce::FileBrowserComponent::canSelectMultipleItems);

        }

        fileChooser->launchAsync (fileChooserFlags, [this] (const juce::FileChooser& chooser) {
            auto results = chooser.getResults();
            if (results.isEmpty()) return;
            auto resultsView = choc::value::createEmptyArray();
            for (const auto& r : results) {
                resultsView.addArrayElement(r.getFullPathName().toStdString());
            }
            browser.getWebUIConnection().eval("window.ui.juceFileOpened", {resultsView});
        });
    }

    static WebUIPluginEditor* createFromHTMLString(WebProcessor& p, const juce::String& html) {
        auto editor = new WebUIPluginEditor(p);
        editor->browser.getWebUIConnection().setHTML(html.toStdString());
        return editor;
    }

    void resized() override {
        auto b = getLocalBounds();
        browser.setBounds(b);

        resources->getConfigFile()->setValue("defaultWidth", b.getWidth());
        resources->getConfigFile()->setValue("defaultHeight", b.getHeight());
        resources->getConfigFile()->save();
    }

    void paint(juce::Graphics &g) override {
        g.fillAll(juce::Colour(234, 229, 219));
    }

    ChocBrowserComponent& getBrowser() { return browser; }

    void setSizeScaled(int width, int height) {
        auto scale = getWindowsTextScaleFactor();
        setSize(width * scale, height * scale);
    }

    float getWindowsTextScaleFactor()
    {
#if JUCE_WINDOWS
        winrt::Windows::UI::ViewManagement::UISettings settings;
        return settings.TextScaleFactor();
#endif
        return 1.0f; // Default to 1.0 if we couldn't get the scale factor
    }

private:
    ChocBrowserComponent browser;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::SharedResourcePointer<Resources> resources;
};

}
