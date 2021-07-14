#include "ReplayVehicle.hpp"
#include "Script.hpp"
#include "ReplayScriptUtils.hpp"
#include "Util/UI.hpp"
#include "Util/Logger.hpp"
#include "Util/Math.hpp"
#include "Memory/VehicleExtensions.hpp"
#include <inc/natives.h>
#include <fmt/format.h>

using VExt = VehicleExtensions;

CReplayVehicle::CReplayVehicle(const CScriptSettings& settings, CReplayData* activeReplay,
    const std::function<void(Vehicle)>& onCleanup)
    : mSettings(settings)
    , mActiveReplay(activeReplay)
    , mReplayVehicle(0)
    , mReplayState(EReplayState::Idle)
    , mOnCleanup(onCleanup) {
    mLastNode = activeReplay->Nodes.begin();
    createReplayVehicle(activeReplay->VehicleModel, activeReplay, activeReplay->Nodes[0].Pos);
    hideVehicle();
}

CReplayVehicle::~CReplayVehicle() {
    if (!Dll::Unloading())
        resetReplay();
}

void CReplayVehicle::UpdateCollision(bool enable) {
    ENTITY::SET_ENTITY_COLLISION(mReplayVehicle, enable, false);
}

bool CReplayVehicle::HasCollision() {
    return !ENTITY::GET_ENTITY_COLLISION_DISABLED(mReplayVehicle);
}

bool CReplayVehicle::UpdatePlayback(double replayTime, bool startPassedThisTick, bool finishPassedThisTick) {
    bool startedReplayThisTick = false;
    if (!mActiveReplay) {
        mReplayState = EReplayState::Idle;
        resetReplay();
        return startedReplayThisTick;
    }

    switch (mReplayState) {
        case EReplayState::Idle: {
            if (startPassedThisTick) {
                mReplayState = EReplayState::Playing;
                startReplay();
                startedReplayThisTick = true;
            }
            break;
        }
        case EReplayState::Paused: {
            if (mActiveReplay && mLastNode == mActiveReplay->Nodes.end()) {
                logger.Write(DEBUG, "Updating currentNode to activeReplay begin");
                mLastNode = mActiveReplay->Nodes.begin();
                startReplay();
                startedReplayThisTick = true;
            }
            showNode(mLastNode->Timestamp, mLastNode, mLastNode);
            break;
        }
        case EReplayState::Playing: {
            if (replayTime < 0.0) {
                showNode(0.0, mActiveReplay->Nodes.begin(), mActiveReplay->Nodes.begin());
                break;
            }

            // Find the node where the timestamp is larger than the current virtual timestamp (replayTime).
            auto nodeNext = std::upper_bound(mLastNode, mActiveReplay->Nodes.end(), SReplayNode{ replayTime });

            // The ghost has reached the end of the recording
            if (nodeNext == mActiveReplay->Nodes.end()) {
                mReplayState = EReplayState::Idle;
                resetReplay();
                break;
            }

            // Update the node passed, if progressed past it.
            if (mLastNode != std::prev(nodeNext)) {
                if (nodeNext != mActiveReplay->Nodes.begin()) {
                    mLastNode = std::prev(nodeNext);
                }
                else {
                    mLastNode = nodeNext;
                    nodeNext = std::next(nodeNext);
                }
            }

            // Show the node, with <nodePrev> <now> <nodeNext>.
            showNode(replayTime, mLastNode, nodeNext);
            break;
        }
    }

    // Always abort a paused or slower ghost lap when the player has finished.
    // And restart it again, if the start was passed in the same tick.
    if (mReplayState != EReplayState::Idle) {
        if (finishPassedThisTick) {
            mReplayState = EReplayState::Idle;
            resetReplay();
            if (startPassedThisTick) {
                mReplayState = EReplayState::Playing;
                startReplay();
                startedReplayThisTick = true;
            }
        }
    }

    return startedReplayThisTick;
}

