#include "ReplayData.hpp"
#include "Constants.hpp"
#include "Util/Paths.hpp"
#include "Util/Logger.hpp"
#include "ReplayScriptUtils.hpp"
#include "Script.hpp"

#include <nlohmann/json.hpp>
#include <format>
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

CReplayData CReplayData::Read(const std::filesystem::path& replayFile) {
    CReplayData replayData(replayFile);

    nlohmann::ordered_json replayJson;
    std::ifstream replayFileStream;
    if (replayFile.extension().string() == BinExt) {
        replayFileStream.open(replayFile, std::ios::binary | std::ios::ate);
    }
    else {
        replayFileStream.open(replayFile);
    }

    if (!replayFileStream.is_open()) {
        LOG(Error, "[Replay] Failed to open {}", replayFile.string());
        return replayData;
    }

    try {
        if (replayFile.extension().string() == BinExt) {
            std::streamsize size = replayFileStream.tellg();
            replayFileStream.seekg(0, std::ios::beg);
            std::vector<uint8_t> binData(size);

            if (!replayFileStream.read(reinterpret_cast<char*>(binData.data()), size)) {
                LOG(Error, "[Replay] Failed to read {}", replayFile.string());
                return replayData;
            }

            replayJson = nlohmann::json::from_msgpack(binData);
        }
        else {
            replayFileStream >> replayJson;
        }

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

            if (jsonNode.contains("Roof"))
                node.Roof = jsonNode.at("Roof").get<int>();

            replayData.Nodes.push_back(node);
        }
        LOG(Debug, "[Replay] Parsed {}", replayFile);
        return replayData;
    }
    catch (std::exception& ex) {
        LOG(Error, "[Replay] Failed to open {}, exception: {}", replayFile.string(), ex.what());
        return replayData;
    }
}

CReplayData::CReplayData(const std::filesystem::path& replayFile)
    : MarkedForDeletion(false)
    , Timestamp(0)
    , VehicleModel(0)
    , mFilePath(replayFile) {}

void CReplayData::write(int storageType) {
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
        if (Node.Roof != std::nullopt)
            node.push_back({ "Roof", *Node.Roof });

        replayJson["Nodes"].push_back(node);
    }


    switch (storageType) {
        case 0: {
            std::ofstream replayFile(mFilePath);
            replayFile << std::setw(2) << replayJson << std::endl;
            break;
        }
        case 1: {
            std::ofstream replayFile(mFilePath);
            replayFile << replayJson.dump();
            break;
        }
        case 2:
            [[fallthrough]];
        default: {
            std::ofstream replayFile(mFilePath, std::ios::binary);
            const std::vector<uint8_t> binData = nlohmann::json::to_msgpack(replayJson);
            replayFile.write(reinterpret_cast<const char*>(binData.data()), binData.size());
            replayFile.close();
        }
    }

    LOG(Info, "[Replay] Written {}", mFilePath.string());
}

void CReplayData::generateFileName(int storageType) {
    const std::filesystem::path replaysPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Replays";

    std::string cleanName = Util::StripString(Name);
    unsigned count = 0;
    std::string suffix;
    std::string extension = storageType == 2 ? "grbin" : "json";

    while (std::filesystem::exists(replaysPath / std::format("{}{}.{}", cleanName, suffix, extension))) {
        if (suffix.empty()) {
            suffix = "_0";
        }
        else {
            suffix = std::format("_{}", ++count);
        }
    }

    mFilePath = replaysPath / std::format("{}{}.{}", cleanName, suffix, extension);
}

void CReplayData::WriteAsync(CReplayData& replayData) {
    int storageType = GhostReplay::GetSettings().Record.StorageType;
    replayData.generateFileName(storageType);
    std::thread([replayData, storageType]() {
        CReplayData myCopy = replayData;
        myCopy.write(storageType);
    }).detach();
}

void CReplayData::Delete() const {
    std::filesystem::remove(mFilePath);
}
