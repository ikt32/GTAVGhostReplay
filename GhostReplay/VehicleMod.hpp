#pragma once
#include <inc/types.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

struct VehicleMod {
    enum class ModType {
        Color,
        ColorCustom1,
        ColorCustom2,
        ExtraColors,
        Mod,
        ModToggle,
        WheelType,
        WindowTint,
        TyresCanBurst,
        Livery,
        TyreSmoke,
        ModColor1,
        ModColor2,
        Extra,
        Neon,
        NeonColor,
        XenonColor,
        PlateType,
        InteriorColor,
        DashboardColor,
    };

    ModType Type;
    int ModId;

    int Value0;
    int Value1;
    int Value2;
    int Value3;

    VehicleMod Get(Vehicle vehicle);
    void Set(Vehicle vehicle);
};

struct VehicleModData {
    std::string Plate;

    std::vector<VehicleMod> VehicleMods;

    static VehicleModData LoadFrom(Vehicle vehicle);
    static void ApplyTo(Vehicle vehicle, VehicleModData modData);
};

inline void to_json(nlohmann::ordered_json& j, const VehicleMod& mod) {
    j = nlohmann::json{
        { "ModType", mod.Type },
        { "ModId", mod.ModId },
        { "Value0", mod.Value0 },
        { "Value1", mod.Value1 },
        { "Value2", mod.Value2 },
        { "Value3", mod.Value3 },
    };
}

inline void from_json(const nlohmann::ordered_json& j, VehicleMod& mod) {
    j.at("ModType").get_to(mod.Type  );
    j.at("ModId"  ).get_to(mod.ModId );
    j.at("Value0" ).get_to(mod.Value0);
    j.at("Value1" ).get_to(mod.Value1);
    j.at("Value2" ).get_to(mod.Value2);
    j.at("Value3" ).get_to(mod.Value3);
}

inline void to_json(nlohmann::ordered_json& j, const VehicleModData& modData) {
    j = nlohmann::ordered_json{
        { "Plate", modData.Plate },
        { "VehicleMods", modData.VehicleMods },
    };
}

inline void from_json(const nlohmann::ordered_json& j, VehicleModData& modData) {
    j.at("Plate").get_to(modData.Plate);
    j.at("VehicleMods").get_to(modData.VehicleMods);
}
