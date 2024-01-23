//
// Created by August Pemberton on 22/01/2024.
//

#pragma once
#include "mime-types/MimeTypes.h"
#include "Ultralight/Ultralight.h"

using namespace ultralight;
namespace imagiro {
    class JuceFileSystem : public FileSystem {
    public:
        JuceFileSystem(const juce::String& prefix = "", const juce::String& inspectorPrefix = "");

        bool FileExists(const ultralight::String &file_path) override;
        String GetFileMimeType(const ultralight::String &file_path) override;
        String GetFileCharset(const ultralight::String &file_path) override;
        RefPtr<Buffer> OpenFile(const ultralight::String &file_path) override;

        static void DestroyCallback(void *userData, void *data);

    private:
        juce::File getFileFromPath(const ultralight::String& filePath);
        juce::String pathPrefix;
        juce::String inspectorPathPrefix;
    };
}