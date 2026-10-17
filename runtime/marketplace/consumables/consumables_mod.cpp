#include <Windows.h>
#include <bcrypt.h>
#include <ShlObj.h>
#include <MinHook.h>
#include <intrin.h>
#include <atomic>
#include <cstdio>
#include <string>
#include <vector>
#include "consumables_policy.h"
#include "consumables_targets.h"
#include "consumables_hooks.h"

namespace {
using namespace crimson::marketplace::consumables;
HMODULE module{};
std::atomic<bool> started=false;
std::atomic<bool> policy_enabled=false;
Address base{};
Settings settings{};
std::atomic<Use> original_use{};
std::atomic<Lookup> original_lookup{};
std::atomic<Pop> original_pop{};
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
    bool queue_enable(Address target) {return MH_QueueEnableHook(reinterpret_cast<void*>(target))==MH_OK;}
    bool apply() {return MH_ApplyQueued()==MH_OK;}
    bool disable(Address target) {
        const auto status=MH_DisableHook(reinterpret_cast<void*>(target));
        return status==MH_OK || status==MH_ERROR_DISABLED;
    }
    bool remove(Address target) {return MH_RemoveHook(reinterpret_cast<void*>(target))==MH_OK;}
    bool uninitialize() {return !owns_initialization || MH_Uninitialize()==MH_OK;}
} backend;
HookTransaction<Backend> hooks(backend,policy_enabled);

void log(const char* message) {
    wchar_t path[32768];
    if (!GetModuleFileNameW(module,path,32768)) return;
    std::wstring output(path);
    output=output.substr(0,output.find_last_of(L"\\/"))+L"\\CrimsonPreserveConsumables.log";
    FILE* file{};
    if (!_wfopen_s(&file,output.c_str(),L"a") && file) {
        std::fprintf(file,"%llu %s\n",GetTickCount64(),message); std::fclose(file);
    }
}
bool hash_matches(const wchar_t* path) {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,
                            OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
    if (file==INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE algorithm{}; BCRYPT_HASH_HANDLE hash{}; bool matches=false;
    if (BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0 &&
        BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0) {
        std::vector<unsigned char> buffer(1<<20); DWORD count{}; bool valid=true;
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
    CloseHandle(file); return matches;
}
Settings load_setting() {
    PWSTR folder{};
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,KF_FLAG_DEFAULT,nullptr,&folder))) return {};
    const std::wstring path=std::wstring(folder)+L"\\local.crimson.workbench\\preserve-consumables.dat";
    CoTaskMemFree(folder);
    HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,
                            OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if (file==INVALID_HANDLE_VALUE) return {};
    BY_HANDLE_FILE_INFORMATION info{};
    unsigned char bytes[8+kItems.size()*4+1]{}; DWORD count{};
    const bool valid=GetFileInformationByHandle(file,&info) &&
        !(info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY)) &&
        !info.nFileSizeHigh && info.nFileSizeLow>=8 && info.nFileSizeLow<sizeof(bytes) && info.nNumberOfLinks==1 &&
        ReadFile(file,bytes,sizeof(bytes),&count,nullptr);
    CloseHandle(file);
    return valid ? setting(bytes,count) : Settings{};
}
void* __fastcall on_use(void* inventory,void* result,void* actor,void* target,std::uint16_t handle,
    void* amount,std::uint16_t definition_handle,void* source,std::uint16_t slot_index,std::uint64_t extra) {
    const auto original=original_use.load(std::memory_order_acquire);
    if (!policy_enabled.load(std::memory_order_acquire))
        return original(inventory,result,actor,target,handle,amount,definition_handle,source,slot_index,extra);
    return dispatch_use(original,base,settings,inventory,result,actor,target,handle,amount,definition_handle,source,slot_index,extra);
}
void* __fastcall on_lookup(const std::uint32_t* handle) {
    const auto original=original_lookup.load(std::memory_order_acquire);
    if (!policy_enabled.load(std::memory_order_acquire)) return original(handle);
    const auto caller=reinterpret_cast<Address>(_ReturnAddress());
    return dispatch_lookup(original,caller,handle);
}
void* __fastcall on_pop(void* transaction,void* result,void* actor,const RequestVector* input,
    std::uint64_t time,std::uint8_t type,std::uint16_t reason,std::uint8_t flag,std::uint64_t scratch) {
    const auto original=original_pop.load(std::memory_order_acquire);
    if (!policy_enabled.load(std::memory_order_acquire))
        return original(transaction,result,actor,input,time,type,reason,flag,scratch);
    const auto caller=reinterpret_cast<Address>(_ReturnAddress());
    return dispatch_pop(original,caller,transaction,result,actor,input,time,type,reason,flag,scratch);
}
bool pins_match() {
    __try {
        for (const auto& pin:kPins)
            if (std::memcmp(reinterpret_cast<const void*>(base+pin.rva),pin.bytes,pin.length)) return false;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
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
        log("DISABLED: executable/hash or audited item-use/empty-transaction consumers differ; no hooks installed."); return 0;
    }
    settings=load_setting();
    if (!settings.count) {log("NORMAL: no supported items selected. Save a selection in Workbench before the next game start.");return 0;}
    const std::array<Address,3> callbacks{reinterpret_cast<Address>(&on_use),reinterpret_cast<Address>(&on_lookup),reinterpret_cast<Address>(&on_pop)};
    if (!hooks.install(base,callbacks,[](Address,std::size_t){return pins_match();},[](const std::array<Address,3>& trampolines) {
        original_use.store(reinterpret_cast<Use>(trampolines[0]),std::memory_order_release);
        original_lookup.store(reinterpret_cast<Lookup>(trampolines[1]),std::memory_order_release);
        original_pop.store(reinterpret_cast<Pop>(trampolines[2]),std::memory_order_release);return true;
    })) {
        log("DISABLED: hook transaction failed. Policy stays in passthrough; published resources retained after an activation attempt or failed disable.");return 0;
    }
    char ready[256]{};
    std::snprintf(ready,sizeof(ready),"READY: %u selected consumable types; owned selected-player stacks only. Native effects/cooldowns and other costs unchanged. Build 2976 only.",settings.count);
    log(ready);return 0;
}
void start() {
    if (started.exchange(true)) return;
    HANDLE thread=CreateThread(nullptr,0,initialize,nullptr,0,nullptr);
    if (thread) CloseHandle(thread);
}
} // namespace
extern "C" __declspec(dllexport) void InitializeASI() {start();}
BOOL APIENTRY DllMain(HMODULE handle,DWORD reason,LPVOID) {
    if (reason==DLL_PROCESS_ATTACH) {module=handle;DisableThreadLibraryCalls(handle);start();}
    return TRUE;
}
