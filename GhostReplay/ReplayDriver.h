#pragma once
#include <inc/types.h>
#include <nlohmann/json.hpp>
#include <vector>

enum ePedVariationData
{
    PED_VARIATION_FACE = 0,
    PED_VARIATION_HEAD = 1,
    PED_VARIATION_HAIR = 2,
    PED_VARIATION_TORSO = 3,
    PED_VARIATION_LEGS = 4,
    PED_VARIATION_HANDS = 5,
    PED_VARIATION_FEET = 6,
    PED_VARIATION_EYES = 7,
    PED_VARIATION_ACCESSORIES = 8,
    PED_VARIATION_TASKS = 9,
    PED_VARIATION_TEXTURES = 10,
    PED_VARIATION_TORSO2 = 11,
    MAX_PED_VARIATION
};

struct PedComponentVariationData {
    int ComponentId;
    int DrawableId;
    int TextureId;
    int PaletteId;
};

struct ReplayDriverData {
public:
    Hash Model;
    int Type;
    std::vector<PedComponentVariationData> ComponentVariations;

    static ReplayDriverData LoadFrom(Ped ped);
    static void ApplyTo(Ped ped, ReplayDriverData driverData);
};

inline void to_json(nlohmann::ordered_json& j, const PedComponentVariationData& componentVariationData) {
    j = nlohmann::ordered_json{
        { "ComponentId", componentVariationData.ComponentId },
        { "DrawableId", componentVariationData.DrawableId },
        { "TextureId", componentVariationData.TextureId },
        { "PaletteId", componentVariationData.PaletteId },
    };
}

inline void from_json(const nlohmann::ordered_json& j, PedComponentVariationData& componentVariationData) {
    j.at("ComponentId").get_to(componentVariationData.ComponentId);
    j.at("DrawableId").get_to(componentVariationData.DrawableId);
    j.at("TextureId").get_to(componentVariationData.TextureId);
    j.at("PaletteId").get_to(componentVariationData.PaletteId);
}


inline void to_json(nlohmann::ordered_json& j, const ReplayDriverData& replayDriver) {
    j = nlohmann::ordered_json{
        { "Model", replayDriver.Model },
        { "Type", replayDriver.Type },
        { "ComponentVariations", replayDriver.ComponentVariations }
    };
}

inline void from_json(const nlohmann::ordered_json& j, ReplayDriverData& replayDriver) {
    j.at("Model").get_to(replayDriver.Model);
    j.at("Type").get_to(replayDriver.Type);
    j.at("ComponentVariations").get_to(replayDriver.ComponentVariations);
}
