#include "Image.hpp"

#include "Util/Logger.hpp"
#include "Util/String.hpp"

#include <jpegsize/jpegsize.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

std::optional<std::pair<uint32_t, uint32_t>> Img::GetIMGDimensions(const std::string& path) {
    std::string ext = Util::to_lower(fs::path(path).extension().string());

    std::optional<std::pair<uint32_t, uint32_t>> result;

    if (ext == ".png")
        result = GetPNGDimensions(path);
    else if (ext == ".jpg" || ext == ".jpeg")
        result = GetJPGDimensions(path);
    else if (ext == ".webp")
        result = GetWebPDimensions(path);
    else
        return {};

    return result;
}

std::optional<std::pair<uint32_t, uint32_t>> Img::GetPNGDimensions(const std::string& path) {
    const uint64_t pngSig = 0x89504E470D0A1A0A;

    std::ifstream img(path, std::ios::binary);

    if (!img.good()) {
        LOG(Error, "[IMG]: {} failed to open", path);
        return {};
    }

    uint64_t imgSig = 0x0;

    img.seekg(0);
    img.read((char*)&imgSig, sizeof(uint64_t));

    imgSig = _byteswap_uint64(imgSig);

    if (imgSig == pngSig) {
        uint32_t w, h;

        img.seekg(16);
        img.read((char*)&w, 4);
        img.read((char*)&h, 4);

        w = _byteswap_ulong(w);
        h = _byteswap_ulong(h);

        return { {w, h} };
    }

    LOG(Error, "[IMG]: {} not a PNG file, sig: 0x{:16X}", path, imgSig);
    return {};
}

std::optional<std::pair<uint32_t, uint32_t>> Img::GetJPGDimensions(const std::string& path) {
    FILE* image = nullptr;

    errno_t err = fopen_s(&image, path.c_str(), "rb");  // open JPEG image file
    if (!image || err) {
        LOG(Error, "[IMG]: {}: Failed to open file", fs::path(path).filename().string());
        if (image)
            fclose(image);
        return {};
    }
    int w, h;
    int result = scanhead(image, &w, &h);
    fclose(image);

    if (result == 1) {
        return { {w, h} };
    }
    else {
        return {};
    }
}

std::optional<std::pair<uint32_t, uint32_t>> Img::GetWebPDimensions(const std::string& path) {
    const uint32_t riffSig = 'RIFF';
    const uint32_t webpSig = 'WEBP';

    std::ifstream img(path, std::ios::binary);

    if (!img.good()) {
        LOG(Error, "[IMG]: {} failed to open", path);
        return {};
    }

    uint32_t imgRiffSig = 0x0;
    uint32_t imgWebPSig = 0x0;

    img.seekg(0);
    img.read((char*)&imgRiffSig, sizeof(uint32_t));
    imgRiffSig = _byteswap_ulong(imgRiffSig);

    img.seekg(2 * sizeof(uint32_t));
    img.read((char*)&imgWebPSig, sizeof(uint32_t));
    imgWebPSig = _byteswap_ulong(imgWebPSig);

    if (imgRiffSig == riffSig && imgWebPSig == webpSig) {
        uint32_t vp8Sig = 0x0;

        img.seekg(3 * sizeof(uint32_t));
        img.read((char*)&vp8Sig, sizeof(uint32_t));
        vp8Sig = _byteswap_ulong(vp8Sig);

        switch (vp8Sig) {
        case 'VP8 ': {
            uint8_t sigBytes[3] = { 0x0, 0x0, 0x0 };
            img.seekg(0x17);
            img.read((char*)sigBytes, 3);
            if (sigBytes[0] != 0x9D || sigBytes[1] != 0x01 || sigBytes[2] != 0x2A) {
                LOG(Error, "[IMG]: {} failed to find VP8 (Lossy) signature bytes, got 0x{:02X} 0x{:02X} 0x{:02X}",
                    path, sigBytes[0], sigBytes[1], sigBytes[2]);
                return {};
            }

            uint16_t w = 0x0;
            uint16_t h = 0x0;

            img.read((char*)&w, 2);
            img.read((char*)&h, 2);

            return { {w, h} };
        }
        case 'VP8L': {
            uint8_t sigByte = 0x0;
            img.seekg(5 * sizeof(uint32_t));
            img.read((char*)&sigByte, 1);
            if (sigByte != 0x2F) {
                LOG(Error, "[IMG]: {} failed to find VP8 (Lossless) signature byte, got 0x{:02X}",
                    path, sigByte);
                return {};
            }

            uint32_t wh = 0x0;

            img.read((char*)&wh, 4);
            wh = wh & 0x0FFFFFFF;

            uint16_t w = wh & 0x3FFF;
            uint16_t h = (wh >> 14) & 0x3FFF;
            return { {w + 1, h + 1} };
        }
        default:
            LOG(Error, "[IMG]: {} unrecognized WebP format. FourCC: 0x{:04X}",
                path, vp8Sig);
            return {};
        }
    }
    else {
        LOG(Error, "[IMG]: {} not a WebP file. RIFF (0x{:04X}): 0x{:04X}, WEBP (0x{:04X}): 0x{:04X}",
            path, riffSig, imgRiffSig, webpSig, imgWebPSig);
        return {};
    }
}