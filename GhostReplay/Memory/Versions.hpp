#pragma once
#include <string>
#include <vector>
#include "../Util/FileVersion.hpp"

static std::vector<std::string> GameVersionString = {
    "v1.0.335.2.STEAM",      // 00
    "v1.0.335.2.NOSTEAM",    // 01

    "v1.0.350.1.STEAM",      // 02
    "v1.0.350.2.NOSTEAM",    // 03

    "v1.0.372.2.STEAM",      // 04
    "v1.0.372.2.NOSTEAM",    // 05

    "v1.0.393.2.STEAM",      // 06
    "v1.0.393.2.NOSTEAM",    // 07

    "v1.0.393.4.STEAM",      // 08
    "v1.0.393.4.NOSTEAM",    // 09

    "v1.0.463.1.STEAM",      // 10
    "v1.0.463.1.NOSTEAM",    // 11

    "v1.0.505.2.STEAM",      // 12
    "v1.0.505.2.NOSTEAM",    // 13

    "v1.0.573.1.STEAM",      // 14
    "v1.0.573.1.NOSTEAM",    // 15

    "v1.0.617.1.STEAM",      // 16
    "v1.0.617.1.NOSTEAM",    // 17

    "v1.0.678.1.STEAM",      // 18
    "v1.0.678.1.NOSTEAM",    // 19

    "v1.0.757.2.STEAM",      // 20
    "v1.0.757.2.NOSTEAM",    // 21

    "v1.0.757.4.STEAM",      // 22
    "v1.0.757.4.NOSTEAM",    // 23

    "v1.0.791.2.STEAM",      // 24
    "v1.0.791.2.NOSTEAM",    // 25

    "v1.0.877.1.STEAM",      // 26
    "v1.0.877.1.NOSTEAM",    // 27

    "v1.0.944.2.STEAM",      // 28
    "v1.0.944.2.NOSTEAM",    // 29

    "v1.0.1011.1.STEAM",     // 30
    "v1.0.1011.1.NOSTEAM",   // 31

    "v1.0.1032.1.STEAM",     // 32
    "v1.0.1032.1.NOSTEAM",   // 33

    "v1.0.1103.2.STEAM",     // 34
    "v1.0.1103.2.NOSTEAM",   // 35

    "v1.0.1180.2.STEAM",     // 36
    "v1.0.1180.2.NOSTEAM",   // 37

    "v1.0.1290.1.STEAM",     // 38
    "v1.0.1290.1.NOSTEAM",   // 39

    "v1.0.1365.1.STEAM",     // 40
    "v1.0.1365.1.NOSTEAM",   // 41

    "v1.0.1493.0.STEAM",     // 42
    "v1.0.1493.0.NOSTEAM",   // 43

    "v1.0.1493.1.STEAM",     // 44
    "v1.0.1493.1.NOSTEAM",   // 45

    "v1.0.1604.0.STEAM",     // 46
    "v1.0.1604.0.NOSTEAM",   // 47

    "v1.0.1604.1.STEAM",     // 48
    "v1.0.1604.1.NOSTEAM",   // 49

    //"v1.0.1734.0.STEAM",   // XX
    //"v1.0.1734.0.NOSTEAM", // XX

    "v1.0.1737.0.STEAM",     // 50
    "v1.0.1737.0.NOSTEAM",   // 51

    "v1.0.1737.6.STEAM",     // 52
    "v1.0.1737.6.NOSTEAM",   // 53

    "v1.0.1868.0.STEAM",     // 54
    "v1.0.1868.0.NOSTEAM",   // 55

    "v1.0.1868.1.STEAM",     // 56
    "v1.0.1868.1.NOSTEAM",   // 57

    "v1.0.1868.4.EGS",       // 58

    "v1.0.2060.0.STEAM",     // 59
    "v1.0.2060.0.NOSTEAM",   // 60

    "v1.0.2060.1.STEAM",     // 61
    "v1.0.2060.1.NOSTEAM",   // 62

    "v1.0.2189.0.STEAM",     // 63
    "v1.0.2189.0.NOSTEAM",   // 64

    "v1.0.2215.0.STEAM",     // 65
    "v1.0.2215.0.NOSTEAM",   // 66

    "v1.0.2245.0.STEAM",     // 67
    "v1.0.2245.0.NOSTEAM",   // 68

    "v1.0.2372.0.STEAM",     // 69
    "v1.0.2372.0.NOSTEAM",   // 70

    "v1.0.2545.0.STEAM",     // 71
    "v1.0.2545.0.NOSTEAM",   // 72

    "v1.0.2612.1.STEAM",     // 73
    "v1.0.2612.1.NOSTEAM",   // 74

    "v1.0.2628.2.STEAM",     // 75
    "v1.0.2628.2.NOSTEAM",   // 76

    "v1.0.2699.0.STEAM",     // 77
    "v1.0.2699.0.NOSTEAM",   // 78

    "v1.0.2699.16",          // 79
    "v1.0.2802.0",           // 80
    "v1.0.2824.0",           // 81
    "v1.0.2845.0",           // 82
    "v1.0.2944.0",           // 83
    "v1.0.3028.0",           // 84
    "v1.0.3095.0",           // 85
    "v1.0.3179.0",           // 86
    "v1.0.3258.0",           // 87
    "v1.0.3274.0",           // 88
    "v1.0.3323.0",           // 89
    "v1.0.3337.0",           // 90
    "v1.0.3351.0",           // 91
    "v1.0.3407.0",           // 92
    "v1.0.3411.0",           // 93
};

