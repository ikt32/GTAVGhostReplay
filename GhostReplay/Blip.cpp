#include "Blip.hpp"
#include "Script.hpp"
#include <inc/natives.h>

CWrappedBlip::CWrappedBlip(Vector3 pos, eBlipSprite blip, std::string name, eBlipColor color)
    : mHandle(HUD::ADD_BLIP_FOR_COORD(pos))
    , mEntity(0)
    , mName(name) {
    SetSprite(blip);
    SetName(name);
    SetColor(color);
    ShowHeading(false);
}

CWrappedBlip::CWrappedBlip(Entity entity, eBlipSprite blip, std::string name, eBlipColor color, bool showHeading)
    : mHandle(HUD::ADD_BLIP_FOR_ENTITY(entity))
    , mEntity(entity)
    , mName(name) {
    SetSprite(blip);
    SetName(name);
    SetColor(color);
    ShowHeading(showHeading);
}

CWrappedBlip::~CWrappedBlip() {
    if (Dll::Unloading())
        return;

    Delete();
}

Vector3 CWrappedBlip::GetPosition() const {
    return HUD::GET_BLIP_INFO_ID_COORD(mHandle);
}

void CWrappedBlip::SetPosition(Vector3 coord) {
    HUD::SET_BLIP_COORDS(mHandle, coord);
}

eBlipColor CWrappedBlip::GetColor() const {
    return (eBlipColor)HUD::GET_BLIP_COLOUR(mHandle);
}

void CWrappedBlip::SetColor(eBlipColor color) {
    HUD::SET_BLIP_COLOUR(mHandle, color);
}

int CWrappedBlip::GetAlpha() const {
    return HUD::GET_BLIP_ALPHA(mHandle);
}

void CWrappedBlip::SetAlpha(int alpha) {
    HUD::SET_BLIP_ALPHA(mHandle, alpha);
}

void CWrappedBlip::SetRotation(int rotation) {
    HUD::SET_BLIP_ROTATION(mHandle, rotation);
}

void CWrappedBlip::SetScale(float scale) {
    HUD::SET_BLIP_SCALE(mHandle, scale);
}

eBlipSprite CWrappedBlip::GetSprite() const {
    return (eBlipSprite)HUD::GET_BLIP_SPRITE(mHandle);
}

void CWrappedBlip::SetSprite(eBlipSprite sprite) {
    HUD::SET_BLIP_SPRITE(mHandle, sprite);
}

Entity CWrappedBlip::GetEntity() const {
    return mEntity; // HUD::GET_BLIP_INFO_ID_ENTITY_INDEX(mHandle);
}

std::string CWrappedBlip::GetName() {
    return mName;
}

void CWrappedBlip::SetName(const std::string& name) {
    mName = name;
    HUD::BEGIN_TEXT_COMMAND_SET_BLIP_NAME("STRING");
    HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(name.c_str());
    HUD::END_TEXT_COMMAND_SET_BLIP_NAME(mHandle);
}

void CWrappedBlip::Delete() {
    HUD::SET_BLIP_ALPHA(mHandle, 0);
    HUD::REMOVE_BLIP(&mHandle);
}

bool CWrappedBlip::Exists() const {
    return HUD::DOES_BLIP_EXIST(mHandle);
}

Blip CWrappedBlip::GetHandle() const {
    return mHandle;
}

void CWrappedBlip::ShowHeading(bool b) {
    HUD::SHOW_HEADING_INDICATOR_ON_BLIP(mHandle, b);
}
