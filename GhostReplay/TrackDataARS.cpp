#include "TrackData.hpp"
#include "Util/Logger.hpp"
#include "Util/Math.hpp"
#include <pugixml/src/pugixml.hpp>
#include <format>
#include <fstream>

#define VERIFY_NODE(parent, node, name) \
    parent.child((name)); \
    if (!(node)) {\
        logger.Write(ERROR, "[XML %s] Missing node [%s]", (trackFile.c_str()), (name));\
        goto errorHandling;\
    }

struct Point {
    Vector3 v;
    float w;
};

CTrackData CTrackData::ReadARS(const std::string& trackFile) {
    CTrackData trackData("ARS");

    pugi::xml_document trackXml;
    pugi::xml_parse_result result = trackXml.load_file(trackFile.c_str());
    if (result) {
        auto dataNode = VERIFY_NODE(trackXml, dataNode, "Data");
        auto nameNode = VERIFY_NODE(dataNode, nameNode, "Name");
        auto routeNode = VERIFY_NODE(dataNode, routeNode, "Route");

        std::vector<Point> trackCoords;

        for (auto pointNode : routeNode.children("Point")) {
            auto xNode = VERIFY_NODE(pointNode, xNode, "X");
            auto yNode = VERIFY_NODE(pointNode, yNode, "Y");
            auto zNode = VERIFY_NODE(pointNode, zNode, "Z");
            auto wNode = VERIFY_NODE(pointNode, wNode, "Wide");

            Vector3 v{
                xNode.text().as_float(), 0,
                yNode.text().as_float(), 0,
                zNode.text().as_float(), 0 };
            Point p{};
            p.v = v;
            p.w = wNode.text().as_float();
            trackCoords.push_back(p);
        }

        if (trackCoords.size() < 10) {
            logger.Write(ERROR, "[Track-ARS] < 10 points (%d), unlikely a proper track", trackCoords.size());
            goto errorHandling;
        }

        trackData.Name = std::format("[ARS] {}", nameNode.text().as_string());

        bool circuit = Distance(trackCoords.front().v, trackCoords.back().v) < 5.0f;

        if (circuit)
            trackData.Description = "Circuit Track loaded from ARS";
        else
            trackData.Description = "Sprint Track loaded from ARS";

        if (circuit) {
            Vector3 startFirst = trackCoords[2].v;
            Vector3 startSecond = trackCoords[3].v;
            Vector3 cwStart = GetPerpendicular(startFirst, startSecond, trackCoords[2].w, false);
            Vector3 ccwStart = GetPerpendicular(startFirst, startSecond, trackCoords[2].w, true);

            Vector3 finishFirst = trackCoords[0].v;
            Vector3 finishSecond = trackCoords[1].v;
            Vector3 cwFinish = GetPerpendicular(finishFirst, finishSecond, trackCoords[0].w, false);
            Vector3 ccwFinish = GetPerpendicular(finishFirst, finishSecond, trackCoords[0].w, true);

            // Check the func.
            trackData.StartLine.A = ccwStart;
            trackData.StartLine.B = cwStart;

            trackData.FinishLine.A = ccwFinish;
            trackData.FinishLine.B = cwFinish;
        }
        else {
            Vector3 startFirst = trackCoords[0].v;
            Vector3 startSecond = trackCoords[1].v;
            Vector3 cwStart = GetPerpendicular(startFirst, startSecond, trackCoords[0].w, false);
            Vector3 ccwStart = GetPerpendicular(startFirst, startSecond, trackCoords[0].w, true);

            Vector3 finishFirst = trackCoords.end()[-2].v;
            Vector3 finishSecond = trackCoords.end()[-1].v;
            Vector3 cwFinish = GetPerpendicular(finishFirst, finishSecond, trackCoords.end()[-1].w, false);
            Vector3 ccwFinish = GetPerpendicular(finishFirst, finishSecond, trackCoords.end()[-1].w, true);

            trackData.StartLine.A = ccwStart;
            trackData.StartLine.B = cwStart;

            trackData.FinishLine.A = ccwFinish;
            trackData.FinishLine.B = cwFinish;
        }
        logger.Write(DEBUG, "[Track-ARS] Parsed %s", trackFile.c_str());
    }
    else {
errorHandling:
        logger.Write(ERROR, "[Track-ARS] XML [%s] parsed with errors", trackFile.c_str());
        logger.Write(ERROR, "    Error: %s", result.description());
        logger.Write(ERROR, "    Offset: %td", result.offset);
    }
    return trackData;
}
