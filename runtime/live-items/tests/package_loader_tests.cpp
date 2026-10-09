#include <Windows.h>
#include <mmsystem.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include "../../extra-sockets/src/socket_io.h"

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
        for (const auto name : {L"CrimsonLiveItems", L"CrimsonExtraSockets"}) {
            const auto module = GetModuleHandleW((std::wstring(name) + L".asi").c_str());
            if (!module || !GetProcAddress(module, "InitializeASI")) { done = false; continue; }
            std::ifstream file(root / (std::wstring(name) + L".log"));
            const std::string log((std::istreambuf_iterator<char>(file)), {});
            if (log.find("READY") != std::string::npos || log.find("ARMED") != std::string::npos) return 2;
            if (log.find("DISABLED:") == std::string::npos) done = false;
        }
        if (done) {
            std::cout << "PASS: pinned winmm loader forwards and loads both ASIs; unsupported synthetic EXE rejected before hooks; tick=" << tick << "\n";
            return 0;
        }
        Sleep(20);
    }
    std::cerr << "Package loader smoke timed out\n";
    return 3;
}
