#include "ScriptSettings.hpp"
#include "SettingsCommon.hpp"

#include "Util/Logger.hpp"
#include "Util/Misc.hpp"
#include "Util/String.hpp"

#include <simpleini/SimpleIni.h>

#define CHECK_LOG_SI_ERROR(result, operation) \
    if ((result) < 0) { \
        LOG(Error, "[Config] {} Failed to {}, SI_Error [{}]", \
        __FUNCTION__, operation, static_cast<int>(result)); \
    }

#define SAVE_VAL(section, key, option) \
    SetValue(ini, section, key, option)

#define LOAD_VAL(section, key, option) \
    option = GetValue(ini, section, key, option)

CScriptSettings::CScriptSettings(std::string settingsFile)
    : mSettingsFile(std::move(settingsFile)) {
    
}

void CScriptSettings::Load() {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(mSettingsFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    LOAD_VAL("Main", "NotifyLaps", Main.NotifyLaps);
    LOAD_VAL("Main", "DrawStartFinish", Main.DrawStartFinish);
    LOAD_VAL("Main", "GhostBlips", Main.GhostBlips);
    LOAD_VAL("Main", "StartStopBlips", Main.StartStopBlips);
    LOAD_VAL("Main", "ShowRecordTime", Main.ShowRecordTime);
    LOAD_VAL("Main", "ReplaySortBy", Main.ReplaySortBy);
    //LOAD_VAL("Main", "RagePresence", Main.RagePresence);
    LOAD_VAL("Main", "Debug", Main.Debug);

    LOAD_VAL("Record", "AutoGhost", Record.AutoGhostFast);
    LOAD_VAL("Record", "AutoGhostAll", Record.AutoGhostAll);

    if (Record.AutoGhostFast && Record.AutoGhostAll) {
        Record.AutoGhostAll = false;
    }

    LOAD_VAL("Record", "DeltaMillis", Record.DeltaMillis);
    LOAD_VAL("Record", "ReduceFileSize", Record.ReduceFileSize);

    LOAD_VAL("Record", "Optional.Lights", Record.Optional.Lights);
    LOAD_VAL("Record", "Optional.Indicators", Record.Optional.Indicators);
    LOAD_VAL("Record", "Optional.Siren", Record.Optional.Siren);

    LOAD_VAL("Replay", "OffsetSeconds", Replay.OffsetSeconds);
    LOAD_VAL("Replay", "VehicleAlpha", Replay.VehicleAlpha);
    LOAD_VAL("Replay", "ForceLights", Replay.ForceLights);
    LOAD_VAL("Replay", "ForceRoof", Replay.ForceRoof);

    LOAD_VAL("Replay", "ScrubDistanceSeconds", Replay.ScrubDistanceSeconds);
    LOAD_VAL("Replay", "FallbackModel", Replay.FallbackModel);
    LOAD_VAL("Replay", "ForceFallbackModel", Replay.ForceFallbackModel);
    LOAD_VAL("Replay", "AutoLoadGhost", Replay.AutoLoadGhost);
    LOAD_VAL("Replay", "ZeroVelocityOnPause", Replay.ZeroVelocityOnPause);

    auto syncType = as_int(Replay.SyncType);
    LOAD_VAL("Replay", "SyncType", syncType);
    LOAD_VAL("Replay", "SyncDistance", Replay.SyncDistance);
    LOAD_VAL("Replay", "SyncCompensation", Replay.SyncCompensation);

    LOAD_VAL("Replay", "EnableDrivers", Replay.EnableDrivers);
    std::string driverModels = ini.GetValue("Replay", "DriverModels", "");
    if (!driverModels.empty()) {
        Replay.DriverModels = Util::split(driverModels, ' ');
    }
    LOAD_VAL("Replay", "EnableCollision", Replay.EnableCollision);

    // Correct invalid settings
    if (Main.ReplaySortBy < 0 || Main.ReplaySortBy > 2) {
        LOG(Warning, "[Settings] Main.ReplaySortBy: Invalid value '{}', using 0", Main.ReplaySortBy);
        Main.ReplaySortBy = 0;
    }

    if (Replay.ForceLights < 0 || Replay.ForceLights > 2) {
        LOG(Warning, "[Settings] Replay.ForceLights: Invalid value '{}', using 0", Replay.ForceLights);
        Replay.ForceLights = 0;
    }

    if (Replay.ForceRoof < 0 || Replay.ForceRoof > 2) {
        LOG(Warning, "[Settings] Replay.ForceRoof: Invalid value '{}', using 0", Replay.ForceRoof);
        Replay.ForceRoof = 0;
    }

    if (syncType < 0 || syncType > ESyncTypeMax) {
        LOG(Warning, "[Settings] Replay.SyncType: Invalid value '{}', using 0", syncType);
        syncType = 0;
    }
    Replay.SyncType = static_cast<ESyncType>(syncType);
}

void CScriptSettings::Save() {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(mSettingsFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    SAVE_VAL("Main", "NotifyLaps", Main.NotifyLaps);
    SAVE_VAL("Main", "DrawStartFinish", Main.DrawStartFinish);
    SAVE_VAL("Main", "GhostBlips", Main.GhostBlips);
    SAVE_VAL("Main", "StartStopBlips", Main.StartStopBlips);
    SAVE_VAL("Main", "ShowRecordTime", Main.ShowRecordTime);
    SAVE_VAL("Main", "ReplaySortBy", Main.ReplaySortBy);
    //SAVE_VAL("Main", "RagePresence", Main.RagePresence);
    SAVE_VAL("Main", "Debug", Main.Debug);

    SAVE_VAL("Record", "AutoGhost", Record.AutoGhostFast);
    SAVE_VAL("Record", "AutoGhostAll", Record.AutoGhostAll);

    SAVE_VAL("Record", "DeltaMillis", Record.DeltaMillis);
    SAVE_VAL("Record", "ReduceFileSize", Record.ReduceFileSize);

    SAVE_VAL("Record", "Optional.Lights", Record.Optional.Lights);
    SAVE_VAL("Record", "Optional.Indicators", Record.Optional.Indicators);
    SAVE_VAL("Record", "Optional.Siren", Record.Optional.Siren);

    SAVE_VAL("Replay", "OffsetSeconds", Replay.OffsetSeconds);
    SAVE_VAL("Replay", "VehicleAlpha", Replay.VehicleAlpha);
    SAVE_VAL("Replay", "ForceLights", Replay.ForceLights);
    SAVE_VAL("Replay", "ForceRoof", Replay.ForceRoof);

    SAVE_VAL("Replay", "ScrubDistanceSeconds", Replay.ScrubDistanceSeconds);
    SAVE_VAL("Replay", "FallbackModel", Replay.FallbackModel);
    SAVE_VAL("Replay", "ForceFallbackModel", Replay.ForceFallbackModel);
    SAVE_VAL("Replay", "AutoLoadGhost", Replay.AutoLoadGhost);
    SAVE_VAL("Replay", "ZeroVelocityOnPause", Replay.ZeroVelocityOnPause);

    SAVE_VAL("Replay", "SyncType", as_int(Replay.SyncType));
    SAVE_VAL("Replay", "SyncDistance", Replay.SyncDistance);
    SAVE_VAL("Replay", "SyncCompensation", Replay.SyncCompensation);

    SAVE_VAL("Replay", "EnableDrivers", Replay.EnableDrivers);
    // DriverModels not editable in-game
    SAVE_VAL("Replay", "EnableCollision", Replay.EnableCollision);

    result = ini.SaveFile(mSettingsFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}
