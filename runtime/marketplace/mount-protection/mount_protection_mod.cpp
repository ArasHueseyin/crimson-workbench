#include <Windows.h>
#include <bcrypt.h>
#include <MinHook.h>
#include <atomic>
#include <cstdio>
#include <string>
#include <vector>
#include "mount_protection_policy.h"
#include "mount_protection_lifecycle.h"
#include "mount_protection_targets.h"
#include "../blackstar/targets.h"

namespace {
using namespace crimson::marketplace::mount_protection;
HMODULE module{};
std::atomic<bool> started = false;
Address base{};
Delta original_delta{};

void log(const char* message) {
    wchar_t path[32768];
    if (!GetModuleFileNameW(module, path, 32768)) return;
    std::wstring output(path);
    output = output.substr(0, output.find_last_of(L"\\/")) + L"\\CrimsonMountProtection.log";
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
std::uintptr_t __fastcall on_delta(void* owner,std::uint16_t index,std::uint64_t time,std::int64_t delta,void* output) {
    return dispatch(original_delta,base,owner,index,time,delta,output);
}
bool code_matches() {
    for (const auto& code:kCodeSpans)
        if (std::memcmp(reinterpret_cast<const void*>(base+code.rva),code.bytes,code.count)) return false;
    return true;
}
struct NativeHook {
    void* target;
    bool pin_module() {
        HMODULE pinned{};
        return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&on_delta),&pinned) && pinned==module;
    }
    bool initialize() {return MH_Initialize()==MH_OK;}
    bool create() {
        return MH_CreateHook(target,reinterpret_cast<void*>(&on_delta),
                             reinterpret_cast<void**>(&original_delta))==MH_OK;
    }
    bool enable() {return MH_EnableHook(target)==MH_OK;}
};
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
        log("DISABLED: executable/hash or reviewed mount ownership/HP consumers differ; no hook installed.");
        return 0;
    }
    NativeHook native{reinterpret_cast<void*>(base+kDeltaRva)};
    switch (install_hook(native)) {
    case InstallResult::pin_failed:log("DISABLED: module lifetime could not be pinned; no hook created.");return 0;
    case InstallResult::initialize_failed:log("DISABLED: hook initialization failed.");return 0;
    case InstallResult::create_failed:log("DISABLED: hook creation failed.");return 0;
    case InstallResult::activation_failed:log("ACTIVATION ERROR: pinned module and published trampoline retained for potentially entered callbacks.");return 0;
    case InstallResult::ready:break;
    }
    log("READY: prevent negative HP deltas for the currently ridden mount or the selected summoned owned mount, including after dismount, with fresh client/server main-slot and full serial/actor ownership checks. Inactive registered animals, player health, healing and other resources retain native behavior. Build 2976 only.");
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
