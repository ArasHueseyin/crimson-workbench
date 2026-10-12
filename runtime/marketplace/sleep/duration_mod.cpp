#include <Windows.h>
#include <bcrypt.h>
#include <MinHook.h>
#include <intrin.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "duration_policy.h"
#include "duration_targets.h"

namespace {
HMODULE module=nullptr;
std::atomic<bool> started=false;
std::uintptr_t expected_return=0;
using EventFn=void(__fastcall*)(void*,void*);
using TargetFn=void(__fastcall*)(void*,const GameCalendar*,std::uint8_t,std::uintptr_t,std::uint8_t,std::uint32_t);
EventFn original_event=nullptr;
TargetFn original_target=nullptr;

void log(const char* message) {
    wchar_t path[32768]; if(!GetModuleFileNameW(module,path,32768))return;
    std::wstring name(path);name=name.substr(0,name.find_last_of(L"\\/"))+L"\\CrimsonSleepDuration.log";
    FILE* f=nullptr;if(_wfopen_s(&f,name.c_str(),L"a")==0&&f){std::fprintf(f,"%llu %s\n",GetTickCount64(),message);std::fclose(f);}
}
bool hash_matches(const wchar_t* path) {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;bool ok=false;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0&&BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0){
        std::vector<unsigned char> buffer(1<<20);DWORD n=0;bool good=true;
        for(;;){if(!ReadFile(file,buffer.data(),static_cast<DWORD>(buffer.size()),&n,nullptr)){good=false;break;}if(!n)break;if(BCryptHashData(hash,buffer.data(),n,0)<0){good=false;break;}}
        unsigned char digest[32];if(good&&BCryptFinishHash(hash,digest,32,0)>=0){char text[65]{};for(int i=0;i<32;++i)std::sprintf(text+2*i,"%02x",digest[i]);ok=std::strcmp(text,kExeHash)==0;}
    }
    if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);CloseHandle(file);return ok;
}
void __fastcall on_event(void* event,void* context) {
    SleepScope scope(is_twelve_hour_sleep(static_cast<unsigned char*>(event)));
    original_event(event,context);
}
void __fastcall on_target(void* self,const GameCalendar* target,std::uint8_t flag,std::uintptr_t unused,std::uint8_t mode,std::uint32_t key) {
    GameCalendar adjusted=*target;
    if(extend_scoped_target(adjusted,reinterpret_cast<std::uintptr_t>(_ReturnAddress())==expected_return)) {
        original_target(self,&adjusted,flag,unused,mode,key);
        log("APPLIED: 12-hour sleep target extended by 18 hours to 30 hours.");
    } else original_target(self,target,flag,unused,mode,key);
}
DWORD WINAPI initialize(void*) {
    wchar_t path[32768];if(!GetModuleFileNameW(nullptr,path,32768))return 0;
    const auto name=wcsrchr(path,L'\\');if(_wcsicmp(name?name+1:path,L"CrimsonDesert.exe"))return 0;
    const auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    auto nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    if(nt->FileHeader.TimeDateStamp!=1790086912||nt->OptionalHeader.SizeOfImage!=389722112||!hash_matches(path)||
       std::memcmp(reinterpret_cast<void*>(base+kEventRva),kEventPrologue,sizeof(kEventPrologue))||
       std::memcmp(reinterpret_cast<void*>(base+kTargetRva),kTargetPrologue,sizeof(kTargetPrologue))||
       std::memcmp(reinterpret_cast<void*>(base+kClockBuildRva),kClockBuildPrologue,sizeof(kClockBuildPrologue))) {
        log("DISABLED: executable version/hash or target code differs.");return 0;
    }
    expected_return=base+kClockReturnRva;
    void* event=reinterpret_cast<void*>(base+kEventRva);void* target=reinterpret_cast<void*>(base+kTargetRva);
    if(MH_Initialize()!=MH_OK){log("DISABLED: MinHook initialization failed.");return 0;}
    bool first=MH_CreateHook(event,reinterpret_cast<void*>(&on_event),reinterpret_cast<void**>(&original_event))==MH_OK;
    bool second=first&&MH_CreateHook(target,reinterpret_cast<void*>(&on_target),reinterpret_cast<void**>(&original_target))==MH_OK;
    if(!second||MH_QueueEnableHook(event)!=MH_OK||MH_QueueEnableHook(target)!=MH_OK||MH_ApplyQueued()!=MH_OK){
        if(first){MH_DisableHook(event);MH_RemoveHook(event);}if(second){MH_DisableHook(target);MH_RemoveHook(target);}
        log("DISABLED: hooks could not be installed together.");return 0;
    }
    log("READY: 12-hour sleep option advances 30 game hours; other durations unchanged; build 2976.");return 0;
}
void start(){if(started.exchange(true))return;HANDLE t=CreateThread(nullptr,0,initialize,nullptr,0,nullptr);if(t)CloseHandle(t);}
}
extern "C" __declspec(dllexport) void InitializeASI(){start();}
BOOL APIENTRY DllMain(HMODULE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){module=h;DisableThreadLibraryCalls(h);start();}return TRUE;}