double CReplayVehicle::GetReplayProgress() {
    if (mActiveReplay && mLastNode != mActiveReplay->Nodes.end()) {
        return mLastNode->Timestamp;
    }
    return 0;
}

void CReplayVehicle::TogglePause(bool pause) {
    if (mReplayState == EReplayState::Idle) {
        startReplay();
    }
    if (pause) {
        mReplayState = EReplayState::Paused;
    }
    else {
        mReplayState = EReplayState::Playing;
    }
}

void CReplayVehicle::SetReplayTime(double replayTime) {
    // Find node with smaller timestamp than our replayTime
    auto nodeCurr = std::lower_bound(
        mActiveReplay->Nodes.begin(),
        mActiveReplay->Nodes.end(),
        SReplayNode{ replayTime });

    // Allow ghost to end when scrubbing when not paused
    if (mReplayState == EReplayState::Playing) {
        // The ghost has reached the end of the recording
        if (nodeCurr == mActiveReplay->Nodes.end() || std::next(nodeCurr) == mActiveReplay->Nodes.end()) {
            mReplayState = EReplayState::Idle;
            resetReplay();
            return;
        }
    }

    if (nodeCurr != mActiveReplay->Nodes.begin())
        mLastNode = std::prev(nodeCurr);
    else {
        mLastNode = nodeCurr;
    }
}

uint64_t CReplayVehicle::GetNumFrames() {
    if (mActiveReplay)
        return mActiveReplay->Nodes.size();
    return 0;
}

uint64_t CReplayVehicle::GetFrameIndex() {
    if (!mActiveReplay || mLastNode == mActiveReplay->Nodes.end())
        return 0;

    return static_cast<uint64_t>(std::distance(mActiveReplay->Nodes.begin(), mLastNode));
}

double CReplayVehicle::FramePrev() {
    if (!mActiveReplay)
        return 0.0;

    if (mLastNode == mActiveReplay->Nodes.begin())
        return 0.0;

    auto prevIt = std::prev(mLastNode);
    auto delta = mLastNode->Timestamp - prevIt->Timestamp;
    mLastNode = prevIt;
    return delta;
}

double CReplayVehicle::FrameNext() {
    if (!mActiveReplay || mLastNode == mActiveReplay->Nodes.end())
        return 0.0;

    if (std::next(mLastNode) == mActiveReplay->Nodes.end())
        return 0.0;

    auto nextIt = std::next(mLastNode);
    auto delta = nextIt->Timestamp - mLastNode->Timestamp;
    mLastNode = nextIt;
    return delta;
}

void CReplayVehicle::startReplay() {
    unhideVehicle();
    mLastNode = mActiveReplay->Nodes.begin();

    if (mSettings.Main.GhostBlips)
        createBlip();
}

