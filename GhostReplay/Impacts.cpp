#include "Impacts.hpp"
#include "Script.hpp"
#include "Memory/NativeMemory.hpp"
#include "Util/Logger.hpp"
#include <MinHook.h>
#include <cstdint>

// Thanks @Dot.!
typedef bool (*ShouldFindImpacts_t)(uint64_t, uint64_t);
ShouldFindImpacts_t ShouldFindImpactsOriginal = nullptr;

namespace {
    void* fnAddr = nullptr;
}

bool ShouldFindImpactsHook(uint64_t a1, uint64_t a2) {
    auto entity1 = *(unsigned int*)(a1 + 0x5C) | ((unsigned __int64)*(unsigned int*)(a1 + 0x4C) << 32);
    auto entity2 = *(unsigned int*)(a2 + 0x5C) | ((unsigned __int64)*(unsigned int*)(a2 + 0x4C) << 32);

    if (GhostReplay::HasSettings() && GhostReplay::GetSettings().Replay.EnableCollision) {
        return ShouldFindImpactsOriginal(a1, a2);
    }

    auto scriptInst = GhostReplay::GetScript();
    if (scriptInst) {
        auto playerVehicle = scriptInst->GetPlayerVehicle();
        auto addrPlayerVeh = mem::GetAddressOfEntity(playerVehicle);
        int matched = 0;

        bool playerInComparison = addrPlayerVeh == entity1 || addrPlayerVeh == entity2;
        bool playerInGhost = scriptInst->IsPassengerModeActive();

        for (const auto& replayVehicle : scriptInst->GetReplayVehicles()) {
            if (replayVehicle->GetReplayState() == EReplayState::Idle)
                continue;
            auto addrReplayVeh = mem::GetAddressOfEntity(replayVehicle->GetVehicle());
            if (addrReplayVeh == entity1 ||
                addrReplayVeh == entity2)
                matched++;

            if (matched == 2 || matched >= 1 && playerInComparison && !playerInGhost)
                return false;
        }
    }

    return ShouldFindImpactsOriginal(a1, a2);
}

bool Impacts::Found() {
    return fnAddr != nullptr;
}

void Impacts::Setup() {
    // Latest confirmed working game build for this:
    // 2845
    fnAddr = reinterpret_cast<void*>(mem::FindPattern(
        "48 8B C4 48 89 58 08 48 89 68 10 "
        "48 89 70 18 48 89 78 20 41 56 "
        "48 83 EC 20 48 8B EA 4C 8B F1 E8 ? ? ? ? 33 DB"));

    if (fnAddr == 0) {
        LOG(Error, "Couldn't find ShouldFindImpacts");
        return;
    }
    LOG(Debug, "Found ShouldFindImpacts at 0x{:X}", fnAddr);

    auto result = MH_Initialize();
    if (result != MH_OK) {
        LOG(Error, "MH_Initialize failed: {}", static_cast<int>(result));
        return;
    }

    result = MH_CreateHook(fnAddr, &ShouldFindImpactsHook, reinterpret_cast<LPVOID*>(&ShouldFindImpactsOriginal));
    if (result != MH_OK) {
        LOG(Error, "MH_CreateHook failed: {}", static_cast<int>(result));
        return;
    }

    result = MH_EnableHook(MH_ALL_HOOKS);
    if (result != MH_OK) {
        LOG(Error, "MH_EnableHook failed: {}", static_cast<int>(result));
        return;
    }
}

void Impacts::Clear() {
    if (!fnAddr)
        return;
    MH_DisableHook(MH_ALL_HOOKS);
    MH_RemoveHook(fnAddr);
    MH_Uninitialize();
}
