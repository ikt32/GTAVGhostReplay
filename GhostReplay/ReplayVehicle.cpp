#include "ReplayVehicle.hpp"
#include "Script.hpp"
#include "ReplayScriptUtils.hpp"
#include "Util/UI.hpp"
#include "Util/Logger.hpp"
#include "Util/Math.hpp"
#include "Util/Game.hpp"
#include "Util/Random.hpp"
#include "Memory/VehicleExtensions.hpp"
#include <inc/natives.h>
#include <fmt/format.h>

using VExt = VehicleExtensions;

CReplayVehicle::CReplayVehicle(const CScriptSettings& settings, std::shared_ptr<CReplayData> activeReplay,
    const std::function<void(Vehicle)>& onCleanup)
    : mSettings(settings)
    , mActiveReplay(activeReplay)
    , mReplayVehicle(0)
    , mReplayState(EReplayState::Idle)
    , mOnCleanup(onCleanup) {
    mLastNode = activeReplay->GetNodes().begin();
    createReplayVehicle(activeReplay->VehicleModel, activeReplay, activeReplay->GetNodes()[0].Pos);
    if (mSettings.Replay.EnableDrivers)
        createReplayPed();
    hideVehicle();
}

CReplayVehicle::~CReplayVehicle() {
    if (!Dll::Unloading()) {
        resetReplay();
        if (ENTITY::DOES_ENTITY_EXIST(mReplayPed)) {
            ENTITY::SET_ENTITY_AS_MISSION_ENTITY(mReplayPed, false, false);
            ENTITY::DELETE_ENTITY(&mReplayPed);
        }

        ENTITY::SET_ENTITY_AS_MISSION_ENTITY(mReplayVehicle, false, false);
        ENTITY::DELETE_ENTITY(&mReplayVehicle);
    }
}

