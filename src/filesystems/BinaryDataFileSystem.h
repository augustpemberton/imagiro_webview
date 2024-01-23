//
// Created by August Pemberton on 22/01/2024.
//

#pragma once
#include "mime-types/MimeTypes.h"
#include "Ultralight/Ultralight.h"

using namespace ultralight;
namespace imagiro {
    class BinaryDataFileSystem : public FileSystem {
    public:
        BinaryDataFileSystem();

        bool FileExists(const ultralight::String &file_path) override;
        String GetFileMimeType(const ultralight::String &file_path) override;
        String GetFileCharset(const ultralight::String &file_path) override;
        RefPtr<Buffer> OpenFile(const ultralight::String &file_path) override;

        static void DestroyCallback(void *userData, void *data);

    private:
        std::optional<juce::String> getBinaryDataName(const ultralight::String& filePath);
        juce::String pathPrefix;
        juce::String inspectorPathPrefix;

        void populateBinaryDataMap();

        std::map<juce::String, juce::String> binaryDataMap;
    };
}