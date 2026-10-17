#include "control_queue.h"
#include "control_pipe.h"
#include "world_native_targets.h"
#include "world_hooks.h"
#include <bcrypt.h>
#include <MinHook.h>
#include <atomic>
#include <cstdio>
#include <new>
#include <string>
#include <vector>

namespace {
using namespace crimson::marketplace::world_controls;
using WeatherTick=void(__fastcall*)(void*,float);
using WeatherClient=void(__fastcall*)(void*);
HMODULE module{};std::atomic<bool> started{},active{};
std::atomic<bool> hooks_ready{};
std::atomic<WeatherTick> original_weather{};
std::atomic<WeatherTick> original_secondary{};
std::atomic<WeatherClient> original_client{};
std::atomic<ControlQueue*> queue{};
ClockBindings clock_bindings{};WeatherBindings weather_bindings{};std::uintptr_t base{};
thread_local bool weather_entered{};
void log(const char* message) {
    wchar_t path[32768];if(!GetModuleFileNameW(module,path,32768))return;
    std::wstring output(path);output=output.substr(0,output.find_last_of(L"\\/"))+L"\\CrimsonWorldControls.log";
    FILE* file{};if(!_wfopen_s(&file,output.c_str(),L"a")&&file){std::fprintf(file,"%llu %s\n",GetTickCount64(),message);std::fclose(file);}
}
bool pin_module() noexcept {
    HMODULE pinned{};
    return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(module),&pinned)&&pinned==module;
}
struct HookBackend {
    bool owns{};
    bool pin(){return pin_module();}
    bool initialize(){const auto result=MH_Initialize();owns=result==MH_OK;return owns||result==MH_ERROR_ALREADY_INITIALIZED;}
    bool create(std::uintptr_t target,std::uintptr_t callback,std::uintptr_t& trampoline) {
        void* value{};const auto result=MH_CreateHook(reinterpret_cast<void*>(target),reinterpret_cast<void*>(callback),&value);trampoline=reinterpret_cast<std::uintptr_t>(value);return result==MH_OK;
    }
    bool enable(std::uintptr_t target){return MH_EnableHook(reinterpret_cast<void*>(target))==MH_OK;}
    bool disable(std::uintptr_t target){const auto result=MH_DisableHook(reinterpret_cast<void*>(target));return result==MH_OK||result==MH_ERROR_DISABLED;}
    bool remove(std::uintptr_t target){return MH_RemoveHook(reinterpret_cast<void*>(target))==MH_OK;}
    bool uninitialize(){return !owns||MH_Uninitialize()==MH_OK;}
} hook_backend;
WorldHookTransaction<HookBackend> hooks(hook_backend,hooks_ready);
bool hash_matches(const wchar_t* path) {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};bool matches=false;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0&&BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0) {
        std::vector<unsigned char> buffer(1<<20);DWORD count{};bool valid=true;
        for(;;){if(!ReadFile(file,buffer.data(),static_cast<DWORD>(buffer.size()),&count,nullptr)){valid=false;break;}if(!count)break;if(BCryptHashData(hash,buffer.data(),count,0)<0){valid=false;break;}}
        unsigned char digest[32]{};if(valid&&BCryptFinishHash(hash,digest,32,0)>=0){char value[65]{};for(unsigned i=0;i<32;++i)std::snprintf(value+2*i,3,"%02x",digest[i]);matches=!std::strcmp(value,kWorldExeHash);}
    }
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);CloseHandle(file);return matches;
}
bool pins_match() noexcept {
    __try {
        const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<=0||dos->e_lfanew>0x1000)return false;
        const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
        if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||nt->FileHeader.TimeDateStamp!=1790086912||nt->OptionalHeader.SizeOfImage!=389722112)return false;
        for(const auto& pin:kWorldCodePins)if(std::memcmp(reinterpret_cast<const void*>(base+pin.rva),pin.bytes,pin.length))return false;
        for(const auto& pin:kWorldTablePins)if(*reinterpret_cast<const std::uintptr_t*>(base+pin.slot)!=base+pin.target)return false;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
