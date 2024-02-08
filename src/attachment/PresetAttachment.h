//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include "WebUIAttachment.h"
#include <imagiro_processor/imagiro_processor.h>
#include <imagiro_util/imagiro_util.h>

namespace imagiro {
class PresetAttachment : public WebUIAttachment, public Processor::PresetListener, public FileSystemWatcher::Listener {
    public:
        using WebUIAttachment::WebUIAttachment;

        PresetAttachment(WebProcessor& processor, WebViewManager& manager)
                : WebUIAttachment(processor, manager)
        {
            watcher.addFolder(resources->getPresetsFolder());
            watcher.addListener(this);

            loadDefaultPreset();
        }

        ~PresetAttachment() override {
            watcher.removeListener(this);
            processor.removePresetListener(this);
        }

        void loadDefaultPreset() {
            auto defaultPresetPath = resources->getConfigFile()->getValue("defaultPresetPath", "");
            if (!defaultPresetPath.isEmpty()) {
                auto defaultPresetFile = juce::File(defaultPresetPath);
                if (defaultPresetFile.exists()) {
                    auto preset = FileBackedPreset::createFromFile(defaultPresetPath);
                    if (preset) {
                        processor.queuePreset(preset.value(), true);
                        return;
                    }
                }
            }

            auto defaultPresetString = resources->getConfigFile()->getValue("defaultPresetState", "");
            if (!defaultPresetString.isEmpty()) {
                auto presetValue = choc::json::parse(defaultPresetString.toStdString());
                auto preset = Preset::fromState(presetValue);
                DBG("loading default preset: " << preset.getName());
                processor.queuePreset(preset, true);
            }
        }

        std::mutex fileActionMutex;

        void folderChanged(const juce::File) override {
            std::lock_guard g (fileActionMutex);
            resources->reloadPresets(&processor);
            webViewManager.evaluateJavascript("window.ui.reloadPresets()");
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
                        if (reloadCache) resources->reloadPresetsMap(&processor);
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
                        auto preset = processor.createPreset(name, false);
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
                                juce::String(time.getYear()),
                                true
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
                        auto preset = FileBackedPreset::createFromFile(presetFile, &processor);
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

            webViewManager.bind(
                    "juce_hasPresetBeenUpdated",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        return choc::value::Value(false); // perfomance

                        if (!processor.lastLoadedPreset) return choc::value::Value(false);
                        auto lastLoaded = processor.lastLoadedPreset->getPreset();
                        auto current = processor.createPreset(lastLoaded.getName(),
                                                              true);

                        if (lastLoaded.getParamStates().size() != current.getParamStates().size())
                            return choc::value::Value(true);

                        for (auto i=0; i<lastLoaded.getParamStates().size(); i++) {
                            if (lastLoaded.getParamStates()[i] != current.getParamStates()[i]) {
                                return choc::value::Value(true);
                            }
                        }

                        // commented out to make upgrading presets easier
                        // in theory sensible defaults should be set
//                        auto currentData = choc::json::toString(current.getData());
//                        auto lastData = choc::json::toString(lastLoaded.getData());
//                        if (currentData != lastData)
//                            return choc::value::Value(true);

                        return choc::value::Value(false);
                    }
            );

            webViewManager.bind(
                    "juce_setDefaultPreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto relpath = args[0].toString();
                        auto presetFile = resources->getPresetsFolder().getChildFile(relpath);
                        if (!presetFile.exists()) return {};

                        auto preset = FileBackedPreset::createFromFile(presetFile, &processor);
                        if (!preset) return {};

                        auto presetString = choc::json::toString(preset.value().getState());
                        resources->getConfigFile()->setValue("defaultPresetPath", presetFile.getFullPathName());
                        resources->getConfigFile()->setValue("defaultPresetState", juce::String(presetString));
                        resources->getConfigFile()->save();

                        return {};
                    });

            webViewManager.bind(
                    "juce_clearDefaultPreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        resources->getConfigFile()->removeValue("defaultPresetPath");
                        resources->getConfigFile()->removeValue("defaultPresetState");
                        resources->getConfigFile()->save();
                        return {};
                    });

            webViewManager.bind(
                    "juce_getDefaultPreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto path = resources->getConfigFile()->getValue("defaultPresetPath", "").toStdString();
                        auto file = juce::File(path);
                        if (!file.exists()) return choc::value::Value {""};

                        auto relpath = file.getRelativePathFrom(Resources::getInstance()->getPresetsFolder()).toStdString();
                        return choc::value::Value {relpath};
                    });


            webViewManager.bind("juce_revealPreset",
                    [&](const choc::value::ValueView& args) -> choc::value::Value {
                        auto relpath = args[0].toString();
                        auto presetFile = resources->getPresetsFolder().getChildFile(relpath);
                        if (!presetFile.exists()) return {};
                        presetFile.revealToUser();
                        return {};
                    });
        }

        void OnPresetChange(Preset &preset) override {
            webViewManager.evaluateJavascript("window.ui.presetChanged()");
        }

    private:
        juce::SharedResourcePointer<Resources> resources;
        FileSystemWatcher watcher;
    };
}
