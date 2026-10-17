#include <Windows.h>
#include <bcrypt.h>
#include <MinHook.h>
#include <intrin.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "auto_loot_native.h"
#include "auto_loot_targets.h"
#include "auto_loot_hooks.h"

namespace {
using namespace crimson::marketplace::auto_loot;
HMODULE module{};
Address base{};
std::atomic<bool> started{},policy_enabled{};
std::atomic<Nearby> original_nearby{};
Native native{0,{}};
Attempts attempts;
struct Backend {
    bool owns_initialization{};
    bool pin() {
        HMODULE pinned{};
        return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(module),&pinned)&&pinned==module;
    }
    bool initialize(){const auto status=MH_Initialize();owns_initialization=status==MH_OK;return owns_initialization||status==MH_ERROR_ALREADY_INITIALIZED;}
    bool create(Address target,Address callback,Address& trampoline) {
        void* result{};const auto status=MH_CreateHook(reinterpret_cast<void*>(target),reinterpret_cast<void*>(callback),&result);
        trampoline=reinterpret_cast<Address>(result);return status==MH_OK;
    }
    bool queue_enable(Address target){return MH_QueueEnableHook(reinterpret_cast<void*>(target))==MH_OK;}
    bool apply(){return MH_ApplyQueued()==MH_OK;}
    bool disable(Address target){const auto status=MH_DisableHook(reinterpret_cast<void*>(target));return status==MH_OK||status==MH_ERROR_DISABLED;}
    bool remove(Address target){return MH_RemoveHook(reinterpret_cast<void*>(target))==MH_OK;}
    bool uninitialize(){return !owns_initialization||MH_Uninitialize()==MH_OK;}
} hook_backend;
HookTransaction<Backend> hooks(hook_backend,policy_enabled);

void log(const char* message) {
    wchar_t path[32768];if(!GetModuleFileNameW(module,path,32768))return;
    std::wstring output(path);output=output.substr(0,output.find_last_of(L"\\/"))+L"\\CrimsonAutoLoot.log";
    FILE* file{};
    if(!_wfopen_s(&file,output.c_str(),L"a")&&file){std::fprintf(file,"%llu %s\n",GetTickCount64(),message);std::fclose(file);}
}
bool hash_matches(const wchar_t* path) {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};bool matches=false;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0&&
       BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0) {
        std::vector<unsigned char> bytes(1<<20);DWORD count{};bool valid=true;
        for(;;) {
            if(!ReadFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&count,nullptr)){valid=false;break;}
            if(!count)break;
            if(BCryptHashData(hash,bytes.data(),count,0)<0){valid=false;break;}
        }
        unsigned char digest[32]{};
        if(valid&&BCryptFinishHash(hash,digest,32,0)>=0) {
            char text[65]{};for(int i=0;i<32;++i)std::snprintf(text+2*i,3,"%02x",digest[i]);
            matches=!std::strcmp(text,kExeHash);
        }
    }
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    CloseHandle(file);return matches;
}
bool pins_match() noexcept {
    __try {
        for(const auto& pin:kPins)if(std::memcmp(reinterpret_cast<const void*>(base+pin.rva),pin.bytes.data(),pin.bytes.size()))return false;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void __fastcall on_nearby(void* field,void* actor) {
    dispatch_nearby(native,attempts,original_nearby.load(std::memory_order_acquire),policy_enabled.load(std::memory_order_acquire),
        reinterpret_cast<Address>(_ReturnAddress()),GetTickCount64(),field,actor);
}
DWORD WINAPI initialize(void*) {
    wchar_t path[32768];if(!GetModuleFileNameW(nullptr,path,32768))return 0;
    const auto* name=wcsrchr(path,L'\\');if(_wcsicmp(name?name+1:path,L"CrimsonDesert.exe"))return 0;
    base=reinterpret_cast<Address>(GetModuleHandleW(nullptr));
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    if(nt->FileHeader.TimeDateStamp!=1790086912||nt->OptionalHeader.SizeOfImage!=389722112||!hash_matches(path)||!pins_match()) {
        log("DISABLED: executable/hash or audited native pickup/lease pins differ; no hooks installed.");return 0;
    }
    native=Native(base,{reinterpret_cast<Definition>(base+0x3888b0),reinterpret_cast<Definition>(base+0x3885b0),
        reinterpret_cast<Check>(base+0x9b94b0),reinterpret_cast<CanPickUp>(base+0x8f5850),
        reinterpret_cast<Submit>(base+0xabf770),reinterpret_cast<Lock>(base+0x393df0),
        reinterpret_cast<Scan>(base+0x9ba050),reinterpret_cast<Resolve>(base+0x8ae940),reinterpret_cast<Release>(base+0x1434880),
        reinterpret_cast<FindInteraction>(base+0x2124060),reinterpret_cast<ClearInteractions>(base+0x9c0710),reinterpret_cast<CanLoot>(base+0x2144860),
        reinterpret_cast<Definition>(base+0x389570),reinterpret_cast<Definition>(base+0x3883d0),reinterpret_cast<Relationship>(base+0x2215750)});
    const std::array<Address,1> callbacks{reinterpret_cast<Address>(&on_nearby)};
    if(!hooks.install(base,callbacks,pins_match,[](const std::array<Address,1>& trampolines) {
        original_nearby.store(reinterpret_cast<Nearby>(trampolines[0]),std::memory_order_release);return true;
    })) {log("DISABLED: hook transaction failed; policy stays in passthrough and published resources are retained.");return 0;}
    log("READY: nearby free ground pickup and normal Dead_Loot/Simple/Swim corpses with native ownership/crime guards; native radius, fresh actor leases, normal eligibility/capacity/request path. Build 2976 only.");
    return 0;
}
void start() {
    if(started.exchange(true))return;
    // Pin before the asynchronous hash/init worker can outlive ASI startup.
    if(!hook_backend.pin())return;
    HANDLE thread=CreateThread(nullptr,0,initialize,nullptr,0,nullptr);if(thread)CloseHandle(thread);
}
} // namespace
extern "C" __declspec(dllexport) void InitializeASI(){start();}
BOOL APIENTRY DllMain(HMODULE handle,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH){module=handle;DisableThreadLibraryCalls(handle);start();}return TRUE;
}
