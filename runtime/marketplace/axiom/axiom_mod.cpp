#include <Windows.h>
#include <bcrypt.h>
#include <ShlObj.h>
#include <atomic>
#include <cstdio>
#include <string>
#include <vector>
#include "axiom_policy.h"
#include "axiom_targets.h"

namespace {
using namespace crimson::marketplace::axiom;
HMODULE module{};
std::atomic<bool> started{false};
struct Memory {
    template<class T> bool read(Address address, T& value) {
        __try { std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(value)); return true; }
        __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    bool compare_exchange(Address address, std::uint32_t before, std::uint32_t after) {
        __try {
            return static_cast<std::uint32_t>(InterlockedCompareExchange(
                reinterpret_cast<volatile LONG*>(address), static_cast<LONG>(after), static_cast<LONG>(before))) == before;
        } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
};
void log(const char* message) {
    wchar_t path[32768]{};
    const auto count = GetModuleFileNameW(module, path, 32768);
    if (!count || count >= 32768) return;
    std::wstring output(path);
    output = output.substr(0, output.find_last_of(L"\\/")) + L"\\CrimsonAxiomReach.log";
    FILE* file{};
    if (!_wfopen_s(&file, output.c_str(), L"a") && file) {
        std::fprintf(file, "%llu %s\n", GetTickCount64(), message); std::fclose(file);
    }
}
bool hash_matches(const wchar_t* path) {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE algorithm{}; BCRYPT_HASH_HANDLE hash{}; bool matches = false;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0 &&
        BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) >= 0) {
        std::vector<unsigned char> buffer(1 << 20); DWORD count{}; bool valid = true;
        for (;;) {
            if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr)) { valid = false; break; }
            if (!count) break;
            if (BCryptHashData(hash, buffer.data(), count, 0) < 0) { valid = false; break; }
        }
        unsigned char digest[32]{};
        if (valid && BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0) {
            char text[65]{};
            for (int i = 0; i < 32; ++i) std::snprintf(text + 2 * i, 3, "%02x", digest[i]);
            matches = !std::strcmp(text, kExeHash);
        }
    }
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file); return matches;
}
bool image_matches(Address base) {
    __try {
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
        const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
            nt->FileHeader.TimeDateStamp != 1790086912 || nt->OptionalHeader.SizeOfImage != 389722112) return false;
        for (const auto& pin : kPins)
            if (std::memcmp(reinterpret_cast<const void*>(base + pin.rva), pin.bytes, pin.length)) return false;
        for (const auto& init : kInitializers)
            if (*reinterpret_cast<const Address*>(base + init.cell) != base + init.entry) return false;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
std::uint32_t load_setting() {
    PWSTR folder{};
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &folder))) return 100;
    const std::wstring path = std::wstring(folder) + L"\\local.crimson.workbench\\axiom-reach.dat";
    CoTaskMemFree(folder);
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE) return 100;
    BY_HANDLE_FILE_INFORMATION info{}; unsigned char bytes[9]{}; DWORD count{};
    const bool valid = GetFileInformationByHandle(file, &info) &&
        !(info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) &&
        info.nFileSizeHigh == 0 && info.nFileSizeLow == 8 && info.nNumberOfLinks == 1 &&
        ReadFile(file, bytes, sizeof(bytes), &count, nullptr);
    CloseHandle(file); return valid ? setting(bytes, count) : 100;
}
DWORD WINAPI initialize(void*) {
    wchar_t path[32768]{};
    const auto count = GetModuleFileNameW(nullptr, path, 32768);
    if (!count || count >= 32768) return 0;
    const auto name = wcsrchr(path, L'\\');
    if (_wcsicmp(name ? name + 1 : path, L"CrimsonDesert.exe")) return 0;
    const auto percent = load_setting();
    const auto base = reinterpret_cast<Address>(GetModuleHandleW(nullptr));
    if (!hash_matches(path) || !image_matches(base)) {
        log("DISABLED: executable/hash or audited Axiom constructors/consumers differ; no game memory changed."); return 0;
    }
    if (percent == 100) { log("NORMAL: 1x Axiom reach; no game memory changed."); return 0; }
    log("WAITING: load a character. Axiom reach is applied once after native static initialization and tuning overrides have completed.");
    Memory memory;
    for (;;) {
        // The module is pinned before this worker starts. A main-menu wait has
        // no timeout: a late save load must not disable the requested setting.
        const auto result = apply(memory, base, percent);
        if (result == Result::Waiting) { Sleep(250); continue; }
        if (result == Result::Applied) {
            char message[256]{};
            std::snprintf(message, sizeof(message),
                "READY: shared Axiom reach %u%%; acquisition, drag and native release distance limits scaled. Default cells, non-distance rules, archives and saves retained. Build 2976 only.", percent);
            log(message);
        } else if (result == Result::RollbackConflict) {
            log("DISABLED: concurrent tuning edit prevented full rollback; foreign values were preserved. No automatic retry.");
        } else if (result == Result::RolledBack) {
            log("DISABLED: Axiom transaction failed; all owned changes rolled back. No automatic retry.");
        } else {
            log("DISABLED: Axiom tuning differs from the supported native baseline; no game memory changed. No automatic retry.");
        }
        return 0;
    }
}
void start() {
    if (started.exchange(true)) return;
    HMODULE pinned{};
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(module), &pinned) || pinned != module) return;
    HANDLE thread = CreateThread(nullptr, 0, initialize, nullptr, 0, nullptr);
    if (thread) CloseHandle(thread);
}
} // namespace
extern "C" __declspec(dllexport) void InitializeASI() { start(); }
BOOL APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) { module = handle; DisableThreadLibraryCalls(handle); start(); }
    return TRUE;
}
