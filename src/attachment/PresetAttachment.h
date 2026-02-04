//
// Created by August Pemberton on 28/10/2023.
//

#pragma once
#include "UIAttachment.h"
#include <choc/text/choc_JSON.h>
#include <filesystem>

#include "imagiro_processor/processor/Processor.h"
#include "imagiro_util/filewatcher/gin_filewatcher.h"

namespace imagiro {

class PresetAttachment : public UIAttachment, public FileSystemWatcher::Listener {
public:
    PresetAttachment(UIConnection& connection, Processor& p)
            : UIAttachment(connection), processor(p)
    {
        watcher.addFolder(resources->getPresetsFolder());
        watcher.addListener(this);
    }

    ~PresetAttachment() override {
        watcher.removeListener(this);
    }

    void folderChanged(const juce::File) override {
        reloadAndNotify();
    }

    void addBindings() override {
        connection.bind(
                "juce_getActivePreset",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    if (!lastLoadedPreset) return {};
                    return presetToUIState(*lastLoadedPreset, lastLoadedPresetPath);
                }
        );

        connection.bind(
                "juce_getAvailablePresets",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto reloadCache = args[0].getWithDefault(false);
                    if (reloadCache || presetsCache.empty()) {
                        reloadPresets();
                    }

                    auto presetState = choc::value::createObject("Presets");
                    for (const auto& [category, presets] : presetsCache) {
                        auto categoryState = choc::value::createEmptyArray();
                        for (const auto& presetInfo : presets) {
                            categoryState.addArrayElement(presetInfo);
                        }
                        presetState.addMember(category, categoryState);
                    }
                    return presetState;
                }
        );

