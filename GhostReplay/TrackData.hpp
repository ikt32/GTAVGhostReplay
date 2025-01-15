#pragma once
#include <inc/types.h>
#include <filesystem>
#include <string>

struct SLineDef {
    Vector3 A{};
    Vector3 B{};
};

class CTrackData {
public:
    static CTrackData Read(const std::filesystem::path& trackFile);
    static CTrackData ReadARS(const std::filesystem::path& trackFile);

    CTrackData(const std::filesystem::path& fileName);
    void Write();
    void Delete() const;
    std::string FileName() const { return mFilePath.string(); }

    bool MarkedForDeletion;

    std::string Name;
    std::string Description;
    SLineDef StartLine;
    SLineDef FinishLine;
private:
    std::filesystem::path mFilePath;
};

