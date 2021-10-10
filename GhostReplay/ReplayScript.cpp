#include "ReplayScript.hpp"
#include "Script.hpp"
#include "VehicleMod.hpp"

#include "Compatibility.hpp"
#include "Constants.hpp"
#include "ReplayScriptUtils.hpp"

#include "Memory/VehicleExtensions.hpp"
#include "Util/Math.hpp"
#include "Util/Paths.hpp"
#include "Util/Game.hpp"
#include "Util/UI.hpp"
#include "Util/Logger.hpp"
#include "Util/String.hpp"
#include "Util/Inputs.hpp"

#include <inc/enums.h>
#include <inc/natives.h>
#include <fmt/format.h>
#include <filesystem>
#include <algorithm>

using VExt = VehicleExtensions;

CReplayScript::CReplayScript(
    CScriptSettings& settings,
    std::vector<std::shared_ptr<CReplayData>>& replays,
    std::vector<CTrackData>& tracks,
    std::vector<CImage>& trackImages,
    std::vector<CTrackData>& arsTracks)
    : mCurrentTime(static_cast<double>(MISC::GET_GAME_TIMER()))
    , mGlobalReplayState(EReplayState::Idle)
    , mSettings(settings)
    , mReplays(replays)
    , mCompatibleReplays()
    , mTracks(tracks)
    , mTrackImages(trackImages)
    , mArsTracks(arsTracks)
    , mActiveTrack(nullptr)
    , mScriptMode(EScriptMode::ReplayActive)
    , mRecordState(ERecordState::Idle)
    , mLastPos(Vector3{})
    , mCurrentRun("") {
}

CReplayScript::~CReplayScript() = default;

void CReplayScript::Tick() {
    mPlayerVehicle = PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false);
    updateSteppedTime();

    switch(mScriptMode) {
        case EScriptMode::DefineTrack:
            updateTrackDefine();
            break;
        case EScriptMode::ReplayActive:
            updateReplay();
            break;
    }
}

void CReplayScript::StopAllReplays() {
    for (const auto& replayVehicle : mReplayVehicles) {
        replayVehicle->StopReplay();
    }
    mReplayStartTime = 0.0;
    mReplayCurrentTime = 0.0;
}

void CReplayScript::SetTrack(const std::string& trackName) {
    StopAllReplays();
    ClearSelectedReplays();
    mCompatibleReplays.clear();
    clearPtfx();
    clearTrackBlips();

    if (trackName.empty()) {
        mActiveTrack = nullptr;
        mRecordState = ERecordState::Idle;
        return;
    }

    CTrackData* foundTrack = nullptr;
    if (!trackName._Starts_with("[ARS] ")) {
        for (auto& track : mTracks) {
            if (track.Name == trackName) {
                foundTrack = &track;
                break;
            }
        }
    }
    else {
        for (auto& track : mArsTracks) {
            if (track.Name == trackName) {
                foundTrack = &track;
                break;
            }
        }
    }

    if (foundTrack != nullptr) {
        mActiveTrack = foundTrack;
        mRecordState = ERecordState::Idle;
        mCompatibleReplays = GetCompatibleReplays(trackName);
        if (mSettings.Main.DrawStartFinish)
            createPtfx(*foundTrack);

        if (mSettings.Main.StartStopBlips)
            createTrackBlips(*foundTrack);

        if (mSettings.Replay.AutoLoadGhost) {
            Vehicle vehicle = PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false);
            auto fastestReplay = GetFastestReplay(trackName, ENTITY::GET_ENTITY_MODEL(vehicle));
            if (fastestReplay != nullptr && ENTITY::DOES_ENTITY_EXIST(vehicle)) {
                SelectReplay(fastestReplay->Name, fastestReplay->Timestamp);
            }
        }
        return;
    }

    logger.Write(ERROR, "SetTrack: No track found with the name [%s]", trackName.c_str());
}

void CReplayScript::SelectReplay(const std::string& replayName, unsigned long long timestamp) {
    for (auto& replay : mReplays) {
        bool nameOK = replay->Name == replayName;
        bool timeOK = timestamp == 0 ? true : replay->Timestamp == timestamp;

        if (nameOK && timeOK) {
            if (!replay->FullyParsed()) {
                replay->CompleteReadAsync();
            }

            mActiveReplays.push_back(replay);
            mReplayVehicles.push_back(std::make_unique<CReplayVehicle>(mSettings, &*replay,
                std::bind(static_cast<void(CReplayScript::*)(int)>(&CReplayScript::ghostCleanup),
                    this, std::placeholders::_1)));

            updateSlowestReplay();

            // If the vehicle was added while a replay is running - also let it join!
            if (mGlobalReplayState != EReplayState::Idle) {
                // Trigger Idle -> Playing state
                mReplayVehicles.back()->UpdatePlayback(mReplayCurrentTime, true, false);
                // Set Paused/Playing state
                mReplayVehicles.back()->TogglePause(mGlobalReplayState == EReplayState::Paused);
                // Set correct node
                mReplayVehicles.back()->SetReplayTime(mReplayCurrentTime);
            }

            return;
        }
    }

    // createReplayVehicle should've notified already if we're here.
    if (mReplayVehicles.back()->GetVehicle() == 0) {
        mReplayVehicles.pop_back();
        mRecordState = ERecordState::Idle;
        return;
    }
    logger.Write(ERROR, "SetReplay: No replay found with the name [%s]", replayName.c_str());
}

