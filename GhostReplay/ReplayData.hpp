#pragma once
#include "VehicleMod.hpp"
#include "ReplayDriver.hpp"

#include <inc/types.h>
#include <optional>
#include <string>
#include <vector>
#include <mutex>

struct SReplayNode {
    double Timestamp;
    Vector3 Pos;
    Vector3 Rot;
    std::vector<float> WheelRotations;
    std::vector<float> SuspensionCompressions;

    float SteeringAngle;
    float Throttle;
    float Brake;

    int Gear;
    float RPM;

    std::optional<bool> LowBeams;
    std::optional<bool> HighBeams;
    std::optional<bool> IndicatorLeft;
    std::optional<bool> IndicatorRight;
    std::optional<bool> Siren;
    std::optional<int> Roof;

    bool operator<(const SReplayNode& other) const {
        return Timestamp < other.Timestamp;
    }
};

class CReplayData {
public:
    void ReadMeta(/*const std::string& replayFile*/);
    static void WriteAsync(CReplayData& replayData);
    static void WriteMetadataSync(CReplayData& replayData);

    CReplayData(std::string fileName);

    // Copy constructor - skip mutex
    CReplayData(const CReplayData& obj) {
        MarkedForDeletion = obj.MarkedForDeletion;
        Timestamp = obj.Timestamp;
        Name = obj.Name;
        Track = obj.Track;
        VehicleModel = obj.VehicleModel;
        VehicleMods = obj.VehicleMods;
        ReplayDriver = obj.ReplayDriver;

        mFileName = obj.mFileName;
        mFullyParsed = obj.mFullyParsed;
        mNodes = obj.mNodes;
    }

    // Move constructor - skip mutex
    CReplayData& operator=(const CReplayData&& obj) {
        MarkedForDeletion = obj.MarkedForDeletion;
        Timestamp = obj.Timestamp;
        Name = obj.Name;
        Track = obj.Track;
        VehicleModel = obj.VehicleModel;
        VehicleMods = obj.VehicleMods;
        ReplayDriver = obj.ReplayDriver;

        mFileName = obj.mFileName;
        mFullyParsed = obj.mFullyParsed;
        mNodes = obj.mNodes;

        return *this;
    }

    std::string FileName() const { return mFileName; }
    void Delete() const;
    void CompleteReadAsync();

    bool FullyParsed();
    void SetFullyParsed(bool newValue);

    bool MarkedForDeletion;

    unsigned long long Timestamp;
    std::string Name;
    std::string Track;

    Hash VehicleModel;
    VehicleModData VehicleMods;
    SReplayDriverData ReplayDriver;

    // Nodes.front() shall be used to figure out which CReplayDatas apply to a given starting point.
    // The menu shall be used to select the one that applies, so multiple CReplayData recordings can be
    // chosen from.

    // TODO: Node access while not completely loaded -> Crash
    // Needs a mutex, but then things are slow again.
    // Could *not* start a replay when still loading if the player triggers a start?
    std::vector<SReplayNode>& GetNodes();
    void ClearNodes();
    void AddNode(const SReplayNode& node);

private:
    // Make sure mFileName has been set before calling this.
    void write(bool pretty);
    void writeMetadata(bool pretty);

    // Only run this before asynchronously calling write().
    void generateFileName();

    void completeRead(bool fullRead);

    std::string mFileName;

    std::mutex mFullyParsedMtx;
    bool mFullyParsed;

    std::mutex mNodesMutex;
    std::vector<SReplayNode> mNodes;
};
