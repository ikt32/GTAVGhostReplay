#include "Compatibility.hpp"
#include "Script.hpp"
#include "Util/Logger.hpp"

#include <Windows.h>

#define CHECK_ADDR(NAME) \
    NAME##_ = CheckAddr<decltype(NAME##_)>(Module, #NAME);

namespace RagePresence {

    HMODULE Module = nullptr;
    void Setup();

    void(*SetCustomMission_)(const char* mission);
    const char*(*GetCustomMission_)();
    bool (*IsCustomMissionSet_)();
    void (*ClearCustomMission_)();

    void (*SetCustomDetails_)(const char* details);
    const char* (*GetCustomDetails_)();
    bool (*AreCustomDetailsSet_)();
    void (*ClearCustomDetails_)();

    void (*SetCustomState_)(const char* state);
    const char* (*GetCustomState_)();
    bool (*IsCustomStateSet_)();
    void (*ClearCustomState_)();
}

template <typename T>
T CheckAddr(HMODULE lib, const std::string& funcName) {
    FARPROC func = GetProcAddress(lib, funcName.c_str());
    if (!func) {
        LOG(Error, "[Compat] Couldn't get function [{}]", funcName);
        return nullptr;
    }
    LOG(Debug, "[Compat] Found function [{}]", funcName);
    return reinterpret_cast<T>(func);
}

void Compatibility::Setup() {
    //RagePresence::Setup();
}

void Compatibility::Release() {
    //RagePresence::SetCustomMission_ = nullptr;
}

void RagePresence::Setup() {
    LOG(Info, "[Compat] Setting up RagePresence (Lemon)");
    Module = GetModuleHandle(L"RagePresence.asi");
    if (!Module) {
        LOG(Info, "[Compat] No RagePresence");
        return;
    }

    CHECK_ADDR(SetCustomMission);
    CHECK_ADDR(GetCustomMission);
    CHECK_ADDR(IsCustomMissionSet);
    CHECK_ADDR(ClearCustomMission);

    CHECK_ADDR(SetCustomDetails);
    CHECK_ADDR(GetCustomDetails);
    CHECK_ADDR(AreCustomDetailsSet);
    CHECK_ADDR(ClearCustomDetails);

    CHECK_ADDR(SetCustomState);
    CHECK_ADDR(GetCustomState);
    CHECK_ADDR(IsCustomStateSet);
    CHECK_ADDR(ClearCustomState);
}

void RagePresence::SetCustomMission(const char* mission) {
    if (SetCustomMission_ && GhostReplay::GetSettings().Main.RagePresence)
        SetCustomMission_(mission);
}

const char* RagePresence::GetCustomMission() {
    if (GetCustomMission_ && GhostReplay::GetSettings().Main.RagePresence)
        return GetCustomMission_();
    return "";
}

bool RagePresence::IsCustomMissionSet() {
    if (IsCustomMissionSet_ && GhostReplay::GetSettings().Main.RagePresence)
        return IsCustomMissionSet_();
    return false;
}

void RagePresence::ClearCustomMission() {
    if (ClearCustomMission_ && GhostReplay::GetSettings().Main.RagePresence)
        ClearCustomMission_();
}

void RagePresence::SetCustomDetails(const char* details) {
    if (SetCustomDetails_ && GhostReplay::GetSettings().Main.RagePresence)
        SetCustomDetails_(details);
}

const char* RagePresence::GetCustomDetails() {
    if (GetCustomDetails_ && GhostReplay::GetSettings().Main.RagePresence)
        return GetCustomDetails_();
    return "";
}

bool RagePresence::AreCustomDetailsSet() {
    if (AreCustomDetailsSet_ && GhostReplay::GetSettings().Main.RagePresence)
        return AreCustomDetailsSet_();
    return false;
}

void RagePresence::ClearCustomDetails() {
    if (ClearCustomDetails_ && GhostReplay::GetSettings().Main.RagePresence)
        ClearCustomDetails_();
}

void RagePresence::SetCustomState(const char* state) {
    if (SetCustomState_ && GhostReplay::GetSettings().Main.RagePresence)
        SetCustomState_(state);
}

const char* RagePresence::GetCustomState() {
    if (GetCustomState_ && GhostReplay::GetSettings().Main.RagePresence)
        return GetCustomState_();
    return "";
}

bool RagePresence::IsCustomStateSet() {
    if (IsCustomStateSet_ && GhostReplay::GetSettings().Main.RagePresence)
        return IsCustomStateSet_();
    return false;
}

void RagePresence::ClearCustomState() {
    if (ClearCustomState_ && GhostReplay::GetSettings().Main.RagePresence)
        ClearCustomState_();
}