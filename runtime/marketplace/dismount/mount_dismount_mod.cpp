#include <Windows.h>
#include <bcrypt.h>
#include <MinHook.h>
#include <atomic>
#include <cstdio>
#include <string>
#include <vector>
#include "mount_dismount_policy.h"
#include "mount_dismount_hooks.h"
#include "../blackstar/targets.h"

namespace {
using namespace crimson::marketplace::dismount;
HMODULE module{};
std::atomic<bool> started=false,enabled=false;
std::atomic<Handler> original_handler{};
Address base{};
struct Backend {
    bool owns_initialization{};
    bool pin() {
        HMODULE pinned{};
        return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(module),&pinned) && pinned==module;
    }
    bool initialize() {
        const auto status=MH_Initialize();owns_initialization=status==MH_OK;
        return owns_initialization || status==MH_ERROR_ALREADY_INITIALIZED;
    }
    bool create(Address target,Address callback,Address& trampoline) {
        void* value{};
        const auto status=MH_CreateHook(reinterpret_cast<void*>(target),reinterpret_cast<void*>(callback),&value);
        trampoline=reinterpret_cast<Address>(value);return status==MH_OK;
    }
    bool enable(Address target) {return MH_EnableHook(reinterpret_cast<void*>(target))==MH_OK;}
    bool disable(Address target) {
        const auto status=MH_DisableHook(reinterpret_cast<void*>(target));
        return status==MH_OK || status==MH_ERROR_DISABLED;
    }
    bool remove(Address target) {return MH_RemoveHook(reinterpret_cast<void*>(target))==MH_OK;}
    bool uninitialize() {return !owns_initialization || MH_Uninitialize()==MH_OK;}
} backend;
HookTransaction<Backend> hooks(backend,enabled);
void log(const char* message) {
    wchar_t path[32768];
    if (!GetModuleFileNameW(module,path,32768)) return;
    std::wstring output(path);output=output.substr(0,output.find_last_of(L"\\/"))+L"\\CrimsonMountDismount.log";
    FILE* file{};
    if (!_wfopen_s(&file,output.c_str(),L"a") && file) {
        std::fprintf(file,"%llu %s\n",GetTickCount64(),message);std::fclose(file);
    }
}
bool hash_matches(const wchar_t* path) {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,
        OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
    if (file==INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};bool matches=false;
    if (BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0 &&
        BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0) {
        std::vector<unsigned char> buffer(1<<20);DWORD count{};bool valid=true;
        for (;;) {
            if (!ReadFile(file,buffer.data(),static_cast<DWORD>(buffer.size()),&count,nullptr)) {valid=false;break;}
            if (!count) break;
            if (BCryptHashData(hash,buffer.data(),count,0)<0) {valid=false;break;}
        }
        unsigned char digest[32]{};
        if (valid && BCryptFinishHash(hash,digest,32,0)>=0) {
            char text[65]{};
            for (int i=0;i<32;++i) std::snprintf(text+2*i,3,"%02x",digest[i]);
            matches=!std::strcmp(text,kExeHash);
        }
    }
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm,0);
    CloseHandle(file);return matches;
}
bool pins_match() {
    __try {
        for (const auto& pin:kCode)
            if (std::memcmp(reinterpret_cast<const void*>(base+pin.rva),pin.bytes,pin.size)) return false;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
std::uint8_t __fastcall on_height(void* control,const void* reference,void* shared,void* context,
    std::uint8_t mode,void* flag,void* result) {
    return dispatch(original_handler.load(std::memory_order_acquire),base,control,reference,shared,context,
        mode,flag,result,enabled.load(std::memory_order_acquire));
}
DWORD WINAPI initialize(void*) {
    wchar_t path[32768];
    if (!GetModuleFileNameW(nullptr,path,32768)) return 0;
    const auto name=wcsrchr(path,L'\\');
    if (_wcsicmp(name?name+1:path,L"CrimsonDesert.exe")) return 0;
    base=reinterpret_cast<Address>(GetModuleHandleW(nullptr));
    const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    if (nt->FileHeader.TimeDateStamp!=1790086912 || nt->OptionalHeader.SizeOfImage!=389722112 ||
        !hash_matches(path) || !pins_match()) {
        log("DISABLED: executable/hash or audited fall comparison/context differ; no hook installed.");return 0;
    }
    if (!hooks.install(base,reinterpret_cast<Address>(&on_height),pins_match,[](Address trampoline) {
        original_handler.store(reinterpret_cast<Handler>(trampoline),std::memory_order_release);return true;
    })) {
        log("DISABLED: condition hook transaction failed; native passthrough and published trampoline lifetime preserved.");return 0;
    }
    log("READY: selected lion/Rokade automatic fall threshold x5 (5 to 25) in two verified ridden-package programs. Native ground/time guards and manual/combat/death removal retained. Build 2976; gameplay acceptance pending.");
    return 0;
}
void start() {
    if (started.exchange(true)) return;
    // Close the interval before the worker reaches installation: an unload
    // during executable hashing must not remove its executing module either.
    if (!backend.pin()) return;
    HANDLE thread=CreateThread(nullptr,0,initialize,nullptr,0,nullptr);
    if (thread) CloseHandle(thread);
}
} // namespace
extern "C" __declspec(dllexport) void InitializeASI() {start();}
BOOL APIENTRY DllMain(HMODULE handle,DWORD reason,LPVOID) {
    if (reason==DLL_PROCESS_ATTACH) {module=handle;DisableThreadLibraryCalls(handle);start();}
    return TRUE;
}
