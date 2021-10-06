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

CReplayData CReplayData::Read(const std::string& replayFile) {
    CReplayData replayData(replayFile);

    nlohmann::ordered_json replayJson;
    std::ifstream replayFileStream(replayFile.c_str());
    if (!replayFileStream.is_open()) {
        logger.Write(ERROR, "[Replay] Failed to open %s", replayFile.c_str());
        return replayData;
    }

    try {
        replayFileStream >> replayJson;

        replayData.Timestamp = replayJson.value("Timestamp", 0ull);
        replayData.Name = replayJson["Name"];
        replayData.Track = replayJson["Track"];
        replayData.VehicleModel = replayJson["VehicleModel"];
        replayData.VehicleMods = replayJson.value("Mods", VehicleModData());
        replayData.ReplayDriver = replayJson.value("Driver", SReplayDriverData());

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

            replayData.Nodes.push_back(node);
        }
        logger.Write(DEBUG, "[Replay] Parsed %s", replayFile.c_str());
        return replayData;
    }
    catch (std::exception& ex) {
        logger.Write(ERROR, "[Replay] Failed to open %s, exception: %s", replayFile.c_str(), ex.what());
        return replayData;
    }
}

CReplayData::CReplayData(std::string fileName)
    : MarkedForDeletion(false)
    , Timestamp(0)
    , VehicleModel(0)
    , mFileName(std::move(fileName)) {}

void CReplayData::write(bool pretty) {
    nlohmann::ordered_json replayJson;

    replayJson["Timestamp"] = Timestamp;
    replayJson["Name"] = Name;
    replayJson["Track"] = Track;
    replayJson["VehicleModel"] = VehicleModel;
    replayJson["Mods"] = VehicleMods;
    replayJson["Driver"] = ReplayDriver;

    for (auto& Node : Nodes) {
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

        replayJson["Nodes"].push_back(node);
    }

    std::ofstream replayFile(mFileName);

    if (pretty)
        replayFile << std::setw(2) << replayJson << std::endl;
    else
        replayFile << replayJson.dump();

    logger.Write(INFO, "[Replay] Written %s", mFileName.c_str());
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

void CReplayData::WriteAsync(CReplayData& replayData) {
    replayData.generateFileName();
    bool pretty = !GhostReplay::GetSettings().Record.ReduceFileSize;
    std::thread([replayData, pretty]() {
        CReplayData myCopy = replayData;
        myCopy.write(pretty);
    }).detach();
}

void CReplayData::Delete() const {
    std::filesystem::remove(std::filesystem::path(mFileName));
}