enum G_GameVersion : int {
    G_VER_1_0_335_2_STEAM,      // 00
    G_VER_1_0_335_2_NOSTEAM,    // 01

    G_VER_1_0_350_1_STEAM,      // 02
    G_VER_1_0_350_2_NOSTEAM,    // 03

    G_VER_1_0_372_2_STEAM,      // 04
    G_VER_1_0_372_2_NOSTEAM,    // 05

    G_VER_1_0_393_2_STEAM,      // 06
    G_VER_1_0_393_2_NOSTEAM,    // 07

    G_VER_1_0_393_4_STEAM,      // 08
    G_VER_1_0_393_4_NOSTEAM,    // 09

    G_VER_1_0_463_1_STEAM,      // 10
    G_VER_1_0_463_1_NOSTEAM,    // 11

    G_VER_1_0_505_2_STEAM,      // 12
    G_VER_1_0_505_2_NOSTEAM,    // 13

    G_VER_1_0_573_1_STEAM,      // 14
    G_VER_1_0_573_1_NOSTEAM,    // 15

    G_VER_1_0_617_1_STEAM,      // 16
    G_VER_1_0_617_1_NOSTEAM,    // 17

    G_VER_1_0_678_1_STEAM,      // 18
    G_VER_1_0_678_1_NOSTEAM,    // 19

    G_VER_1_0_757_2_STEAM,      // 20
    G_VER_1_0_757_2_NOSTEAM,    // 21

    G_VER_1_0_757_4_STEAM,      // 22
    G_VER_1_0_757_4_NOSTEAM,    // 23

    G_VER_1_0_791_2_STEAM,      // 24
    G_VER_1_0_791_2_NOSTEAM,    // 25

    G_VER_1_0_877_1_STEAM,      // 26
    G_VER_1_0_877_1_NOSTEAM,    // 27

    G_VER_1_0_944_2_STEAM,      // 28
    G_VER_1_0_944_2_NOSTEAM,    // 29

    G_VER_1_0_1011_1_STEAM,     // 30
    G_VER_1_0_1011_1_NOSTEAM,   // 31

    G_VER_1_0_1032_1_STEAM,     // 32
    G_VER_1_0_1032_1_NOSTEAM,   // 33

    G_VER_1_0_1103_2_STEAM,     // 34
    G_VER_1_0_1103_2_NOSTEAM,   // 35

    G_VER_1_0_1180_2_STEAM,     // 36
    G_VER_1_0_1180_2_NOSTEAM,   // 37

    G_VER_1_0_1290_1_STEAM,     // 38
    G_VER_1_0_1290_1_NOSTEAM,   // 39

    G_VER_1_0_1365_1_STEAM,     // 40
    G_VER_1_0_1365_1_NOSTEAM,   // 41

    G_VER_1_0_1493_0_STEAM,     // 42
    G_VER_1_0_1493_0_NOSTEAM,   // 43

    G_VER_1_0_1493_1_STEAM,     // 44
    G_VER_1_0_1493_1_NOSTEAM,   // 45

    G_VER_1_0_1604_0_STEAM,     // 46
    G_VER_1_0_1604_0_NOSTEAM,   // 47

    G_VER_1_0_1604_1_STEAM,     // 48
    G_VER_1_0_1604_1_NOSTEAM,   // 49

    //G_VER_1_0_1734_0_STEAM,   // XX
    //G_VER_1_0_1734_0_NOSTEAM, // XX

    G_VER_1_0_1737_0_STEAM,     // 50
    G_VER_1_0_1737_0_NOSTEAM,   // 51

    G_VER_1_0_1737_6_STEAM,     // 52
    G_VER_1_0_1737_6_NOSTEAM,   // 53

    G_VER_1_0_1868_0_STEAM,     // 54
    G_VER_1_0_1868_0_NOSTEAM,   // 55

    G_VER_1_0_1868_1_STEAM,     // 56
    G_VER_1_0_1868_1_NOSTEAM,   // 57