Reply handle(const Request& request,std::uint64_t now) {
    if(auto value=queue.load(std::memory_order_acquire))return value->handle(request,now);return {};
}
void __cdecl on_task(const crimson::live_items::TaskObservationV1* task) {
    if(!task||!active.load(std::memory_order_acquire))return;
    if(auto value=queue.load(std::memory_order_acquire))value->observe(clock_bindings,*task);
}
void __fastcall on_weather(void* manager,float delta) {
    const auto original=original_weather.load(std::memory_order_acquire);if(!original)return;
    WeatherEntryGuard entry(weather_entered);if(!entry.entered){original(manager,delta);return;}
    const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    original(manager,delta);
    if(active.load(std::memory_order_acquire))if(auto value=queue.load(std::memory_order_acquire))value->weather_producer(weather_bindings,manager,caller,GetTickCount64());
}
void __fastcall on_client(void* field) {
    const auto original=original_client.load(std::memory_order_acquire);if(!original)return;
    WeatherEntryGuard entry(weather_entered);if(!entry.entered){original(field);return;}
    const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    original(field);
    if(active.load(std::memory_order_acquire))if(auto value=queue.load(std::memory_order_acquire))value->weather_client(weather_bindings,field,caller,GetTickCount64());
}
void __fastcall on_secondary(void* manager,float delta) {
    const auto original=original_secondary.load(std::memory_order_acquire);if(!original)return;
    WeatherEntryGuard entry(weather_entered);if(!entry.entered){original(manager,delta);return;}
    const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    original(manager,delta);
    if(active.load(std::memory_order_acquire))if(auto value=queue.load(std::memory_order_acquire))value->weather_producer(weather_bindings,manager,caller,GetTickCount64());
}
bool install_weather_hooks() noexcept {
    return hooks.install(base,{reinterpret_cast<std::uintptr_t>(&on_weather),reinterpret_cast<std::uintptr_t>(&on_secondary),reinterpret_cast<std::uintptr_t>(&on_client)},pins_match,[](const auto& trampolines) {
        original_weather.store(reinterpret_cast<WeatherTick>(trampolines[0]),std::memory_order_release);
        original_secondary.store(reinterpret_cast<WeatherTick>(trampolines[1]),std::memory_order_release);
        original_client.store(reinterpret_cast<WeatherClient>(trampolines[2]),std::memory_order_release);return true;
    });
}
DWORD WINAPI initialize(void*) {
    wchar_t path[32768];if(!GetModuleFileNameW(nullptr,path,32768))return 0;
    const auto name=wcsrchr(path,L'\\');if(_wcsicmp(name?name+1:path,L"CrimsonDesert.exe"))return 0;
    base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    if(!hash_matches(path)||!pins_match()){log("DISABLED: original build hash or native ownership/publication witnesses differ.");return 0;}
    std::uint64_t session{};
    if(BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&session),sizeof(session),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0||!session){log("DISABLED: no control session entropy.");return 0;}
    auto value=new(std::nothrow) ControlQueue(session);if(!value)return 0;
    clock_bindings=clock_bindings_2976(base,true);weather_bindings=weather_bindings_2976(base,true);queue.store(value,std::memory_order_release);
    // The existing item runtime owns 0x2774b20. Registration adds no second
    // hook there, and the broker supplies the fresh server actor lease.
    crimson::live_items::RegisterTaskObserverV1 registration{};
    for(unsigned attempt=0;attempt<50&&!registration;++attempt) {
        if(auto items=GetModuleHandleW(L"CrimsonLiveItems.asi"))registration=reinterpret_cast<crimson::live_items::RegisterTaskObserverV1>(GetProcAddress(items,crimson::live_items::kRegisterTaskObserverExportV1));
        if(!registration)Sleep(200);
    }
    if(!registration||!install_weather_hooks()||!registration(crimson::live_items::kTaskObserverAbiV1,sizeof(crimson::live_items::TaskObservationV1),on_task)) {
        log("DISABLED: leased task broker or weather hook activation unavailable; published trampolines retained for native passthrough.");return 0;
    }
    active.store(true,std::memory_order_release);
    log("READY: native field-time and owned weather controls registered; context and per-command receipts remain required. No live acceptance has been measured by offline builds.");
    serve_control_pipe(handle);return 0;
}
void start() {
    if(started.exchange(true)||!pin_module())return;
    HANDLE worker=CreateThread(nullptr,0,initialize,nullptr,0,nullptr);if(worker)CloseHandle(worker);
}
} // namespace
extern "C" __declspec(dllexport) void InitializeASI(){start();}
BOOL APIENTRY DllMain(HMODULE handle,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH){module=handle;DisableThreadLibraryCalls(handle);start();}return TRUE;
}
