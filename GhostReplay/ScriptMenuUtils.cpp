#include "ScriptMenuUtils.h"

#include "Util/UI.hpp"

#include <inc/natives.h>
#include <format>

bool MenuUtils::GetKbInt(int& val) {
    UI::Notify("Enter value", true);
    MISC::DISPLAY_ONSCREEN_KEYBOARD(LOCALIZATION::GET_CURRENT_LANGUAGE() == 0, "FMMC_KEY_TIP8", "",
        std::format("{}", val).c_str(), "", "", "", 64);
    while (MISC::UPDATE_ONSCREEN_KEYBOARD() == 0) {
        WAIT(0);
    }
    if (!MISC::GET_ONSCREEN_KEYBOARD_RESULT()) {
        UI::Notify("Cancelled value entry", true);
        return false;
    }

    std::string intStr = MISC::GET_ONSCREEN_KEYBOARD_RESULT();
    if (intStr.empty()) {
        UI::Notify("Cancelled value entry", true);
        return false;
    }

    char* pEnd;
    int parsedValue = strtol(intStr.c_str(), &pEnd, 10);

    if (parsedValue == 0 && *pEnd != 0) {
        UI::Notify("Failed to parse entry.", true);
        return false;
    }

    val = parsedValue;
    return true;
}

bool MenuUtils::GetKbFloat(float& val) {
    UI::Notify("Enter value", true);
    MISC::DISPLAY_ONSCREEN_KEYBOARD(LOCALIZATION::GET_CURRENT_LANGUAGE() == 0, "FMMC_KEY_TIP8", "",
        std::format("{:f}", val).c_str(), "", "", "", 64);
    while (MISC::UPDATE_ONSCREEN_KEYBOARD() == 0) {
        WAIT(0);
    }
    if (!MISC::GET_ONSCREEN_KEYBOARD_RESULT()) {
        UI::Notify("Cancelled value entry", true);
        return false;
    }

    std::string floatStr = MISC::GET_ONSCREEN_KEYBOARD_RESULT();
    if (floatStr.empty()) {
        UI::Notify("Cancelled value entry", true);
        return false;
    }

    char* pEnd;
    float parsedValue = strtof(floatStr.c_str(), &pEnd);

    if (parsedValue == 0.0f && *pEnd != 0) {
        UI::Notify("Failed to parse entry.", true);
        return false;
    }

    val = parsedValue;
    return true;
}

bool MenuUtils::GetKbString(std::string& val) {
    UI::Notify("Enter name", true);
    MISC::DISPLAY_ONSCREEN_KEYBOARD(LOCALIZATION::GET_CURRENT_LANGUAGE() == 0, "FMMC_KEY_TIP8", "",
        std::format("{}", val).c_str(), "", "", "", 64);
    while (MISC::UPDATE_ONSCREEN_KEYBOARD() == 0) {
        WAIT(0);
    }
    if (!MISC::GET_ONSCREEN_KEYBOARD_RESULT()) {
        UI::Notify("Cancelled value entry", true);
        return false;
    }

    std::string entry = MISC::GET_ONSCREEN_KEYBOARD_RESULT();
    if (entry.empty()) {
        UI::Notify("Cancelled value entry", true);
        return false;
    }

    val = entry;
    return true;
}
