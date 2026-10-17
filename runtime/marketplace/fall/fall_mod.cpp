#include <Windows.h>
#include <bcrypt.h>
#include <MinHook.h>
#include <atomic>
#include <cstdio>
#include <string>
#include <vector>
#include "fall_policy.h"
#include "fall_targets.h"
#include "../blackstar/targets.h"
using namespace crimson::marketplace::fall;

namespace {
HMODULE module{};
std::atomic<bool> started=false;
std::atomic<unsigned> reported=0;
Fall original_fall{};
Resolver resolver{};
void log(const char* message){
    wchar_t path[32768];if(!GetModuleFileNameW(module,path,32768))return;
    std::wstring p(path);p=p.substr(0,p.find_last_of(L"\\/"))+L"\\CrimsonNoFall.log";
    FILE* f=nullptr;if(!_wfopen_s(&f,p.c_str(),L"a")&&f){std::fprintf(f,"%llu %s\n",GetTickCount64(),message);std::fclose(f);}
}
bool hash_matches(const wchar_t* path){
    HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
    if(f==INVALID_HANDLE_VALUE)return false;
    BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE hash{};bool ok=false;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0&&BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0){
        std::vector<unsigned char>b(1<<20);DWORD n;bool good=true;
        for(;;){if(!ReadFile(f,b.data(),static_cast<DWORD>(b.size()),&n,nullptr)){good=false;break;}if(!n)break;if(BCryptHashData(hash,b.data(),n,0)<0){good=false;break;}}
        unsigned char digest[32];if(good&&BCryptFinishHash(hash,digest,32,0)>=0){char text[65]{};for(int i=0;i<32;++i)std::snprintf(text+2*i,3,"%02x",digest[i]);ok=!std::strcmp(text,kExeHash);}
    }
    if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);CloseHandle(f);return ok;
}
void __fastcall on_fall(void* self,void* source,float height,void* event,void* detail){
    if(dispatch_fall(original_fall,resolver,self,source,height,event,detail)){
        if(reported.fetch_add(1)<12){const auto key=guarded_key(self,resolver);char message[192];std::snprintf(message,sizeof(message),"PREVENTED: fall damage for CharacterInfo %u; height %.3f. Combat damage untouched.",key,height);log(message);}
    }
}
DWORD WINAPI initialize(void*){
    wchar_t path[32768];if(!GetModuleFileNameW(nullptr,path,32768))return 0;
    const auto name=wcsrchr(path,L'\\');if(_wcsicmp(name?name+1:path,L"CrimsonDesert.exe"))return 0;
    auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    auto nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    bool valid=nt->FileHeader.TimeDateStamp==1790086912&&nt->OptionalHeader.SizeOfImage==389722112&&hash_matches(path);
    valid=valid&&!std::memcmp(reinterpret_cast<void*>(base+kFallRva),kFallPrologue,sizeof(kFallPrologue))&&!std::memcmp(reinterpret_cast<void*>(base+kResolverRva),kResolverPrologue,sizeof(kResolverPrologue));
    // Exact native ignore-fall query and buff-kind 3 call in the damage routine.
    const unsigned char query[]={0xff,0x90,0x38,0x02,0,0,0x84,0xc0,0x74,0x24};
    const unsigned char contents[]={0x45,0x33,0xc0,0xb2,0x03,0x49,0x8b,0xcd,0xe8,0xc7,0xb9,0x65,0xff};
    valid=valid&&!std::memcmp(reinterpret_cast<void*>(base+0x21a093a),query,sizeof(query))&&!std::memcmp(reinterpret_cast<void*>(base+0x21a09fc),contents,sizeof(contents));
    if(!valid){log("DISABLED: executable/hash or fall routine differs; no hook installed.");return 0;}
    resolver=reinterpret_cast<Resolver>(base+kResolverRva);
    if(MH_Initialize()!=MH_OK){log("DISABLED: hook initialization failed.");return 0;}
    auto target=reinterpret_cast<void*>(base+kFallRva);
    if(MH_CreateHook(target,reinterpret_cast<void*>(&on_fall),reinterpret_cast<void**>(&original_fall))!=MH_OK){log("DISABLED: hook creation failed.");return 0;}
    if(MH_EnableHook(target)!=MH_OK){MH_RemoveHook(target);log("DISABLED: hook activation failed.");return 0;}
    log("READY: fall HP/death routine protected for Kliff, Damiane, Oongka and 339 rideable character definitions, including lion, Rokade and Blackstar. One hook; combat, quests, sockets and stats untouched. Build 2976 only.");
    return 0;
}
void start(){if(started.exchange(true))return;HANDLE t=CreateThread(nullptr,0,initialize,nullptr,0,nullptr);if(t)CloseHandle(t);}
}
extern "C" __declspec(dllexport) void InitializeASI(){start();}
BOOL APIENTRY DllMain(HMODULE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){module=h;DisableThreadLibraryCalls(h);start();}return TRUE;}
