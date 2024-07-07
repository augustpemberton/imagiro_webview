//
// Created by August Pemberton on 07/07/2024.
//

#pragma once

struct WebviewDataValue {
    std::string value;
    bool saveInPreset {false};
};

class WebviewData {
public:

    void set(std::string key, std::string value, bool saveInPreset) {
        values.insert({key, {value, saveInPreset}});
    }

    choc::value::Value getValue(bool onlyPresetData) {
        auto vals = choc::value::createObject("WebviewData");
        for (auto& [key, val] : values) {
            if (onlyPresetData && !val.saveInPreset) continue;
            vals.addMember(key, val.value);
        }

        return vals;
    }

private:
    std::map<std::string, WebviewDataValue> values;
};

