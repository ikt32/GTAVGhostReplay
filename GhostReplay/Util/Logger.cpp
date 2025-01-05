#include "Logger.hpp"

#include <array>
#include <chrono>
#include <format>
#include <fstream>
#include <vector>

CLogger gLogger;

namespace {
constexpr std::array<const char*, 5> sLevelNames{
    " DEBUG ",
    " INFO  ",
    "WARNING",
    " ERROR ",
    " FATAL "
};

constexpr const char* GetLevelText(ELogLevel level) {
    if (level < 0 || level >= sLevelNames.size())
        return "UNKNOWN";
    return sLevelNames[level];
}
}

void CLogger::SetPath(const std::filesystem::path& logFilePath) {
    mLogFilePath = logFilePath;
}

void CLogger::SetLogLevel(ELogLevel level) {
    mLogLevel = level;
}

void CLogger::Clear() {
    std::lock_guard lock(mMutex);
    std::ofstream logFile(mLogFilePath, std::ofstream::out | std::ofstream::trunc);
    logFile.close();
    if (logFile.fail())
        mError = true;
}

bool CLogger::IsInError() const {
    return mError;
}

void CLogger::ClearError() {
    mError = false;
}

void CLogger::WriteImpl(ELogLevel level, const std::string& txt) {
    if (level < mLogLevel) return;

    std::lock_guard lock(mMutex);
    std::ofstream logFile(mLogFilePath, std::ios_base::out | std::ios_base::app);

    const auto now = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
    logFile << std::format("[{:%Y-%m-%dT%H:%M:%SZ}] [{}] {}\n",
                           now,
                           GetLevelText(level),
                           txt);

    logFile.close();
    if (logFile.fail())
        mError = true;
}
