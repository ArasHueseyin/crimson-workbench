#include <Windows.h>
#include <bcrypt.h>
#include <ShlObj.h>
#include <MinHook.h>
#include <atomic>
#include <cstdio>
#include <string>
#include <vector>
#include "stamina_policy.h"
#include "stamina_targets.h"
#include "../spirit/resource_file.h"
#include "../blackstar/targets.h"

namespace {
using namespace crimson::marketplace::stamina;
namespace resources = crimson::marketplace::resources;
HMODULE module{};
std::atomic<bool> started = false;
Address base{};
Commit original_commit{};
std::uint32_t resource_flags = resources::kLegacyDefault;

void log(const char* message) {
    wchar_t path[32768];
    if (!GetModuleFileNameW(module, path, 32768)) return;
    std::wstring output(path);
    output = output.substr(0, output.find_last_of(L"\\/")) + L"\\CrimsonPlayerStamina.log";
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
std::uint64_t __fastcall on_commit(void* entry, std::uint64_t time, std::int64_t target, std::uint16_t flag) {
    return dispatch_commit(original_commit, base, entry, time, target, flag, resource_flags);
}
std::uint32_t load_setting() {
    PWSTR folder{};
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &folder))) return 0;
    const std::wstring directory = std::wstring(folder) + L"\\local.crimson.workbench";
    CoTaskMemFree(folder);
    return resources::load_setting_from_directory(directory);
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
        !hash_matches(path) || std::memcmp(reinterpret_cast<void*>(base + kCommitRva), kCommitPrologue, sizeof(kCommitPrologue)) ||
        std::memcmp(reinterpret_cast<void*>(base + kBoundsRva), kBounds, sizeof(kBounds)) ||
        std::memcmp(reinterpret_cast<void*>(base + 0xc7eff12), kCommitStores, sizeof(kCommitStores))) {
        log("DISABLED: executable/hash or resource commit differs; no hook installed.");
        return 0;
    }
    resource_flags = load_setting();
    if (!resource_flags) { log("DISABLED: normal stamina and spirit selected, or an existing setting is invalid/unsafe; no hook installed."); return 0; }
    HMODULE pinned{};
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                          reinterpret_cast<LPCWSTR>(&on_commit), &pinned) || pinned != module) {
        log("DISABLED: module lifetime could not be pinned; no hook created."); return 0;
    }
    if (MH_Initialize() != MH_OK) { log("DISABLED: hook initialization failed."); return 0; }
    auto target = reinterpret_cast<void*>(base + kCommitRva);
    if (MH_CreateHook(target, reinterpret_cast<void*>(&on_commit), reinterpret_cast<void**>(&original_commit)) != MH_OK) {
        log("DISABLED: hook creation failed."); return 0;
    }
    if (MH_EnableHook(target) != MH_OK) { log("ACTIVATION ERROR: pinned module and published trampoline retained for potentially entered callbacks."); return 0; }
    char message[512]{};
    std::snprintf(message, sizeof(message), "READY: selected-player resource protection, stamina=%s spirit=%s. One native commit hook; current positive values retained on decreases within native bounds. Grants, regeneration, zero resources and unrelated actors/resources retain native behavior. Build 2976 only.",
                  resource_flags & resources::kStamina ? "protected" : "normal", resource_flags & resources::kSpirit ? "protected" : "normal");
    log(message);
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
