#include "String.hpp"

#include <sstream>
#include <algorithm>

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
}

std::string Util::to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::vector<std::string> Util::split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    ::split(s, delim, std::back_inserter(elems));
    return elems;
}

std::string Util::ByteArrayToString(uint8_t* byteArray, size_t length) {
    std::string instructionBytes;
    for (int i = 0; i < length; ++i) {
        char buff[4];
        snprintf(buff, 4, "%02X ", byteArray[i]);
        instructionBytes += buff;
    }
    return instructionBytes;
}
