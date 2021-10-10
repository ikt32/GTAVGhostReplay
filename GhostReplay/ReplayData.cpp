#include "ReplayData.hpp"
#include "Constants.hpp"
#include "Util/Paths.hpp"
#include "Util/Logger.hpp"
#include "ReplayScriptUtils.hpp"
#include "Script.hpp"

#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <utility>

void to_json(nlohmann::ordered_json& j, const Vector3& vector3) {
    j = nlohmann::ordered_json{
        { "X", vector3.x },
        { "Y", vector3.y },
        { "Z", vector3.z },
    };
}

void from_json(const nlohmann::ordered_json& j, Vector3& vector3) {
    j.at("X").get_to(vector3.x);
    j.at("Y").get_to(vector3.y);
    j.at("Z").get_to(vector3.z);
}

void CReplayData::ReadMeta() {
    CReplayData& replayData = *this;

    std::string metaFileName = mFileName.substr(0, mFileName.find_last_of('.')) + ".meta";
    nlohmann::ordered_json metaJson;
    std::ifstream metaFileStream(metaFileName.c_str());

    if (!metaFileStream.is_open()) {
        logger.Write(DEBUG, "[Meta] %s missing, generating .meta file", metaFileName.c_str());
        completeRead(true);
        if (FullyParsed())
            WriteMetadataSync(*this);
        return;
    }

    try {
        metaFileStream >> metaJson;

        replayData.Timestamp = metaJson.value("Timestamp", 0ull);
        replayData.Name = metaJson["Name"];
        replayData.Track = metaJson["Track"];
        replayData.VehicleModel = metaJson["VehicleModel"];

        std::scoped_lock lock(mNodesMutex);
        for (auto& jsonNode : metaJson["IdNodes"]) {
            SReplayNode node{};
            node.Timestamp = jsonNode["T"];
            node.Pos = jsonNode["Pos"];
            replayData.mNodes.push_back(node);
        }
    }
    catch (std::exception& ex) {
        logger.Write(ERROR, "[Meta] Failed to open %s, exception: %s", metaFileName.c_str(), ex.what());
    }
}

CReplayData::CReplayData(std::string fileName)
    : MarkedForDeletion(false)
    , Timestamp(0)
    , VehicleModel(0)
    , mFileName(std::move(fileName))
    , mFullyParsed(false) {}

std::vector<SReplayNode>& CReplayData::GetNodes() {
    std::scoped_lock lock(mNodesMutex);
    return mNodes;
}

void CReplayData::ClearNodes() {
    std::scoped_lock lock(mNodesMutex);
    mNodes.clear();
}

void CReplayData::AddNode(const SReplayNode& node) {
    std::scoped_lock lock(mNodesMutex);
    mNodes.push_back(node);
}

void CReplayData::write(bool pretty) {
    nlohmann::ordered_json replayJson;

    replayJson["Timestamp"] = Timestamp;
    replayJson["Name"] = Name;
    replayJson["Track"] = Track;
    replayJson["VehicleModel"] = VehicleModel;
    replayJson["Mods"] = VehicleMods;
    replayJson["Driver"] = ReplayDriver;

    // Nodes during write access is (probably) safe without mutex.
    for (auto& Node : mNodes) {
        nlohmann::ordered_json node = {
            { "T", Node.Timestamp },
            { "Pos", Node.Pos },
            { "Rot", Node.Rot },
            { "WheelRotations", Node.WheelRotations },
            { "SuspensionCompressions", Node.SuspensionCompressions },
            { "Steering", Node.SteeringAngle },
            { "Throttle", Node.Throttle },
            { "Brake", Node.Brake },
            { "Gear", Node.Gear },
            { "RPM", Node.RPM },
        };

        if (Node.LowBeams != std::nullopt)
            node.push_back({ "LowBeams", *Node.LowBeams });
        if (Node.HighBeams != std::nullopt)
            node.push_back({ "HighBeams", *Node.HighBeams });
        if (Node.IndicatorLeft != std::nullopt)
            node.push_back({ "IndicatorLeft", *Node.IndicatorLeft });
        if (Node.IndicatorRight != std::nullopt)
            node.push_back({ "IndicatorRight", *Node.IndicatorRight });
        if (Node.Siren != std::nullopt)
            node.push_back({ "Siren", *Node.Siren });
        if (Node.Roof != std::nullopt)
            node.push_back({ "Roof", *Node.Roof });

        replayJson["Nodes"].push_back(node);
    }

    std::ofstream replayFile(mFileName);

    if (pretty)
        replayFile << std::setw(2) << replayJson << std::endl;
    else
        replayFile << replayJson.dump();

    logger.Write(INFO, "[Replay] Written %s", mFileName.c_str());
}

void CReplayData::writeMetadata(bool pretty) {
    const std::string replaysPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Replays";
    std::string metaFileName = fmt::format("{}\\{}.meta", replaysPath, Util::StripString(Name));

    nlohmann::ordered_json metaJson;

    metaJson["Timestamp"] = Timestamp;
    metaJson["Name"] = Name;
    metaJson["Track"] = Track;
    metaJson["VehicleModel"] = VehicleModel;

    nlohmann::ordered_json nodeFirst = {
        { "T", mNodes.begin()->Timestamp},
        { "Pos", mNodes.begin()->Pos },
    };
    metaJson["IdNodes"].push_back(nodeFirst);

    nlohmann::ordered_json nodeLast = {
        { "T", mNodes.back().Timestamp},
        { "Pos", mNodes.back().Pos },
    };
    metaJson["IdNodes"].push_back(nodeLast);

    std::ofstream metaFile(metaFileName);

    if (pretty)
        metaFile << std::setw(2) << metaJson << std::endl;
    else
        metaFile << metaJson.dump();

    logger.Write(INFO, "[Meta] Written %s", metaFileName.c_str());
}

