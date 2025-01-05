#include "Script.hpp"
#include "Constants.hpp"
#include "GitInfo.h"

#include "Memory/Versions.hpp"
#include "Util/Logger.hpp"
#include "Util/Paths.hpp"
#include <inc/main.h>

#include <Windows.h>
#include <Psapi.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace Dll {
    bool unloading = false;
}

bool Dll::Unloading() { return unloading; }


BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved) {
    const std::string modPath = Paths::GetModuleFolder(hInstance) + Constants::ModDir;
    const std::string logFile = modPath + "\\" + Paths::GetModuleNameWithoutExtension(hInstance) + ".log";

    if (!fs::is_directory(modPath) || !fs::exists(modPath)) {
        fs::create_directory(modPath);
    }

    gLogger.SetPath(logFile);
    gLogger.SetLogLevel(Debug);
    Paths::SetOurModuleHandle(hInstance);

    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            Dll::unloading = false;
            gLogger.Clear();
            int gameVersion = getGameVersion();
            LOG(Info, "GhostReplay {} (built {} {}) ({})",
                Constants::DisplayVersion, __DATE__, __TIME__, GIT_HASH GIT_DIFF);
            LOG(Info, "Game version: {} ({})", eGameVersionToString(gameVersion), gameVersion);

            scriptRegister(hInstance, GhostReplay::ScriptMain);

            LOG(Info, "Script registered");
            Dll::SetupHooks();
            break;
        }
        case DLL_PROCESS_DETACH: {
            Dll::unloading = true;
            GhostReplay::TriggerLoadStop();
            scriptUnregister(hInstance);
            Dll::ClearHooks();
            break;
        }
        default:
            // Do nothing
            break;
    }
    return TRUE;
}
