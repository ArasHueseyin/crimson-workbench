#include "photo_policy.h"
#include "photo_pipe.h"
#include "photo_targets.h"
#include "photo_hooks.h"
#include "../../repair/src/hash.h"
#include <MinHook.h>
#include <bcrypt.h>
#include <intrin.h>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <new>

namespace {
using namespace crimson::marketplace::photo;
HMODULE module{};Address base{};
std::atomic<bool> started{},enabled{};
std::atomic<FrameUpdate> original_update{};
Native native{0,{}};std::atomic<Policy*> policy{};
struct Backend {
    bool owns_initialization{};
    bool pin(){HMODULE pinned{};return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(module),&pinned)&&pinned==module;}
    bool initialize(){auto s=MH_Initialize();owns_initialization=s==MH_OK;return owns_initialization||s==MH_ERROR_ALREADY_INITIALIZED;}
    bool create(Address target,Address callback,Address& trampoline){void* value{};auto s=MH_CreateHook(reinterpret_cast<void*>(target),reinterpret_cast<void*>(callback),&value);trampoline=reinterpret_cast<Address>(value);return s==MH_OK;}
    bool queue_enable(Address target){return MH_QueueEnableHook(reinterpret_cast<void*>(target))==MH_OK;}
    bool apply(){return MH_ApplyQueued()==MH_OK;}
    bool disable(Address target){auto s=MH_DisableHook(reinterpret_cast<void*>(target));return s==MH_OK||s==MH_ERROR_DISABLED;}
    bool remove(Address target){return MH_RemoveHook(reinterpret_cast<void*>(target))==MH_OK;}
    bool uninitialize(){return !owns_initialization||MH_Uninitialize()==MH_OK;}
} backend;
HookTransaction<Backend> hooks(backend,enabled);
void log(const char* message) {
    wchar_t path[32768];if(!GetModuleFileNameW(module,path,32768))return;
    auto end=wcsrchr(path,L'\\');if(!end)return;wcscpy_s(end+1,32768-(end+1-path),L"CrimsonPhotoMode.log");
    FILE* file{};if(!_wfopen_s(&file,path,L"a")&&file){std::fprintf(file,"%llu %s\n",GetTickCount64(),message);std::fclose(file);}
}
bool image_pins() noexcept {
    __try {
        auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<=0||dos->e_lfanew>0x1000)return false;
        auto nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
        if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||nt->FileHeader.TimeDateStamp!=1790086912||nt->OptionalHeader.SizeOfImage!=389722112)return false;
        for(auto pin:kTablePins)if(*reinterpret_cast<const Address*>(base+pin.slot)!=base+pin.target)return false;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool routine_pins() {
    if(!image_pins())return false;
    for(auto pin:kCodePins)if(crimson::repair::hash(std::span(reinterpret_cast<const std::uint8_t*>(base+pin.rva),pin.size))!=pin.hash)return false;
    return true;
}
void __fastcall update(void* view) {
    const auto original=original_update.load(std::memory_order_acquire);
    if(auto value=policy.load(std::memory_order_acquire))dispatch(native,*value,original,enabled.load(std::memory_order_acquire),base,reinterpret_cast<Address>(_ReturnAddress()),0,view,[]()->std::uint64_t{return GetTickCount64();});
    else if(original)original(view);
}
Reply handle(const Request& request,std::uint64_t now){auto value=policy.load(std::memory_order_acquire);return value?value->handle(request,now):Reply{};}
DWORD WINAPI initialize(void*) {
    try {
        wchar_t path[32768];if(!GetModuleFileNameW(nullptr,path,32768))return 0;
        auto name=wcsrchr(path,L'\\');if(_wcsicmp(name?name+1:path,L"CrimsonDesert.exe"))return 0;
        base=reinterpret_cast<Address>(GetModuleHandleW(nullptr));
        std::ifstream executable(std::filesystem::path(path),std::ios::binary);
        if(!executable||crimson::repair::hash_stream(executable)!=kExeHash||!routine_pins()){log("DISABLED: executable or audited native photo/UI lease routines differ.");return 0;}
        std::uint64_t session{};if(BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&session),sizeof(session),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0||!session)return 0;
        auto value=new(std::nothrow) Policy(session);if(!value)return 0;
        native=Native(base,{reinterpret_cast<Lookup>(base+0x35da2a0),reinterpret_cast<GetView>(base+0x3e72bb0),
            reinterpret_cast<TimeScale>(base+0x35cd990),reinterpret_cast<Enter>(base+0x10ddad0),
            reinterpret_cast<UiAction>(base+0x10df010),reinterpret_cast<UiAction>(base+0x10dee30),reinterpret_cast<Input>(base+0x10de190)});
        policy.store(value,std::memory_order_release);
        if(!hooks.install(base,{reinterpret_cast<Address>(&update)},routine_pins,[](const std::array<Address,1>& values){original_update.store(reinterpret_cast<FrameUpdate>(values[0]),std::memory_order_release);return true;})) {
            log("DISABLED: hook transaction failed; passthrough resources retained.");return 0;
        }
        log("READY: explicit normal PhotoCamera entry/exit, native HUD controls and UI scene pause. Native queued is not observed activation. Offline verification only; no live gameplay acceptance claimed.");
        serve_photo_pipe(handle);
    } catch(...){log("DISABLED: photo initialization or pointer-free pipe failed.");}
    return 0;
}
void start(){if(started.exchange(true)||!backend.pin())return;auto worker=CreateThread(nullptr,0,initialize,nullptr,0,nullptr);if(worker)CloseHandle(worker);}
}
extern "C" __declspec(dllexport) void InitializeASI(){start();}
BOOL APIENTRY DllMain(HMODULE value,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){module=value;DisableThreadLibraryCalls(value);start();}return TRUE;}
