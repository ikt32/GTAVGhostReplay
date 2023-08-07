#include "VehicleMod.hpp"

#include "Memory/Versions.hpp"

#include <inc/natives.h>
#include <format>

namespace VEHICLE {
    // b877
    void _GET_VEHICLE_INTERIOR_COLOUR(Vehicle vehicle, int* colour) {
        if (getGameVersion() < G_VER_1_0_877_1_STEAM)
            return;
        invoke<Void>(0x7D1464D472D32136, vehicle, colour);
    }

    void _SET_VEHICLE_INTERIOR_COLOUR(Vehicle vehicle, int colour) {
        if (getGameVersion() < G_VER_1_0_877_1_STEAM)
            return;
        invoke<Void>(0xF40DD601A65F7F19, vehicle, colour);
    }

    void _GET_VEHICLE_DASHBOARD_COLOUR(Vehicle vehicle, int* colour) {
        if (getGameVersion() < G_VER_1_0_877_1_STEAM)
            return;
        invoke<Void>(0xB7635E80A5C31BFF, vehicle, colour);
    }

    void _SET_VEHICLE_DASHBOARD_COLOUR(Vehicle vehicle, int colour) {
        if (getGameVersion() < G_VER_1_0_877_1_STEAM)
            return;
        invoke<Void>(0x6089CDF6A57F326C, vehicle, colour);
    }

    // b1604
    static void _SET_VEHICLE_XENON_LIGHTS_COLOUR(Vehicle vehicle, int colorIndex) {
        if (getGameVersion() < G_VER_1_0_1604_0_STEAM)
            return;
        invoke<Void>(0xE41033B25D003A07, vehicle, colorIndex);
    }

    static int _GET_VEHICLE_XENON_LIGHTS_COLOUR(Vehicle vehicle) {
        if (getGameVersion() < G_VER_1_0_1604_0_STEAM)
            return 0;
        return invoke<int>(0x3DFF319A831E0CDB, vehicle);
    }
}

VehicleMod VehicleMod::Get(Vehicle vehicle) {
    switch (Type) {
        case ModType::Color:
            VEHICLE::GET_VEHICLE_COLOURS(vehicle, &Value0, &Value1);
            break;
        case ModType::ExtraColors:
            VEHICLE::GET_VEHICLE_EXTRA_COLOURS(vehicle, &Value0, &Value1);
            break;
        case ModType::ColorCustom1:
            Value0 = VEHICLE::GET_IS_VEHICLE_PRIMARY_COLOUR_CUSTOM(vehicle);
            VEHICLE::GET_VEHICLE_CUSTOM_PRIMARY_COLOUR(vehicle, &Value1, &Value2, &Value3);
            break;
        case ModType::ColorCustom2:
            Value0 = VEHICLE::GET_IS_VEHICLE_SECONDARY_COLOUR_CUSTOM(vehicle);
            VEHICLE::GET_VEHICLE_CUSTOM_SECONDARY_COLOUR(vehicle, &Value1, &Value2, &Value3);
            break;
        case ModType::Mod:
            Value0 = VEHICLE::GET_VEHICLE_MOD(vehicle, ModId);
            Value1 = VEHICLE::GET_VEHICLE_MOD_VARIATION(vehicle, ModId);
            break;
        case ModType::ModToggle:
            Value0 = VEHICLE::IS_TOGGLE_MOD_ON(vehicle, ModId);
            break;
        case ModType::WheelType:
            Value0 = VEHICLE::GET_VEHICLE_WHEEL_TYPE(vehicle);
            break;
        case ModType::WindowTint:
            Value0 = VEHICLE::GET_VEHICLE_WINDOW_TINT(vehicle);
            break;
        case ModType::TyresCanBurst:
            Value0 = VEHICLE::GET_VEHICLE_TYRES_CAN_BURST(vehicle);
            break;
        case ModType::TyreSmoke:
            VEHICLE::GET_VEHICLE_TYRE_SMOKE_COLOR(vehicle, &Value0, &Value1, &Value2);
            break;
        case ModType::Livery:
            Value0 = VEHICLE::GET_VEHICLE_LIVERY(vehicle);
            break;
        case ModType::ModColor1:
            VEHICLE::GET_VEHICLE_MOD_COLOR_1(vehicle, &Value0, &Value1, &Value2);
            break;
        case ModType::ModColor2:
            VEHICLE::GET_VEHICLE_MOD_COLOR_2(vehicle, &Value0, &Value1);
            break;
        case ModType::Extra:
            if (VEHICLE::DOES_EXTRA_EXIST(vehicle, ModId))
                Value0 = VEHICLE::IS_VEHICLE_EXTRA_TURNED_ON(vehicle, ModId);
            break;
        case ModType::Neon:
            Value0 = VEHICLE::_IS_VEHICLE_NEON_LIGHT_ENABLED(vehicle, ModId);
            break;
        case ModType::NeonColor:
            VEHICLE::_GET_VEHICLE_NEON_LIGHTS_COLOUR(vehicle, &Value0, &Value1, &Value2);
            break;
        case ModType::XenonColor:
            Value0 = VEHICLE::_GET_VEHICLE_XENON_LIGHTS_COLOUR(vehicle);
            break;
        case ModType::PlateType:
            Value0 = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT_INDEX(vehicle);
            break;
        case ModType::InteriorColor:
            VEHICLE::_GET_VEHICLE_INTERIOR_COLOUR(vehicle, &Value0);
            break;
        case ModType::DashboardColor:
            VEHICLE::_GET_VEHICLE_DASHBOARD_COLOUR(vehicle, &Value0);
            break;
        default:;
    }
    return *this;
}

