#include <Windows.h>
#include <mmsystem.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include "../../extra-sockets/src/socket_io.h"
#include "../src/task_observer.h"

namespace {
unsigned observer_calls{};
void __cdecl on_task(const crimson::live_items::TaskObservationV1*) {++observer_calls;}
}

// This executable is synthetic. Neither a game installation nor its bytes are
// needed: the real loader must load our ASIs and each must reject this EXE.
int main() {
    const auto tick = timeGetTime();
    wchar_t path[32768];
    if (!GetModuleFileNameW(nullptr, path, 32768)) return 1;
    const auto root = std::filesystem::path(path).parent_path();
    extra::Config config{};
    if (!extra::load((root / L"CrimsonExtraSockets.dat").wstring(), config)
        || config.version != 2 || config.count != 0) {
        std::cerr << "Package configuration is not a valid empty V2 file\n";
        return 4;
    }
    for (unsigned i = 0; i < 250; ++i) {
        bool done = true;
        for (const auto name : {L"CrimsonLiveItems", L"CrimsonExtraSockets", L"CrimsonBlackstar", L"CrimsonSleepCooldown", L"CrimsonSleepDuration", L"CrimsonNoFall", L"CrimsonPlayerStamina", L"CrimsonMountJump", L"CrimsonPlayerJump", L"CrimsonPlayerSpeed", L"CrimsonPreserveConsumables", L"CrimsonMountProtection", L"CrimsonFreeCrafting", L"CrimsonForcePalm", L"CrimsonForcePalmHeight", L"CrimsonAxiomReach", L"CrimsonGlideSpeed", L"CrimsonMountDismount", L"CrimsonAutoLoot", L"CrimsonWorldControls", L"CrimsonPhotoMode", L"CrimsonTeleport"}) {
            const auto module = GetModuleHandleW((std::wstring(name) + L".asi").c_str());
            if (!module || !GetProcAddress(module, "InitializeASI")) { done = false; continue; }
            std::ifstream file(root / (std::wstring(name) + L".log"));
            const std::string log((std::istreambuf_iterator<char>(file)), {});
            if (log.find("READY") != std::string::npos || log.find("ARMED") != std::string::npos) return 2;
            if (log.find("DISABLED:") == std::string::npos) done = false;
        }
        if (done) {
            using namespace crimson::live_items;
            const auto live=GetModuleHandleW(L"CrimsonLiveItems.asi");
            const auto add=reinterpret_cast<RegisterTaskObserverV1>(GetProcAddress(live,kRegisterTaskObserverExportV1));
            if (!add || add(2,56,on_task) || add(1,55,on_task) || add(1,56,nullptr) ||
                !add(1,56,on_task) || !add(1,56,on_task) || observer_calls) return 5;
            std::cout << "PASS: pinned winmm loader forwards and loads all packaged ASIs; unsupported synthetic EXE rejected before hooks; tick=" << tick << "\n";
            return 0;
        }
        Sleep(20);
    }
    std::cerr << "Package loader smoke timed out\n";
    return 3;
}
