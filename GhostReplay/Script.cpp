#include "Script.hpp"

#include "ReplayScript.hpp"
#include "ScriptMenu.hpp"
#include "Constants.hpp"
#include "Compatibility.hpp"
#include "Impacts.hpp"
#include "ReplayDriver.hpp"

#include "Memory/VehicleExtensions.hpp"
#include "Memory/NativeMemory.hpp"

#include "Util/Logger.hpp"
#include "Util/Paths.hpp"
#include "Util/String.hpp"

#include <inc/natives.h>
#include <inc/main.h>
#include <format>
#include <memory>
#include <filesystem>
#include <thread>
#include <mutex>

using namespace GhostReplay;
namespace fs = std::filesystem;

namespace {
    std::shared_ptr<CScriptSettings> settings;
    std::shared_ptr<CReplayScript> scriptInst;

    std::atomic_bool stopLoading = false;
    std::atomic<uint32_t> totalReplays = 0;
    std::atomic<uint32_t> loadedReplays = 0;
    std::vector<std::string> pendingReplays;

    std::mutex currentLoadingReplayMutex;
    std::string currentLoadingReplay;

    std::mutex replaysMutex;
    std::vector<std::shared_ptr<CReplayData>> replays;
    std::vector<CTrackData> tracks;
    std::vector<CTrackData> arsTracks;
    std::vector<CImage> trackImages;

    void clearFileFlags() {
        for (auto& track : tracks) {
            track.MarkedForDeletion = false;
        }

        std::lock_guard replaysLock(replaysMutex);
        for (auto& replay : replays) {
            replay->MarkedForDeletion = false;
        }
    }
}

void Dll::SetupHooks() {
    Driver::SetupHeadBlendDataFunctions();
    Impacts::Setup();
}

void Dll::ClearHooks() {
    Impacts::Clear();
}

void GhostReplay::ScriptMain() {
    stopLoading = false;

    const std::string settingsGeneralPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\settings_general.ini";
    const std::string settingsMenuPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\settings_menu.ini";

    settings = std::make_shared<CScriptSettings>(settingsGeneralPath);
    settings->Load();
    logger.Write(INFO, "Settings loaded");

    GhostReplay::CreateDirectories();
    GhostReplay::LoadReplays();
    GhostReplay::LoadTracks();
    GhostReplay::LoadARSTracks();
    GhostReplay::LoadTrackImages();

    scriptInst = std::make_shared<CReplayScript>(*settings, replays, tracks, trackImages, arsTracks);

    VehicleExtensions::Init();
    Compatibility::Setup();

    CScriptMenu menu(settingsMenuPath, 
        []() {
            // OnInit
            settings->Load();
            // GhostReplay::LoadReplays();
            // GhostReplay::LoadTracks();
        },
        []() {
            // OnExit
            scriptInst->SetScriptMode(EScriptMode::ReplayActive);
            settings->Save();
            ::clearFileFlags();
        },
        BuildMenu()
    );

    while(true) {
        scriptInst->Tick();
        menu.Tick(*scriptInst);
        WAIT(0);
    }
}

bool GhostReplay::HasSettings() {
    return settings != nullptr;
}

CScriptSettings& GhostReplay::GetSettings() {
    return *settings;
}

CReplayScript* GhostReplay::GetScript() {
    return scriptInst.get();
}

bool GhostReplay::ReplaysLocked() {
    return totalReplays != loadedReplays;
}

uint32_t GhostReplay::ReplaysLoaded() {
    return loadedReplays;
}

uint32_t GhostReplay::ReplaysTotal() {
    return totalReplays;
}

std::string GhostReplay::CurrentLoadingReplay() {
    std::lock_guard nameMutex(currentLoadingReplayMutex);
    return currentLoadingReplay;
}

void GhostReplay::CreateDirectories() {
    const std::string replaysPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Replays";

    const std::string tracksPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Tracks";

    if (!(fs::exists(fs::path(replaysPath)) && fs::is_directory(fs::path(replaysPath)))) {
        logger.Write(INFO, "[Replay] Creating missing directory [%s]", replaysPath.c_str());
        if (!std::filesystem::create_directory(fs::path(replaysPath))) {
            logger.Write(ERROR, "[Replay] Failed to create directory, replays will not be saved!");
        }
    }

    if (!(fs::exists(fs::path(tracksPath)) && fs::is_directory(fs::path(tracksPath)))) {
        logger.Write(INFO, "[Track] Creating missing directory [%s]", tracksPath.c_str());
        if (!std::filesystem::create_directory(fs::path(tracksPath))) {
            logger.Write(ERROR, "[Track] Failed to create directory, tracks will not be saved!");
        }
    }
}

