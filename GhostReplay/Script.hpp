#pragma once
#include "ReplayData.hpp"
#include "ReplayScript.hpp"
#include "ScriptMenu.hpp"
#include "ScriptSettings.hpp"

namespace GhostReplay {
    void ScriptMain();

    std::vector<CScriptMenu<CReplayScript>::CSubmenu> BuildMenu();

    CScriptSettings& GetSettings();

    CReplayScript* GetScript();

    bool ReplaysLocked();
    uint32_t ReplaysLoaded();
    uint32_t ReplaysTotal();
    std::string CurrentLoadingReplay();

    void LoadReplays();
    uint32_t LoadTracks();
    uint32_t LoadTrackImages();
    uint32_t LoadARSTracks();


    void AddReplay(const CReplayData& replay);

    void TriggerLoadStop();
}

namespace Dll {
    void SetupHooks();
    void ClearHooks();

    bool Unloading();
}
