#include <Windows.h>
#include <bcrypt.h>
#include <ShlObj.h>
#include <MinHook.h>
#include <intrin.h>
#include <atomic>
#include <cstdio>
#include <string>
#include <vector>
#include "player_speed_policy.h"
#include "player_speed_targets.h"
#include "player_speed_settings.h"
#include "../blackstar/targets.h"

namespace {
using namespace crimson::marketplace::player_speed;
HMODULE module{};
std::atomic<bool> started = false;
Address base{};
Step original_step{};
std::uint32_t speed_percent = 100;

void log(const char* message) {
    wchar_t path[32768];
    if (!GetModuleFileNameW(module, path, 32768)) return;
    std::wstring output(path);
    output = output.substr(0, output.find_last_of(L"\\/")) + L"\\CrimsonPlayerSpeed.log";
    FILE* file{};
    if (!_wfopen_s(&file, output.c_str(), L"a") && file) {
        std::fprintf(file, "%llu %s\n", GetTickCount64(), message);
        std::fclose(file);
    }
}
bool hash_matches(const wchar_t* path) {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
                             nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE algorithm{};
    BCRYPT_HASH_HANDLE hash{};
    bool matches = false;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0 &&
        BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) >= 0) {
        std::vector<unsigned char> buffer(1 << 20);
        DWORD count{};
        bool valid = true;
        for (;;) {
            if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr)) { valid = false; break; }
            if (!count) break;
            if (BCryptHashData(hash, buffer.data(), count, 0) < 0) { valid = false; break; }
        }
        unsigned char digest[32]{};
        if (valid && BCryptFinishHash(hash, digest, 32, 0) >= 0) {
            char text[65]{};
            for (int i = 0; i < 32; ++i) std::snprintf(text + 2 * i, 3, "%02x", digest[i]);
            matches = !std::strcmp(text, kExeHash);
        }
    }
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file);
    return matches;
}
std::uint32_t load_setting() {
    PWSTR folder{};
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &folder))) return 100;
    const std::wstring path = std::wstring(folder) + L"\\local.crimson.workbench\\player-speed.dat";
    CoTaskMemFree(folder);
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr,
                              OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE) return 100;
    BY_HANDLE_FILE_INFORMATION info{};
    unsigned char bytes[9]{};
    DWORD count{};
    const bool valid = GetFileInformationByHandle(file, &info) &&
        !(info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) &&
        info.nFileSizeHigh == 0 && info.nFileSizeLow == 8 && info.nNumberOfLinks == 1 &&
        ReadFile(file, bytes, sizeof(bytes), &count, nullptr);
    CloseHandle(file);
    return valid ? setting(bytes, count) : 100;
}
std::uintptr_t __fastcall on_step(void* controller, float dt, const float* velocity, std::uintptr_t flag,
                        void* output, std::uintptr_t arg6, std::uintptr_t arg7) {
    const auto caller = reinterpret_cast<Address>(_ReturnAddress());
    return dispatch(original_step, base, caller, speed_percent, controller, dt, velocity, flag, output, arg6, arg7);
}
bool code_matches() {
    for (const auto& code : kCodeSpans)
        if (std::memcmp(reinterpret_cast<const void*>(base + code.rva), code.bytes, code.count)) return false;
    return true;
}
DWORD WINAPI initialize(void*) {
    wchar_t path[32768];
    if (!GetModuleFileNameW(nullptr, path, 32768)) return 0;
    const auto name = wcsrchr(path, L'\\');
    if (_wcsicmp(name ? name + 1 : path, L"CrimsonDesert.exe")) return 0;
    base = reinterpret_cast<Address>(GetModuleHandleW(nullptr));
    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->FileHeader.TimeDateStamp != 1790086912 || nt->OptionalHeader.SizeOfImage != 389722112 ||
        !hash_matches(path) || !code_matches()) {
        log("DISABLED: executable/hash or reviewed player locomotion consumers differ; no hook installed.");
        return 0;
    }
    speed_percent = load_setting();
    if (speed_percent == 100) { log("NORMAL: player movement speed 100%; no hook installed. Save a higher setting in Workbench before the next game start."); return 0; }
    if (MH_Initialize() != MH_OK) { log("DISABLED: hook initialization failed."); return 0; }
    auto target = reinterpret_cast<void*>(base + kStepRva);
    if (MH_CreateHook(target, reinterpret_cast<void*>(&on_step), reinterpret_cast<void**>(&original_step)) != MH_OK) {
        log("DISABLED: hook creation failed."); return 0;
    }
    if (MH_EnableHook(target) != MH_OK) { MH_RemoveHook(target); log("DISABLED: hook activation failed."); return 0; }
    char ready[256]{};
    std::snprintf(ready, sizeof(ready), "READY: selected player ground locomotion %u%%. Original vertical velocity, gravity, time step, airborne movement and mounts preserved. Build 2976 only.", speed_percent);
    log(ready);
    return 0;
}
void start() {
    if (started.exchange(true)) return;
    HANDLE thread = CreateThread(nullptr, 0, initialize, nullptr, 0, nullptr);
    if (thread) CloseHandle(thread);
}
} // namespace
extern "C" __declspec(dllexport) void InitializeASI() { start(); }
BOOL APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) { module = handle; DisableThreadLibraryCalls(handle); start(); }
    return TRUE;
}