void CReplayVehicle::showNode(
    double replayTime,
    std::vector<SReplayNode>::iterator nodeCurr,
    std::vector<SReplayNode>::iterator nodeNext) {
    // In paused state, nodeCurr == nodeNext, so fake-progress nodeNext.
    // Make sure the frame control doesn't go further than std::next(nodeCurr) == end();
    bool paused = nodeCurr == nodeNext;
    if (paused) {
        if (std::next(nodeNext) != mActiveReplay->Nodes.end()) {
            nodeNext = std::next(nodeNext);
        }
        else {
            nodeCurr = std::prev(nodeCurr);
        }
    }

    float progress = static_cast<float>((replayTime - nodeCurr->Timestamp) /
        (nodeNext->Timestamp - nodeCurr->Timestamp));
    double deltaT = ((nodeNext->Timestamp - nodeCurr->Timestamp) * 0.001); // Seconds
    Vector3 pos = lerp(nodeCurr->Pos, nodeNext->Pos, progress);
    Vector3 rot = lerp(nodeCurr->Rot, nodeNext->Rot, progress, -180.0f, 180.0f);
    mLastPos = pos;
    mLastRot = rot;
    auto posEntity = ENTITY::GET_ENTITY_COORDS(mReplayVehicle, !ENTITY::IS_ENTITY_DEAD(mReplayVehicle, false));

    float dist = Distance(pos, posEntity);

    if (mSettings.Main.Debug) {
        UI::DrawSphere(pos, 0.125f, 0, 255, 0, 128);
        UI::DrawSphere(posEntity, 0.125f, 0, 0, 255, 128);
        UI::DrawLine(pos, posEntity, 255, 255, 255, 255);
        std::string syncType;
        switch (mSettings.Replay.SyncType) {
            case ESyncType::Distance: syncType = "Distance"; break;
            case ESyncType::Constant: syncType = "Constant"; break;
            default: syncType = fmt::format("Invalid: {}", mSettings.Replay.SyncType);
        }
        Vector3 vehUp = posEntity;
        Vector3 dimMin, dimMax;
        MISC::GET_MODEL_DIMENSIONS(ENTITY::GET_ENTITY_MODEL(mReplayVehicle), &dimMin, &dimMax);
        vehUp.z += dimMax.z + 1.0f;
        UI::ShowText3D(vehUp, 10.0f, {
            Util::FormatMillisTime(mActiveReplay->Nodes.back().Timestamp),
            Util::FormatMillisTime(GetReplayProgress()),
            fmt::format("Type: {}", syncType),
            fmt::format("Limit: {:.3f}", mSettings.Replay.SyncDistance),
            fmt::format("Drift: {:.3f}", dist),
        });
    }

    if (nodeCurr == mActiveReplay->Nodes.begin() || paused ||
        mSettings.Replay.SyncType == ESyncType::Distance && dist > mSettings.Replay.SyncDistance ||
        mSettings.Replay.SyncType == ESyncType::Constant) {
        ENTITY::SET_ENTITY_COORDS_NO_OFFSET(mReplayVehicle, pos.x, pos.y, pos.z, false, false, false);
    }
    ENTITY::SET_ENTITY_ROTATION(mReplayVehicle, rot.x, rot.y, rot.z, 0, false);

    // Prevent the third person camera from rotating backwards
    if (!(paused && mSettings.Replay.ZeroVelocityOnPause)) {
        Vector3 vel = (nodeNext->Pos - nodeCurr->Pos) * static_cast<float>(1.0 / deltaT);

        if (mSettings.Replay.SyncType != ESyncType::Constant) {
            Vector3 dVel = (pos - posEntity) * mSettings.Replay.SyncCompensation;
            vel = vel + dVel;
        }

        ENTITY::SET_ENTITY_VELOCITY(mReplayVehicle, vel.x, vel.y, vel.z);
    }

    // Base wheel rotation (for dashboard speed) on forward speed,
    // as it's not possible to accurately deduce speed from wheel rotation alone.
    Vector3 rotInRad = nodeCurr->Rot;
    rotInRad.x = deg2rad(rotInRad.x);
    rotInRad.y = deg2rad(rotInRad.y);
    rotInRad.z = deg2rad(rotInRad.z);
    Vector3 offNext = GetRelativeOffsetGivenWorldCoords(nodeCurr->Pos, nodeNext->Pos, rotInRad);
    Vector3 relVel = offNext * static_cast<float>(1.0 / deltaT);

    auto wheelDims = VExt::GetWheelDimensions(mReplayVehicle);
    if (VExt::GetNumWheels(mReplayVehicle) == nodeCurr->WheelRotations.size()) {
        for (uint8_t idx = 0; idx < VExt::GetNumWheels(mReplayVehicle); ++idx) {
            float wheelRot = lerp(nodeCurr->WheelRotations[idx], nodeNext->WheelRotations[idx], progress,
                -static_cast<float>(M_PI), static_cast<float>(M_PI));
            VExt::SetWheelRotation(mReplayVehicle, idx, wheelRot);

            float wheelRotVel = -relVel.y / wheelDims[idx].TyreRadius;
            VExt::SetWheelRotationSpeed(mReplayVehicle, idx, wheelRotVel);
        }
    }

    VExt::SetSteeringAngle(mReplayVehicle, lerp(nodeCurr->SteeringAngle, nodeNext->SteeringAngle, progress));
    VExt::SetThrottle(mReplayVehicle, lerp(nodeCurr->Throttle, nodeNext->Throttle, progress));
    VExt::SetThrottleP(mReplayVehicle, lerp(nodeCurr->Throttle, nodeNext->Throttle, progress));
    VExt::SetBrakeP(mReplayVehicle, lerp(nodeCurr->Brake, nodeNext->Brake, progress));

    if (nodeCurr->Gear >= 0 && nodeCurr->RPM > 0.0f) {
        VExt::SetGearCurr(mReplayVehicle, static_cast<uint16_t>(nodeCurr->Gear));
        VExt::SetGearNext(mReplayVehicle, static_cast<uint16_t>(nodeCurr->Gear));
        VExt::SetCurrentRPM(mReplayVehicle, lerp(nodeCurr->RPM, nodeNext->RPM, progress));
    }

    if (VExt::GetNumWheels(mReplayVehicle) == nodeCurr->SuspensionCompressions.size()) {
        for (uint8_t idx = 0; idx < VExt::GetNumWheels(mReplayVehicle); ++idx) {
            float susp = lerp(nodeCurr->SuspensionCompressions[idx], nodeNext->SuspensionCompressions[idx], progress);
            VExt::SetWheelCompression(mReplayVehicle, idx, susp);
        }
    }

    VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(mReplayVehicle, nodeCurr->Brake > 0.1f);

    switch (mSettings.Replay.ForceLights) {
        case 2: // Force On
            VEHICLE::SET_VEHICLE_LIGHTS(mReplayVehicle, 3);
            VEHICLE::SET_VEHICLE_FULLBEAM(mReplayVehicle, false);
            break;
        case 1: // Force Off
            VEHICLE::SET_VEHICLE_LIGHTS(mReplayVehicle, 4);
            VEHICLE::SET_VEHICLE_FULLBEAM(mReplayVehicle, false);
            break;
        case 0: // Default
        default:
            VEHICLE::SET_VEHICLE_LIGHTS(mReplayVehicle, nodeCurr->LowBeams ? 3 : 4);
            VEHICLE::SET_VEHICLE_FULLBEAM(mReplayVehicle, nodeCurr->HighBeams);
    }
}

