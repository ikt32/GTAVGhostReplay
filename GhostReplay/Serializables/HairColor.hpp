#pragma once
#include <nlohmann/json.hpp>

struct SHairColor {
    int ColorId;
    int HighlightColorId;
};

inline void to_json(nlohmann::ordered_json& j, const SHairColor& hairColor) {
    j = nlohmann::ordered_json{
        { "ColorId", hairColor.ColorId },
        { "HighlightColorId", hairColor.HighlightColorId },
    };
}

inline void from_json(const nlohmann::ordered_json& j, SHairColor& hairColor) {
    j.at("ColorId").get_to(hairColor.ColorId);
    j.at("HighlightColorId").get_to(hairColor.HighlightColorId);
}
