#include "NativeMemory.hpp"

#include "../Util/Logger.hpp"

#include <Windows.h>
#include <Psapi.h>
#include <sstream>

namespace {
    template<typename Out>
    void split(const std::string& s, char delim, Out result) {
        std::stringstream ss;
        ss.str(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            *(result++) = item;
        }
    }

    std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> elems;
        ::split(s, delim, std::back_inserter(elems));
        return elems;
    }
}

namespace mem {

    uintptr_t(*GetAddressOfEntity)(int entity) = nullptr;
    uintptr_t(*GetModelInfo)(unsigned int modelHash, int* index) = nullptr;

    void init() {
        auto addr = FindPattern("\x83\xF9\xFF\x74\x31\x4C\x8B\x0D\x00\x00\x00\x00\x44\x8B\xC1\x49\x8B\x41\x08",
                                                            "xxxxxxxx????xxxxxxx");
        if (!addr) LOG(Error, "Couldn't find GetAddressOfEntity");
        GetAddressOfEntity = reinterpret_cast<uintptr_t(*)(int)>(addr);
        
        addr = FindPattern("\x0F\xB7\x05\x00\x00\x00\x00"
            "\x45\x33\xC9\x4C\x8B\xDA\x66\x85\xC0"
            "\x0F\x84\x00\x00\x00\x00"
            "\x44\x0F\xB7\xC0\x33\xD2\x8B\xC1\x41\xF7\xF0\x48"
            "\x8B\x05\x00\x00\x00\x00"
            "\x4C\x8B\x14\xD0\xEB\x09\x41\x3B\x0A\x74\x54",
            "xxx????xxxxxxxxxxx????"
            "xxxxxxxxxxxxxx????xxxxxxxxxxx");
        if (!addr) LOG(Error, "Couldn't find GetModelInfo");
        GetModelInfo = reinterpret_cast<uintptr_t(*)(unsigned int modelHash, int *index)>(addr);
    }

    uintptr_t FindPattern(const char* pattern, const char* mask) {
        MODULEINFO modInfo = { nullptr };
        GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

        const char* start_offset = reinterpret_cast<const char *>(modInfo.lpBaseOfDll);
        const uintptr_t size = static_cast<uintptr_t>(modInfo.SizeOfImage);

        intptr_t pos = 0;
        const uintptr_t searchLen = static_cast<uintptr_t>(strlen(mask) - 1);

        for (const char* retAddress = start_offset; retAddress < start_offset + size; retAddress++) {
            if (*retAddress == pattern[pos] || mask[pos] == '?') {
                if (mask[pos + 1] == '\0')
                    return (reinterpret_cast<uintptr_t>(retAddress) - searchLen);
                pos++;
            }
            else {
                pos = 0;
            }
        }
        return 0;
    }

    std::vector<uintptr_t> FindPatterns(const char* pattern, const char* mask) {
        std::vector <uintptr_t> addresses;

        MODULEINFO modInfo = { nullptr };
        GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

        const char* start_offset = reinterpret_cast<const char *>(modInfo.lpBaseOfDll);
        const uintptr_t size = static_cast<uintptr_t>(modInfo.SizeOfImage);

        intptr_t pos = 0;
        const uintptr_t searchLen = static_cast<uintptr_t>(strlen(mask) - 1);

        for (const char* retAddress = start_offset; retAddress < start_offset + size; retAddress++) {
            if (*retAddress == pattern[pos] || mask[pos] == '?') {
                if (mask[pos + 1] == '\0')
                    addresses.push_back(reinterpret_cast<uintptr_t>(retAddress) - searchLen);
                pos++;
            }
            else {
                pos = 0;
            }
        }
        return addresses;
    }

    uintptr_t FindPattern(const std::vector<uint8_t>& pattern_bytes, const std::string& mask) {
        MODULEINFO mod_info;
        if (!GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &mod_info, sizeof(mod_info))) {
            return 0;
        }

        uintptr_t base_address = reinterpret_cast<uintptr_t>(mod_info.lpBaseOfDll);
        size_t module_size = mod_info.SizeOfImage;

        for (size_t offset = 0; offset < module_size - pattern_bytes.size(); ++offset) {
            bool found = true;

            for (size_t j = 0; j < pattern_bytes.size(); ++j) {
                if (mask[j] == 'x' && pattern_bytes[j] != *reinterpret_cast<uint8_t*>(base_address + offset + j)) {
                    found = false;
                    break;
                }
            }

            if (found) {
                return base_address + offset;
            }
        }

        return 0;
    }

    uintptr_t FindPattern(const std::string& pattern) {
        std::vector<uint8_t> pattern_bytes;
        std::string mask;

        size_t i = 0;
        while (i < pattern.size()) {
            if (pattern[i] == '?') {
                pattern_bytes.push_back(0);
                mask += '?';

                // Handle ??
                if (i + 1 < pattern.size() && pattern[i + 1] == '?') {
                    i++;
                }
            }
            else if (isxdigit(pattern[i]) && i + 1 < pattern.size() && isxdigit(pattern[i + 1])) {
                 // Convert hex string to byte
                std::string byte_str = pattern.substr(i, 2);
                pattern_bytes.push_back(static_cast<uint8_t>(std::strtoul(byte_str.c_str(), nullptr, 16)));
                mask += 'x';
                i++;  // Skip next character since we processed two
            }
            i++;
        }

        return FindPattern(pattern_bytes, mask);
    }
}
