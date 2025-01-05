#include "ReplayScriptUtils.hpp"
#include <inc/natives.h>
#include <format>

#include <chrono>
#include <iomanip>
#include <sstream>

namespace {
    const std::string illegalChars = "\\/:?\"<>|";
}

std::string Util::StripString(std::string input) {
    auto it = input.begin();
    for (it = input.begin(); it < input.end(); ++it) {
        bool found = illegalChars.find(*it) != std::string::npos;
        if (found) {
            *it = '.';
        }
    }
    return input;
}

std::string Util::FormatMillisTime(double totalTime) {
    auto timeMillis = static_cast<uint32_t>(totalTime);
    unsigned long long milliseconds = timeMillis % 1000;
    unsigned long long seconds = (timeMillis / 1000) % 60;
    unsigned long long minutes = (timeMillis / 1000) / 60;
    return std::format("{}:{:02d}.{:03d}", minutes, seconds, milliseconds);
}

std::string Util::FormatReplayName(
    double totalTime,
    const std::string& trackName,
    const std::string& vehicleName) {
    std::string timeStr = Util::FormatMillisTime(totalTime);

    return std::format("{} - {} - {}", trackName, vehicleName, timeStr);
}

std::string Util::GetVehicleName(Hash model) {
    const char* name = VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(model);
    std::string displayName = HUD::GET_FILENAME_FOR_AUDIO_CONVERSATION(name);
    if (displayName == "NULL") {
        displayName = name;
    }
    return displayName;
}

std::string Util::GetVehicleMake(Hash model) {
    if (getGameVersion() < 54)
        return "";

    const char* name = VEHICLE::GET_MAKE_NAME_FROM_VEHICLE_MODEL(model);
    std::string displayName = HUD::GET_FILENAME_FOR_AUDIO_CONVERSATION(name);
    if (displayName == "NULL") {
        displayName = name;
    }
    return displayName;
}

std::string Util::GetTimestampReadable(unsigned long long unixTimestampMs) {
    const auto durationSinceEpoch = std::chrono::milliseconds(unixTimestampMs);
    const std::chrono::time_point<std::chrono::system_clock> tp_after_duration(durationSinceEpoch);
    time_t time_after_duration = std::chrono::system_clock::to_time_t(tp_after_duration);

    std::stringstream timess;
    struct tm newtime {};
    auto err = localtime_s(&newtime, &time_after_duration);

    if (err != 0) {
        return "Invalid timestamp";
    }

    timess << std::put_time(&newtime, "%Y %b %d, %H:%M:%S");
    return std::format("{}", timess.str());
}

Vector3 Util::GroundZ(Vector3 v, float off) {
    float z;
    if (MISC::GET_GROUND_Z_FOR_3D_COORD(v, &z, false, false)) {
        v.z = z;
    }
    v.z += off;
    return v;
}

int Util::CreateParticleFxAtCoord(const char* assetName, const char* effectName, Vector3 coord, 
    float r, float g, float b, float a) {
    GRAPHICS::USE_PARTICLE_FX_ASSET(assetName);
    int handle = GRAPHICS::START_PARTICLE_FX_LOOPED_AT_COORD(
        effectName,
        coord,
        { 0.0f, 0.0f, 0.0f },
        1.0f, false, false, false, false);
    GRAPHICS::SET_PARTICLE_FX_LOOPED_ALPHA(handle, a);
    GRAPHICS::SET_PARTICLE_FX_LOOPED_COLOUR(handle, r, g, b, true);
    return handle;
}