void CReplayScript::DeselectReplay(const std::string& replayName, unsigned long long timestamp) {
    auto replayIt = std::find_if(mActiveReplays.begin(), mActiveReplays.end(),
        [&](const auto activeReplay) {
            return activeReplay->Name == replayName &&
                activeReplay->Timestamp == timestamp;
        }
    );

    auto replayVehicleIt = std::find_if(mReplayVehicles.begin(), mReplayVehicles.end(),
        [&](const std::unique_ptr<CReplayVehicle>& replayVehicle) {
            return replayVehicle->GetReplay()->Name == replayName &&
                replayVehicle->GetReplay()->Timestamp == timestamp;
        }
    );

    if (replayIt != mActiveReplays.end()) {
        mActiveReplays.erase(replayIt);
    }

    if (replayVehicleIt != mReplayVehicles.end()) {
        if (replayVehicleIt->get() == mPassengerVehicle) {
            DeactivatePassengerMode(mPassengerVehicle->GetVehicle());
            mPassengerVehicle = nullptr;
        }
        mReplayVehicles.erase(replayVehicleIt);
    }

    updateSlowestReplay();
}

void CReplayScript::ClearSelectedReplays() {
    mActiveReplays.clear();
    mReplayVehicles.clear();
    mSlowestReplayTime = 0.0;
    mPassengerVehicle = nullptr;
    mGlobalReplayState = EReplayState::Idle;
}

void CReplayScript::ClearUnsavedRuns() {
    mUnsavedRuns.clear();
}

std::vector<CReplayData>::iterator CReplayScript::EraseUnsavedRun(std::vector<CReplayData>::iterator runIt) {
    return mUnsavedRuns.erase(runIt);
}

std::vector<std::shared_ptr<CReplayData>> CReplayScript::GetCompatibleReplays(const std::string& trackName) {
    std::vector<std::shared_ptr<CReplayData>> replays;
    for (auto& replay : mReplays) {
        if (replay->Track == trackName)
            replays.push_back(replay);
    }
    return replays;
}

void CReplayScript::AddCompatibleReplay(const CReplayData& value) {
    mCompatibleReplays.push_back(std::make_shared<CReplayData>(value));
}

bool CReplayScript::IsFastestLap(const std::string& trackName, Hash vehicleModel, double timestamp) {
    auto fastestReplay = GetFastestReplay(trackName, vehicleModel);

    if (fastestReplay != nullptr)
        return timestamp < fastestReplay->GetNodes().back().Timestamp;

    return false;
}

std::shared_ptr<CReplayData> CReplayScript::GetFastestReplay(const std::string& trackName, Hash vehicleModel) {
    std::shared_ptr<CReplayData> fastestReplay = nullptr;

    for (auto& replay : mReplays) {
        if (replay->Track != trackName)
            continue;

        if (vehicleModel != 0 && replay->VehicleModel != vehicleModel)
            continue;

        if (fastestReplay == nullptr ||
            replay->GetNodes().back().Timestamp < fastestReplay->GetNodes().back().Timestamp) {
            fastestReplay = replay;
        }
    }
    return fastestReplay;
}

bool CReplayScript::StartLineDef(SLineDef& lineDef, const std::string& lineName) {
    int progress = 0;

    const eControl cancelControl = eControl::ControlFrontendCancel;
    const eControl registerControl = eControl::ControlContext;

    createPtfx(*mActiveTrack);

    bool result;
    while(true) {
        if (PAD::IS_CONTROL_JUST_RELEASED(0, cancelControl)) {
            result = false;
            goto exit;
        }

        auto coords = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), true);

        UI::DrawSphere(coords, 0.20f, 255, 255, 255, 255);

        if (progress == 1) {
            UI::DrawSphere(lineDef.A, 0.25f, 255, 255, 255, 255);
            UI::DrawLine(lineDef.A, coords, 255, 255, 255, 255);
        }

        if (PAD::IS_CONTROL_JUST_RELEASED(0, registerControl)) {
            switch (progress) {
                case 0: {
                    lineDef.A = coords;
                    progress++;
                    break;
                }
                case 1: {
                    lineDef.B = coords;
                    result = true;
                    goto exit;
                }
                default: {
                    result = false;
                    goto exit;
                }
            }
        }

        std::string currentStepName = fmt::format("{} {} point", progress == 0 ? "left" : "right", lineName);

        UI::ShowHelpText(fmt::format(
            "Press {} to register {} at your current location. "
            "Press {} to cancel and exit.", 
            Inputs::GetControlString(registerControl), currentStepName,
            Inputs::GetControlString(cancelControl)));
        WAIT(0);
    }

exit:
    clearPtfx();
    return result;
}

void CReplayScript::DeleteTrack(const CTrackData& track) {
    auto trackIt = std::find_if(mTracks.begin(), mTracks.end(), [&](const CTrackData& x) {
        return x.FileName() == track.FileName();
    });

    if (trackIt == mTracks.end()) {
        logger.Write(ERROR, "[Track] Attempted to delete track [%s] but didn't find it in the list? Filename: [%s]",
            track.Name.c_str(), track.FileName().c_str());
        UI::Notify(fmt::format("[Track] Failed to delete {}", track.Name));
        return;
    }

    if (mActiveTrack) {
        if (mActiveTrack->FileName() == track.FileName()) {
            SetTrack("");
        }
    }
    track.Delete();
    mTracks.erase(trackIt);
    logger.Write(INFO, "[Track] Deleted track [%s], Filename: [%s]",
        track.Name.c_str(), track.FileName().c_str());
}

