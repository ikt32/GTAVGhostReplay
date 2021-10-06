#pragma once
#include <nlohmann/json.hpp>

struct SHeadOverlayData {
    int Value;
    float Alpha;
    int ColorType;
    int ColorId;
    int HighlightId;
};

inline void to_json(nlohmann::ordered_json& j, const SHeadOverlayData& hod) {
    j = nlohmann::ordered_json{
        { "Value", hod.Value },
        { "Alpha", hod.Alpha },
        { "ColorType", hod.ColorType },
        { "ColorId", hod.ColorId },
        { "HighlightId", hod.HighlightId },
    };
}

inline void from_json(const nlohmann::ordered_json& j, SHeadOverlayData& hod) {
    j.at("Value").get_to(hod.Value);
    j.at("Alpha").get_to(hod.Alpha);
    j.at("ColorType").get_to(hod.ColorType);
    j.at("ColorId").get_to(hod.ColorId);
    j.at("HighlightId").get_to(hod.HighlightId);
}