void CReplayVehicle::resetReplay() {
    if (mOnCleanup)
        mOnCleanup(mReplayVehicle);

    hideVehicle();
    mLastNode = mActiveReplay->Nodes.begin();
    deleteBlip();
}

void CReplayVehicle::createReplayVehicle(Hash model, CReplayData* activeReplay, Vector3 pos) {
    if (model == 0 || activeReplay == nullptr) {
        return;
    }

    const char* fallbackModelName = mSettings.Replay.FallbackModel.c_str();

    if (mSettings.Replay.ForceFallbackModel)
        model = MISC::GET_HASH_KEY(fallbackModelName);

    if (!(STREAMING::IS_MODEL_IN_CDIMAGE(model) && STREAMING::IS_MODEL_A_VEHICLE(model))) {
        // Vehicle doesn't exist
        std::string msg =
            fmt::format("Error: Couldn't find model 0x{:08X}. Falling back to ({}).", model, fallbackModelName);
        UI::Notify(msg, false);
        logger.Write(ERROR, fmt::format("[Replay] {}", msg));

        model = MISC::GET_HASH_KEY(fallbackModelName);
        if (!(STREAMING::IS_MODEL_IN_CDIMAGE(model) && STREAMING::IS_MODEL_A_VEHICLE(model))) {
            msg = "Error: Failed to find fallback model.";
            UI::Notify(msg, false);
            logger.Write(ERROR, fmt::format("[Replay] {}", msg));
            return;
        }
    }
    STREAMING::REQUEST_MODEL(model);
    auto startTime = GetTickCount64();

    while (!STREAMING::HAS_MODEL_LOADED(model)) {
        WAIT(0);
        if (GetTickCount64() > startTime + 5000) {
            // Couldn't load model
            WAIT(0);
            STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
            std::string msg = fmt::format("Error: Failed to load model 0x{:08X}.", model);
            UI::Notify(msg, false);
            logger.Write(ERROR, fmt::format("[Replay] {}", msg));
            return;
        }
    }

    mReplayVehicle = VEHICLE::CREATE_VEHICLE(model,
        pos.x, pos.y, pos.z, 0, false, true, false);

    ENTITY::SET_ENTITY_VISIBLE(mReplayVehicle, false, false);
    ENTITY::SET_ENTITY_ALPHA(mReplayVehicle, 0, false);
    UpdateCollision(false);

    if (VEHICLE::IS_THIS_MODEL_A_PLANE(model) ||
        VEHICLE::IS_THIS_MODEL_A_HELI(model)) {
        VEHICLE::CONTROL_LANDING_GEAR(mReplayVehicle, 3);
    }

    ENTITY::SET_ENTITY_INVINCIBLE(mReplayVehicle, true);
    ENTITY::SET_ENTITY_CAN_BE_DAMAGED(mReplayVehicle, false);
    VEHICLE::SET_VEHICLE_CAN_BE_VISIBLY_DAMAGED(mReplayVehicle, false);
    VEHICLE::_SET_VEHICLE_LIGHTS_CAN_BE_VISIBLY_DAMAGED(mReplayVehicle, false);

    VehicleModData modData = mActiveReplay->VehicleMods;
    VehicleModData::ApplyTo(mReplayVehicle, modData);
}

