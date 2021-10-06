#pragma once
#include <inc/types.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

void setupHeadBlendDataFunctions();

struct SPedComponentVariationData {
    int ComponentId;
    int DrawableId;
    int TextureId;
    int PaletteId;
};

struct SHeadBlendData {
    int Shape1;     int32_t pad0;
    int Shape2;     int32_t pad1;
    int Shape3;     int32_t pad2;
    int Skin1;      int32_t pad3;
    int Skin2;      int32_t pad4;
    int Skin3;      int32_t pad5;
    float ShapeMix; int32_t pad6;
    float SkinMix;  int32_t pad7;
    float ThirdMix; int32_t pad8;
};

struct SHeadOverlayData {
    int Value;
    float Alpha;
    int ColorType;
    int ColorId;
    int HighlightId;
};

struct SReplayDriverData {
    Hash Model;
    int Type;
    std::vector<SPedComponentVariationData> ComponentVariations;
    std::optional<SHeadBlendData> HeadBlendData;
    std::optional<std::vector<SHeadOverlayData>> HeadOverlays;

    static SReplayDriverData LoadFrom(Ped ped);
    static void ApplyTo(Ped ped, SReplayDriverData driverData);
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

inline void to_json(nlohmann::ordered_json& j, const SHeadBlendData& hbd) {
    j = nlohmann::ordered_json{
        { "Shape1", hbd.Shape1 },
        { "Shape2", hbd.Shape2 },
        { "Shape3", hbd.Shape3 },
        { "Skin1", hbd.Skin1 },
        { "Skin2", hbd.Skin2 },
        { "Skin3", hbd.Skin3 },
        { "ShapeMix", hbd.ShapeMix },
        { "SkinMix", hbd.SkinMix },
        { "ThirdMix", hbd.ThirdMix },
    };
}

inline void from_json(const nlohmann::ordered_json& j, SHeadBlendData& hbd) {
    j.at("Shape1").get_to(hbd.Shape1);
    j.at("Shape2").get_to(hbd.Shape2);
    j.at("Shape3").get_to(hbd.Shape3);
    j.at("Skin1").get_to(hbd.Skin1);
    j.at("Skin2").get_to(hbd.Skin2);
    j.at("Skin3").get_to(hbd.Skin3);
    j.at("ShapeMix").get_to(hbd.ShapeMix);
    j.at("SkinMix").get_to(hbd.SkinMix);
    j.at("ThirdMix").get_to(hbd.ThirdMix);
}

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

inline void to_json(nlohmann::ordered_json& j, const SReplayDriverData& replayDriver) {
    j = nlohmann::ordered_json{
        { "Model", replayDriver.Model },
        { "Type", replayDriver.Type },
        { "ComponentVariations", replayDriver.ComponentVariations },
    };

    if (replayDriver.HeadBlendData != std::nullopt)
        j.push_back({ "HeadBlendData", replayDriver.HeadBlendData.value() });

    if (replayDriver.HeadOverlays != std::nullopt)
        j.push_back({ "HeadOverlays", replayDriver.HeadOverlays.value() });
}

inline void from_json(const nlohmann::ordered_json& j, SReplayDriverData& replayDriver) {
    j.at("Model").get_to(replayDriver.Model);
    j.at("Type").get_to(replayDriver.Type);
    j.at("ComponentVariations").get_to(replayDriver.ComponentVariations);
    
    if (j.contains("HeadBlendData")) {
        SHeadBlendData hbd = j.value("HeadBlendData", SHeadBlendData());
        replayDriver.HeadBlendData.emplace(hbd);
    }

    if (j.contains("HeadOverlays")) {
        std::vector<SHeadOverlayData> hod = j.value("HeadOverlays", std::vector<SHeadOverlayData>());
        replayDriver.HeadOverlays.emplace(hod);
    }
}
