#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../src/attachment/PresetAttachment.h"
#include "imagiro_processor/processor/Processor.h"
#include "TestUtils.h"

using namespace imagiro;
using namespace Catch::Matchers;

namespace {
    // JUCE initialization for tests
    JuceTestInit juceInit;

    // Mock processor for testing
    class TestProcessor : public Processor {
    public:
        TestProcessor() : Processor("test", {}) {}

        void processBlock(const ProcessState& state, juce::AudioBuffer<float>& buffer) override {}

        Preset savePreset(PresetMetadata metadata = {}) const override {
            // Create a simple test preset
            nlohmann::json state;
            state["params"] = {{"test_param", 0.5f}};
            return Preset(metadata, state);
        }

        void loadPreset(const Preset& preset) override {
            // Mock implementation
        }
    };

    // Mock UI connection for testing
    class TestUIConnection : public UIConnection {
    public:
        std::vector<std::string> evalCalls;

        void eval(const std::string& code) override {
            evalCalls.push_back(code);
        }

        void bind(const std::string& name, std::function<choc::value::Value(const choc::value::ValueView&)> func) override {
            bindings[name] = func;
        }

        choc::value::Value call(const std::string& name, const choc::value::ValueView& args) {
            if (bindings.count(name)) {
                return bindings[name](args);
            }
            return {};
        }

        void clearEvalCalls() { evalCalls.clear(); }

    private:
        std::map<std::string, std::function<choc::value::Value(const choc::value::ValueView&)>> bindings;
    };
}

TEST_CASE("PresetAttachment initialization", "[preset][attachment][init]") {
    TestProcessor processor;
    TestUIConnection connection;

    SECTION("Creates attachment without errors") {
        REQUIRE_NOTHROW(PresetAttachment(connection, processor));
    }

    SECTION("Registers all required bindings") {
        PresetAttachment attachment(connection, processor);

        // Verify key bindings are registered
        auto args = choc::value::createEmptyArray();
        REQUIRE_NOTHROW(connection.call("juce_getAvailablePresets", args));
        REQUIRE_NOTHROW(connection.call("juce_getActivePreset", args));
    }
}

TEST_CASE("Preset file deduplication", "[preset][attachment][dedup]") {
    TestProcessor processor;
    TestUIConnection connection;
    PresetAttachment attachment(connection, processor);

    SECTION("Prefers .json over .impreset with same metadata name") {
        // Create test presets folder
        auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("preset_dedup_test_" + juce::Uuid().toString());
        tempDir.createDirectory();
        TempFileCleanup cleanup(tempDir);

        // Create a .impreset file
        auto impresetFile = tempDir.getChildFile("Test Preset.impreset");
        auto preset1 = processor.savePreset({"Test Preset", "Old format"});
        preset1.saveToFile(impresetFile.getFullPathName().toStdString());

        // Create a .json file with same preset name
        auto jsonFile = tempDir.getChildFile("Test Preset New.json");
        auto preset2 = processor.savePreset({"Test Preset", "New format"});
        preset2.saveToFile(jsonFile.getFullPathName().toStdString());

        // Trigger reload
        attachment.reloadPresets();

        // Verify .impreset was deleted (deduplication)
        REQUIRE_FALSE(impresetFile.existsAsFile());
        REQUIRE(jsonFile.existsAsFile());
    }

    SECTION("Keeps .impreset if no matching .json exists") {
        auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("preset_keep_test_" + juce::Uuid().toString());
        tempDir.createDirectory();
        TempFileCleanup cleanup(tempDir);

        // Create only .impreset file
        auto impresetFile = tempDir.getChildFile("Unique Preset.impreset");
        auto preset = processor.savePreset({"Unique Preset", "Only impreset"});
        preset.saveToFile(impresetFile.getFullPathName().toStdString());

        attachment.reloadPresets();

        // Verify .impreset is kept
        REQUIRE(impresetFile.existsAsFile());
    }
}

