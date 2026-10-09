#include <Windows.h>
#include <MinHook.h>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <span>
#include "hash.h"
#include "native.h"
#include "pipe.h"
#include "dispatch.h"
#include "pump.h"

namespace {
using namespace crimson::live_items;
HMODULE module{};std::atomic<bool> started=false;
Queue queue(GetCurrentProcessId(),GetTickCount64());
Bindings bindings{};std::uintptr_t base{};
TaskDispatch original{};
void log(const char* message);
std::atomic<bool> context_logged=false;
void after_task(void* context,int* error,std::uintptr_t caller) {
    if(pump(queue,bindings,base,context,error,caller,GetTickCount64())&&!context_logged.exchange(true))log("CONTEXT: automatic server-player task confirmed; native lease, active identity and context pool validated.");
}
int* __fastcall hook(int* error,void* task,void* context) {
    return forward_task(original,after_task,error,task,context,reinterpret_cast<std::uintptr_t>(_ReturnAddress()));
}
void log(const char* message) {
    wchar_t path[32768];if(!GetModuleFileNameW(module,path,32768))return;
    auto end=wcsrchr(path,L'\\');if(!end)return;wcscpy_s(end+1,32768-(end+1-path),L"CrimsonLiveItems.log");
    FILE* f{};if(!_wfopen_s(&f,path,L"a")&&f){fprintf(f,"%llu %s\n",GetTickCount64(),message);fclose(f);}
}
struct Pin {std::uintptr_t rva;std::size_t bytes;const char* hash;};
DWORD WINAPI initialize(void*) {
    try {
        wchar_t path[32768];if(!GetModuleFileNameW(nullptr,path,32768))return 0;
        auto name=wcsrchr(path,L'\\');if(_wcsicmp(name?name+1:path,L"CrimsonDesert.exe"))return 0;
        base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
        auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);auto nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
        std::ifstream file(std::filesystem::path(path),std::ios::binary);
        if(nt->FileHeader.TimeDateStamp!=1790086912||nt->OptionalHeader.SizeOfImage!=389722112||!file||
           crimson::repair::hash_stream(file)!="57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7"){log("DISABLED: unsupported executable.");return 0;}
        constexpr Pin pins[]={
            {0x2774b20,0x38,"8ef6c164fd9207e3ec484b894d6544ff3e30b279977535a172fface8eb7cf030"},
            {0x2774730,0x3ed,"f6ad2ead93d7bc7ac743ad7e03a11c3c33f9ab0ba01dd52b3b11459dee9d821e"},
            {0x2773c20,0x80d,"fa4e7fe728b53a31b44d1f78b4a6841ee546085c7491a365a1e4ee0f945c9c63"},
            {0x2781780,0x128,"d20055ccb3ad4fb138b7a23c4d6cbe8b6741470c954ac04b7f6c5fe95ba87dbc"},
            {0x8802fb0,0x9c,"224d3bde294f1b92bb4637645f0b4599488a54e98f5f5082ba3f4aac245dc85e"},
            {0x38ab60,0x128,"35e8889ccc6a7507443bbfa57b4bdbb81dba83dc6196fba368bab5dbbad71972"},
            {0x2409970,0x3ac,"928af4217c4b9ee80492678b940a7b1a21b6b25cbc5651760ae4ae84a6dd3d71"},
            {0x2449730,0xfa,"6ba3a7e768e6f0d9892b47185f15165a2b8160f5371d963418198e1e02f0076e"},
            {0x240cac0,0x4cb,"568144d0c69c494c4b189a19e8e3a8f2ffb847091b25275816a101ac33a80708"},
            {0x244c4b0,0x87,"861d6452f3e40fe0176bbc5d6cd26c86aa3b79335cb954945cbfb009edee89a5"},
            {0x2449830,0x111,"963124fb473a3fafe4c853da2dbba3a7cb8c3ad4abc9c66ba53db324a3557457"},
            {0xf493f00,0x109,"70f165fcc4b5e446009040c339357debe2071bf7b6432e870ee9872c0eb327e1"},
            {0x2b3ee50,0x63e,"54ea1a50126862e1e5667a4a7fd83383ce67d8f534f8c0975d3a7f365386f5b4"},
        };
        for(auto pin:pins)if(crimson::repair::hash(std::span(reinterpret_cast<const std::uint8_t*>(base+pin.rva),pin.bytes))!=pin.hash){log("DISABLED: native routine mismatch.");return 0;}
        bindings={reinterpret_cast<Lookup>(base+0x8802fb0),reinterpret_cast<Definition>(base+0x38ab60),reinterpret_cast<Construct>(base+0x2409970),reinterpret_cast<Convert>(base+0x2449730),reinterpret_cast<Destroy>(base+0xf493f00),reinterpret_cast<Grant>(base+0x2b3ee50)};
        if(MH_Initialize()!=MH_OK){log("DISABLED: hook engine unavailable.");return 0;}
        auto target=reinterpret_cast<void*>(base+0x2774b20);
        if(MH_CreateHook(target,reinterpret_cast<void*>(&hook),reinterpret_cast<void**>(&original))!=MH_OK){log("DISABLED: hook creation failed.");return 0;}
        if(MH_EnableHook(target)!=MH_OK){MH_RemoveHook(target);log("DISABLED: hook activation failed.");return 0;}
        log("ARMED: build2976 automatic native item grants on leased server-player tasks. No pickup required. Protocol2; requests expire after30 seconds; no retry after an unknown result.");
        serve(queue,base);
    } catch(...) {log("DISABLED: initialization failed.");}
    return 0;
}
void start(){if(started.exchange(true))return;auto thread=CreateThread(nullptr,0,initialize,nullptr,0,nullptr);if(thread)CloseHandle(thread);}
}
extern "C" __declspec(dllexport) void InitializeASI(){start();}
BOOL APIENTRY DllMain(HMODULE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){module=h;DisableThreadLibraryCalls(h);start();}return TRUE;}
