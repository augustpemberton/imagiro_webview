//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include "WebUIAttachment.h"
#include <imagiro_processor/imagiro_processor.h>

namespace imagiro {
    class PresetAttachment : public WebUIAttachment, public Processor::PresetListener {
    public:
        using WebUIAttachment::WebUIAttachment;
        ~PresetAttachment() override {
            processor.removePresetListener(this);
        }

        void addListeners() override {
            processor.addPresetListener(this);
        }

        void addBindings() override {
            webViewManager.bind(
                    "juce_getActivePreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        if (!processor.lastLoadedPreset) return {};
                        return processor.lastLoadedPreset->getState();
                    }
            );
            webViewManager.bind(
                    "juce_getAvailablePresets",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto reloadCache = args[0].getWithDefault(false);
                        if (reloadCache) resources->reloadPresetsMap();
                        auto presets = resources->getPresets(false);

                        auto presetState = choc::value::createObject("Presets");
                        for (const auto& category : presets) {
                            auto categoryState = choc::value::createEmptyArray();
                            for (auto& fbp : category.second) {
                                categoryState.addArrayElement(fbp.getState());
                            }
                            presetState.addMember(category.first.toStdString(), categoryState);
                        }
                        return presetState;
                    }
            );
            webViewManager.bind(
                    "juce_createPreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto name = args[0].toString();
                        auto category = args[1].toString();
                        auto preset = processor.createPreset(name);
                        FileBackedPreset::save(preset, category);
                        webViewManager.evaluateJavascript("window.ui.reloadPresets()");
                        return {};
                    }
            );
            webViewManager.bind(
                    "juce_getCurrentSettingsAsPreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto time = juce::Time();
                        auto preset = processor.createPreset(
                                juce::String(time.getDayOfMonth()) +
                                juce::String("-") +
                                juce::String(time.getMonthName(true)) +
                                juce::String("-") +
                                juce::String(time.getYear())
                        );

                        return preset.getState();
                    }
            );

            webViewManager.bind(
                    "juce_setPresetFromString",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto preset = Preset::fromState(args[0]);
                        processor.queuePreset(preset);
                        return {};
                    }
            );


            webViewManager.bind(
                    "juce_deletePreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto relpath = args[0].toString();
                        auto presetFile = resources->getPresetsFolder().getChildFile(relpath);
                        if (!presetFile.exists()) return {};

                        presetFile.deleteFile();
                        resources->reloadPresetsMap();
                        return {};
                    }
            );

            webViewManager.bind(
                    "juce_favoritePreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto relpath = args[0].toString();
                        auto shouldFavorite = args[1].getWithDefault(true);

                        auto presetFile = resources->getPresetsFolder().getChildFile(relpath);
                        if (!presetFile.exists()) return {};
                        auto preset = FileBackedPreset::createFromFile(presetFile);
                        preset->setFavorite(shouldFavorite);

                        return {};
                    }
            );

            webViewManager.bind(
                    "juce_loadPreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto relpath = args[0].toString();
                        auto presetFile = resources->getPresetsFolder().getChildFile(relpath);
                        if (!presetFile.exists()) return {};
                        auto preset = FileBackedPreset::createFromFile(presetFile);
                        if (!preset) return {};
                        processor.queuePreset(*preset);
                        return {};
                    }
            );

            webViewManager.bind(
                    "juce_nextPreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto presetsList = resources->getPresetsList();
                        if (presetsList.empty()) return {};

                        auto nextPreset = resources->getPresetsList()[0];
                        if (processor.lastLoadedPreset) {
                            nextPreset = resources->getNextPreset(processor.lastLoadedPreset->getFile());
                        }

                        processor.queuePreset(nextPreset);

                        return {};
                    }
            );
            webViewManager.bind(
                    "juce_prevPreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto presetsList = resources->getPresetsList();
                        if (presetsList.empty()) return {};

                        auto list = resources->getPresetsList();
                        auto nextPreset = list[list.size()-1];
                        if (processor.lastLoadedPreset) {
                            nextPreset = resources->getPrevPreset(processor.lastLoadedPreset->getFile());
                        }

                        processor.queuePreset(nextPreset);
                        return {};
                    }
            );

            webViewManager.bind("juce_revealPresetsFolder",
            [&](const choc::value::ValueView& args) -> choc::value::Value {
                resources->getPresetsFolder().revealToUser();
                return {};
            });
        }

        void OnPresetChange(Preset &preset) override {
            webViewManager.evaluateJavascript("window.ui.reloadPresets()");
        }

    private:
        juce::SharedResourcePointer<Resources> resources;
    };
}
