#pragma once

namespace Compatibility {
    void Setup();
    void Release();
}

namespace RagePresence {
    void SetCustomMission(const char* mission);
    const char* GetCustomMission();
    bool IsCustomMissionSet();
    void ClearCustomMission();

    void SetCustomDetails(const char* details);
    const char* GetCustomDetails();
    bool AreCustomDetailsSet();
    void ClearCustomDetails();

    void SetCustomState(const char* state);
    const char* GetCustomState();
    bool IsCustomStateSet();
    void ClearCustomState();
}