void GhostReplay::LoadReplays() {
    const std::string replaysPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Replays";

    logger.Write(DEBUG, "[Replay] Clearing and reloading replays");
    
    if (!(fs::exists(fs::path(replaysPath)) && fs::is_directory(fs::path(replaysPath)))) {
        logger.Write(ERROR, "[Replay] Directory [%s] not found!", replaysPath.c_str());
        return;
    }
    
    {
        std::lock_guard replaysLock(replaysMutex);
        replays.clear();
    }

    pendingReplays.clear();
    totalReplays = 0;
    loadedReplays = 0;

    for (const auto& file : fs::directory_iterator(replaysPath)) {
        if (Util::to_lower(fs::path(file).extension().string()) != ".json") {
            continue;
        }
        pendingReplays.push_back(fs::path(file).string());
        ++totalReplays;
    }

    std::thread([replaysPath]() {
        for (const auto& file : pendingReplays) {
            if (stopLoading) {
                return;
            }

            {
                std::lock_guard nameMutex(currentLoadingReplayMutex);
                currentLoadingReplay = std::filesystem::path(file).stem().string();
            }


            CReplayData replay = CReplayData::Read(file);
            if (!replay.Nodes.empty()) {
                std::lock_guard replaysLock(replaysMutex);
                replays.push_back(std::make_shared<CReplayData>(replay));
            }
            else {
                logger.Write(WARN, "[Replay] Skipping [%s] - not a valid file", file.c_str());
            }

            logger.Write(DEBUG, "[Replay] Loaded replay [%s]", replay.Name.c_str());
            ++loadedReplays;
        }

        logger.Write(INFO, "[Replay] Replays loaded: %d", replays.size());
        pendingReplays.clear();

        std::lock_guard nameMutex(currentLoadingReplayMutex);
        currentLoadingReplay = std::string();
    }).detach();
}

uint32_t GhostReplay::LoadTracks() {
    const std::string tracksPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Tracks";

    logger.Write(DEBUG, "[Track] Clearing and reloading tracks");

    tracks.clear();

    if (!(fs::exists(fs::path(tracksPath)) && fs::is_directory(fs::path(tracksPath)))) {
        logger.Write(ERROR, "[Track] Directory [%s] not found!", tracksPath.c_str());
        return 0;
    }

    for (const auto& file : fs::recursive_directory_iterator(tracksPath)) {
        if (Util::to_lower(fs::path(file).extension().string()) != ".json") {
            continue;
        }

        CTrackData track = CTrackData::Read(fs::path(file).string());
        if (!track.Name.empty())
            tracks.push_back(track);
        else
            logger.Write(WARN, "[Track] Skipping [%s] - not a valid file", fs::path(file).string().c_str());

        logger.Write(DEBUG, "[Track] Loaded track [%s]", track.Name.c_str());
    }
    logger.Write(INFO, "[Track] Tracks loaded: %d", tracks.size());

    return static_cast<unsigned>(tracks.size());
}

uint32_t GhostReplay::LoadTrackImages() {
    const std::string tracksPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Tracks";

    logger.Write(DEBUG, "[TrackImg] Clearing and reloading track images");

    trackImages.clear();

    if (!(fs::exists(fs::path(tracksPath)) && fs::is_directory(fs::path(tracksPath)))) {
        logger.Write(ERROR, "[TrackImg] Directory [%s] not found!", tracksPath.c_str());
        return 0;
    }

    for (const auto& file : fs::recursive_directory_iterator(tracksPath)) {
        auto stem = fs::path(file).stem().string();
        auto ext = Util::to_lower(fs::path(file).extension().string());
        if (ext == ".json") {
            continue;
        }

        std::string fileName = fs::path(file).string();
        auto dims = Img::GetIMGDimensions(fileName);
        unsigned width;
        unsigned height;

        if (dims) {
            width = dims->first;
            height = dims->second;
        }
        else {
            logger.Write(WARN, "[TrackImg] Skipping [%s]: not an valid image.", fs::path(file).string().c_str());
            continue;
        }

        int handle = createTexture(fileName.c_str());
        trackImages.emplace_back(handle, stem, width, height);

        logger.Write(DEBUG, "[TrackImg] Loaded track image [%s]", stem.c_str());
    }
    logger.Write(INFO, "[TrackImg] Track images loaded: %d", trackImages.size());

    return static_cast<unsigned>(trackImages.size());
}

uint32_t GhostReplay::LoadARSTracks() {
    const std::string tracksPath =
        Paths::GetRunningExecutableFolder() + "\\Scripts\\ARS\\Tracks";

    logger.Write(DEBUG, "[Track-ARS] Clearing and reloading tracks");

    arsTracks.clear();

    if (!(fs::exists(fs::path(tracksPath)) && fs::is_directory(fs::path(tracksPath)))) {
        logger.Write(ERROR, "[Track-ARS] Directory [%s] not found!", tracksPath.c_str());
        return 0;
    }

    for (const auto& file : fs::recursive_directory_iterator(tracksPath)) {
        if (Util::to_lower(fs::path(file).extension().string()) != ".xml") {
            continue;
        }

        CTrackData track = CTrackData::ReadARS(fs::path(file).string());
        if (!track.Name.empty()) {
            arsTracks.push_back(track);
            logger.Write(DEBUG, "[Track-ARS] Loaded track [%s]", track.Name.c_str());
        }
        else {
            logger.Write(WARN, "[Track-ARS] Skipping [%s] - not a valid file", fs::path(file).string().c_str());
        }
    }
    logger.Write(INFO, "[Track-ARS] Tracks loaded: %d", arsTracks.size());

    return static_cast<unsigned>(arsTracks.size());
}

void GhostReplay::AddReplay(const CReplayData& replay) {
    std::lock_guard replaysLock(replaysMutex);
    replays.push_back(std::make_shared<CReplayData>(replay));
}

void GhostReplay::TriggerLoadStop() {
    stopLoading = true;
}