void CReplayVehicle::createBlip() {
    mReplayVehicleBlip = std::make_unique<CWrappedBlip>(
        mReplayVehicle,
        eBlipSprite::BlipSpriteStandard,
        mActiveReplay->Name,
        eBlipColor::BlipColorWhite, true);
}

void CReplayVehicle::deleteBlip() {
    mReplayVehicleBlip.reset();
}

void CReplayVehicle::hideVehicle() {
    auto pos = ENTITY::GET_ENTITY_COORDS(mReplayVehicle, !ENTITY::IS_ENTITY_DEAD(mReplayVehicle, false));

    ENTITY::SET_ENTITY_VISIBLE(mReplayVehicle, false, false);
    ENTITY::SET_ENTITY_ALPHA(mReplayVehicle, 0, false);
    UpdateCollision(false);
    VEHICLE::SET_VEHICLE_ENGINE_ON(mReplayVehicle, false, true, true);

    // Freeze invisible vehicle in air, so it won't get damaged (falling in water, etc).
    ENTITY::FREEZE_ENTITY_POSITION(mReplayVehicle, true);
    ENTITY::SET_ENTITY_COORDS_NO_OFFSET(mReplayVehicle, pos.x, pos.y, pos.z + 100.0f, false, false, false);

    VExt::SetCurrentRPM(mReplayVehicle, 0.0f);
}

void CReplayVehicle::unhideVehicle() {
    ENTITY::SET_ENTITY_VISIBLE(mReplayVehicle, true, true);
    ENTITY::SET_ENTITY_ALPHA(mReplayVehicle, map(mSettings.Replay.VehicleAlpha, 0, 100, 0, 255), false);
    if (mSettings.Replay.VehicleAlpha == 100) {
        ENTITY::RESET_ENTITY_ALPHA(mReplayVehicle);
    }
    UpdateCollision(mSettings.Replay.SyncType != ESyncType::Constant);
    VEHICLE::SET_VEHICLE_ENGINE_ON(mReplayVehicle, true, true, false);
    ENTITY::FREEZE_ENTITY_POSITION(mReplayVehicle, false);
}
