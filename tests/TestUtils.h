#pragma once
#include <juce_events/juce_events.h>
#include <juce_core/juce_core.h>

namespace imagiro {

// RAII wrapper to initialize JUCE for tests
struct JuceTestInit {
    JuceTestInit() {
        juce::MessageManager::getInstance();
    }

    ~JuceTestInit() {
        juce::MessageManager::deleteInstance();
    }
};

// RAII cleanup for temporary files/directories
struct TempFileCleanup {
    juce::File file;

    explicit TempFileCleanup(const juce::File& f) : file(f) {}

    ~TempFileCleanup() {
        if (file.exists()) {
            file.deleteRecursively();
        }
    }
};

// Mock UIConnection base class for testing
class UIConnection {
public:
    virtual ~UIConnection() = default;
    virtual void eval(const std::string& code) = 0;
    virtual void bind(const std::string& name,
                     std::function<choc::value::Value(const choc::value::ValueView&)> func) = 0;
};

} // namespace imagiro
