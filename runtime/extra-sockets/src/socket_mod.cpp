#include <Windows.h>
#include <MinHook.h>
#include <atomic>
#include <cstdio>
#include <mutex>
#include "socket_io.h"
#include "pins.h"
namespace {
HMODULE module{};std::atomic<bool> started=false;std::atomic<unsigned> logged[extra::maxRecords]{};
using Ctor=void*(__fastcall*)(void*,const void*);
using Writer=void(__fastcall*)(void*,const void*,std::uint16_t);
using Convert=void(__fastcall*)(const void*,std::uint16_t,void*);
Ctor originalCtor{};Writer originalWriter{};Convert originalConvert{};extra::Resolve resolve{};extra::Find find{};extra::Reserve reserve{};
extra::Config config{};std::mutex configMutex;std::wstring configPath;
void log(const char* message){wchar_t p[32768];if(!GetModuleFileNameW(module,p,32768))return;std::wstring path(p);path=path.substr(0,path.find_last_of(L"\\/"))+L"\\CrimsonExtraSockets.log";
    FILE* f=nullptr;if(!_wfopen_s(&f,path.c_str(),L"a")&&f){std::fprintf(f,"%llu %s\n",GetTickCount64(),message);std::fclose(f);}}
bool hashMatches(const wchar_t* path,const char* expected=kExeHash){
    HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr);if(f==INVALID_HANDLE_VALUE)return false;
    BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE hash{};bool ok=false;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0&&BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0){
        std::vector<unsigned char>b(1<<20);DWORD n;bool good=true;
        for(;;){if(!ReadFile(f,b.data(),static_cast<DWORD>(b.size()),&n,nullptr)){good=false;break;}if(!n)break;if(BCryptHashData(hash,b.data(),n,0)<0){good=false;break;}}
        unsigned char digest[32];if(good&&BCryptFinishHash(hash,digest,32,0)>=0){char t[65]{};for(int i=0;i<32;++i)std::sprintf(t+2*i,"%02x",digest[i]);ok=!std::strcmp(t,expected);}}
    if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);CloseHandle(f);return ok;
}
void* __fastcall onCtor(void* self,const void* init){
    void* result=originalCtor(self,init);
    const auto uid=extra::read<std::uint64_t>(self,0);const auto key=extra::read<std::uint32_t>(init,8);
    int index;extra::Record record;{std::lock_guard<std::mutex> lock(configMutex);index=extra::target(config,uid,key);if(index<0)return result;record=config.records[index];}
    const bool ok=extra::extend(self,record,find,resolve,reserve);
    if(logged[index].fetch_add(1)<8){char msg[192];std::sprintf(msg,"%s: item %u UID %llu; socket count %u; opened %u; expected %u.",ok?"EXTENDED":"SKIPPED",key,uid,extra::read<unsigned>(self,0x68),extra::read<unsigned char>(self,0x70),record.baseline+10);log(msg);}
    return result;
}
void __fastcall onWriter(void* saved,const void* item,std::uint16_t slot){
    const auto handle=extra::read<std::uint16_t>(item,8);const void* row=resolve(&handle);
    const auto key=row?extra::read<unsigned>(row,0):0;
    int index;extra::Record record;{std::lock_guard<std::mutex> lock(configMutex);index=extra::target(config,extra::read<std::uint64_t>(item,0),key);if(index<0){record={};}else record=config.records[index];}
    extra::SaveView view;
    if(index<0||!extra::saveView(item,record,resolve,view)){originalWriter(saved,item,slot);return;}
    // Original writer sees exactly its proven five-entry format. Live Item remains untouched.
    originalWriter(saved,view.bytes.data(),slot);
    if(view.index>=0){std::lock_guard<std::mutex> lock(configMutex);auto after=config;
        std::memcpy(after.records[index].gems,view.extra.data(),sizeof(after.records[index].gems));
        if(std::memcmp(config.records,after.records,sizeof(config.records))){
            if(extra::persist(configPath,config,after)){config=after;log("SAVED: additional sockets persisted separately; native five-entry game save retained.");}
            else log("ERROR: additional sockets could not be persisted; config changed or disk write failed. Game save retains vanilla format.");}
    }
}
void __fastcall onConvert(const void* item,std::uint16_t slot,void* init){
    const auto handle=extra::read<std::uint16_t>(item,8);const void* row=resolve(&handle);
    int index;extra::Record record;{std::lock_guard<std::mutex> lock(configMutex);index=extra::target(config,extra::read<std::uint64_t>(item,0),row?extra::read<unsigned>(row,0):0);if(index<0)record={};else record=config.records[index];}
    extra::SaveView view;
    // Network InitData has five inline records. A sixth overwrites its opened byte.
    // The client constructor restores extras from the same separate configuration.
    originalConvert(index>=0&&extra::saveView(item,record,resolve,view)
        ?view.bytes.data():item,slot,init);
}
DWORD WINAPI initialize(void*){
    wchar_t p[32768];if(!GetModuleFileNameW(nullptr,p,32768))return 0;const auto name=wcsrchr(p,L'\\');if(_wcsicmp(name?name+1:p,L"CrimsonDesert.exe"))return 0;
    auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    bool valid=nt->FileHeader.TimeDateStamp==1790086912&&nt->OptionalHeader.SizeOfImage==389722112&&hashMatches(p);
    valid=valid&&!std::memcmp((void*)(base+kCtorRva),kCtorPin,sizeof(kCtorPin))&&!std::memcmp((void*)(base+kWriterRva),kWriterPin,sizeof(kWriterPin))
        &&!std::memcmp((void*)(base+kReserveRva),kReservePin,sizeof(kReservePin))&&!std::memcmp((void*)(base+kResolveRva),kResolvePin,sizeof(kResolvePin))&&!std::memcmp((void*)(base+kFindRva),kFindPin,sizeof(kFindPin))
        &&!std::memcmp((void*)(base+kConvertRva),kConvertPin,sizeof(kConvertPin));
    if(!valid){log("DISABLED: executable/hash or native function pins differ; no hooks installed.");return 0;}
    if(!GetModuleFileNameW(module,p,32768))return 0;configPath=p;configPath=configPath.substr(0,configPath.find_last_of(L"\\/"))+L"\\CrimsonExtraSockets.dat";
    if(!extra::load(configPath,config)){log("DISABLED: config missing, corrupt or unsupported; no hooks installed.");return 0;}
    // LiveItems attests this converter before arming its task hook. Let its
    // validation finish before replacing the converter, retaining both guards.
    const auto peerPath=configPath.substr(0,configPath.find_last_of(L"\\/"))+L"\\CrimsonLiveItems.asi";
    if(!hashMatches(peerPath.c_str(),"576dad0e64dfd352b89bca2dd2b031ff8256c824cdd279732cd1dd186ea74d05")){log("DISABLED: expected LiveItems peer is missing or changed; no hooks installed.");return 0;}
    const auto deadline=GetTickCount64()+30000;
    while(!GetModuleHandleW(L"CrimsonLiveItems.asi")||*reinterpret_cast<volatile unsigned char*>(base+0x2774b20)!=0xe9){
        if(GetTickCount64()>=deadline){log("DISABLED: LiveItems startup validation did not finish; no hooks installed.");return 0;}Sleep(50);
    }
    if(std::memcmp((void*)(base+kConvertRva),kConvertPin,sizeof(kConvertPin))){log("DISABLED: converter changed during startup; no hooks installed.");return 0;}
    resolve=reinterpret_cast<extra::Resolve>(base+kResolveRva);find=reinterpret_cast<extra::Find>(base+kFindRva);reserve=reinterpret_cast<extra::Reserve>(base+kReserveRva);
    auto ctor=(void*)(base+kCtorRva),writer=(void*)(base+kWriterRva),convert=(void*)(base+kConvertRva);
    if(MH_Initialize()!=MH_OK){log("DISABLED: MinHook initialization failed.");return 0;}
    if(MH_CreateHook(writer,(void*)&onWriter,(void**)&originalWriter)!=MH_OK||MH_CreateHook(convert,(void*)&onConvert,(void**)&originalConvert)!=MH_OK||MH_CreateHook(ctor,(void*)&onCtor,(void**)&originalCtor)!=MH_OK){MH_RemoveHook(writer);MH_RemoveHook(convert);MH_RemoveHook(ctor);log("DISABLED: hooks could not be created.");return 0;}
    if(MH_QueueEnableHook(writer)!=MH_OK||MH_QueueEnableHook(convert)!=MH_OK||MH_QueueEnableHook(ctor)!=MH_OK||MH_ApplyQueued()!=MH_OK){MH_DisableHook(ctor);MH_DisableHook(convert);MH_DisableHook(writer);log("DISABLED: hooks could not be enabled.");return 0;}
    log("READY v2: ten additional sockets per configured equipment UID. Save and network InitData retain the original five-entry format; client and server restore extras. Manage in Workbench. Build 2976 only.");return 0;
}
void start(){if(started.exchange(true))return;HANDLE t=CreateThread(nullptr,0,initialize,nullptr,0,nullptr);if(t)CloseHandle(t);}
}
extern "C" __declspec(dllexport) void InitializeASI(){start();}
BOOL APIENTRY DllMain(HMODULE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){module=h;DisableThreadLibraryCalls(h);start();}return TRUE;}