void CReplayVehicle::UpdateCollision(bool enable) {
    if (enable) {
        ENTITY::SET_ENTITY_COLLISION(mReplayVehicle, true, true);
    }
    else {
        ENTITY::SET_ENTITY_COLLISION(mReplayVehicle, false, false);
    }
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
            if (mActiveReplay && mLastNode == mActiveReplay->GetNodes().end()) {
                logger.Write(DEBUG, "Updating currentNode to activeReplay begin");
                mLastNode = mActiveReplay->GetNodes().begin();
                startReplay();
                startedReplayThisTick = true;
            }
            showNode(mLastNode->Timestamp, mLastNode, mLastNode);
            break;
        }
        case EReplayState::Playing: {
            if (replayTime < 0.0) {
                showNode(0.0, mActiveReplay->GetNodes().begin(), mActiveReplay->GetNodes().begin());
                break;
            }

            // Find the node where the timestamp is larger than the current virtual timestamp (replayTime).
            auto nodeNext = std::upper_bound(mLastNode, mActiveReplay->GetNodes().end(), SReplayNode{ replayTime });

            // The ghost has reached the end of the recording
            if (nodeNext == mActiveReplay->GetNodes().end()) {
                mReplayState = EReplayState::Idle;
                resetReplay();
                break;
            }

            // Update the node passed, if progressed past it.
            if (mLastNode != std::prev(nodeNext)) {
                if (nodeNext != mActiveReplay->GetNodes().begin()) {
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
    if (mActiveReplay && mLastNode != mActiveReplay->GetNodes().end()) {
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
        mActiveReplay->GetNodes().begin(),
        mActiveReplay->GetNodes().end(),
        SReplayNode{ replayTime });

    // Allow ghost to end when scrubbing when not paused
    if (mReplayState == EReplayState::Playing) {
        // The ghost has reached the end of the recording
        if (nodeCurr == mActiveReplay->GetNodes().end() || std::next(nodeCurr) == mActiveReplay->GetNodes().end()) {
            mReplayState = EReplayState::Idle;
            resetReplay();
            return;
        }
    }

    if (nodeCurr != mActiveReplay->GetNodes().begin())
        mLastNode = std::prev(nodeCurr);
    else {
        mLastNode = nodeCurr;
    }
}

uint64_t CReplayVehicle::GetNumFrames() {
    if (mActiveReplay)
        return mActiveReplay->GetNodes().size();
    return 0;
}

uint64_t CReplayVehicle::GetFrameIndex() {
    if (!mActiveReplay || mLastNode == mActiveReplay->GetNodes().end())
        return 0;

    return static_cast<uint64_t>(std::distance(mActiveReplay->GetNodes().begin(), mLastNode));
}

double CReplayVehicle::FramePrev() {
    if (!mActiveReplay)
        return 0.0;

    if (mLastNode == mActiveReplay->GetNodes().begin())
        return 0.0;

    auto prevIt = std::prev(mLastNode);
    auto delta = mLastNode->Timestamp - prevIt->Timestamp;
    mLastNode = prevIt;
    return delta;
}

double CReplayVehicle::FrameNext() {
    if (!mActiveReplay || mLastNode == mActiveReplay->GetNodes().end())
        return 0.0;

    if (std::next(mLastNode) == mActiveReplay->GetNodes().end())
        return 0.0;

    auto nextIt = std::next(mLastNode);
    auto delta = nextIt->Timestamp - mLastNode->Timestamp;
    mLastNode = nextIt;
    return delta;
}

void CReplayVehicle::ToggleDriver(bool enable) {
    if (enable) {
        createReplayPed();
    }
    else {
        if (ENTITY::DOES_ENTITY_EXIST(mReplayPed)) {
            PED::REMOVE_PED_ELEGANTLY(&mReplayPed);
            mReplayPed = 0;
        }
    }
}

void CReplayVehicle::startReplay() {
    unhideVehicle();
    mLastNode = mActiveReplay->GetNodes().begin();

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
        if (std::next(nodeNext) != mActiveReplay->GetNodes().end()) {
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

    if (mSettings.Main.Debug) {
        Vector3 rotRad = {
            deg2rad(rot.x), 0,
            deg2rad(rot.y), 0,
            deg2rad(rot.z), 0,
        };
        Util::DrawModelExtents(ENTITY::GET_ENTITY_MODEL(GetVehicle()), pos, rotRad, 255, 255, 255);
    }

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
            Util::FormatMillisTime(mActiveReplay->GetNodes().back().Timestamp),
            Util::FormatMillisTime(GetReplayProgress()),
            fmt::format("Type: {}", syncType),
            fmt::format("Limit: {:.3f}", mSettings.Replay.SyncDistance),
            fmt::format("Drift: {:.3f}", dist),
        });
    }

    if (nodeCurr == mActiveReplay->GetNodes().begin() || paused ||
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
            if (nodeCurr->LowBeams != std::nullopt)
                VEHICLE::SET_VEHICLE_LIGHTS(mReplayVehicle, *nodeCurr->LowBeams ? 3 : 4);
            if (nodeCurr->HighBeams != std::nullopt)
                VEHICLE::SET_VEHICLE_FULLBEAM(mReplayVehicle, *nodeCurr->HighBeams);
    }

    if (nodeCurr->IndicatorLeft != std::nullopt) {
        VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(mReplayVehicle, EIndicators::IndicatorLeft, *nodeCurr->IndicatorLeft);
    }

    if (nodeCurr->IndicatorRight != std::nullopt) {
        VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(mReplayVehicle, EIndicators::IndicatorRight, *nodeCurr->IndicatorRight);
    }

    if (nodeCurr->Siren != std::nullopt) {
        if (*nodeCurr->Siren && !VEHICLE::IS_VEHICLE_SIREN_ON(mReplayVehicle))
            VEHICLE::SET_VEHICLE_SIREN(mReplayVehicle, true);
        
        if (!*nodeCurr->Siren && VEHICLE::IS_VEHICLE_SIREN_ON(mReplayVehicle))
            VEHICLE::SET_VEHICLE_SIREN(mReplayVehicle, false);
    }

    if (VEHICLE::IS_VEHICLE_A_CONVERTIBLE(mReplayVehicle, false)) {
        auto roofState = VEHICLE::GET_CONVERTIBLE_ROOF_STATE(mReplayVehicle);
        switch (mSettings.Replay.ForceRoof) {
            case 2: // Force Up
                if (roofState == 1 || roofState == 2)
                    VEHICLE::RAISE_CONVERTIBLE_ROOF(mReplayVehicle, true);
                break;
            case 1: // Force Down
                if (roofState == 0 || roofState == 3)
                    VEHICLE::LOWER_CONVERTIBLE_ROOF(mReplayVehicle, true);
                break;
            case 0: // Default
            default:
                if (nodeCurr->Roof != std::nullopt) {
                    if ((nodeCurr->Roof == 0 || nodeCurr->Roof == 3) &&
                        (roofState == 1 || roofState == 2)) {
                        VEHICLE::RAISE_CONVERTIBLE_ROOF(mReplayVehicle, false);
                    }
                    else if ((nodeCurr->Roof == 1 || nodeCurr->Roof == 2) &&
                        (roofState == 0 || roofState == 3)) {
                        VEHICLE::LOWER_CONVERTIBLE_ROOF(mReplayVehicle, false);
                    }
                }
        }
    }

    if (mReplayPed) {
        AUDIO::STOP_CURRENT_PLAYING_SPEECH(mReplayPed);
        AUDIO::STOP_CURRENT_PLAYING_AMBIENT_SPEECH(mReplayPed);
    }
}

