#include "teleport_policy.h"
#include "teleport_pipe.h"
#include "teleport_store.h"
#include "teleport_targets.h"
#include "marker_targets.h"
#include "marker_factory.h"
#include "persistence_revision.h"
#include "../../live-items/src/native.h"
#include "../../repair/src/hash.h"
#include <bcrypt.h>
#include <atomic>
#include <cstdio>
#include <new>

namespace {
using namespace crimson::marketplace::teleport;
HMODULE module{};std::atomic<bool> started{},active{};
std::atomic<TeleportPolicy*> policy{};NativeBindings bindings{};MarkerBindings marker_bindings{};BookmarkStore* store{};
PersistenceRevision persistence;
void log(const char* text) {
    wchar_t path[32768];if(!GetModuleFileNameW(module,path,32768))return;
    auto end=wcsrchr(path,L'\\');if(!end)return;wcscpy_s(end+1,32768-(end+1-path),L"CrimsonTeleport.log");
    FILE* file{};if(!_wfopen_s(&file,path,L"a")&&file){std::fprintf(file,"%llu %s\n",GetTickCount64(),text);std::fclose(file);}
}
bool pin_module() noexcept {
    HMODULE pinned{};return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(module),&pinned)&&pinned==module;
}
bool image_pins(std::uintptr_t base) noexcept {
    __try {
        const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<=0||dos->e_lfanew>0x1000)return false;
        const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
        if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||nt->FileHeader.TimeDateStamp!=1790086912||nt->OptionalHeader.SizeOfImage!=389722112)return false;
        for(const auto& pin:kTablePins)if(*reinterpret_cast<const std::uintptr_t*>(base+pin.slot)!=base+pin.target)return false;
        for(const auto& pin:kMarkerTablePins)if(*reinterpret_cast<const std::uintptr_t*>(base+pin.slot)!=base+pin.target)return false;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool routine_pins(std::uintptr_t base) {
    for(const auto& pin:kCodePins)if(crimson::repair::hash(std::span(reinterpret_cast<const std::uint8_t*>(base+pin.rva),pin.bytes))!=pin.hash)return false;
    for(const auto& pin:kMarkerCodePins)if(crimson::repair::hash(std::span(reinterpret_cast<const std::uint8_t*>(base+pin.rva),pin.bytes))!=pin.hash)return false;
    return true;
}
NativeBindings supported_bindings(std::uintptr_t base) {
    NativeBindings b{};b.verified=true;b.base=base;b.actor_type=base+0x5b1e2b0;b.transform_type=base+0x5b2cf60;b.field_type=base+0x5b29a60;
    b.acquire=reinterpret_cast<AcquireField>(base+0x2bc4280);b.release=reinterpret_cast<ReleaseField>(base+0x2bc42c0);b.release_reference=reinterpret_cast<ReleaseReference>(base+0x1434880);
    b.stage=reinterpret_cast<GetStage>(base+0x2af7d10);b.stage_info=reinterpret_cast<GetStageInfo>(base+0x4a2010);b.position=reinterpret_cast<GetPosition>(base+0x4f8220);b.yaw=reinterpret_cast<GetYaw>(base+0x17e9170);
    b.read_lock=reinterpret_cast<ReadLock>(base+0x393df0);b.validate=reinterpret_cast<ValidateMove>(base+0x21a1580);b.begin=reinterpret_cast<BeginFieldMove>(base+0x2bbf630);b.realm=native_realm;b.selected=crimson::live_items::selected_identity;return b;
}
void __cdecl observe(const crimson::live_items::TaskObservationV1* task) {
    if(!task||!active.load(std::memory_order_acquire))return;
    if(auto value=policy.load(std::memory_order_acquire))value->observe(bindings,*task,&marker_bindings);
}
Reply handle(const Request& request,std::uint64_t now) {
    auto value=policy.load(std::memory_order_acquire);if(!value)return {};
    auto reply=value->handle(request,now);
    reply.storage_status=store?persistence.persist(reply.bookmarks_revision,[&]{return store->save(reply.bookmarks,kExeHash);}):2;
    return reply;
}
DWORD WINAPI initialize(void*) {
    try {
        wchar_t path[32768];if(!GetModuleFileNameW(nullptr,path,32768))return 0;
        const auto name=wcsrchr(path,L'\\');if(_wcsicmp(name?name+1:path,L"CrimsonDesert.exe"))return 0;
        const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
        std::ifstream executable(std::filesystem::path(path),std::ios::binary);
        if(!executable||crimson::repair::hash_stream(executable)!=kExeHash||!image_pins(base)||!routine_pins(base)){log("DISABLED: executable identity or native queue/position/lease pins differ.");return 0;}
        std::uint64_t session{};
        if(BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&session),sizeof(session),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0||!session)return 0;
        auto value=new(std::nothrow) TeleportPolicy(session);if(!value)return 0;
        store=new(std::nothrow) BookmarkStore;
        if(store&&!value->restore(store->load(kExeHash)))log("IGNORED: stored bookmark schema is invalid.");
        bindings=supported_bindings(base);marker_bindings=supported_marker_bindings(bindings,true);policy.store(value,std::memory_order_release);
        crimson::live_items::RegisterTaskObserverV1 registration{};
        for(unsigned attempt=0;attempt<50&&!registration;++attempt){
            if(auto items=GetModuleHandleW(L"CrimsonLiveItems.asi"))registration=reinterpret_cast<crimson::live_items::RegisterTaskObserverV1>(GetProcAddress(items,crimson::live_items::kRegisterTaskObserverExportV1));
            if(!registration)Sleep(200);
        }
        if(!registration||!registration(crimson::live_items::kTaskObserverAbiV1,sizeof(crimson::live_items::TaskObservationV1),observe)){log("DISABLED: LiveItems leased-task broker is unavailable.");return 0;}
        active.store(true,std::memory_order_release);
        log("READY: captured bookmarks, server-owned mainland map pins/waypoints and native ground/placement validation. Missing geometry and alternate layers reject. Queue acceptance is not arrival; no live acceptance has been measured offline.");
        serve_teleport_pipe(handle);
    } catch(...){log("DISABLED: initialization failed.");}
    return 0;
}
void start(){if(started.exchange(true)||!pin_module())return;const auto worker=CreateThread(nullptr,0,initialize,nullptr,0,nullptr);if(worker)CloseHandle(worker);}
}
extern "C" __declspec(dllexport) void InitializeASI(){start();}
BOOL APIENTRY DllMain(HMODULE value,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){module=value;DisableThreadLibraryCalls(value);start();}return TRUE;}