void CReplayScript::DeleteReplay(const CReplayData& replay) {
    auto replayIt = std::find_if(mReplays.begin(), mReplays.end(), [&](const auto& x) {
        return x->FileName() == replay.FileName();
    });
    // Because it's a copy, let's just update this in parallel.
    auto replayCompIt = std::find_if(mCompatibleReplays.begin(), mCompatibleReplays.end(), [&](const auto& x) {
        return x->FileName() == replay.FileName();
    });

    if (replayIt == mReplays.end()) {
        logger.Write(ERROR, "[Replay] Attempted to delete replay [%s] but didn't find it in the list? Filename: [%s]",
            replay.Name.c_str(), replay.FileName().c_str());
        UI::Notify(fmt::format("[Replay] Failed to delete {}", replay.Name));
        return;
    }
    
    if (replayCompIt == mCompatibleReplays.end()) {
        logger.Write(ERROR, "[Replay] Attempted to delete replay [%s] but didn't find it in the compatible list? Filename: [%s]",
            replay.Name.c_str(), replay.FileName().c_str());
        UI::Notify(fmt::format("[Replay] Failed to delete {}", replay.Name));
        return;
    }

    DeselectReplay(replay.Name, replay.Timestamp);
    replay.Delete();
    mReplays.erase(replayIt);
    mCompatibleReplays.erase(replayCompIt);
    logger.Write(INFO, "[Replay] Deleted replay [%s], Filename: [%s]",
        replay.Name.c_str(), replay.FileName().c_str());
}

std::string CReplayScript::GetTrackImageMenuString(const std::string& trackName) {
    std::string extra;

    auto imgIt = std::find_if(mTrackImages.begin(), mTrackImages.end(), [trackName](const CImage& img) {
        return img.Name == trackName;
    });

    if (imgIt != mTrackImages.end()) {
        extra = fmt::format("{}W{}H{}",
            imgIt->TextureID,
            imgIt->ResX,
            imgIt->ResY);
    }
    return extra;
}

void CReplayScript::ActivatePassengerMode() {
    if (!mActiveTrack || mReplayVehicles.empty()) {
        mPassengerModeActive = false;
        return;
    }

    CReplayVehicle* passengerVehicle = mPassengerVehicle;
    if (passengerVehicle == nullptr) {
        passengerVehicle = mReplayVehicles[0].get();
    }
    if (mGlobalReplayState != EReplayState::Idle &&
        passengerVehicle->GetReplayState() == EReplayState::Idle) {
        UI::Notify("Can't enter passenger mode, vehicle is not driving.");
        return;
    }

    StopRecording();

    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vehicle playerVehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);
    if (ENTITY::DOES_ENTITY_EXIST(playerVehicle)) {
        mPassengerModePlayerVehicle = playerVehicle;
        if (ENTITY::IS_ENTITY_A_MISSION_ENTITY(playerVehicle)) {
            mPassengerModPlayerVehicleManagedByThisScript = false;
        }
        else {
            ENTITY::SET_ENTITY_AS_MISSION_ENTITY(playerVehicle, true, true);
            mPassengerModPlayerVehicleManagedByThisScript = true;
        }

        auto pos = ENTITY::GET_ENTITY_COORDS(playerVehicle, !ENTITY::IS_ENTITY_DEAD(playerVehicle, false));
        ENTITY::FREEZE_ENTITY_POSITION(playerVehicle, true);
        ENTITY::SET_ENTITY_COORDS_NO_OFFSET(playerVehicle, pos.x, pos.y, pos.z + 100.0f, 0, 0, 0);
        ENTITY::SET_ENTITY_VISIBLE(playerVehicle, false, false);
    }

    bool replaying = mGlobalReplayState != EReplayState::Idle;

    // Spawn the vehicles and leave 'em there in paused state.
    if (!replaying) {
        TogglePause(true);
    }

    // Move player into a vehicle, if not already in one.
    if (mPassengerVehicle == nullptr) {
        Vehicle replayVehicle = mReplayVehicles[0]->GetVehicle();
        setPlayerIntoVehicleFreeSeat(replayVehicle);
        mPassengerVehicle = mReplayVehicles[0].get();
    }
    else {
        setPlayerIntoVehicleFreeSeat(mPassengerVehicle->GetVehicle());
    }

    PED::SET_PED_CAN_BE_KNOCKED_OFF_VEHICLE(playerPed, eKnockOffVehicle::KNOCKOFFVEHICLE_NEVER);
    mPassengerFlagWindscreen = PED::GET_PED_CONFIG_FLAG(playerPed, 32, true);
    PED::SET_PED_CONFIG_FLAG(playerPed, 32, false);

    mPassengerModeActive = true;
}

void CReplayScript::DeactivatePassengerMode() {
    if (mPassengerVehicle) {
        DeactivatePassengerMode(mPassengerVehicle->GetVehicle());
    }
}

