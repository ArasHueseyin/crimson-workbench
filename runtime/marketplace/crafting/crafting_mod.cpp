#include <Windows.h>
#include <bcrypt.h>
#include <MinHook.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <new>
#include <string>
#include <vector>
#include "crafting_policy.h"
#include "crafting_hooks.h"
#include "crafting_shadow.h"
#include "crafting_shadow_callers.h"

namespace {
using namespace crimson::marketplace::crafting;
HMODULE module{};
Address base{};
std::atomic<bool> started{}, policy_enabled{};
using Lookup=void*(__fastcall*)(const std::uint16_t*);
std::atomic<Lookup> original_lookup{};
alignas(ShadowPool) unsigned char shadow_storage[sizeof(ShadowPool)]{};
ShadowPool* shadow_pool{};
void log(const char* text) {
    wchar_t path[32768]{};
    if (!GetModuleFileNameW(module,path,32768)) return;
    std::wstring file(path);
    file=file.substr(0,file.find_last_of(L"\\/"))+L"\\CrimsonFreeCrafting.log";
    FILE* output{};
    if (!_wfopen_s(&output,file.c_str(),L"a") && output) {
        std::fprintf(output,"%llu %s\n",GetTickCount64(),text);
        std::fclose(output);
    }
}
bool digest(BCRYPT_HASH_HANDLE hash,const char* expected) {
    unsigned char bytes[32]{};
    if (BCryptFinishHash(hash,bytes,sizeof(bytes),0)<0) return false;
    char hexadecimal[65]{};
    for(unsigned i=0;i<32;++i)std::snprintf(hexadecimal+2*i,3,"%02x",bytes[i]);
    return std::strcmp(hexadecimal,expected)==0;
}
bool file_matches(const wchar_t* path) {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};bool matches=false;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0 &&
        BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0) {
        std::vector<unsigned char> buffer(1<<20);DWORD count{};bool valid=true;
        for(;;) {
            if(!ReadFile(file,buffer.data(),static_cast<DWORD>(buffer.size()),&count,nullptr)){valid=false;break;}
            if(!count)break;
            if(BCryptHashData(hash,buffer.data(),count,0)<0){valid=false;break;}
        }
        if(valid)matches=digest(hash,kExecutableHash);
    }
    if(hash)BCryptDestroyHash(hash);
    if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    CloseHandle(file);
    return matches;
}
bool native_spans_match() {
    BCRYPT_ALG_HANDLE algorithm{};
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
    bool matches=true;
    for(const auto& span:kFingerprints) {
        BCRYPT_HASH_HANDLE hash{};
        if(BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)<0){matches=false;break;}
        const auto bytes=reinterpret_cast<unsigned char*>(base+span.rva);
        matches=BCryptHashData(hash,bytes,span.length,0)>=0 && digest(hash,span.sha256);
        BCryptDestroyHash(hash);
        if(!matches)break;
    }
    BCryptCloseAlgorithmProvider(algorithm,0);
    return matches;
}
Edit shadow_decision(Site site,const Context& context) noexcept {
    // Initialization always sees the original valid rows. No archive with
    // empty-input recipes is needed, so worker/DB startup ordering is irrelevant.
    if(site==Site::Database)return {};
    __try {
        Memory native;
        ShadowRead read(native,*shadow_pool,base);
        return decide(read,base,site,context);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return {}; }
}
Address shadow_lookup(const std::uint16_t* handle,Address original) noexcept {
    __try {
        Memory native; ShadowSource source;
        if(!handle || !capture_shadow_source(native,base,*handle,original,source))return original;
        const auto result=shadow_pool->view(source);
        return result?result:original;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return original; }
}
void* __fastcall on_lookup(const std::uint16_t* handle) {
    const auto original=original_lookup.load(std::memory_order_acquire);
    if(!original)__fastfail(FAST_FAIL_INVALID_ARG);
    // Preserve the native resolver's call and exception behavior. Only the
    // subsequent read-only adapter has a fault fallback, never the game call.
    const auto row=original(handle);
    if(!policy_enabled.load(std::memory_order_acquire) || !shadow_caller(base,reinterpret_cast<Address>(_ReturnAddress())))return row;
    return reinterpret_cast<void*>(shadow_lookup(handle,reinterpret_cast<Address>(row)));
}
void context_callback(Context& context,Site site) noexcept {
    // Until all five bridges AND the row lookup are active, all callbacks pass
    // through. Even a partial failure keeps the original valid recipe database.
    if(!policy_enabled.load(std::memory_order_acquire))return;
    const auto edit=shadow_decision(site,context);
    apply(context,edit);
}
struct Backend {
    bool initialize() noexcept {return MH_Initialize()==MH_OK;}
    bool create(Address target,Address entry,Address& trampoline) noexcept {
        void* original{};
        if(MH_CreateHook(reinterpret_cast<void*>(target),reinterpret_cast<void*>(entry),&original)!=MH_OK)return false;
        trampoline=reinterpret_cast<Address>(original);return true;
    }
    bool queue_enable(Address target) noexcept {return MH_QueueEnableHook(reinterpret_cast<void*>(target))==MH_OK;}
    bool apply() noexcept {return MH_ApplyQueued()==MH_OK;}
    bool disable(Address target) noexcept {
        const auto status=MH_DisableHook(reinterpret_cast<void*>(target));
        return status==MH_OK || status==MH_ERROR_DISABLED;
    }
    bool remove(Address target) noexcept {return MH_RemoveHook(reinterpret_cast<void*>(target))==MH_OK;}
    bool uninitialize() noexcept {return MH_Uninitialize()==MH_OK;}
};
Backend backend;
// ASI hooks cannot safely unload while an interrupted game frame can still be
// inside a bridge. The module is pinned before creating a hook. Do not register
// a static C++ destructor that would suspend threads inside loader-lock teardown.
alignas(HookTransaction<Backend>) unsigned char hook_storage[sizeof(HookTransaction<Backend>)]{};
HookTransaction<Backend>* hooks{};
DWORD WINAPI initialize(void*) {
    wchar_t path[32768]{};
    if(!GetModuleFileNameW(nullptr,path,32768))return 0;
    const auto name=wcsrchr(path,L'\\');
    if(_wcsicmp(name?name+1:path,L"CrimsonDesert.exe"))return 0;
    base=reinterpret_cast<Address>(GetModuleHandleW(nullptr));
    const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 || dos->e_lfanew>0x100000){log("DISABLED: invalid executable header.");return 0;}
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 ||
        nt->FileHeader.TimeDateStamp!=1790086912 || nt->OptionalHeader.SizeOfImage!=389722112 ||
        !file_matches(path) || !native_spans_match()) {
        log("DISABLED: build 2976 executable or native crafting witnesses differ; no hooks installed.");return 0;
    }
    HMODULE pinned{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&context_callback),&pinned)) {log("DISABLED: module lifetime could not be secured; no hooks installed.");return 0;}
    hooks=new(hook_storage)HookTransaction<Backend>(backend);
    shadow_pool=new(shadow_storage)ShadowPool;
    const bool installed=hooks->install(base,bridge_entries(),reinterpret_cast<Address>(&on_lookup),
        [](Address,std::size_t){return true;},[](BridgeConfiguration configuration,Address lookup){
        configuration.callback=context_callback;
        if(!configure_bridge(configuration))return false;
        original_lookup.store(reinterpret_cast<Lookup>(lookup),std::memory_order_release);
        return true;
    });
    if(!installed) {
        policy_enabled.store(false,std::memory_order_release);
        log(hooks->cleanup()?"DISABLED: native setup failed; owned hook resources removed.":
            "DISABLED: native setup failed; cleanup could not detach all owned hooks. Passthrough resources retained.");
        return 0;
    }
    policy_enabled.store(true,std::memory_order_release);
    log("STAGED: immutable ordinary-recipe views and all scoped crafting bridges active, build 2976. Original archive/manager rows and database validation unchanged; retained currency fees, prerequisites/results/native inventory validation preserved. In-game acceptance pending; experimental local module.");
    return 0;
}
void start() {
    if(started.exchange(true))return;
    HANDLE thread=CreateThread(nullptr,0,initialize,nullptr,0,nullptr);
    if(thread)CloseHandle(thread);
}
} // namespace
extern "C" __declspec(dllexport) void InitializeASI(){start();}
BOOL APIENTRY DllMain(HMODULE handle,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH){module=handle;DisableThreadLibraryCalls(handle);start();}
    return TRUE;
}