    G_VER_1_0_1868_4_EGS,       // 58

    G_VER_1_0_2060_0_STEAM,     // 59
    G_VER_1_0_2060_0_NOSTEAM,   // 60

    G_VER_1_0_2060_1_STEAM,     // 61
    G_VER_1_0_2060_1_NOSTEAM,   // 62

    G_VER_1_0_2189_0_STEAM,     // 63
    G_VER_1_0_2189_0_NOSTEAM,   // 64

    G_VER_1_0_2215_0_STEAM,     // 65
    G_VER_1_0_2215_0_NOSTEAM,   // 66

    G_VER_1_0_2245_0_STEAM,     // 67
    G_VER_1_0_2245_0_NOSTEAM,   // 68

    G_VER_1_0_2372_0_STEAM,     // 69
    G_VER_1_0_2372_0_NOSTEAM,   // 70

    G_VER_1_0_2545_0_STEAM,     // 71
    G_VER_1_0_2545_0_NOSTEAM,   // 72

    G_VER_1_0_2612_1_STEAM,     // 73
    G_VER_1_0_2612_1_NOSTEAM,   // 74

    G_VER_1_0_2628_2_STEAM,     // 75
    G_VER_1_0_2628_2_NOSTEAM,   // 76

    G_VER_1_0_2699_0_STEAM,     // 77
    G_VER_1_0_2699_0_NOSTEAM,   // 78

    G_VER_1_0_2699_16,          // 79
    G_VER_1_0_2802_0,           // 80
    G_VER_1_0_2824_0,           // 81
    G_VER_1_0_2845_0,           // 82
    G_VER_1_0_2944_0,           // 83
    G_VER_1_0_3028_0,           // 84
    G_VER_1_0_3095_0,           // 85
    G_VER_1_0_3179_0,           // 86
    G_VER_1_0_3258_0,           // 87
    G_VER_1_0_3274_0,           // 88
    G_VER_1_0_3323_0,           // 89
    G_VER_1_0_3337_0,           // 90
    G_VER_1_0_3351_0,           // 91
    G_VER_1_0_3407_0,           // 92
    G_VER_1_0_3411_0,           // 93
};