void CReplayScript::DeactivatePassengerMode(Vehicle vehicle) {
    if (!mPassengerModeActive) {
        return;
    }

    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vehicle replayVehicle = vehicle;

    Vector3 currRot = ENTITY::GET_ENTITY_ROTATION(replayVehicle, 0);
    Vector3 currCoords = ENTITY::GET_ENTITY_COORDS(replayVehicle, !ENTITY::IS_ENTITY_DEAD(replayVehicle, false));
    Vector3 forward = ENTITY::GET_ENTITY_FORWARD_VECTOR(replayVehicle);
    currCoords.x -= forward.x * 10.0f;
    currCoords.y -= forward.y * 10.0f;
    float groundZ = currCoords.z;
    bool groundZOk = MISC::GET_GROUND_Z_FOR_3D_COORD(currCoords.x, currCoords.y, currCoords.z, &groundZ, 0, 0);
    if (!groundZOk) {
        groundZ = currCoords.z;
    }

    if (mPassengerModePlayerVehicle) {
        PED::SET_PED_INTO_VEHICLE(playerPed, mPassengerModePlayerVehicle, -1);

        ENTITY::SET_ENTITY_COORDS(mPassengerModePlayerVehicle, currCoords.x, currCoords.y, groundZ,
            false, false, false, false);
        ENTITY::SET_ENTITY_ROTATION(mPassengerModePlayerVehicle, currRot.x, currRot.y, currRot.z, 0, false);

        ENTITY::FREEZE_ENTITY_POSITION(mPassengerModePlayerVehicle, false);
        ENTITY::SET_ENTITY_VISIBLE(mPassengerModePlayerVehicle, true, false);

        if (mPassengerModPlayerVehicleManagedByThisScript) {
            ENTITY::SET_ENTITY_AS_MISSION_ENTITY(mPassengerModePlayerVehicle, false, true);
            ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&mPassengerModePlayerVehicle);

            VEHICLE::SET_VEHICLE_FIXED(mPassengerModePlayerVehicle);
            VEHICLE::SET_VEHICLE_DEFORMATION_FIXED(mPassengerModePlayerVehicle);

            mPassengerModPlayerVehicleManagedByThisScript = false;
        }

        mPassengerModePlayerVehicle = 0;
    }
    else {
        ENTITY::SET_ENTITY_COORDS(playerPed, currCoords.x, currCoords.y, groundZ,
            false, false, false, false);
    }

    PED::SET_PED_CAN_BE_KNOCKED_OFF_VEHICLE(playerPed, eKnockOffVehicle::KNOCKOFFVEHICLE_DEFAULT);
    PED::SET_PED_CONFIG_FLAG(playerPed, 32, mPassengerFlagWindscreen);

    mPassengerModeActive = false;
}

void CReplayScript::PassengerVehicleNext() {
    if (mReplayVehicles.size() < 2)
        return;

    auto selectedVehicle = std::find_if(mReplayVehicles.begin(), mReplayVehicles.end(),
        [&](const auto& replayVehicle) { return replayVehicle.get() == mPassengerVehicle; });

    if (!mPassengerModeActive) {
        if (selectedVehicle == mReplayVehicles.end() ||
            std::next(selectedVehicle) == mReplayVehicles.end())
            mPassengerVehicle = mReplayVehicles.begin()->get();
        else
            mPassengerVehicle = std::next(selectedVehicle)->get();
    }
    else {
        if (mGlobalReplayState != EReplayState::Idle) {
            do {
                if (selectedVehicle == mReplayVehicles.end()) {
                    selectedVehicle = mReplayVehicles.begin();
                }
                else {
                    selectedVehicle = std::next(selectedVehicle);
                }
            } while (selectedVehicle == mReplayVehicles.end() ||
                selectedVehicle->get()->GetReplayState() == EReplayState::Idle);
            mPassengerVehicle = selectedVehicle->get();
            setPlayerIntoVehicleFreeSeat(mPassengerVehicle->GetVehicle());
        }
    }
}

void CReplayScript::PassengerVehiclePrev() {
    if (mReplayVehicles.size() < 2)
        return;

    auto selectedVehicle = std::find_if(mReplayVehicles.begin(), mReplayVehicles.end(),
        [&](const auto& replayVehicle) {return replayVehicle.get() == mPassengerVehicle; });

    if (!mPassengerModeActive) {
        if (selectedVehicle == mReplayVehicles.end() ||
            selectedVehicle == mReplayVehicles.begin())
            mPassengerVehicle = mReplayVehicles.back().get();
        else
            mPassengerVehicle = std::prev(selectedVehicle)->get();
    }
    else {
        if (mGlobalReplayState != EReplayState::Idle) {
            do {
                if (selectedVehicle == mReplayVehicles.end() ||
                    selectedVehicle == mReplayVehicles.begin()) {
                    selectedVehicle = std::prev(mReplayVehicles.end());
                }
                else {
                    selectedVehicle = std::prev(selectedVehicle);
                }
            } while (selectedVehicle == mReplayVehicles.end() ||
                selectedVehicle->get()->GetReplayState() == EReplayState::Idle);
            mPassengerVehicle = selectedVehicle->get();
            setPlayerIntoVehicleFreeSeat(mPassengerVehicle->GetVehicle());
        }
    }
}

CReplayVehicle* CReplayScript::GetPassengerVehicle() {
    return mPassengerVehicle;
}

double CReplayScript::GetReplayProgress() {
    return mReplayCurrentTime;
}

double CReplayScript::GetSlowestActiveReplay() {
    return mSlowestReplayTime;
}

void CReplayScript::TogglePause(bool pause) {
    bool anyDriving = mGlobalReplayState != EReplayState::Idle;
    for (auto& replayVehicle : mReplayVehicles) {
        if (!anyDriving ||
            replayVehicle->GetReplayState() != EReplayState::Idle) {
            replayVehicle->TogglePause(pause);
        }
    }

    if (anyDriving && !pause) {
        mReplayStartTime = getSteppedTime() - mReplayCurrentTime;
    }
    else if (!anyDriving && !pause) {
        mReplayStartTime = getSteppedTime();
        mReplayCurrentTime = 0.0;
    }
}

void CReplayScript::ScrubBackward(double step) {
    if (mReplayCurrentTime < step) {
        step = mReplayCurrentTime;
    }

    if (step <= 0.0) {
        return;
    }

    mReplayCurrentTime -= step;
    mReplayStartTime += step;

    for (auto& replayVehicle : mReplayVehicles) {
        if (replayVehicle->GetReplayState() != EReplayState::Idle)
            replayVehicle->SetReplayTime(mReplayCurrentTime + mSettings.Replay.OffsetSeconds * 1000.0);
    }
}

