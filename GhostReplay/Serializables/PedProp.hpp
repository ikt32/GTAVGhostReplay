#pragma once
#include <nlohmann/json.hpp>

struct SPedProp {
    int ComponentId;
    int Index;
    int TextureIndex;
};

inline void to_json(nlohmann::ordered_json& j, const SPedProp& prop) {
    j = nlohmann::ordered_json{
        { "ComponentId", prop.ComponentId },
        { "Index", prop.Index },
        { "TextureIndex", prop.TextureIndex },
    };
}

inline void from_json(const nlohmann::ordered_json& j, SPedProp& prop) {
    j.at("ComponentId").get_to(prop.ComponentId);
    j.at("Index").get_to(prop.Index);
    j.at("TextureIndex").get_to(prop.TextureIndex);
}
