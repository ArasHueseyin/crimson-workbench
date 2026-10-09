#include <Windows.h>
#include <MinHook.h>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <span>
#include "hash.h"
#include "observe.h"

namespace {
HMODULE module{};
std::atomic<bool> started=false;
std::atomic<unsigned> reports=0;
std::atomic<unsigned> other_reports=0;
crimson::live_items::NativeDispatch original{};
constexpr std::uintptr_t rva=0x2b3e460;
constexpr char exe_hash[]="57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7";
constexpr char routine_hash[]="3b8d3142521329d5c3df05801bf123619578fe140090682e526d3f6356c761a0";
void log(const char* message) noexcept {
    wchar_t path[32768];if(!GetModuleFileNameW(module,path,32768))return;
    __try {
        auto end=wcsrchr(path,L'\\');if(!end)return;
        wcscpy_s(end+1,32768-(end+1-path),L"CrimsonLiveItemsProbe.log");
        FILE* f{};if(!_wfopen_s(&f,path,L"a")&&f){std::fprintf(f,"%llu %s\n",GetTickCount64(),message);std::fclose(f);}
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
}
void record(crimson::live_items::Observation o) {
    if(o.player_holder ? reports.fetch_add(1)>=24 : other_reports.fetch_add(1)>=4)return;
    char text[192];std::sprintf(text,"OBSERVATION readable=%u player_holder=%u realm=%u thread=%lu; no item grant or inventory write.",o.readable,o.player_holder,o.realm,o.thread);log(text);
}
void* __fastcall hook(void* holder,int* error) {return crimson::live_items::forward(original,record,holder,error);}
DWORD WINAPI initialize(void*) {
    try {
        wchar_t path[32768];if(!GetModuleFileNameW(nullptr,path,32768))return 0;
        const auto name=wcsrchr(path,L'\\');if(_wcsicmp(name?name+1:path,L"CrimsonDesert.exe"))return 0;
        auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
        auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        auto nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
        std::ifstream file(std::filesystem::path(path),std::ios::binary);
        if(nt->FileHeader.TimeDateStamp!=1790086912||nt->OptionalHeader.SizeOfImage!=389722112||!file||crimson::repair::hash_stream(file)!=exe_hash||
           crimson::repair::hash(std::span(reinterpret_cast<const std::uint8_t*>(base+rva),std::size_t{0x9e1}))!=routine_hash) {
            log("DISABLED: unknown executable or inventory routine; no hook.");return 0;
        }
        if(MH_Initialize()!=MH_OK){log("DISABLED: MinHook initialization failed.");return 0;}
        auto target=reinterpret_cast<void*>(base+rva);
        if(MH_CreateHook(target,reinterpret_cast<void*>(&hook),reinterpret_cast<void**>(&original))!=MH_OK){log("DISABLED: hook creation failed.");return 0;}
        if(MH_EnableHook(target)!=MH_OK){MH_RemoveHook(target);log("DISABLED: hook activation failed.");return 0;}
        log("READY: read-only inventory-context probe for build 2976. Original dispatch preserved. No grant command, no retained object pointers, no save/registry changes.");
    } catch(...) {log("DISABLED: build verification failed.");}
    return 0;
}
void start(){if(started.exchange(true))return;auto thread=CreateThread(nullptr,0,initialize,nullptr,0,nullptr);if(thread)CloseHandle(thread);}
}
extern "C" __declspec(dllexport) void InitializeASI(){start();}
BOOL APIENTRY DllMain(HMODULE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){module=h;DisableThreadLibraryCalls(h);start();}return TRUE;}
