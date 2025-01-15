#pragma once
#include "VehicleMod.hpp"
#include "ReplayDriver.hpp"

#include <inc/types.h>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

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
    inline static std::string BinExt = ".grbin";
    static CReplayData Read(const std::filesystem::path& replayFile);
    static void WriteAsync(CReplayData& replayData);

    CReplayData(const std::filesystem::path& replayFile);
    std::string FileName() const { return mFilePath.string(); }
    void Delete() const;

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
    std::vector<SReplayNode> Nodes;
private:
    // Make sure mFileName has been set before calling this.
    void write(int storageType);
    // Only run this before asynchronously calling write().
    void generateFileName(int storageType);

    std::filesystem::path mFilePath;
};