static std::vector<std::pair<SVersion, std::vector<int>>> ExeVersionMap = {
    { {   0, 0 },   { -1} },
    { { 335, 2 },   { G_VER_1_0_335_2_STEAM, G_VER_1_0_335_2_NOSTEAM } },
    { { 350, 1 },   { G_VER_1_0_350_1_STEAM, G_VER_1_0_350_2_NOSTEAM } },
    { { 372, 2 },   { G_VER_1_0_372_2_STEAM, G_VER_1_0_372_2_NOSTEAM } },
    { { 393, 2 },   { G_VER_1_0_393_2_STEAM, G_VER_1_0_393_2_NOSTEAM } },
    { { 393, 4 },   { G_VER_1_0_393_4_STEAM, G_VER_1_0_393_4_NOSTEAM } },
    { { 463, 1 },   { G_VER_1_0_463_1_STEAM, G_VER_1_0_463_1_NOSTEAM } },
    { { 505, 2 },   { G_VER_1_0_505_2_STEAM, G_VER_1_0_505_2_NOSTEAM } },
    { { 573, 1 },   { G_VER_1_0_573_1_STEAM, G_VER_1_0_573_1_NOSTEAM } },
    { { 617, 1 },   { G_VER_1_0_617_1_STEAM, G_VER_1_0_617_1_NOSTEAM } },
    { { 678, 1 },   { G_VER_1_0_678_1_STEAM, G_VER_1_0_678_1_NOSTEAM } },
    { { 757, 2 },   { G_VER_1_0_757_2_STEAM, G_VER_1_0_757_2_NOSTEAM } },
    { { 757, 4 },   { G_VER_1_0_757_4_STEAM, G_VER_1_0_757_4_NOSTEAM } },
    { { 791, 2 },   { G_VER_1_0_791_2_STEAM, G_VER_1_0_791_2_NOSTEAM } },
    { { 877, 1 },   { G_VER_1_0_877_1_STEAM, G_VER_1_0_877_1_NOSTEAM } },
    { { 944, 2 },   { G_VER_1_0_944_2_STEAM, G_VER_1_0_944_2_NOSTEAM } },
    { { 1011, 1 },  { G_VER_1_0_1011_1_STEAM, G_VER_1_0_1011_1_NOSTEAM } },
    { { 1032, 1 },  { G_VER_1_0_1032_1_STEAM, G_VER_1_0_1032_1_NOSTEAM } },
    { { 1103, 2 },  { G_VER_1_0_1103_2_STEAM, G_VER_1_0_1103_2_NOSTEAM } },
    { { 1180, 2 },  { G_VER_1_0_1180_2_STEAM, G_VER_1_0_1180_2_NOSTEAM } },
    { { 1290, 1 },  { G_VER_1_0_1290_1_STEAM, G_VER_1_0_1290_1_NOSTEAM } },
    { { 1365, 1 },  { G_VER_1_0_1365_1_STEAM, G_VER_1_0_1365_1_NOSTEAM } },
    { { 1493, 0 },  { G_VER_1_0_1493_0_STEAM, G_VER_1_0_1493_0_NOSTEAM } },
    { { 1493, 1 },  { G_VER_1_0_1493_1_STEAM, G_VER_1_0_1493_1_NOSTEAM } },
    { { 1604, 0 },  { G_VER_1_0_1604_0_STEAM, G_VER_1_0_1604_0_NOSTEAM } },
    { { 1604, 1 },  { G_VER_1_0_1604_1_STEAM, G_VER_1_0_1604_1_NOSTEAM } },
  //{ { 1734, 0 },  { G_VER_1_0_1734_0_STEAM, G_VER_1_0_1734_0_NOSTEAM } },
    { { 1737, 0 },  { G_VER_1_0_1737_0_STEAM, G_VER_1_0_1737_0_NOSTEAM } },
    { { 1737, 6 },  { G_VER_1_0_1737_6_STEAM, G_VER_1_0_1737_6_NOSTEAM } },
    { { 1868, 0 },  { G_VER_1_0_1868_0_STEAM, G_VER_1_0_1868_0_NOSTEAM } },
    { { 1868, 1 },  { G_VER_1_0_1868_1_STEAM, G_VER_1_0_1868_1_NOSTEAM } },
    { { 1868, 4 },  { G_VER_1_0_1868_4_EGS} },
    { { 2060, 0 },  { G_VER_1_0_2060_0_STEAM, G_VER_1_0_2060_0_NOSTEAM} },
    { { 2060, 1 },  { G_VER_1_0_2060_1_STEAM, G_VER_1_0_2060_1_NOSTEAM} },
    { { 2189, 0 },  { G_VER_1_0_2189_0_STEAM, G_VER_1_0_2189_0_NOSTEAM} },
    { { 2215, 0 },  { G_VER_1_0_2215_0_STEAM, G_VER_1_0_2215_0_NOSTEAM} },
    { { 2245, 0 },  { G_VER_1_0_2245_0_STEAM, G_VER_1_0_2245_0_NOSTEAM} },
    { { 2372, 0 },  { G_VER_1_0_2372_0_STEAM, G_VER_1_0_2372_0_NOSTEAM} },
    { { 2545, 0 },  { G_VER_1_0_2545_0_STEAM, G_VER_1_0_2545_0_NOSTEAM} },
    { { 2612, 1 },  { G_VER_1_0_2612_1_STEAM, G_VER_1_0_2612_1_NOSTEAM} },
    { { 2628, 2 },  { G_VER_1_0_2628_2_STEAM, G_VER_1_0_2628_2_NOSTEAM} },
    { { 2699, 0 },  { G_VER_1_0_2699_0_STEAM, G_VER_1_0_2699_0_NOSTEAM} },
    { { 2699, 16 }, { G_VER_1_0_2699_16} },
    { { 2802, 0 },  { G_VER_1_0_2802_0} },
    { { 2824, 0 },  { G_VER_1_0_2824_0} },
    { { 2845, 0 },  { G_VER_1_0_2845_0} },
    { { 2944, 0 },  { G_VER_1_0_2944_0} },
    { { 3028, 0 },  { G_VER_1_0_3028_0} },
    { { 3095, 0 },  { G_VER_1_0_3095_0} },
    { { 3179, 0 },  { G_VER_1_0_3179_0} },
    { { 3258, 0 },  { G_VER_1_0_3258_0} },
    { { 3274, 0 },  { G_VER_1_0_3274_0} },
    { { 3323, 0 },  { G_VER_1_0_3323_0} },
    { { 3337, 0 },  { G_VER_1_0_3337_0} },
    { { 3351, 0 },  { G_VER_1_0_3351_0} },
    { { 3407, 0 },  { G_VER_1_0_3407_0} },
    { { 3411, 0 },  { G_VER_1_0_3411_0} },
};

static std::string eGameVersionToString(int version) {
    if (version > GameVersionString.size() - 1 || version < 0) {
        return std::to_string(version);
    }
    return GameVersionString[version];
}

template < typename K, typename V >
V findNextLowest(const std::vector<std::pair<K, V>>& map, const K& key) {
    for (auto it = map.rbegin(); it != map.rend(); ++it) {
        if (it->first <= key)
            return it->second;
    }
    return map.begin()->second;
}
