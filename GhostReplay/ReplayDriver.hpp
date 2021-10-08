#pragma once
#include "Serializables/HairColor.hpp"
#include "Serializables/HeadBlendData.hpp"
#include "Serializables/HeadOverlayData.hpp"
#include "Serializables/PedComponentVariationData.hpp"
#include "Serializables/PedProp.hpp"

#include <inc/types.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

namespace Driver {
    void SetupHeadBlendDataFunctions();
}

struct SReplayDriverData {
    Hash Model;
    int Type;
    std::vector<SPedComponentVariationData> ComponentVariations;
    std::vector<SPedProp> Props;
    std::optional<SHeadBlendData> HeadBlendData;
    std::optional<std::vector<SHeadOverlayData>> HeadOverlays;
    std::optional<SHairColor> HairColor;
    std::optional<int> EyeColor;

    static SReplayDriverData LoadFrom(Ped ped);
    static void ApplyTo(Ped ped, SReplayDriverData driverData);
};

inline void to_json(nlohmann::ordered_json& j, const SReplayDriverData& replayDriver) {
    j = nlohmann::ordered_json{
        { "Model", replayDriver.Model },
        { "Type", replayDriver.Type },
        { "ComponentVariations", replayDriver.ComponentVariations },
        { "Props", replayDriver.Props },
    };

    if (replayDriver.HeadBlendData != std::nullopt)
        j.push_back({ "HeadBlendData", replayDriver.HeadBlendData.value() });

    if (replayDriver.HeadOverlays != std::nullopt)
        j.push_back({ "HeadOverlays", replayDriver.HeadOverlays.value() });

    if (replayDriver.HairColor != std::nullopt)
        j.push_back({ "HairColor", replayDriver.HairColor.value() });
    
    if (replayDriver.EyeColor != std::nullopt)
        j.push_back({ "EyeColor", replayDriver.EyeColor.value() });
}

inline void from_json(const nlohmann::ordered_json& j, SReplayDriverData& replayDriver) {
    j.at("Model").get_to(replayDriver.Model);
    j.at("Type").get_to(replayDriver.Type);
    j.at("ComponentVariations").get_to(replayDriver.ComponentVariations);

    if (j.contains("Props"))
        j.at("Props").get_to(replayDriver.Props);

    if (j.contains("HeadBlendData")) {
        SHeadBlendData hbd = j.value("HeadBlendData", SHeadBlendData());
        replayDriver.HeadBlendData.emplace(hbd);
    }

    if (j.contains("HeadOverlays")) {
        std::vector<SHeadOverlayData> hod = j.value("HeadOverlays", std::vector<SHeadOverlayData>());
        replayDriver.HeadOverlays.emplace(hod);
    }

    if (j.contains("HairColor")) {
        SHairColor hairColor = j.value("HairColor", SHairColor());
        replayDriver.HairColor.emplace(hairColor);
    }

    if (j.contains("EyeColor")) {
        int eyeColor = j.value("EyeColor", 0);
        replayDriver.EyeColor.emplace(eyeColor);
    }
}