void CReplayVehicle::resetReplay() {
    if (mOnCleanup)
        mOnCleanup(mReplayVehicle);

    hideVehicle();
    mLastNode = mActiveReplay->GetNodes().begin();
    deleteBlip();
}

void CReplayVehicle::createReplayVehicle(Hash model, std::shared_ptr<CReplayData> activeReplay, Vector3 pos) {
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

    mReplayVehicle = Util::CreateVehicle(model, &mActiveReplay->VehicleMods, pos);

    if (mReplayVehicle == 0)
        return;

    ENTITY::SET_ENTITY_AS_MISSION_ENTITY(mReplayVehicle, true, false);

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
    VEHICLE::SET_VEHICLE_HAS_UNBREAKABLE_LIGHTS(mReplayVehicle, false);
    VEHICLE::_SET_DISABLE_VEHICLE_WINDOW_COLLISIONS(mReplayVehicle, true);
}

void CReplayVehicle::createReplayPed() {
    if (mActiveReplay->ReplayDriver.Model == 0 &&
        mSettings.Replay.DriverModels.empty()) {
        std::string msg = "Error: No ped models available, skipping ped creation.";
        UI::Notify(msg, false);
        logger.Write(ERROR, fmt::format("[Replay] {}", msg));
        mReplayPed = 0;
        return;
    }

    random_selector selector{};
    Hash randomModel = MISC::GET_HASH_KEY(selector(mSettings.Replay.DriverModels).c_str());

    Hash model = 0;
    int pedType = 0;
    bool validModel = false;
    bool fallback = false;

    if (mActiveReplay->ReplayDriver.Model &&
        STREAMING::IS_MODEL_IN_CDIMAGE(mActiveReplay->ReplayDriver.Model) &&
        STREAMING::IS_MODEL_A_PED(mActiveReplay->ReplayDriver.Model)) {

        model = mActiveReplay->ReplayDriver.Model;
        pedType = mActiveReplay->ReplayDriver.Type;
        validModel = true;
    }
    
    if (!validModel &&
        STREAMING::IS_MODEL_IN_CDIMAGE(randomModel) &&
        STREAMING::IS_MODEL_A_PED(randomModel)) {

        model = randomModel;
        pedType = ePedType::PED_TYPE_CIVMALE;
        validModel = true;
        fallback = true;
    }
    
    if (!validModel) {
        std::string msg =
            fmt::format("Error: Couldn't find ped model 0x{:08X} or 0x{:08X}. Skipping ped creation.",
                mActiveReplay->ReplayDriver.Model, randomModel);
        UI::Notify(msg, false);
        logger.Write(ERROR, fmt::format("[Replay] {}", msg));
        mReplayPed = 0;
        return;
    }
    STREAMING::REQUEST_MODEL(model);
    auto startTime = GetTickCount64();
    
    while (!STREAMING::HAS_MODEL_LOADED(model)) {
        WAIT(0);
        if (GetTickCount64() > startTime + 5000) {
            // Couldn't load model
            WAIT(0);
            STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
            std::string msg = fmt::format("Error: Failed to load ped model 0x{:08X}.", model);
            UI::Notify(msg, false);
            logger.Write(ERROR, fmt::format("[Replay] {}", msg));
            return;
        }
    }
    
    mReplayPed = PED::CREATE_PED_INSIDE_VEHICLE(mReplayVehicle, pedType, model, -1, false, false);

    if (mReplayPed == 0)
        return;

    if (!fallback)
        SReplayDriverData::ApplyTo(mReplayPed, mActiveReplay->ReplayDriver);

    PED::SET_PED_CAN_BE_KNOCKED_OFF_VEHICLE(mReplayPed, eKnockOffVehicle::KNOCKOFFVEHICLE_NEVER);
    PED::SET_PED_CONFIG_FLAG(mReplayPed, 32, false);
    ENTITY::SET_ENTITY_INVINCIBLE(mReplayPed, true);
    ENTITY::SET_ENTITY_AS_MISSION_ENTITY(mReplayPed, true, false);
}

