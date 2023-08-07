#include "TrackData.hpp"
#include "Constants.hpp"
#include "Util/Paths.hpp"
#include "Util/Logger.hpp"
#include "ReplayScriptUtils.hpp"

#include <nlohmann/json.hpp>
#include <format>
#include <filesystem>
#include <fstream>

CTrackData CTrackData::Read(const std::string& trackFile) {
    CTrackData trackData(trackFile);

    nlohmann::json trackJson;
    std::ifstream replayFileStream(trackFile.c_str());
    if (!replayFileStream.is_open()) {
        logger.Write(ERROR, "[Track] Failed to open %s", trackFile.c_str());
        return trackData;
    }

    try {
        replayFileStream >> trackJson;

        trackData.Name = trackJson["Name"];
        trackData.Description = trackJson.value("Description", "None");

        trackData.StartLine.A.x = trackJson["StartLine"]["A"]["X"];
        trackData.StartLine.A.y = trackJson["StartLine"]["A"]["Y"];
        trackData.StartLine.A.z = trackJson["StartLine"]["A"]["Z"];

        trackData.StartLine.B.x = trackJson["StartLine"]["B"]["X"];
        trackData.StartLine.B.y = trackJson["StartLine"]["B"]["Y"];
        trackData.StartLine.B.z = trackJson["StartLine"]["B"]["Z"];

        trackData.FinishLine.A.x = trackJson["FinishLine"]["A"]["X"];
        trackData.FinishLine.A.y = trackJson["FinishLine"]["A"]["Y"];
        trackData.FinishLine.A.z = trackJson["FinishLine"]["A"]["Z"];

        trackData.FinishLine.B.x = trackJson["FinishLine"]["B"]["X"];
        trackData.FinishLine.B.y = trackJson["FinishLine"]["B"]["Y"];
        trackData.FinishLine.B.z = trackJson["FinishLine"]["B"]["Z"];

        logger.Write(DEBUG, "[Track] Parsed %s", trackFile.c_str());
        return trackData;
    }
    catch (std::exception& ex) {
        logger.Write(ERROR, "[Track] Failed to open %s, exception: %s", trackFile.c_str(), ex.what());
        return trackData;
    }
}

CTrackData::CTrackData(std::string fileName)
    : MarkedForDeletion(false)
    , StartLine(SLineDef{})
    , FinishLine(SLineDef{})
    , mFileName(std::move(fileName)) {
}

void CTrackData::Write() {
    nlohmann::ordered_json trackJson;

    trackJson["Name"] = Name;
    trackJson["StartLine"]["A"]["X"] = StartLine.A.x;
    trackJson["StartLine"]["A"]["Y"] = StartLine.A.y;
    trackJson["StartLine"]["A"]["Z"] = StartLine.A.z;

    trackJson["StartLine"]["B"]["X"] = StartLine.B.x;
    trackJson["StartLine"]["B"]["Y"] = StartLine.B.y;
    trackJson["StartLine"]["B"]["Z"] = StartLine.B.z;

    trackJson["FinishLine"]["A"]["X"] = FinishLine.A.x;
    trackJson["FinishLine"]["A"]["Y"] = FinishLine.A.y;
    trackJson["FinishLine"]["A"]["Z"] = FinishLine.A.z;

    trackJson["FinishLine"]["B"]["X"] = FinishLine.B.x;
    trackJson["FinishLine"]["B"]["Y"] = FinishLine.B.y;
    trackJson["FinishLine"]["B"]["Z"] = FinishLine.B.z;

    const std::string tracksPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Tracks";

    std::string cleanName = Util::StripString(Name);

    const std::string trackFileName = std::format("{}\\{}.json", tracksPath, cleanName);

    std::ofstream trackFile(trackFileName);
    trackFile << std::setw(2) << trackJson << std::endl;

    logger.Write(INFO, "[Track] Written %s", trackFileName.c_str());
    mFileName = trackFileName;
}

void CTrackData::Delete() const {
    std::filesystem::remove(std::filesystem::path(mFileName));
}
