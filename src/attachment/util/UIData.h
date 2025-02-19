//
// Created by August Pemberton on 07/07/2024.
//

#pragma once

struct UIDataValue {
    std::string value;
    bool saveInPreset {false};

    choc::value::Value getState() {
        auto val = choc::value::createObject("UIDataValue");
        val.setMember("value", value);
        return val;
    }

    static UIDataValue fromState(choc::value::ValueView v, bool isPreset) {
        UIDataValue val;
        if (v.isString()) {
            val.value = v.getWithDefault("");
        } else if (v.isObject() && v.hasObjectMember("value")) {
            val.value = v["value"].getWithDefault("");
            val.saveInPreset = isPreset;
        }
        return val;
    }
};

class UIData {
public:

    void set(std::string key, std::string value, bool saveInPreset) {
        if (values.contains(key)) values[key] = {value, saveInPreset};
        values.insert({key, {value, saveInPreset}});
    }

    std::optional<std::string> get(std::string key) {
        if (!values.contains(key)) return {};
        return values[key].value;
    }

    choc::value::Value getState(bool onlyPresetData) {
        auto vals = choc::value::createObject("UIData");
        for (auto& [key, val] : values) {
            if (onlyPresetData && !val.saveInPreset) continue;
            vals.addMember(key, val.getState());
        }

        return vals;
    }

    static UIData fromState(const choc::value::ValueView& v, bool isPreset) {
        UIData d;
        for (int i=0; i<v.size(); i++) {
            auto val = v.getObjectMemberAt(i);
            d.values.insert({val.name, UIDataValue::fromState(val.value, isPreset)});
        }
        return d;
    }


    std::map<std::string, UIDataValue>& getValues() { return values; };

private:
    std::map<std::string, UIDataValue> values;
};