TEST_CASE("Preset creation and saving", "[preset][attachment][save]") {
    TestProcessor processor;
    TestUIConnection connection;
    PresetAttachment attachment(connection, processor);

    SECTION("Can create preset from JSON") {
        auto presetJson = choc::value::createObject("TestPreset");
        presetJson.addMember("metadata", choc::value::createObject("meta"));
        presetJson["metadata"].setMember("name", "My Preset");
        presetJson["metadata"].setMember("description", "Test description");

        nlohmann::json state;
        state["params"] = {{"param1", 0.75f}};
        presetJson.addMember("state", choc::value::createObject("state"));

        auto args = choc::value::createArray({
            choc::value::Value("TestCategory"),
            presetJson
        });

        // Should save without throwing
        REQUIRE_NOTHROW(connection.call("juce_savePresetFromJSON", args));
    }

    SECTION("Throws error for invalid JSON") {
        auto invalidJson = choc::value::createObject("Invalid");
        // Missing required fields

        auto args = choc::value::createArray({
            choc::value::Value("TestCategory"),
            invalidJson
        });

        // Should throw error
        REQUIRE_THROWS(connection.call("juce_savePresetFromJSON", args));
    }
}

TEST_CASE("Thread safety and mutex behavior", "[preset][attachment][threading]") {
    TestProcessor processor;
    TestUIConnection connection;
    PresetAttachment attachment(connection, processor);

    SECTION("reloadAndNotify triggers UI notification") {
        connection.clearEvalCalls();

        attachment.reloadAndNotify();

        // Verify UI was notified
        REQUIRE(connection.evalCalls.size() > 0);
        REQUIRE_THAT(connection.evalCalls.back(),
                    ContainsSubstring("window.ui.reloadPresets"));
    }

    SECTION("Multiple concurrent reloads don't deadlock") {
        std::atomic<int> completedReloads{0};
        std::vector<std::thread> threads;

        // Launch multiple threads calling reload
        for (int i = 0; i < 5; i++) {
            threads.emplace_back([&]() {
                attachment.reloadPresets();
                completedReloads++;
            });
        }

        // Wait for all threads
        for (auto& t : threads) {
            t.join();
        }

        // All should complete successfully
        REQUIRE(completedReloads == 5);
    }
}

TEST_CASE("Preset loading and retrieval", "[preset][attachment][load]") {
    TestProcessor processor;
    TestUIConnection connection;
    PresetAttachment attachment(connection, processor);

    SECTION("Returns empty when no preset loaded") {
        auto args = choc::value::createEmptyArray();
        auto result = connection.call("juce_getActivePreset", args);

        // Should return empty/null value
        REQUIRE(result.isVoid() || result.isObject());
    }

    SECTION("Can load preset from string") {
        // Create a preset JSON
        nlohmann::json presetData;
        presetData["metadata"]["name"] = "Test Preset";
        presetData["metadata"]["description"] = "Description";
        presetData["state"]["params"]["test"] = 0.5f;

        auto jsonString = presetData.dump();
        auto presetValue = choc::json::parse(jsonString);

        auto args = choc::value::createArray({presetValue});

        REQUIRE_NOTHROW(connection.call("juce_loadPresetFromString", args));
    }
}

TEST_CASE("Favorite preset management", "[preset][attachment][favorites]") {
    TestProcessor processor;
    TestUIConnection connection;
    PresetAttachment attachment(connection, processor);

    SECTION("Can favorite a preset") {
        auto args = choc::value::createArray({
            choc::value::Value("TestCategory/MyPreset.json"),
            choc::value::Value(true)
        });

        REQUIRE_NOTHROW(connection.call("juce_favoritePreset", args));
    }

    SECTION("Can unfavorite a preset") {
        auto args = choc::value::createArray({
            choc::value::Value("TestCategory/MyPreset.json"),
            choc::value::Value(false)
        });

        REQUIRE_NOTHROW(connection.call("juce_favoritePreset", args));
    }
}

TEST_CASE("Preset deletion", "[preset][attachment][delete]") {
    TestProcessor processor;
    TestUIConnection connection;
    PresetAttachment attachment(connection, processor);

    SECTION("Can delete existing preset file") {
        // Create temp preset file
        auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("preset_delete_test_" + juce::Uuid().toString());
        tempDir.createDirectory();
        TempFileCleanup cleanup(tempDir);

        auto presetFile = tempDir.getChildFile("ToDelete.json");
        auto preset = processor.savePreset({"ToDelete", "Will be deleted"});
        preset.saveToFile(presetFile.getFullPathName().toStdString());

        REQUIRE(presetFile.existsAsFile());

        // Delete via binding
        auto relPath = presetFile.getRelativePathFrom(tempDir).toStdString();
        auto args = choc::value::createArray({choc::value::Value(relPath)});

        connection.call("juce_deletePreset", args);

        // File should be deleted
        REQUIRE_FALSE(presetFile.existsAsFile());
    }
}
