//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include "UIAttachment.h"
#include <imagiro_processor/imagiro_processor.h>
#include <imagiro_util/imagiro_util.h>

namespace imagiro {
class PresetAttachment : public UIAttachment, public Processor::PresetListener, public FileSystemWatcher::Listener {
    public:
        using UIAttachment::UIAttachment;

        PresetAttachment(UIConnection& connection, Processor& p)
                : UIAttachment(connection), processor(p)
        {
            watcher.addFolder(resources->getPresetsFolder());
            watcher.addListener(this);
        }

        ~PresetAttachment() override {
            watcher.removeListener(this);
            processor.removePresetListener(this);
        }

        std::mutex fileActionMutex;

        void folderChanged(const juce::File) override {
            std::lock_guard g (fileActionMutex);
            resources->reloadPresets(&processor);
            connection.eval("window.ui.reloadPresets");
        }

        void addListeners() override {
            processor.addPresetListener(this);
        }

        void addBindings() override {
            connection.bind(
                    "juce_getActivePreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        if (!processor.lastLoadedPreset) return {};
                        return processor.lastLoadedPreset->getUIState();
                    }
            );
            connection.bind(
                    "juce_getAvailablePresets",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto reloadCache = args[0].getWithDefault(false);
                        if (reloadCache) resources->reloadPresetsMap(&processor);
                        auto presets = resources->getPresets(false);

                        auto presetState = choc::value::createObject("Presets");
                        for (const auto& category : presets) {
                            auto categoryState = choc::value::createEmptyArray();
                            for (auto& fbp : category.second) {
                                categoryState.addArrayElement(fbp.getUIState());
                            }
                            presetState.addMember(category.first.toStdString(), categoryState);
                        }
                        return presetState;
                    }
            );

            connection.bind(
                    "juce_createPreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto name = args[0].toString();
                        auto description = args[1].toString();
                        auto category = args[2].toString();
                        auto preset = processor.createPreset(name, false);
                        preset.setDescription(description);
                        auto fbp = FileBackedPreset::save(preset, category);
                        connection.eval("window.ui.reloadPresets");
                        processor.queuePreset(fbp, true);
                        return {};
                    }
            );

            connection.bind(
                    "juce_getCurrentSettingsAsPreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto time = juce::Time::getCurrentTime();
                        auto preset = processor.createPreset(
                                juce::String(time.getDayOfMonth()) +
                                juce::String("-") +
                                juce::String(time.getMonthName(true)) +
                                juce::String("-") +
                                juce::String(time.getYear()),
                                true
                        );

                        return preset.getUIState();
                    }
            );
            connection.bind(
                    "juce_loadPresetFromString",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto preset = Preset::fromState(args[0]);
                        processor.queuePreset(preset);
                        return {};
                    }
            );


            connection.bind(
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

            connection.bind(
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

            connection.bind(
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

            connection.bind(
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
            connection.bind(
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

            connection.bind("juce_revealPresetsFolder",
            [&](const choc::value::ValueView& args) -> choc::value::Value {
                resources->getPresetsFolder().revealToUser();
                return {};
            });

            connection.bind(
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

            connection.bind(
                    "juce_setDefaultPreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto relpath = args[0].toString();
                        auto presetFile = resources->getPresetsFolder().getChildFile(relpath);
                        if (!presetFile.exists()) return {};

                        auto preset = FileBackedPreset::createFromFile(presetFile, &processor);
                        if (!preset) return {};

                        auto presetString = choc::json::toString(preset.value().getUIState());
                        resources->getConfigFile()->setValue("defaultPresetPath", presetFile.getFullPathName());
                        resources->getConfigFile()->setValue("defaultPresetState", juce::String(presetString));
                        resources->getConfigFile()->save();

                        return {};
                    });

            connection.bind(
                    "juce_clearDefaultPreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        resources->getConfigFile()->removeValue("defaultPresetPath");
                        resources->getConfigFile()->removeValue("defaultPresetState");
                        resources->getConfigFile()->save();
                        return {};
                    });

            connection.bind(
                    "juce_getDefaultPreset",
                    [&](const choc::value::ValueView &args) -> choc::value::Value {
                        auto path = resources->getConfigFile()->getValue("defaultPresetPath", "").toStdString();
                        auto file = juce::File(path);
                        if (!file.exists()) return choc::value::Value {""};

                        auto relpath = file.getRelativePathFrom(resources->getPresetsFolder()).toStdString();
                        return choc::value::Value {relpath};
                    });


            connection.bind("juce_revealPreset",
                    [&](const choc::value::ValueView& args) -> choc::value::Value {
                        auto relpath = args[0].toString();
                        auto presetFile = resources->getPresetsFolder().getChildFile(relpath);
                        if (!presetFile.exists()) return {};
                        presetFile.revealToUser();
                        return {};
                    });
        }

        void OnPresetChange(Preset &preset) override {
            connection.eval("window.ui.presetChanged");
        }

    private:
        Processor& processor;
        juce::SharedResourcePointer<Resources> resources;
        FileSystemWatcher watcher;
    };
}
