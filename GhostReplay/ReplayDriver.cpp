#include "ReplayDriver.hpp"

#include "Memory/NativeMemory.hpp"
#include "Util/Logger.hpp"

#include <inc/natives.h>

// FiveM PedExtraNatives
class CPedHeadBlendData {
public:
    void* vtable; // +0
    DWORD unk_0; // +8
    BYTE unk_1; // +12
    BYTE unk_2; // +13
    BYTE unk_3; // +14
    BYTE unk_4; // +15
    DWORD unk_5; // +16
    char pad_0[20]; // +20
    float shapeMix; // +40
    float skinMix; // +44
    float unknownMix; // +48
    DWORD unk_6; // +52
    float overlayAlpha[13]; // +56
    float overlayAlphaCopy[13]; // +108
    float faceFeature[20]; // +160
    uint8_t overlayColorId[13]; // +240
    uint8_t overlayHighlightId[13]; // +253
    uint8_t overlayColorType[13]; // +266
    uint8_t firstShapeId; // +279
    uint8_t secondShapeId; // +280
    uint8_t thirdShapeId; // +281
    uint8_t firstSkinId; // +282
    uint8_t secondSkinId; // +283
    uint8_t thirdSkinId; // +284
    uint8_t overlayValue[13]; // +285
    uint8_t paletteColorR[4]; // +298
    uint8_t paletteColorG[4]; // +302
    uint8_t paletteColorB[4]; // +306
    uint16_t eyeColour; // +310
    uint8_t hairColour; // +312
    uint8_t hairHighlight; // +313
    BYTE unk_7; // +314
    BYTE isPaletteUsed; // +315
    BYTE unk_8; // +316
    BYTE isParentBlend; // +317
    BYTE unk_9; // +318
};

static uint32_t* _id_CPedHeadBlendData = nullptr;
typedef uint64_t(*ExtensionList_get_t)(void* entity, uint64_t list);
ExtensionList_get_t g_extensionList_get = nullptr;

void Driver::SetupHeadBlendDataFunctions() {
    auto addr1 = mem::FindPattern("48 39 5E 38 74 1B 8B 15 ? ? ? ? 48 8D 4F 10 E8");
    logger.Write(addr1 ? DEBUG : ERROR, "[Driver] addr1: 0x%llX", addr1);

    auto addr2 = mem::FindPattern("41 83 E0 1F 8B 44 81 08 44 0F A3 C0");
    logger.Write(addr2 ? DEBUG : ERROR, "[Driver] addr2: 0x%llX", addr2);

    if (!addr1 || !addr2) {
        logger.Write(ERROR, "[Driver] Unable to retrieve head blend data");
        return;
    }

    _id_CPedHeadBlendData = (uint32_t*)((char*)addr1 + *(int*)((char*)addr1 + 8) + 12);
    g_extensionList_get = (ExtensionList_get_t)(((char*)addr2 - 31));
}

static CPedHeadBlendData* GetPedHeadBlendData(void* entity) {
    if (!entity || !_id_CPedHeadBlendData || !g_extensionList_get)
        return nullptr;

    auto address = (char*)entity;
    if (*(BYTE*)(*(uint64_t*)(address + 32) + 646) & 2) {
        auto data = (CPedHeadBlendData*)g_extensionList_get(address + 16, *_id_CPedHeadBlendData);
        return data;
    }
    return nullptr;
}

SReplayDriverData SReplayDriverData::LoadFrom(Ped ped) {
    if (!ENTITY::IS_ENTITY_A_PED(ped)) {
        return {};
    }

    SReplayDriverData driverData;

    driverData.Model = ENTITY::GET_ENTITY_MODEL(ped);
    driverData.Type = PED::GET_PED_TYPE(ped);

    for (int i = 0; i <= 11; ++i) {
        SPedComponentVariationData variationData{
            .ComponentId = i,
            .DrawableId = PED::GET_PED_DRAWABLE_VARIATION(ped, i),
            .TextureId = PED::GET_PED_TEXTURE_VARIATION(ped, i),
            .PaletteId = PED::GET_PED_PALETTE_VARIATION(ped, i),
        };
        driverData.ComponentVariations.push_back(variationData);
    }

    SHeadBlendData headBlendData;
    bool hasHeadBlendData = PED::GET_PED_HEAD_BLEND_DATA(ped, &headBlendData);
    if (hasHeadBlendData) {
        driverData.HeadBlendData = headBlendData;
    }

    auto* pHeadBlendDataAdv = GetPedHeadBlendData(getScriptHandleBaseAddress(ped));
    if (pHeadBlendDataAdv) {
        driverData.HeadOverlays = std::vector<SHeadOverlayData>();
        for (uint32_t i = 0; i <= 12; ++i) {
            driverData.HeadOverlays->push_back(SHeadOverlayData{
                    .Value = pHeadBlendDataAdv->overlayValue[i],
                    .Alpha = pHeadBlendDataAdv->overlayAlpha[i],
                    .ColorType = pHeadBlendDataAdv->overlayColorType[i],
                    .ColorId = pHeadBlendDataAdv->overlayColorId[i],
                    .HighlightId = pHeadBlendDataAdv->overlayHighlightId[i]
                });
        }
    }

    return driverData;
}

void SReplayDriverData::ApplyTo(Ped ped, SReplayDriverData driverData) {
    for (const auto& componentVariation : driverData.ComponentVariations) {
        PED::SET_PED_COMPONENT_VARIATION(ped,
            componentVariation.ComponentId, componentVariation.DrawableId,
            componentVariation.TextureId, componentVariation.PaletteId);
    }

    if (driverData.HeadBlendData != std::nullopt) {
        auto& hbd = driverData.HeadBlendData.value();
        PED::SET_PED_HEAD_BLEND_DATA(ped,
            hbd.Shape1, hbd.Shape2, hbd.Shape3,
            hbd.Skin1, hbd.Skin2, hbd.Skin3,
            hbd.ShapeMix, hbd.SkinMix, hbd.ThirdMix,
            false);
    }

    if (driverData.HeadOverlays != std::nullopt) {
        for (uint32_t i = 0; i <= 12; ++i) {
            SHeadOverlayData hod = driverData.HeadOverlays.value()[i];
            PED::SET_PED_HEAD_OVERLAY(ped, i,
                hod.Value, hod.Alpha);
            PED::_SET_PED_HEAD_OVERLAY_COLOR(ped, i,
                hod.ColorType, hod.ColorId, hod.HighlightId);
        }
    }
}