void VehicleMod::Set(Vehicle vehicle) {
    switch (Type) {
        case ModType::Color:
            VEHICLE::SET_VEHICLE_COLOURS(vehicle, Value0, Value1);
            break;
        case ModType::ExtraColors:
            VEHICLE::SET_VEHICLE_EXTRA_COLOURS(vehicle, Value0, Value1);
            break;
        case ModType::ColorCustom1:
            VEHICLE::CLEAR_VEHICLE_CUSTOM_PRIMARY_COLOUR(vehicle);
            if (Value0)
                VEHICLE::SET_VEHICLE_CUSTOM_PRIMARY_COLOUR(vehicle, Value1, Value2, Value3);
            break;
        case ModType::ColorCustom2:
            VEHICLE::CLEAR_VEHICLE_CUSTOM_SECONDARY_COLOUR(vehicle);
            if (Value0)
                VEHICLE::SET_VEHICLE_CUSTOM_SECONDARY_COLOUR(vehicle, Value1, Value2, Value3);
            break;
        case ModType::Mod:
            VEHICLE::SET_VEHICLE_MOD(vehicle, ModId, Value0, Value1);
            break;
        case ModType::ModToggle:
            VEHICLE::TOGGLE_VEHICLE_MOD(vehicle, ModId, Value0);
            break;
        case ModType::WheelType:
            VEHICLE::SET_VEHICLE_WHEEL_TYPE(vehicle, Value0);
            break;
        case ModType::WindowTint:
            VEHICLE::SET_VEHICLE_WINDOW_TINT(vehicle, Value0);
            break;
        case ModType::TyresCanBurst:
            VEHICLE::SET_VEHICLE_TYRES_CAN_BURST(vehicle, Value0);
            break;
        case ModType::TyreSmoke:
            VEHICLE::SET_VEHICLE_TYRE_SMOKE_COLOR(vehicle, Value0, Value1, Value2);
            break;
        case ModType::Livery:
            VEHICLE::SET_VEHICLE_LIVERY(vehicle, Value0);
            break;
        case ModType::ModColor1:
            VEHICLE::SET_VEHICLE_MOD_COLOR_1(vehicle, Value0, Value1, Value2);
            break;
        case ModType::ModColor2:
            VEHICLE::SET_VEHICLE_MOD_COLOR_2(vehicle, Value0, Value1);
            break;
        case ModType::Extra:
            if (VEHICLE::DOES_EXTRA_EXIST(vehicle, ModId))
                VEHICLE::SET_VEHICLE_EXTRA(vehicle, ModId, !Value0);
            break;
        case ModType::Neon:
            VEHICLE::_SET_VEHICLE_NEON_LIGHT_ENABLED(vehicle, ModId, Value0);
            break;
        case ModType::NeonColor:
            VEHICLE::_SET_VEHICLE_NEON_LIGHTS_COLOUR(vehicle, Value0, Value1, Value2);
            break;
        case ModType::XenonColor:
            VEHICLE::_SET_VEHICLE_XENON_LIGHTS_COLOUR(vehicle, Value0);
            break;
        case ModType::PlateType:
            VEHICLE::SET_VEHICLE_NUMBER_PLATE_TEXT_INDEX(vehicle, Value0);
            break;
        case ModType::InteriorColor:
            VEHICLE::_SET_VEHICLE_INTERIOR_COLOUR(vehicle, Value0);
            break;
        case ModType::DashboardColor:
            VEHICLE::_SET_VEHICLE_DASHBOARD_COLOUR(vehicle, Value0);
            break;
        default:;
    }
}

VehicleModData VehicleModData::LoadFrom(Vehicle vehicle) {
    VehicleModData data;
    data.Plate = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(vehicle);

    data.VehicleMods.clear();

    // Wheeltype
    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::WheelType }.Get(vehicle));

    // 0 t/m 24: normal; 25 t/m 49: Benny's
    for (uint8_t i = 0; i < 50; ++i) {
        if (i >= 17 && i <= 22) {
            data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::ModToggle, i }.Get(vehicle));
        }
        else {
            data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::Mod, i }.Get(vehicle));
        }
    }

    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::TyresCanBurst }.Get(vehicle));
    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::TyreSmoke }.Get(vehicle));
    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::PlateType }.Get(vehicle));

    for (uint8_t i = 0; i <= 60; ++i) {
        data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::Extra, i }.Get(vehicle));
    }

    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::Livery }.Get(vehicle));

    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::WindowTint }.Get(vehicle));

    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::Neon, 0 }.Get(vehicle));
    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::Neon, 1 }.Get(vehicle));
    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::Neon, 2 }.Get(vehicle));
    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::Neon, 3 }.Get(vehicle));
    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::NeonColor }.Get(vehicle));

    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::ModColor1 }.Get(vehicle));
    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::ModColor2 }.Get(vehicle));

    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::XenonColor }.Get(vehicle));
    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::InteriorColor }.Get(vehicle));
    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::DashboardColor }.Get(vehicle));

    // Custom colors
    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::ColorCustom1 }.Get(vehicle));
    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::ColorCustom2 }.Get(vehicle));

    // Primary/Secondary
    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::Color }.Get(vehicle));
    data.VehicleMods.push_back(VehicleMod{ VehicleMod::ModType::ExtraColors }.Get(vehicle));

    return data;
}

void VehicleModData::ApplyTo(Vehicle vehicle, VehicleModData modData) {
    VEHICLE::SET_VEHICLE_NUMBER_PLATE_TEXT(vehicle, modData.Plate.c_str());
    VEHICLE::SET_VEHICLE_MOD_KIT(vehicle, 0);
    for (auto& vehicleMod : modData.VehicleMods) {
        vehicleMod.Set(vehicle);
    }
}
