#include "ReplayDriver.hpp"
#include <inc/natives.h>

ReplayDriverData ReplayDriverData::LoadFrom(Ped ped) {
    if (!ENTITY::IS_ENTITY_A_PED(ped)) {
        return {};
    }

    ReplayDriverData driverData;

    driverData.Model = ENTITY::GET_ENTITY_MODEL(ped);
    driverData.Type = PED::GET_PED_TYPE(ped);

    for (int i = 0; i < MAX_PED_VARIATION; ++i) {
        PedComponentVariationData variationData{
            .ComponentId = i,
            .DrawableId = PED::GET_PED_DRAWABLE_VARIATION(ped, i),
            .TextureId = PED::GET_PED_TEXTURE_VARIATION(ped, i),
            .PaletteId = PED::GET_PED_PALETTE_VARIATION(ped, i),
        };
        driverData.ComponentVariations.push_back(variationData);
    }
    return driverData;
}

void ReplayDriverData::ApplyTo(Ped ped, ReplayDriverData driverData) {
    for (const auto& componentVariation : driverData.ComponentVariations) {
        PED::SET_PED_COMPONENT_VARIATION(ped,
            componentVariation.ComponentId, componentVariation.DrawableId,
            componentVariation.TextureId, componentVariation.PaletteId);
    }
}