void CReplayScript::ScrubForward(double step) {
    if (mReplayCurrentTime + step > mSlowestReplayTime) {
        step = mSlowestReplayTime - mReplayCurrentTime;
    }

    if (step <= 0.0f) {
        return;
    }

    mReplayCurrentTime += step;
    mReplayStartTime -= step;

    for (auto& replayVehicle : mReplayVehicles) {
        if (replayVehicle->GetReplayState() != EReplayState::Idle)
            replayVehicle->SetReplayTime(mReplayCurrentTime + mSettings.Replay.OffsetSeconds * 1000.0);
    }
}

uint64_t CReplayScript::GetNumFrames() {
    if (mReplayVehicles.size() == 1)
        return mReplayVehicles[0]->GetNumFrames();
    return 1;
}

uint64_t CReplayScript::GetFrameIndex() {
    if (mReplayVehicles.size() == 1)
        return mReplayVehicles[0]->GetFrameIndex();
    return 0;
}

void CReplayScript::FramePrev() {
    if (mReplayVehicles.size() == 1) {
        auto delta = mReplayVehicles[0]->FramePrev();
        mReplayCurrentTime -= delta;
        mReplayStartTime += delta;
    }
}

void CReplayScript::FrameNext() {
    if (mReplayVehicles.size() == 1) {
        auto delta = mReplayVehicles[0]->FrameNext();
        mReplayCurrentTime += delta;
        mReplayStartTime -= delta;
    }
}

void CReplayScript::TeleportToTrack(const CTrackData& trackData) {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vehicle playerVehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);

    if (mPassengerVehicle && playerVehicle) {
        if (mPassengerVehicle->GetVehicle() == playerVehicle) {
            UI::Notify("Can't teleport while riding ghost vehicle.");
            return;
        }
    }

    Vector3 startLineAB = (trackData.StartLine.A + trackData.StartLine.B) * 0.5f;
    Vector3 startOffset = GetPerpendicular(startLineAB, trackData.StartLine.B, 15.0f, false);
    float startHeading = MISC::GET_HEADING_FROM_VECTOR_2D(
        trackData.StartLine.B.x - trackData.StartLine.A.x,
        trackData.StartLine.B.y - trackData.StartLine.A.y
    ) + 90.0f;

    if (playerVehicle) {
        ENTITY::SET_ENTITY_COORDS(playerVehicle, startOffset.x, startOffset.y, startOffset.z,
            false, false, false, false);
        ENTITY::SET_ENTITY_HEADING(playerVehicle, startHeading);
    }
    else {
        ENTITY::SET_ENTITY_COORDS(playerPed, startOffset.x, startOffset.y, startOffset.z,
            false, false, false, false);
        ENTITY::SET_ENTITY_HEADING(playerPed, startHeading);
    }
}

void CReplayScript::ToggleDrivers(bool enable) {
    for (auto& replayVehicle : mReplayVehicles) {
        replayVehicle->ToggleDriver(enable);
    }
}

// Prevent needlessly traversing mReplayVehicles multiple times per tick
void CReplayScript::updateGlobalStates() {
    auto replayState = EReplayState::Idle;
    for (auto& replayVehicle : mReplayVehicles) {
        if (replayVehicle->GetReplayState() == EReplayState::Playing) {
            replayState = EReplayState::Playing;
            break;
        }
        if (replayVehicle->GetReplayState() == EReplayState::Paused) {
            replayState = EReplayState::Paused;
        }
    }

    // On transition to idle, reset timers.
    if (mGlobalReplayState != replayState &&
        replayState == EReplayState::Idle) {
        mReplayStartTime = 0.0;
        mReplayCurrentTime = 0.0;
    }
    mGlobalReplayState = replayState;
}

