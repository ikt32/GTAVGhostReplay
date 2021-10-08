#pragma once
#include "ScriptSettings.hpp"
#include "ReplayData.hpp"
#include "TrackData.hpp"
#include "Image.hpp"
#include "Blip.hpp"
#include "ReplayVehicle.hpp"

#include <vector>
#include <string>

enum class EScriptMode {
    DefineTrack,
    ReplayActive,
};

enum class ERecordState {
    Idle,
    Recording,
};

class CReplayScript {
public:
    CReplayScript(
        CScriptSettings& settings,
        std::vector<std::shared_ptr<CReplayData>>& replays,
        std::vector<CTrackData>& tracks,
        std::vector<CImage>& trackImages,
        std::vector<CTrackData>& arsTracks);
    virtual ~CReplayScript();
    virtual void Tick();

    std::vector<std::shared_ptr<CReplayData>>& GetReplays() {
        return mCompatibleReplays;
    }

    std::vector<CTrackData>& GetTracks() {
        return mTracks;
    }

    std::vector<CTrackData>& GetARSTracks() {
        return mArsTracks;
    }

    std::vector<CReplayData>& GetUnsavedRuns() {
        return mUnsavedRuns;
    }

    std::vector<std::shared_ptr<CReplayData>> ActiveReplays() const {
        return mActiveReplays;
    }

    CTrackData* ActiveTrack() const {
        return mActiveTrack;
    }

    EScriptMode ScriptMode() const {
        return mScriptMode;
    }

    void SetScriptMode(EScriptMode mode) {
        mScriptMode = mode;
    }

    void StopAllReplays();

    bool IsRecording() {
        return mRecordState == ERecordState::Recording;
    }

    void StopRecording() {
        mRecordState = ERecordState::Idle;
        mCurrentRun = CReplayData("");
    }

    const std::vector<std::unique_ptr<CReplayVehicle>>& GetReplayVehicles() {
        return mReplayVehicles;
    }

    void SetTrack(const std::string& trackName);
    void SelectReplay(const std::string& replayName, unsigned long long timestamp);
    void DeselectReplay(const std::string& replayName, unsigned long long timestamp);
    void ClearSelectedReplays();
    void ClearUnsavedRuns();
    std::vector<CReplayData>::iterator EraseUnsavedRun(std::vector<CReplayData>::iterator runIt);

    std::vector<std::shared_ptr<CReplayData>> GetCompatibleReplays(const std::string& trackName);
    void AddCompatibleReplay(const CReplayData& value);

    bool IsFastestLap(const std::string& trackName, Hash vehicleModel, double timestamp);
    std::shared_ptr<CReplayData> GetFastestReplay(const std::string& trackName, Hash vehicleModel);

    bool StartLineDef(SLineDef& lineDef, const std::string& lineName);
    void DeleteTrack(const CTrackData& track);
    void DeleteReplay(const CReplayData& replay);

    std::string GetTrackImageMenuString(const std::string& trackName);

    void ActivatePassengerMode();
    void DeactivatePassengerMode();
    void DeactivatePassengerMode(Vehicle vehicle);
    bool IsPassengerModeActive() { return mPassengerModeActive; }
    void PassengerVehicleNext();
    void PassengerVehiclePrev();
    CReplayVehicle* GetPassengerVehicle();

    // Playback control

    // Gets the highest replay state of all ghosts.
    EReplayState GetReplayState() { return mGlobalReplayState; }
    double GetReplayProgress();
    double GetSlowestActiveReplay();
    void TogglePause(bool pause);
    void ScrubBackward(double step);
    void ScrubForward(double step);

    // Debugging playback
    uint64_t GetNumFrames();
    uint64_t GetFrameIndex();
    void FramePrev();
    void FrameNext();

    void TeleportToTrack(const CTrackData& trackData);

    Vehicle GetPlayerVehicle() { return mPlayerVehicle; }

    void ToggleDrivers(bool enable);

protected:
    void updateGlobalStates();
    void updateReplay();
    void updateRecord(double gameTime, bool startPassedThisTick, bool finishPassedThisTick);
    void updateTrackDefine();
    bool passedLineThisTick(SLineDef line, Vector3 oldPos, Vector3 newPos);
    void startRecord(double gameTime, Vehicle vehicle);
    bool saveNode(double gameTime, SReplayNode& node, Vehicle vehicle, bool firstNode);
    void finishRecord(bool saved, const SReplayNode& node);
    std::shared_ptr<CReplayData> getFastestActiveReplay();
    std::shared_ptr<CReplayData> getSlowestActiveReplay();
    void updateSlowestReplay();

    void clearPtfx();
    void createPtfx(const CTrackData& trackData);
    void clearTrackBlips();
    void createTrackBlips(const CTrackData& trackData);

    void updateSteppedTime();
    double getSteppedTime();

    void setPlayerIntoVehicleFreeSeat(Vehicle vehicle);
    // Called by ghost instance on cleanup when instance replay is reset, vehicle is its own vehicle.
    void ghostCleanup(Vehicle vehicle);

    double mCurrentTime;

    EReplayState mGlobalReplayState;

    const CScriptSettings& mSettings;
    std::vector<std::shared_ptr<CReplayData>>& mReplays;
    std::vector<std::shared_ptr<CReplayData>> mCompatibleReplays;
    std::vector<CTrackData>& mTracks;
    std::vector<CImage>& mTrackImages;
    std::vector<CTrackData>& mArsTracks;

    std::vector<std::shared_ptr<CReplayData>> mActiveReplays;
    CTrackData* mActiveTrack;

    std::unique_ptr<CWrappedBlip> mStartBlip;
    std::unique_ptr<CWrappedBlip> mFinishBlip;

    EScriptMode mScriptMode;
    ERecordState mRecordState;
    double mRecordStart = 0.0;

    Vector3 mLastPos;

    std::vector<std::unique_ptr<CReplayVehicle>> mReplayVehicles;
    double mReplayStartTime = 0.0;
    double mReplayCurrentTime = 0.0;
    double mSlowestReplayTime = 0.0;

    std::vector<CReplayData> mUnsavedRuns;
    CReplayData mCurrentRun;

    std::vector<int> mPtfxHandles;

    bool mPassengerModeActive = false;
    Vehicle mPassengerModePlayerVehicle = 0;
    bool mPassengerModPlayerVehicleManagedByThisScript = false;
    CReplayVehicle* mPassengerVehicle = nullptr;
    bool mPassengerFlagWindscreen = true;

    Vehicle mPlayerVehicle = 0;
};
