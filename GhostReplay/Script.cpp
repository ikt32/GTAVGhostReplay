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
    std::vector<std::filesystem::path> pendingReplays;

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
    LOG(Info, "Settings loaded");

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
        LOG(Info, "[Replay] Creating missing directory [{}]", replaysPath);
        if (!std::filesystem::create_directory(fs::path(replaysPath))) {
            LOG(Error, "[Replay] Failed to create directory, replays will not be saved!");
        }
    }

    if (!(fs::exists(fs::path(tracksPath)) && fs::is_directory(fs::path(tracksPath)))) {
        LOG(Info, "[Track] Creating missing directory [{}]", tracksPath);
        if (!std::filesystem::create_directory(fs::path(tracksPath))) {
            LOG(Error, "[Track] Failed to create directory, tracks will not be saved!");
        }
    }
}

void GhostReplay::LoadReplays() {
    const std::string replaysPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Replays";

    LOG(Debug, "[Replay] Clearing and reloading replays");
    
    if (!(fs::exists(fs::path(replaysPath)) && fs::is_directory(fs::path(replaysPath)))) {
        LOG(Error, "[Replay] Directory [{}] not found!", replaysPath);
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
        if (Util::to_lower(fs::path(file).extension().string()) != ".json" &&
            Util::to_lower(fs::path(file).extension().string()) != ".grbin") {
            continue;
        }
        pendingReplays.push_back(fs::path(file));
        ++totalReplays;
    }

    std::thread([replaysPath]() {
        for (const auto& file : pendingReplays) {
            if (stopLoading) {
                return;
            }

            {
                std::lock_guard nameMutex(currentLoadingReplayMutex);
                currentLoadingReplay = file.stem().string();
            }


            CReplayData replay = CReplayData::Read(file);
            if (!replay.Nodes.empty()) {
                std::lock_guard replaysLock(replaysMutex);
                replays.push_back(std::make_shared<CReplayData>(replay));
            }
            else {
                LOG(Warning, "[Replay] Skipping [{}] - not a valid file", file.string());
            }

            LOG(Debug, "[Replay] Loaded replay [{}]", replay.Name);
            ++loadedReplays;
        }

        LOG(Info, "[Replay] Replays loaded: {}", replays.size());
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

    LOG(Debug, "[Track] Clearing and reloading tracks");

    tracks.clear();

    if (!(fs::exists(fs::path(tracksPath)) && fs::is_directory(fs::path(tracksPath)))) {
        LOG(Error, "[Track] Directory [{}] not found!", tracksPath);
        return 0;
    }

    for (const auto& file : fs::recursive_directory_iterator(tracksPath)) {
        if (Util::to_lower(fs::path(file).extension().string()) != ".json" &&
            Util::to_lower(fs::path(file).extension().string()) != ".grbin") {
            continue;
        }

        CTrackData track = CTrackData::Read(fs::path(file).string());
        if (!track.Name.empty())
            tracks.push_back(track);
        else
            LOG(Warning, "[Track] Skipping [{}] - not a valid file", fs::path(file).string());

        LOG(Debug, "[Track] Loaded track [{}]", track.Name);
    }
    LOG(Info, "[Track] Tracks loaded: {}", tracks.size());

    return static_cast<unsigned>(tracks.size());
}

uint32_t GhostReplay::LoadTrackImages() {
    const std::string tracksPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Tracks";

    LOG(Debug, "[TrackImg] Clearing and reloading track images");

    trackImages.clear();

    if (!(fs::exists(fs::path(tracksPath)) && fs::is_directory(fs::path(tracksPath)))) {
        LOG(Error, "[TrackImg] Directory [{}] not found!", tracksPath);
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
            LOG(Warning, "[TrackImg] Skipping [{}]: not an valid image.", fs::path(file).string());
            continue;
        }

        int handle = createTexture(fileName.c_str());
        trackImages.emplace_back(handle, stem, width, height);

        LOG(Debug, "[TrackImg] Loaded track image [{}]", stem);
    }
    LOG(Info, "[TrackImg] Track images loaded: {}", trackImages.size());

    return static_cast<unsigned>(trackImages.size());
}

uint32_t GhostReplay::LoadARSTracks() {
    const std::string tracksPath =
        Paths::GetRunningExecutableFolder() + "\\Scripts\\ARS\\Tracks";

    LOG(Debug, "[Track-ARS] Clearing and reloading tracks");

    arsTracks.clear();

    if (!(fs::exists(fs::path(tracksPath)) && fs::is_directory(fs::path(tracksPath)))) {
        LOG(Error, "[Track-ARS] Directory [{}] not found!", tracksPath);
        return 0;
    }

    for (const auto& file : fs::recursive_directory_iterator(tracksPath)) {
        if (Util::to_lower(fs::path(file).extension().string()) != ".xml") {
            continue;
        }

        CTrackData track = CTrackData::ReadARS(fs::path(file).string());
        if (!track.Name.empty()) {
            arsTracks.push_back(track);
            LOG(Debug, "[Track-ARS] Loaded track [{}]", track.Name);
        }
        else {
            LOG(Warning, "[Track-ARS] Skipping [{}] - not a valid file", fs::path(file).string());
        }
    }
    LOG(Info, "[Track-ARS] Tracks loaded: {}", arsTracks.size());

    return static_cast<unsigned>(arsTracks.size());
}

void GhostReplay::AddReplay(const CReplayData& replay) {
    std::lock_guard replaysLock(replaysMutex);
    replays.push_back(std::make_shared<CReplayData>(replay));
}

void GhostReplay::TriggerLoadStop() {
    stopLoading = true;
}
