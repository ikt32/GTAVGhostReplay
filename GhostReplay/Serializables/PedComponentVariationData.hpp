#pragma once
#include <nlohmann/json.hpp>

struct SPedComponentVariationData {
    int ComponentId;
    int DrawableId;
    int TextureId;
    int PaletteId;
};

inline void to_json(nlohmann::ordered_json& j, const SPedComponentVariationData& componentVariationData) {
    j = nlohmann::ordered_json{
        { "ComponentId", componentVariationData.ComponentId },
        { "DrawableId", componentVariationData.DrawableId },
        { "TextureId", componentVariationData.TextureId },
        { "PaletteId", componentVariationData.PaletteId },
    };
}

inline void from_json(const nlohmann::ordered_json& j, SPedComponentVariationData& componentVariationData) {
    j.at("ComponentId").get_to(componentVariationData.ComponentId);
    j.at("DrawableId").get_to(componentVariationData.DrawableId);
    j.at("TextureId").get_to(componentVariationData.TextureId);
    j.at("PaletteId").get_to(componentVariationData.PaletteId);
}
