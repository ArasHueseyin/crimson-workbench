#include <Windows.h>
#include <bcrypt.h>
#include <MinHook.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>
#include "cooldown_policy.h"

namespace {
constexpr char kExeHash[] = "57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7";
constexpr std::uint32_t kTimeStamp = 1790086912;
constexpr std::uint32_t kImageSize = 389722112;
constexpr std::uintptr_t kReadRva = 0x17e6a60;
constexpr std::uintptr_t kSetRva = 0x17e6b30;
constexpr unsigned char kReadBytes[] = {0x48,0x89,0x5c,0x24,0x10,0x57,0x48,0x83,0xec,0x40,0x48,0x8b,0xd9,0x33,0xff,0x48,0x89,0x7c,0x24,0x20,0x48,0x89,0x7c,0x24,0x28,0x89,0x7c,0x24,0x30,0x40,0x88,0x7c};
constexpr unsigned char kSetBytes[] = {0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x50,0x8b,0xfa,0x48,0x8b,0xd9,0x33,0xf6,0x48,0x89,0x74,0x24,0x30,0x48,0x89,0x74,0x24,0x38};
using ReadFn = std::uint8_t(__fastcall*)(void*);
using SetFn = void(__fastcall*)(void*, std::uint32_t);
ReadFn original_read = nullptr;
SetFn original_set = nullptr;
std::mutex state_mutex;
std::atomic<bool> started = false;
HMODULE our_module = nullptr;

void log(const char* message) {
    wchar_t path[32768];
    if (!GetModuleFileNameW(our_module, path, 32768)) return;
    std::wstring name(path);
    name = name.substr(0, name.find_last_of(L"\\/")) + L"\\CrimsonSleepCooldown.log";
    FILE* f = nullptr;
    if (_wfopen_s(&f, name.c_str(), L"a") == 0 && f) {
        std::fprintf(f, "%llu %s\n", GetTickCount64(), message);
        std::fclose(f);
    }
}

bool hash_matches(const wchar_t* path) {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    bool ok = false;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0 &&
        BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0) >= 0) {
        std::vector<unsigned char> buffer(1 << 20);
        DWORD n = 0; bool read_ok = true;
        for (;;) {
            if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &n, nullptr)) { read_ok = false; break; }
            if (!n) break;
            if (BCryptHashData(hash, buffer.data(), n, 0) < 0) { read_ok = false; break; }
        }
        unsigned char digest[32];
        if (read_ok && BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0) {
            char text[65]{};
            for (int i = 0; i < 32; ++i) std::sprintf(text + i*2, "%02x", digest[i]);
            ok = std::strcmp(text, kExeHash) == 0;
        }
    }
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    CloseHandle(file);
    return ok;
}

std::uint8_t __fastcall hooked_read(void* self) {
    std::lock_guard lock(state_mutex);
    return check_sleep(
        [=] { return original_read(self); },
        [=] { original_set(self, 0); });
}

void __fastcall hooked_set(void* self, std::uint32_t duration) {
    std::lock_guard lock(state_mutex);
    // No additional real-time delay. Retain the original setter's side effects.
    (void)duration;
    original_set(self, 0);
}

DWORD WINAPI initialize(void*) {
    wchar_t exe[32768];
    if (!GetModuleFileNameW(nullptr, exe, 32768) ||
        _wcsicmp(wcsrchr(exe, L'\\') ? wcsrchr(exe, L'\\') + 1 : exe, L"CrimsonDesert.exe") != 0) return 0;
    auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->FileHeader.TimeDateStamp != kTimeStamp || nt->OptionalHeader.SizeOfImage != kImageSize ||
        !hash_matches(exe) || std::memcmp(reinterpret_cast<void*>(base+kReadRva),kReadBytes,sizeof(kReadBytes)) ||
        std::memcmp(reinterpret_cast<void*>(base+kSetRva),kSetBytes,sizeof(kSetBytes))) {
        log("DISABLED: executable version/hash or target code differs; no hooks installed."); return 0;
    }
    auto read_target = reinterpret_cast<void*>(base+kReadRva);
    auto set_target = reinterpret_cast<void*>(base+kSetRva);
    if (MH_Initialize() != MH_OK) { log("DISABLED: MinHook initialization failed."); return 0; }
    bool made_read = MH_CreateHook(read_target, reinterpret_cast<void*>(&hooked_read), reinterpret_cast<void**>(&original_read)) == MH_OK;
    bool made_set = made_read && MH_CreateHook(set_target, reinterpret_cast<void*>(&hooked_set), reinterpret_cast<void**>(&original_set)) == MH_OK;
    if (!made_set || MH_QueueEnableHook(read_target) != MH_OK || MH_QueueEnableHook(set_target) != MH_OK || MH_ApplyQueued() != MH_OK) {
        if (made_read) { MH_DisableHook(read_target); MH_RemoveHook(read_target); }
        if (made_set) { MH_DisableHook(set_target); MH_RemoveHook(set_target); }
        log("DISABLED: hooks could not be installed together."); return 0;
    }
    log("READY: no sleep/wait cooldown; native validity checks preserved; build 2976.");
    return 0;
}
void start() {
    if (started.exchange(true)) return;
    HANDLE thread = CreateThread(nullptr, 0, initialize, nullptr, 0, nullptr);
    if (thread) CloseHandle(thread);
}
}

extern "C" __declspec(dllexport) void InitializeASI() { start(); }
BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        our_module = module;
        DisableThreadLibraryCalls(module);
        start();
    }
    return TRUE;
}
