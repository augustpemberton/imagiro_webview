//
// Created by August Pemberton on 22/01/2024.
//

#include "BinaryDataFileSystem.h"

imagiro::BinaryDataFileSystem::BinaryDataFileSystem() {
    populateBinaryDataMap();
}

bool imagiro::BinaryDataFileSystem::FileExists(const String &file_path) {
    return getBinaryDataName(file_path).has_value();
}

String imagiro::BinaryDataFileSystem::GetFileMimeType(const String &file_path) {
    auto path = juce::String(file_path.utf8().data()).toStdString();
    auto fileExtension = path.substr(path.find_last_of('.') + 1);

    auto mimeType = MimeTypes::getType(fileExtension.c_str());
    return mimeType;
}

String imagiro::BinaryDataFileSystem::GetFileCharset(const String &file_path) {
    int resourceSize = 0;
    auto binaryName = getBinaryDataName(file_path);
    if (!binaryName.has_value()) return "utf-8";

    auto data = BinaryData::getNamedResource(binaryName.value().toRawUTF8(), resourceSize);

    if (data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
        return "utf-8";
    else if (data[0] == 0xFE && data[1] == 0xFF)
        return "utf-16be";
    else if (data[0] == 0xFF && data[1] == 0xFE)
        return "utf-16le";
    else
        return "ascii";
}

void imagiro::BinaryDataFileSystem::DestroyCallback(void *userData, void *data){
    // Assuming data is dynamically allocated memory
//    delete[] reinterpret_cast<char*>(data);
    // If userData holds any resources, deallocate those too.
    // Delete or deallocate userData as per your use case if necessary
}

RefPtr<Buffer> imagiro::BinaryDataFileSystem::OpenFile(const String &file_path) {
    auto binaryName = getBinaryDataName(file_path);
    int resourceSize = 0;

    auto data = BinaryData::getNamedResource(binaryName->toRawUTF8(), resourceSize);

    return Buffer::Create((void*) data, static_cast<size_t>(resourceSize), nullptr, &DestroyCallback);
}

std::optional<juce::String> imagiro::BinaryDataFileSystem::getBinaryDataName(const ultralight::String &filePath) {
    auto path = std::string(filePath.utf8().data());
    auto resourceName = path.substr(path.find_last_of('/') + 1);

    if (!binaryDataMap.contains(resourceName)) return {};
    return binaryDataMap[resourceName];
}

void imagiro::BinaryDataFileSystem::populateBinaryDataMap() {
    for (int index = 0; index < BinaryData::namedResourceListSize; ++index) {
        auto binaryName = BinaryData::namedResourceList[index];
        auto fileName = BinaryData::getNamedResourceOriginalFilename(binaryName);

        binaryDataMap.insert({fileName, binaryName});
    }

}

