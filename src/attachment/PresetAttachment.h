//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include "WebUIAttachment.h"
#include <imagiro_processor/imagiro_processor.h>
#include <choc/text/choc_JSON.h>

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
            viewManager.bind(
                    "juce_getActivePreset",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        if (!processor.lastLoadedPreset) return {};
                        auto presetChoc = processor.lastLoadedPreset->getState();
                        auto presetJS = toJSVal(presetChoc);
                        return presetJS;
                    }
            );
            viewManager.bind(
                    "juce_getAvailablePresets",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        auto reloadCache = args[0].ToBoolean();
                        if (reloadCache) resources->reloadPresetsMap();
                        auto presets = resources->getPresets(false);

                        JSObject presetsJS;
                        for (const auto& category : presets) {
                            JSArray categoryJS;
                            for (auto& fbp : category.second) {
                                DBG("loading");
                                auto presetChoc = fbp.getState();
                                auto presetJS = toJSVal(presetChoc);

                                categoryJS.push(presetJS);
                            }
                            presetsJS[category.first.toRawUTF8()] = JSValue(categoryJS);
                        }

                        return {presetsJS};
                    }
            );

            viewManager.bind(
                    "juce_createPreset",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        auto name = getStdString(args[0]);
                        auto category = getStdString(args[1]);
                        auto preset = processor.createPreset(name, false);
                        FileBackedPreset::save(preset, category);
                        viewManager.evaluateWindowFunction("reloadPresets");
                        return {};
                    }
            );

            viewManager.bind(
                    "juce_getCurrentSettingsAsPreset",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        auto time = juce::Time();
                        auto preset = processor.createPreset(
                                juce::String(time.getDayOfMonth()) +
                                juce::String("-") +
                                juce::String(time.getMonthName(true)) +
                                juce::String("-") +
                                juce::String(time.getYear()),
                                true
                        );

                        auto presetChoc = preset.getState();
                        auto presetJS = toJSVal(presetChoc);
                        return presetJS;
                    }
            );

            viewManager.bind(
                    "juce_setPresetFromString",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        auto preset = Preset::fromState(toChocVal(args[0]));
                        processor.queuePreset(preset);
                        return {};
                    }
            );


            viewManager.bind(
                    "juce_deletePreset",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        auto relpath = getStdString(args[0]);
                        auto presetFile = resources->getPresetsFolder().getChildFile(relpath);
                        if (!presetFile.exists()) return {};

                        presetFile.deleteFile();
                        resources->reloadPresetsMap();
                        return {};
                    }
            );

            viewManager.bind(
                    "juce_favoritePreset",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        auto relpath = getStdString(args[0]);
                        auto shouldFavorite = args[1].ToBoolean();

                        auto presetFile = resources->getPresetsFolder().getChildFile(relpath);
                        if (!presetFile.exists()) return {};
                        auto preset = FileBackedPreset::createFromFile(presetFile);
                        preset->setFavorite(shouldFavorite);

                        return {};
                    }
            );

            viewManager.bind(
                    "juce_loadPreset",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        auto relpath = getStdString(args[0]);
                        auto presetFile = resources->getPresetsFolder().getChildFile(relpath);
                        if (!presetFile.exists()) return {};
                        auto preset = FileBackedPreset::createFromFile(presetFile);
                        if (!preset) return {};
                        processor.queuePreset(*preset);
                        return {};
                    }
            );

            viewManager.bind(
                    "juce_nextPreset",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
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
            viewManager.bind(
                    "juce_prevPreset",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
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

            viewManager.bind("juce_revealPresetsFolder",
            [&](const JSObject& obj, const JSArgs& args) -> JSValue {
                resources->getPresetsFolder().revealToUser();
                return {};
            });

            viewManager.bind(
                    "juce_hasPresetBeenUpdated",
                    [&](const JSObject& obj, const JSArgs &args) -> JSValue {
                        if (!processor.lastLoadedPreset) return JSValue(false);
                        auto lastLoaded = processor.lastLoadedPreset->getPreset();
                        auto current = processor.createPreset(lastLoaded.getName(),
                                                              true);

                        if (lastLoaded.getParamStates().size() != current.getParamStates().size())
                            return JSValue(true);

                        for (auto i=0; i<lastLoaded.getParamStates().size(); i++) {
                            if (lastLoaded.getParamStates()[i] != current.getParamStates()[i]) {
                                return JSValue(true);
                            }
                        }

                        // commented out to make upgrading presets easier
                        // in theory sensible defaults should be set
//                        auto currentData = choc::json::toString(current.getData());
//                        auto lastData = choc::json::toString(lastLoaded.getData());
//                        if (currentData != lastData)
//                            return JSValue(true);

                        return JSValue(false);
                    }
            );
        }

        void OnPresetChange(Preset &preset) override {
            viewManager.evaluateWindowFunction("presetChanged", {});
        }

    private:
        juce::SharedResourcePointer<Resources> resources;
    };
}