void CReplayScript::updateReplay() {
    if (!mActiveTrack) {
        mGlobalReplayState = EReplayState::Idle;
        mReplayStartTime = 0.0;
        mReplayCurrentTime = 0.0;
        return;
    }

    updateGlobalStates();

    Vehicle vehicle = PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false);
    Vector3 nowPos;

    bool inGhostVehicle = false;
    for (const auto& replayVehicle : mReplayVehicles) {
        if (vehicle == replayVehicle->GetVehicle()) {
            inGhostVehicle = true;
            break;
        }
    }

    if (ENTITY::DOES_ENTITY_EXIST(vehicle)) {
        nowPos = ENTITY::GET_ENTITY_COORDS(vehicle, true);
    }
    else {
        nowPos = mLastPos;
    }

    auto gameTime = getSteppedTime();
    bool startPassedThisTick = passedLineThisTick(mActiveTrack->StartLine, mLastPos, nowPos);
    bool finishPassedThisTick = passedLineThisTick(mActiveTrack->FinishLine, mLastPos, nowPos);

    // Prevents default coords from activating record/replay
    if (mLastPos == Vector3{}) {
        startPassedThisTick = false;
        finishPassedThisTick = false;
    }

    mLastPos = nowPos;

    updateRecord(
        gameTime,
        !inGhostVehicle && startPassedThisTick,
        !inGhostVehicle && finishPassedThisTick);

    if (mGlobalReplayState == EReplayState::Playing) {
        mReplayCurrentTime = gameTime - mReplayStartTime;
    }

    double replayTime = mReplayCurrentTime + mSettings.Replay.OffsetSeconds * 1000.0;
    bool anyGhostLapTriggered = false;
    
    float distMaxSq = std::numeric_limits<float>::max();
    Vehicle camIgnoreVehicle = 0;

    std::vector<CReplayVehicle*> replayVehiclesToDelete;
    for (const auto& replayVehicle : mReplayVehicles) {
        if (!replayVehicle->GetReplay()->FullyParsed())
            continue;

        if (!ENTITY::DOES_ENTITY_EXIST(replayVehicle->GetVehicle())) {
            replayVehiclesToDelete.push_back(replayVehicle.get());
            logger.Write(ERROR, fmt::format("[Replay] Managed vehicle {} with ID {:08X} stopped existing. "
                "Another script likely forcefully removed it. "
                "Deselected replay to prevent crashes.",
                Util::GetVehicleName(replayVehicle->GetReplay()->VehicleModel),
                replayVehicle->GetVehicle()));
            continue;
        }

        anyGhostLapTriggered |= replayVehicle->UpdatePlayback(
            replayTime,
            !inGhostVehicle && startPassedThisTick,
            !inGhostVehicle && finishPassedThisTick);

        if (replayVehicle->GetReplayState() != EReplayState::Idle) {
            Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(mPlayerVehicle, true);
            Vector3 ghostPos = ENTITY::GET_ENTITY_COORDS(replayVehicle->GetVehicle(), true);
            float distSq = DistanceSq(playerPos, ghostPos);
            if (distSq < distMaxSq) {
                distMaxSq = distSq;
                camIgnoreVehicle = replayVehicle->GetVehicle();
            
                if (mSettings.Main.Debug) {
                    UI::DrawLine(playerPos, ghostPos, 255, 0, 0, 255);
                    UI::ShowText3D((playerPos + ghostPos) * 0.5f, 10.0f, {
                        fmt::format("{:.2f}m", Distance(playerPos, ghostPos)),
                    });
                }
            }
        }
    }

    if (!replayVehiclesToDelete.empty()) {
        for (auto* deleteVehicle : replayVehiclesToDelete) {
            DeselectReplay(deleteVehicle->GetReplay()->Name, deleteVehicle->GetReplay()->Timestamp);
        }
        UI::Notify("~r~ERROR: Replay vehicle(s) missing, deselected for safety. Details in log.");
    }

    // Still won't go right if it overlaps multiple vehicles, but this is *something*.
    if (camIgnoreVehicle) {
        // Only seems to apply for the last passed entity.
        // _DISABLE_CAM_COLLISION_FOR_ENTITY
        CAM::_DISABLE_CAM_COLLISION_FOR_ENTITY(camIgnoreVehicle);
    
        if (mSettings.Main.Debug) {
            Vector3 infoSpherePos = ENTITY::GET_ENTITY_COORDS(camIgnoreVehicle, true);
            infoSpherePos.z += 2.5f;
            UI::DrawSphere(infoSpherePos, 0.10f, 255, 0, 0, 255);
        }
    }

    if (anyGhostLapTriggered) {
        mReplayStartTime = gameTime;
    }

    if (mPassengerModeActive && inGhostVehicle) {
        auto playerPed = PLAYER::PLAYER_PED_ID();
        AUDIO::STOP_CURRENT_PLAYING_SPEECH(playerPed);
        AUDIO::STOP_CURRENT_PLAYING_AMBIENT_SPEECH(playerPed);

        PAD::DISABLE_CONTROL_ACTION(0, eControl::ControlVehicleExit, true);
    }

    if (mSettings.Main.ShowRecordTime && !IsPassengerModeActive()) {
        if (IsRecording()) {
            UI::ShowText(0.005f, 0.0f, 1.0f, Util::FormatMillisTime(gameTime - mRecordStart));
        }
        else {
            UI::ShowText(0.005f, 0.0f, 1.0f, Util::FormatMillisTime(0.0));
        }
    }
}

void CReplayScript::updateRecord(double gameTime, bool startPassedThisTick, bool finishPassedThisTick) {
    Vehicle vehicle = PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false);
    if (!ENTITY::DOES_ENTITY_EXIST(vehicle) || !mActiveTrack) {
        mRecordState = ERecordState::Idle;
        mCurrentRun = CReplayData("");
        return;
    }

    switch (mRecordState) {
        case ERecordState::Idle: {
            if (startPassedThisTick) {
                mRecordState = ERecordState::Recording;
                startRecord(gameTime, vehicle);
                SReplayNode node{};
                saveNode(gameTime, node, vehicle, true);
            }
            break;
        }
        case ERecordState::Recording: {
            if (ENTITY::GET_ENTITY_MODEL(vehicle) != mCurrentRun.VehicleModel) {
                mRecordState = ERecordState::Idle;
                mCurrentRun = CReplayData("");
                break;
            }
            SReplayNode node{};
            bool saved = saveNode(gameTime, node, vehicle, false);

            if (finishPassedThisTick) {
                mRecordState = ERecordState::Idle;
                finishRecord(saved, node);
                if (startPassedThisTick) {
                    mRecordState = ERecordState::Recording;
                    startRecord(gameTime, vehicle);
                }
            }
            break;
        }
    }
}

void CReplayScript::updateTrackDefine() {

}

bool CReplayScript::passedLineThisTick(SLineDef line, Vector3 oldPos, Vector3 newPos) {
    if (Distance(line.A, newPos) > Distance(line.A, line.B) || Distance(line.B, newPos) > Distance(line.A, line.B)) {
        return false;
    }
    float oldSgn = sgn(GetAngleABC(line.A, line.B, oldPos));
    float newSgn = sgn(GetAngleABC(line.A, line.B, newPos));
    return oldSgn != newSgn && oldSgn > 0.0f;
}

