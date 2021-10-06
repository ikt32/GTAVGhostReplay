#pragma once
#include <nlohmann/json.hpp>
#include <cstdint>

struct SHeadBlendData {
    int Shape1;     int32_t pad0;
    int Shape2;     int32_t pad1;
    int Shape3;     int32_t pad2;
    int Skin1;      int32_t pad3;
    int Skin2;      int32_t pad4;
    int Skin3;      int32_t pad5;
    float ShapeMix; int32_t pad6;
    float SkinMix;  int32_t pad7;
    float ThirdMix; int32_t pad8;
};

inline void to_json(nlohmann::ordered_json& j, const SHeadBlendData& hbd) {
    j = nlohmann::ordered_json{
        { "Shape1", hbd.Shape1 },
        { "Shape2", hbd.Shape2 },
        { "Shape3", hbd.Shape3 },
        { "Skin1", hbd.Skin1 },
        { "Skin2", hbd.Skin2 },
        { "Skin3", hbd.Skin3 },
        { "ShapeMix", hbd.ShapeMix },
        { "SkinMix", hbd.SkinMix },
        { "ThirdMix", hbd.ThirdMix },
    };
}

inline void from_json(const nlohmann::ordered_json& j, SHeadBlendData& hbd) {
    j.at("Shape1").get_to(hbd.Shape1);
    j.at("Shape2").get_to(hbd.Shape2);
    j.at("Shape3").get_to(hbd.Shape3);
    j.at("Skin1").get_to(hbd.Skin1);
    j.at("Skin2").get_to(hbd.Skin2);
    j.at("Skin3").get_to(hbd.Skin3);
    j.at("ShapeMix").get_to(hbd.ShapeMix);
    j.at("SkinMix").get_to(hbd.SkinMix);
    j.at("ThirdMix").get_to(hbd.ThirdMix);
}
