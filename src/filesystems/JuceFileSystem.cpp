//
// Created by August Pemberton on 22/01/2024.
//

#include "JuceFileSystem.h"

imagiro::JuceFileSystem::JuceFileSystem(const juce::String& prefix,
                                        const juce::String& inspectorPrefix)
: pathPrefix(prefix), inspectorPathPrefix(inspectorPrefix) {

}

bool imagiro::JuceFileSystem::FileExists(const String &file_path) {
    return getFileFromPath(file_path).exists();
}

String imagiro::JuceFileSystem::GetFileMimeType(const String &file_path) {
    auto file = getFileFromPath(file_path);
    if (!file.exists()) return "application/unknown";

    auto mimeType = MimeTypes::getType(file.getFileExtension().toRawUTF8());
    DBG("loaded mime type " << mimeType);

    return mimeType;
}

String imagiro::JuceFileSystem::GetFileCharset(const String &file_path) {
    juce::File file = getFileFromPath(file_path);
    juce::FileInputStream stream(file);

    auto size = static_cast<std::streamsize>(4);
    char* firstFewBytes = new char[size];

    if (stream.openedOk()) {
        // read the first few characters to check for Unicode BOM
        stream.read(firstFewBytes, size);

        if (firstFewBytes[0] == 0xEF && firstFewBytes[1] == 0xBB && firstFewBytes[2] == 0xBF)
            return "utf-8";
        else if (firstFewBytes[0] == 0xFE && firstFewBytes[1] == 0xFF)
            return "utf-16be";
        else if (firstFewBytes[0] == 0xFF && firstFewBytes[1] == 0xFE)
            return "utf-16le";
        else
            return "ascii";
    } else {
        std::cerr << "Unable to open file for reading charset, defaulting to UTF-8" << std::endl;
        return "utf-8";
    }
}

void imagiro::JuceFileSystem::DestroyCallback(void *userData, void *data){
    // Assuming data is dynamically allocated memory
    delete[] reinterpret_cast<char*>(data);
    // If userData holds any resources, deallocate those too.
    // Delete or deallocate userData as per your use case if necessary
}

RefPtr<Buffer> imagiro::JuceFileSystem::OpenFile(const String &file_path) {
    auto file = getFileFromPath(file_path);
    if (!file.existsAsFile()) {
        std::cerr << "File not found! " << file.getFullPathName() << std::endl;
        return {};  // null pointer
    }
    juce::FileInputStream stream(file);
    if (!stream.openedOk()) {
        std::cerr << "File could not be opened for reading! " << file.getFullPathName() << std::endl;
        return {};
    }

    auto size = static_cast<std::streamsize>(stream.getTotalLength());
    char* bufferData = new char[size];
    stream.read(bufferData, size);

    return Buffer::Create(bufferData, size, nullptr, &DestroyCallback);
}

juce::File imagiro::JuceFileSystem::getFileFromPath(const String &filePath) {
    auto path = juce::String(filePath.utf8().data());

    if (path.startsWith("inspector/")) {
        path = inspectorPathPrefix + path;
    } else {
        path = pathPrefix + path;
    }

    DBG("loading file " << path);
    auto file = juce::File(path);
    if (!file.exists()) DBG("file doesnt exist!");
    return file;
}