void CReplayScript::startRecord(double gameTime, Vehicle vehicle) {
    mRecordStart = gameTime;
    mCurrentRun.Timestamp = std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::system_clock::now().time_since_epoch()).count();
    mCurrentRun.Track = mActiveTrack->Name;
    mCurrentRun.ClearNodes();
    mCurrentRun.VehicleModel = ENTITY::GET_ENTITY_MODEL(vehicle);

    VehicleModData modData = VehicleModData::LoadFrom(vehicle);
    mCurrentRun.VehicleMods = modData;

    Ped driver = VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1, false);
    SReplayDriverData driverData = SReplayDriverData::LoadFrom(driver);
    mCurrentRun.ReplayDriver = driverData;

    //UI::Notify("Record started", false);
}

bool CReplayScript::saveNode(double gameTime, SReplayNode& node, Vehicle vehicle, bool firstNode) {
    Vector3 nowPos = ENTITY::GET_ENTITY_COORDS(vehicle, true);
    Vector3 nowRot = ENTITY::GET_ENTITY_ROTATION(vehicle, 0);

    node.Timestamp = gameTime - mRecordStart;
    node.Pos = nowPos;
    node.Rot = nowRot;
    node.WheelRotations = VExt::GetWheelRotations(vehicle);
    node.SuspensionCompressions = VExt::GetWheelCompressions(vehicle);

    node.SteeringAngle = VExt::GetSteeringAngle(vehicle);
    node.Throttle = VExt::GetThrottleP(vehicle);
    node.Brake = VExt::GetBrakeP(vehicle);

    node.Gear = VExt::GetGearCurr(vehicle);
    node.RPM = VExt::GetCurrentRPM(vehicle);

    if (mSettings.Record.Optional.Lights) {
        BOOL areLowBeamsOn, areHighBeamsOn;
        VEHICLE::GET_VEHICLE_LIGHTS_STATE(vehicle, &areLowBeamsOn, &areHighBeamsOn);
        node.LowBeams = areLowBeamsOn;
        node.HighBeams = areHighBeamsOn;
    }

    if (mSettings.Record.Optional.Indicators) {
        auto lightStates = VExt::GetLightStates(vehicle);
        node.IndicatorRight = (lightStates & EVehicleLightState::LightStateIndicatorRight) > 0;
        node.IndicatorLeft = (lightStates & EVehicleLightState::LightStateIndicatorLeft) > 0;
    }

    if (mSettings.Record.Optional.Siren) {
        node.Siren = VEHICLE::IS_VEHICLE_SIREN_ON(vehicle);
    }

    if (VEHICLE::IS_VEHICLE_A_CONVERTIBLE(vehicle, false)) {
        node.Roof = VEHICLE::GET_CONVERTIBLE_ROOF_STATE(vehicle);
    }

    bool saved = false;
    double lastNodeTime = 0.0;
    if (!mCurrentRun.GetNodes().empty()) {
        lastNodeTime = mCurrentRun.GetNodes().back().Timestamp;
    }
    if (node.Timestamp >= lastNodeTime + mSettings.Record.DeltaMillis || firstNode) {
        mCurrentRun.AddNode(node);
        saved = true;
    }
    return saved;
}

void CReplayScript::finishRecord(bool saved, const SReplayNode& node) {
    if (!saved) {
        mCurrentRun.AddNode(node);
    }

    bool fastestLap = false;
    bool fasterLap = false;

    if (!mActiveReplays.empty()) {
        auto fastestActiveReplay = getFastestActiveReplay();
        if (fastestActiveReplay)
            fasterLap = node.Timestamp < fastestActiveReplay->GetNodes().back().Timestamp;
    }
    else {
        fasterLap = true;
    }
    if (fasterLap) {
        fastestLap = IsFastestLap(mCurrentRun.Track, 0, node.Timestamp);
    }

    // "FullyParsed" is a lie, it just means it contains all data
    mCurrentRun.SetFullyParsed(true);

    if (mSettings.Record.AutoGhost && (mActiveReplays.empty() || fasterLap)) {
        // Just deselect when there's 1 ghost, otherwise keep adding to the mayhem!
        if (mActiveReplays.size() == 1)
            DeselectReplay(mActiveReplays[0]->Name, mActiveReplays[0]->Timestamp);
        mCurrentRun.Name = Util::FormatReplayName(
            node.Timestamp,
            mCurrentRun.Track,
            Util::GetVehicleName(mCurrentRun.VehicleModel));
        CReplayData::WriteAsync(mCurrentRun);
        GhostReplay::AddReplay(mCurrentRun);
        mCompatibleReplays.push_back(std::make_shared<CReplayData>(mCurrentRun));
        SelectReplay(mCurrentRun.Name, mCurrentRun.Timestamp);
    }
    else {
        mUnsavedRuns.push_back(mCurrentRun);
    }

    std::string lapInfo = fmt::format("Lap time: {}{}",
                                      fastestLap ? "~p~" : fasterLap ? "~g~" : "~y~",
                                      Util::FormatMillisTime(node.Timestamp));

    if (mSettings.Main.NotifyLaps) {
        UI::Notify(lapInfo, false);
    }
}

std::shared_ptr<CReplayData> CReplayScript::getFastestActiveReplay() {
    std::shared_ptr<CReplayData> fastestReplay = nullptr;
    double fastestTime = std::numeric_limits<double>::max();

    for (auto& replay : mActiveReplays) {
        if (replay->GetNodes().back().Timestamp < fastestTime) {
            fastestReplay = replay;
            fastestTime = replay->GetNodes().back().Timestamp;
        }
    }
    return fastestReplay;
}