        connection.bind(
                "juce_createPreset",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto name = std::string(args[0].toString());
                    auto description = std::string(args[1].toString());
                    auto category = std::string(args[2].toString());

                    PresetMetadata metadata;
                    metadata.name = name;
                    metadata.description = description;

                    auto preset = processor.savePreset(metadata);

                    // Create category folder if needed
                    auto categoryFolder = resources->getPresetsFolder().getChildFile(category);
                    if (!categoryFolder.exists()) {
                        categoryFolder.createDirectory();
                    }

                    auto presetFile = categoryFolder.getChildFile(name + ".json");
                    preset.saveToFile(presetFile.getFullPathName().toStdString());

                    // Load the newly created preset
                    lastLoadedPreset = preset;
                    lastLoadedPresetPath = presetFile.getRelativePathFrom(resources->getPresetsFolder()).toStdString();

                    reloadAndNotify();
                    connection.eval("window.ui.presetChanged");
                    return {};
                }
        );

        connection.bind(
                "juce_getCurrentSettingsAsPreset",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto time = juce::Time::getCurrentTime();
                    auto name = juce::String(time.getDayOfMonth()) +
                               juce::String("-") +
                               juce::String(time.getMonthName(true)) +
                               juce::String("-") +
                               juce::String(time.getYear());

                    PresetMetadata metadata;
                    metadata.name = name.toStdString();
                    auto preset = processor.savePreset(metadata);

                    return presetToUIState(preset, "");
                }
        );

        connection.bind(
                "juce_loadPresetFromString",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto jsonString = std::string(choc::json::toString(args[0]));
                    auto j = nlohmann::json::parse(jsonString);
                    auto preset = Preset::fromJson(j);
                    if (preset) {
                        processor.loadPreset(*preset);
                        lastLoadedPreset = *preset;
                        lastLoadedPresetPath = "";
                        connection.eval("window.ui.presetChanged");
                    }
                    return {};
                }
        );

        connection.bind(
                "juce_savePresetFromJSON",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto category = std::string(args[0].toString());
                    auto presetJsonValue = args[1];

                    // Convert choc value to JSON string and parse
                    auto jsonString = std::string(choc::json::toString(presetJsonValue));
                    auto j = nlohmann::json::parse(jsonString);

                    // Parse preset from JSON
                    auto preset = Preset::fromJson(j);
                    if (!preset) {
                        throw std::runtime_error("Failed to parse preset JSON");
                    }

                    // Create category folder if needed
                    auto categoryFolder = resources->getPresetsFolder().getChildFile(category);
                    if (!categoryFolder.exists()) {
                        if (!categoryFolder.createDirectory()) {
                            throw std::runtime_error("Failed to create category folder: " + category);
                        }
                    }

                    // Save preset to file
                    auto presetFile = categoryFolder.getChildFile(preset->metadata().name + ".json");
                    if (!preset->saveToFile(presetFile.getFullPathName().toStdString())) {
                        throw std::runtime_error("Failed to save preset file: " + preset->metadata().name);
                    }

                    reloadAndNotify();
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
                    reloadPresets();
                    return {};
                }
        );

        connection.bind(
                "juce_favoritePreset",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto relpath = std::string(args[0].toString());
                    auto shouldFavorite = args[1].getWithDefault(true);

                    // Store favorite status in config
                    auto& configFile = resources->getConfigFile();
                    auto favoritesKey = "favorites";
                    auto favorites = configFile->getValue(favoritesKey, "").toStdString();

                    std::set<std::string> favoriteSet;
                    if (!favorites.empty()) {
                        std::stringstream ss(favorites);
                        std::string item;
                        while (std::getline(ss, item, '|')) {
                            favoriteSet.insert(item);
                        }
                    }

                    if (shouldFavorite) {
                        favoriteSet.insert(relpath);
                    } else {
                        favoriteSet.erase(relpath);
                    }

                    std::string newFavorites;
                    for (const auto& f : favoriteSet) {
                        if (!newFavorites.empty()) newFavorites += "|";
                        newFavorites += f;
                    }
                    configFile->setValue(favoritesKey, juce::String(newFavorites));
                    configFile->save();

                    reloadPresets();
                    return {};
                }
        );

        connection.bind(
                "juce_loadPreset",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto relpath = std::string(args[0].toString());
                    auto presetFile = resources->getPresetsFolder().getChildFile(relpath);
                    if (!presetFile.exists()) return {};

                    auto preset = Preset::loadFromFile(presetFile.getFullPathName().toStdString());
                    if (!preset) return {};

                    processor.loadPreset(*preset);
                    lastLoadedPreset = *preset;
                    lastLoadedPresetPath = relpath;
                    connection.eval("window.ui.presetChanged");
                    return {};
                }
        );

        connection.bind(
                "juce_nextPreset",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto presetsList = getPresetsList();
                    if (presetsList.empty()) return {};

                    size_t nextIndex = 0;
                    if (!lastLoadedPresetPath.empty()) {
                        for (size_t i = 0; i < presetsList.size(); i++) {
                            if (presetsList[i] == lastLoadedPresetPath) {
                                nextIndex = (i + 1) % presetsList.size();
                                break;
                            }
                        }
                    }

                    auto presetFile = resources->getPresetsFolder().getChildFile(presetsList[nextIndex]);
                    auto preset = Preset::loadFromFile(presetFile.getFullPathName().toStdString());
                    if (preset) {
                        processor.loadPreset(*preset);
                        lastLoadedPreset = *preset;
                        lastLoadedPresetPath = presetsList[nextIndex];
                        connection.eval("window.ui.presetChanged");
                    }
                    return {};
                }
        );

        connection.bind(
                "juce_prevPreset",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto presetsList = getPresetsList();
                    if (presetsList.empty()) return {};

                    size_t prevIndex = presetsList.size() - 1;
                    if (!lastLoadedPresetPath.empty()) {
                        for (size_t i = 0; i < presetsList.size(); i++) {
                            if (presetsList[i] == lastLoadedPresetPath) {
                                prevIndex = (i == 0) ? presetsList.size() - 1 : i - 1;
                                break;
                            }
                        }
                    }

                    auto presetFile = resources->getPresetsFolder().getChildFile(presetsList[prevIndex]);
                    auto preset = Preset::loadFromFile(presetFile.getFullPathName().toStdString());
                    if (preset) {
                        processor.loadPreset(*preset);
                        lastLoadedPreset = *preset;
                        lastLoadedPresetPath = presetsList[prevIndex];
                        connection.eval("window.ui.presetChanged");
                    }
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
                    // For performance, always return false
                    // Could implement state comparison if needed
                    return choc::value::Value(false);
                }
        );

        connection.bind(
                "juce_setDefaultPreset",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto relpath = args[0].toString();
                    auto presetFile = resources->getPresetsFolder().getChildFile(relpath);
                    if (!presetFile.exists()) return {};

                    auto& configFile = resources->getConfigFile();
                    configFile->setValue("defaultPresetPath", presetFile.getFullPathName());
                    configFile->save();

                    return {};
                });

        connection.bind(
                "juce_clearDefaultPreset",
                [&](const choc::value::ValueView &args) -> choc::value::Value {
                    auto& configFile = resources->getConfigFile();
                    configFile->removeValue("defaultPresetPath");
                    configFile->save();
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

private:
    Processor& processor;
    juce::SharedResourcePointer<Resources> resources;
    FileSystemWatcher watcher;
    std::mutex fileActionMutex;

    std::optional<Preset> lastLoadedPreset;
    std::string lastLoadedPresetPath;

    // Cache of presets organized by category
    std::map<std::string, std::vector<choc::value::Value>> presetsCache;

    std::set<std::string> getFavorites() {
        std::set<std::string> favoriteSet;
        auto favorites = resources->getConfigFile()->getValue("favorites", "").toStdString();
        if (!favorites.empty()) {
            std::stringstream ss(favorites);
            std::string item;
            while (std::getline(ss, item, '|')) {
                favoriteSet.insert(item);
            }
        }
        return favoriteSet;
    }

    void reloadAndNotify() {
        reloadPresets();
        connection.eval("window.ui.reloadPresets");
    }

    void reloadPresets() {
        std::scoped_lock lock(fileActionMutex);
        presetsCache.clear();

        auto presetsFolder = resources->getPresetsFolder();
        auto favorites = getFavorites();

        // Scan for preset files (both legacy .impreset and new .json)
        juce::Array<juce::File> impresetFiles;
        juce::Array<juce::File> jsonFiles;
        presetsFolder.findChildFiles(impresetFiles, juce::File::findFiles, true, "*.impreset");
        presetsFolder.findChildFiles(jsonFiles, juce::File::findFiles, true, "*.json");

        // Build map of json preset names (from metadata) for deduplication
        // Key: category + "/" + preset.metadata.name
        std::map<std::string, juce::File> jsonPresetsByName;
        for (const auto& file : jsonFiles) {
            auto parentFolder = file.getParentDirectory();
            auto category = parentFolder == presetsFolder
                ? "Default"
                : parentFolder.getFileName().toStdString();

            auto preset = Preset::loadFromFile(file.getFullPathName().toStdString());
            if (!preset) continue;

            auto presetKey = category + "/" + preset->metadata().name;
            jsonPresetsByName[presetKey] = file;
        }

        // Process .impreset files, but skip if .json version with same preset name exists
        for (const auto& file : impresetFiles) {
            auto parentFolder = file.getParentDirectory();
            auto category = parentFolder == presetsFolder
                ? "Default"
                : parentFolder.getFileName().toStdString();

            auto preset = Preset::loadFromFile(file.getFullPathName().toStdString());
            if (!preset) continue;

            auto presetKey = category + "/" + preset->metadata().name;

            // If a .json version with same metadata name exists, delete the old .impreset and skip
            if (jsonPresetsByName.count(presetKey) > 0) {
                file.deleteFile();
                continue;
            }

            auto relpath = file.getRelativePathFrom(presetsFolder).toStdString();
            auto uiState = presetToUIState(*preset, relpath);
            uiState.setMember("favorite", choc::value::Value(favorites.count(relpath) > 0));
            presetsCache[category].push_back(uiState);
        }

        // Process all .json files
        for (const auto& file : jsonFiles) {
            auto relpath = file.getRelativePathFrom(presetsFolder).toStdString();
            auto parentFolder = file.getParentDirectory();
            auto category = parentFolder == presetsFolder
                ? "Default"
                : parentFolder.getFileName().toStdString();

            auto preset = Preset::loadFromFile(file.getFullPathName().toStdString());
            if (!preset) continue;

            auto uiState = presetToUIState(*preset, relpath);
            uiState.setMember("favorite", choc::value::Value(favorites.count(relpath) > 0));
            presetsCache[category].push_back(uiState);
        }
    }

    std::vector<std::string> getPresetsList() {
        std::vector<std::string> list;
        for (const auto& [category, presets] : presetsCache) {
            for (const auto& preset : presets) {
                if (preset.hasObjectMember("path")) {
                    list.push_back(std::string(preset["path"].getString()));
                }
            }
        }
        return list;
    }

    choc::value::Value presetToUIState(const Preset& preset, const std::string& path) {
        auto state = choc::value::createObject("Preset");
        state.setMember("path", choc::value::Value(path));
        state.setMember("name", choc::value::Value(preset.metadata().name));
        state.setMember("description", choc::value::Value(preset.metadata().description));
        state.setMember("favorite", choc::value::Value(false));
        state.setMember("available", choc::value::Value(true));
        state.setMember("errorString", choc::value::Value(std::string("")));

        // Convert param states from preset state json
        auto paramStates = choc::value::createEmptyArray();
        if (preset.state().contains("params")) {
            auto paramsJson = preset.state()["params"];
            if (paramsJson.is_object()) {
                for (auto& [uid, value] : paramsJson.items()) {
                    auto paramState = choc::value::createObject("ParamState");
                    paramState.setMember("uid", choc::value::Value(uid));
                    if (value.is_number()) {
                        paramState.setMember("value", choc::value::Value(value.get<float>()));
                    }
                    paramState.setMember("config", choc::value::Value(std::string("")));
                    paramState.setMember("locked", choc::value::Value(false));
                    paramStates.addArrayElement(paramState);
                }
            }
        }
        state.setMember("paramStates", paramStates);

        return state;
    }
};

} // namespace imagiro
