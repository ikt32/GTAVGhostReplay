#include "UI.hpp"
#include "Math.hpp"
#include "../Constants.hpp"

#include <inc/enums.h>
#include <inc/natives.h>
#include <format>
#include <vector>

namespace {
    const size_t maxStringLength = 99;
    int notificationId = 0;

    float GetStringWidth(const std::string& text, float scale, int font) {
        HUD::_BEGIN_TEXT_COMMAND_GET_WIDTH("STRING");
        HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text.c_str());
        HUD::SET_TEXT_FONT(font);
        HUD::SET_TEXT_SCALE(scale, scale);
        return HUD::_END_TEXT_COMMAND_GET_WIDTH(true);
    }
}

void showNotification(const std::string& message, int* prevNotification) {
    if (prevNotification != nullptr && *prevNotification != 0) {
        HUD::THEFEED_REMOVE_ITEM(*prevNotification);
    }
    HUD::BEGIN_TEXT_COMMAND_THEFEED_POST("STRING");

    HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(message.c_str());

    int id = HUD::END_TEXT_COMMAND_THEFEED_POST_TICKER(false, false);
    if (prevNotification != nullptr) {
        *prevNotification = id;
    }
}

void UI::Notify(const std::string& message) {
    Notify(message, false);
}

void UI::Notify(const std::string& message, bool removePrevious) {
    int* notifHandleAddr = nullptr;
    if (removePrevious) {
        notifHandleAddr = &notificationId;
    }
    showNotification(std::format("{}\n{}", Constants::NotificationPrefix, message), notifHandleAddr);
}

void UI::ShowText(float x, float y, float scale, const std::string& text, int font, bool outline) {
    HUD::SET_TEXT_FONT(font);
    HUD::SET_TEXT_SCALE(scale, scale);
    HUD::SET_TEXT_COLOUR(255, 255, 255, 255);
    HUD::SET_TEXT_WRAP(0.0, 1.0);
    HUD::SET_TEXT_CENTRE(0);
    if (outline) HUD::SET_TEXT_OUTLINE();
    HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
    HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text.c_str());
    HUD::END_TEXT_COMMAND_DISPLAY_TEXT(x, y, 0);
}

void UI::ShowText(float x, float y, float scale, const std::string& text) {
    ShowText(x, y, scale, text, 4, true);
}

std::string UI::GetKeyboardResult() {
    WAIT(1);
    MISC::DISPLAY_ONSCREEN_KEYBOARD(true, "FMMC_KEY_TIP8", "", "", "", "", "", 127);
    while (MISC::UPDATE_ONSCREEN_KEYBOARD() == 0)
        WAIT(0);
    if (!MISC::GET_ONSCREEN_KEYBOARD_RESULT()) {
        UI::Notify("Cancelled input", false);
        return std::string();
    }
    auto result = MISC::GET_ONSCREEN_KEYBOARD_RESULT();
    if (result == nullptr)
        return std::string();
    return result;
}

void UI::DrawSphere(Vector3 p, float scale, int r, int g, int b, int a) {
    GRAPHICS::DRAW_MARKER(eMarkerType::MarkerTypeDebugSphere,
                          p.x, p.y, p.z,
                          0.0f, 0.0f, 0.0f,
                          0.0f, 0.0f, 0.0f,
                          scale, scale, scale,
                          r, g, b, a,
                          false, false, 2, false, nullptr, nullptr, false);
}

void UI::DrawLine(Vector3 pA, Vector3 pB, int r, int g, int b, int a) {
    GRAPHICS::DRAW_LINE(pA.x, pA.y, pA.z, pB.x, pB.y, pB.z, r, g, b, a);
}

void UI::ShowText3D(Vector3 location, float baseSize, const std::vector<std::string>& textLines) {
    Vector3 cameraPos = CAM::GET_GAMEPLAY_CAM_COORD();
    float distance = Distance(cameraPos, location);
    float totalMult = baseSize / (distance * (CAM::GET_GAMEPLAY_CAM_FOV() / 60.0f));

    float height = 0.0125f * totalMult;

    GRAPHICS::SET_DRAW_ORIGIN(location.x, location.y, location.z, 0);
    int i = 0;

    float szX = 0.000f;
    for (auto line : textLines) {
        float currWidth = GetStringWidth(line, 0.2f * totalMult, 0);
        ShowText(0.0f, 0.0f + height * i, 0.2f * totalMult, line, 0, true);
        if (currWidth > szX)
            szX = currWidth;
        i++;
    }

    float szY = height * i;
    GRAPHICS::DRAW_RECT(0.0f + szX / 2.0f, (height * i) / 2.0f, szX, szY,
        0, 0, 0, 92, 0);
    GRAPHICS::CLEAR_DRAW_ORIGIN();
}


void UI::ShowHelpText(const std::string& message) {
    HUD::BEGIN_TEXT_COMMAND_DISPLAY_HELP("CELL_EMAIL_BCON");

    for (size_t i = 0; i < message.size(); i += maxStringLength) {
        size_t npos = std::min(maxStringLength, static_cast<int>(message.size()) - i);
        HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(message.substr(i, npos).c_str());
    }

    HUD::END_TEXT_COMMAND_DISPLAY_HELP(0, false, false, -1);
}