std::shared_ptr<CReplayData> CReplayScript::getSlowestActiveReplay() {
    std::shared_ptr<CReplayData> slowestReplay = nullptr;
    double slowestTime = 0.0;

    for (auto& replay : mActiveReplays) {
        if (replay->GetNodes().back().Timestamp > slowestTime) {
            slowestReplay = replay;
            slowestTime = replay->GetNodes().back().Timestamp;
        }
    }
    return slowestReplay;
}

void CReplayScript::updateSlowestReplay() {
    auto slowestReplay = getSlowestActiveReplay();
    if (slowestReplay)
        mSlowestReplayTime = slowestReplay->GetNodes().back().Timestamp;
    else
        mSlowestReplayTime = 0.0;
}

void CReplayScript::clearPtfx() {
    for(int i : mPtfxHandles) {
        GRAPHICS::STOP_PARTICLE_FX_LOOPED(i, 0);
    }
    mPtfxHandles.clear();
}

void CReplayScript::createPtfx(const CTrackData& trackData) {
    const char* assetName = "scr_apartment_mp";
    const char* effectName = "scr_finders_package_flare";
    auto startTime = GetTickCount64();
    STREAMING::REQUEST_NAMED_PTFX_ASSET(assetName);
    while (!STREAMING::HAS_NAMED_PTFX_ASSET_LOADED(assetName)) {
        WAIT(0);
        if (GetTickCount64() > startTime + 5000) {
            WAIT(0);
            std::string msg = fmt::format("Error: Failed to load flare assets.");
            UI::Notify(msg, false);
            logger.Write(ERROR, fmt::format("[Replay] {}", msg));
            return;
        }
    }

    Vector3 emptyVec{};
    if (trackData.StartLine.A != emptyVec &&
        trackData.StartLine.B != emptyVec) {
        Vector3 A = Util::GroundZ(trackData.StartLine.A, 0.1f);
        Vector3 B = Util::GroundZ(trackData.StartLine.B, 0.1f);

        mPtfxHandles.push_back(Util::CreateParticleFxAtCoord(assetName, effectName, A, 0.0f, 1.0f, 0.0f, 1.0f));
        mPtfxHandles.push_back(Util::CreateParticleFxAtCoord(assetName, effectName, B, 0.0f, 1.0f, 0.0f, 1.0f));
    }

    if (trackData.FinishLine.A != emptyVec &&
        trackData.FinishLine.B != emptyVec) {
        Vector3 A = Util::GroundZ(trackData.FinishLine.A, 0.1f);
        Vector3 B = Util::GroundZ(trackData.FinishLine.B, 0.1f);

        mPtfxHandles.push_back(Util::CreateParticleFxAtCoord(assetName, effectName, A, 1.0f, 0.0f, 0.0f, 1.0f));
        mPtfxHandles.push_back(Util::CreateParticleFxAtCoord(assetName, effectName, B, 1.0f, 0.0f, 0.0f, 1.0f));
    }
}

void CReplayScript::clearTrackBlips() {
    mStartBlip.reset();
    mFinishBlip.reset();
}

void CReplayScript::createTrackBlips(const CTrackData& trackData) {
    Vector3 startBlipCoord = (trackData.StartLine.A + trackData.StartLine.B) * 0.5f;
    Vector3 finishBlipCoord = (trackData.FinishLine.A + trackData.FinishLine.B) * 0.5f;

    // Skip finish blip if it's within 5 meters of the start blip
    float blipDist = Distance(startBlipCoord, finishBlipCoord);
    bool showFinish = blipDist > 5.0f;

    std::string startName = showFinish ? fmt::format("{} (Start)", trackData.Name) : trackData.Name;
    eBlipColor startColor = showFinish ? eBlipColor::BlipColorBlue : eBlipColor::BlipColorWhite;

    mStartBlip = std::make_unique<CWrappedBlip>(
        startBlipCoord,
        eBlipSprite::BlipSpriteRaceFinish,
        startName,
        startColor);

    if (showFinish) {
        mFinishBlip = std::make_unique<CWrappedBlip>(
            finishBlipCoord,
            eBlipSprite::BlipSpriteRaceFinish,
            fmt::format("{} (Finish)", trackData.Name),
            eBlipColor::BlipColorWhite);
    }
}

void CReplayScript::updateSteppedTime() {
    mCurrentTime = mCurrentTime + static_cast<double>(MISC::GET_FRAME_TIME());
}

double CReplayScript::getSteppedTime() {
    return mCurrentTime * 1000.0;
}

void CReplayScript::setPlayerIntoVehicleFreeSeat(Vehicle vehicle) {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    int numReplayVehicleSeats = VEHICLE::GET_VEHICLE_MODEL_NUMBER_OF_SEATS(ENTITY::GET_ENTITY_MODEL(vehicle));

    if (numReplayVehicleSeats > 1) {
        PED::SET_PED_INTO_VEHICLE(playerPed, vehicle, -2);
    }
    else {
        // Fallback: Driver, but the inputs might conflict.
        PED::SET_PED_INTO_VEHICLE(playerPed, vehicle, -1);
    }
}

void CReplayScript::ghostCleanup(Vehicle vehicle) {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vehicle playerVehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);

    // Only deactivate passenger mode if the player is in the ghost.
    if (ENTITY::DOES_ENTITY_EXIST(playerVehicle) && playerVehicle != vehicle) {
        return;
    }

    DeactivatePassengerMode(vehicle);
}