void CReplayVehicle::createBlip() {
    eBlipSprite blipSprite = eBlipSprite::BlipSpriteStandard;

    if (VEHICLE::IS_THIS_MODEL_A_PLANE(mActiveReplay->VehicleModel)) {
        blipSprite = eBlipSprite::BlipSpriteJet;
    }
    else if (VEHICLE::IS_THIS_MODEL_A_HELI(mActiveReplay->VehicleModel)) {
        blipSprite = eBlipSprite::BlipSpriteHelicopterAnimated;
    }
    else if (VEHICLE::GET_VEHICLE_CLASS(mReplayVehicle) == eVehicleClass::VehicleClassEmergency) {
        blipSprite = eBlipSprite::BlipSpritePoliceOfficer2;
    }

    mReplayVehicleBlip = std::make_unique<CWrappedBlip>(
        mReplayVehicle,
        blipSprite,
        fmt::format("Replay - {} ({})",
            Util::GetVehicleName(mActiveReplay->VehicleModel),
            Util::FormatMillisTime(mActiveReplay->GetNodes().back().Timestamp)),
        eBlipColor::BlipColorWhite, true);

    if (blipSprite == eBlipSprite::BlipSpritePoliceOfficer2) {
        mReplayVehicleBlip->SetScale(0.5f);
    }
}

void CReplayVehicle::deleteBlip() {
    mReplayVehicleBlip.reset();
}

void CReplayVehicle::hideVehicle() {
    if (!ENTITY::DOES_ENTITY_EXIST(mReplayVehicle))
        return;

    auto pos = ENTITY::GET_ENTITY_COORDS(mReplayVehicle, !ENTITY::IS_ENTITY_DEAD(mReplayVehicle, false));

    ENTITY::SET_ENTITY_VISIBLE(mReplayVehicle, false, false);
    ENTITY::SET_ENTITY_ALPHA(mReplayVehicle, 0, false);
    UpdateCollision(false);
    VEHICLE::SET_VEHICLE_ENGINE_ON(mReplayVehicle, false, true, true);

    // Freeze invisible vehicle in air, so it won't get damaged (falling in water, etc).
    ENTITY::FREEZE_ENTITY_POSITION(mReplayVehicle, true);
    ENTITY::SET_ENTITY_COORDS_NO_OFFSET(mReplayVehicle, pos.x, pos.y, pos.z + 100.0f, false, false, false);

    VExt::SetCurrentRPM(mReplayVehicle, 0.0f);

    VEHICLE::SET_VEHICLE_SIREN(mReplayVehicle, false);

    if (VEHICLE::IS_THIS_MODEL_A_HELI(ENTITY::GET_ENTITY_MODEL(mReplayVehicle))) {
        VEHICLE::SET_HELI_BLADES_SPEED(mReplayVehicle, 0.0f);
    }
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

    if (VEHICLE::IS_THIS_MODEL_A_HELI(ENTITY::GET_ENTITY_MODEL(mReplayVehicle))) {
        VEHICLE::SET_HELI_BLADES_SPEED(mReplayVehicle, 1.0f);
    }

    // Force roof state to initial state
    if (VEHICLE::IS_VEHICLE_A_CONVERTIBLE(mReplayVehicle, false)) {
        switch (mSettings.Replay.ForceRoof) {
            case 2: // Force Up
                VEHICLE::RAISE_CONVERTIBLE_ROOF(mReplayVehicle, true);
                break;
            case 1: // Force Down
                VEHICLE::LOWER_CONVERTIBLE_ROOF(mReplayVehicle, true);
                break;
            case 0: // Default
            default:
                if (mActiveReplay->GetNodes()[0].Roof != std::nullopt) {
                    auto nodeRoofState = mActiveReplay->GetNodes()[0].Roof.value();
                    if (nodeRoofState == 0 || nodeRoofState == 3) {
                        VEHICLE::RAISE_CONVERTIBLE_ROOF(mReplayVehicle, true);
                    }
                    else if (nodeRoofState == 1 || nodeRoofState == 2) {
                        VEHICLE::LOWER_CONVERTIBLE_ROOF(mReplayVehicle, true);
                    }
                }
        }
    }
}