void CReplayData::generateFileName() {
    const std::string replaysPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Replays";

    std::string cleanName = Util::StripString(Name);
    unsigned count = 0;
    std::string suffix;

    while (std::filesystem::exists(fmt::format("{}\\{}{}.json", replaysPath, cleanName, suffix))) {
        if (suffix.empty()) {
            suffix = "_0";
        }
        else {
            suffix = fmt::format("_{}", ++count);
        }
    }

    mFileName = fmt::format("{}\\{}{}.json", replaysPath, cleanName, suffix);
}

void CReplayData::completeRead(bool fullRead) {
    CReplayData& replayData(*this);

    nlohmann::ordered_json replayJson;
    std::ifstream replayFileStream(mFileName.c_str());
    if (!replayFileStream.is_open()) {
        logger.Write(ERROR, "[Replay] Failed to open %s", mFileName.c_str());
        return;
    }

    try {
        replayFileStream >> replayJson;

        if (fullRead) {
            replayData.Timestamp = replayJson.value("Timestamp", 0ull);
            replayData.Name = replayJson["Name"];
            replayData.Track = replayJson["Track"];
            replayData.VehicleModel = replayJson["VehicleModel"];
        }

        replayData.VehicleMods = replayJson.value("Mods", VehicleModData());
        replayData.ReplayDriver = replayJson.value("Driver", SReplayDriverData());

        std::scoped_lock lock(mNodesMutex);
        replayData.mNodes.clear();

        for (auto& jsonNode : replayJson["Nodes"]) {
            SReplayNode node{};
            node.Timestamp = jsonNode["T"];
            if (jsonNode.find("PX") == jsonNode.end()) {
                node.Pos = jsonNode["Pos"];
                node.Rot = jsonNode["Rot"];
            }
            else {
                node.Pos.x = jsonNode["PX"];
                node.Pos.y = jsonNode["PY"];
                node.Pos.z = jsonNode["PZ"];
                node.Rot.x = jsonNode["RX"];
                node.Rot.y = jsonNode["RY"];
                node.Rot.z = jsonNode["RZ"];
            }
            node.WheelRotations = jsonNode.value("WheelRotations", std::vector<float>());
            node.SuspensionCompressions = jsonNode.value("SuspensionCompressions", std::vector<float>());
            node.SteeringAngle = jsonNode.value("Steering", 0.0f);
            node.Throttle = jsonNode.value("Throttle", 0.0f);
            node.Brake = jsonNode.value("Brake", 0.0f);
            node.Gear = jsonNode.value("Gear", -1);
            node.RPM = jsonNode.value("RPM", -1.0f);

            if (jsonNode.contains("LowBeams"))
                node.LowBeams = jsonNode.at("LowBeams").get<bool>();

            if (jsonNode.contains("HighBeams"))
                node.HighBeams = jsonNode.at("HighBeams").get<bool>();

            if (jsonNode.contains("IndicatorLeft"))
                node.IndicatorLeft = jsonNode.at("IndicatorLeft").get<bool>();

            if (jsonNode.contains("IndicatorRight"))
                node.IndicatorRight = jsonNode.at("IndicatorRight").get<bool>();

            if (jsonNode.contains("Siren"))
                node.Siren = jsonNode.at("Siren").get<bool>();

            if (jsonNode.contains("Roof"))
                node.Roof = jsonNode.at("Roof").get<int>();

            replayData.mNodes.push_back(node);
        }
        lock.~scoped_lock();

        replayData.SetFullyParsed(true);
        logger.Write(DEBUG, "[Replay] Parsed %s", mFileName.c_str());
    }
    catch (std::exception& ex) {
        logger.Write(ERROR, "[Replay] Failed to open %s, exception: %s", mFileName.c_str(), ex.what());
    }
}

void CReplayData::WriteAsync(CReplayData& replayData) {
    replayData.generateFileName();
    bool pretty = !GhostReplay::GetSettings().Record.ReduceFileSize;
    std::thread([replayData, pretty]() {
        CReplayData myCopy = replayData;
        myCopy.write(pretty);
        myCopy.writeMetadata(pretty);
    }).detach();
}

void CReplayData::WriteMetadataSync(CReplayData& replayData) {
    bool pretty = !GhostReplay::GetSettings().Record.ReduceFileSize;
    replayData.writeMetadata(pretty);
}

void CReplayData::Delete() const {
    std::filesystem::remove(std::filesystem::path(mFileName));
}

void CReplayData::CompleteReadAsync() {
    if (FullyParsed())
        return;

    std::thread([this]() {
        this->completeRead(false);
    }).detach();
}

bool CReplayData::FullyParsed() {
    std::scoped_lock lock(mFullyParsedMtx);
    return mFullyParsed;
}

void CReplayData::SetFullyParsed(bool newValue) {
    std::scoped_lock lock(mFullyParsedMtx);
    mFullyParsed = newValue;
}
