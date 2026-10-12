#include <Windows.h>
#include <bcrypt.h>
#include <MinHook.h>
#include <intrin.h>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "blackstar_policy.h"
#include "targets.h"
#include "flight_targets.h"
#include "blackstar_flight.h"
#include "vtable_slot.h"
#include "blackstar_request.h"
#include "request_targets.h"

namespace {
HMODULE module=nullptr;
std::atomic<bool> started=false,reported_row=false,reported_region=false,reported_rejection=false;
std::mutex row_mutex;
std::uintptr_t region_return=0;
std::uintptr_t platform_return=0;
std::uintptr_t game_base=0,frame_selector_return=0,reserved_selector_return=0;
using Loader=bool(__fastcall*)(void*,void*);
using Resolver=void*(__fastcall*)(const std::uint16_t*);
using Summon=void*(__fastcall*)(void*,void*,const std::uint16_t*);
using Region=std::uint8_t(__fastcall*)(void*,void*);
using Condition=std::uint8_t(__fastcall*)(void*,void*,void*,void*);
using InitialSummon=void*(__fastcall*)(void*,void*,std::uint64_t,void*,std::uint16_t);
using FrameEvent=void*(__fastcall*)(void*,void*,void*,void*,void*);
using ErrorDisplay=std::uintptr_t(__fastcall*)(void*,std::uint32_t);
Loader original_loader=nullptr;Resolver original_resolver=nullptr;
Summon original_summon=nullptr;Region original_region=nullptr;
Condition original_town=nullptr,original_flight_region=nullptr,original_height=nullptr,original_navigation=nullptr,original_voxel=nullptr;
InitialSummon original_initial=nullptr;Resolver original_gimmick=nullptr;
InitialSummon original_common_initial=nullptr;
FrameEvent original_frame=nullptr,original_reserved_frame=nullptr;
ErrorDisplay original_error_display=nullptr;
FlightServices flight{};
std::atomic<bool> reported_town=false,reported_flight=false,reported_height=false,reported_navigation=false,reported_voxel=false,reported_platform=false;
std::atomic<unsigned> reported_refusals=0;
std::atomic<unsigned> reported_attempts=0;
std::mutex error_mutex;
std::uint64_t error_seen[64]{};unsigned error_seen_count=0;
void log(const char* msg){
    wchar_t path[32768];if(!GetModuleFileNameW(module,path,32768))return;
    std::wstring s(path);s=s.substr(0,s.find_last_of(L"\\/"))+L"\\CrimsonBlackstar.log";
    FILE* f=nullptr;if(!_wfopen_s(&f,s.c_str(),L"a")&&f){std::fprintf(f,"%llu %s\n",GetTickCount64(),msg);std::fclose(f);}
}
bool hash_matches(const wchar_t* path){
    HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
    if(f==INVALID_HANDLE_VALUE)return false;
    BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;bool ok=false;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0&&BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0){
        std::vector<unsigned char>b(1<<20);DWORD n;bool good=true;
        for(;;){if(!ReadFile(f,b.data(),static_cast<DWORD>(b.size()),&n,nullptr)){good=false;break;}if(!n)break;if(BCryptHashData(hash,b.data(),n,0)<0){good=false;break;}}
        unsigned char digest[32];if(good&&BCryptFinishHash(hash,digest,32,0)>=0){char text[65]{};for(int i=0;i<32;++i)std::sprintf(text+2*i,"%02x",digest[i]);ok=!std::strcmp(text,kExeHash);}
    }
    if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);CloseHandle(f);return ok;
}
std::uint32_t guarded_key(void* row){__try{return row?read_at<std::uint32_t>(row,0):0;}__except(EXCEPTION_EXECUTE_HANDLER){return 0;}}
RowResult guarded_apply(void* row){__try{return apply_row(row);}__except(EXCEPTION_EXECUTE_HANDLER){return RowResult::Rejected;}}
bool guarded_context(void* context){__try{return context_on_blackstar(context,flight);}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
bool guarded_initial(void* self,std::uint64_t uid){__try{return initial_blackstar(self,uid,flight);}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
bool guarded_town(void* self){__try{return town_parameters(self);}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
void log_refusal(void* out,const char* stage){
    __try{auto result=read_at<std::uint32_t>(out,0);if(result&&reported_refusals.fetch_add(1)<12){char line[160];std::sprintf(line,"REFUSED: Blackstar %s result %u (0x%08x); original placement/state checks retained.",stage,result,result);log(line);}}
    __except(EXCEPTION_EXECUTE_HANDLER){}
}
void update(void* row){
    if(guarded_key(row)!=kBlackstarKey)return;
    std::lock_guard lock(row_mutex);
    auto result=guarded_apply(row);
    if((result==RowResult::Applied||result==RowResult::AlreadyApplied)&&!reported_row.exchange(true))
        log("APPLIED: Blackstar stamina regeneration 20 -> 30000000; summon cooldown 3600 -> 1 second; summon duration 600 -> 60000 seconds (1000 minutes); other stats preserved.");
    if(result==RowResult::Rejected&&!reported_rejection.exchange(true))
        log("REFUSED: Blackstar row does not match the verified layout/baseline. No row edits applied.");
}
bool __fastcall on_loader(void* stream,void* row){bool ok=original_loader(stream,row);if(ok)update(row);return ok;}
void* __fastcall on_resolver(const std::uint16_t* key){
    void* row=original_resolver(key);update(row);
    select_frame_summon(reinterpret_cast<std::uintptr_t>(_ReturnAddress()),guarded_key(row));
    return row;
}
void* __fastcall on_summon(void* out,void* actor,const std::uint16_t* key){
    void* row=key?on_resolver(key):nullptr;
    SummonScope scope(guarded_key(row)==kBlackstarKey);
    auto result=original_summon(out,actor,key);
    if(blackstar_summon)log_refusal(out,"spawn validator");
    return result;
}
void* __fastcall on_initial(void* self,void* out,std::uint64_t uid,void* actor,std::uint16_t stage){
    const bool eligible=guarded_initial(self,uid);
    select_frame_summon(reinterpret_cast<std::uintptr_t>(_ReturnAddress()),eligible?kBlackstarKey:0);
    SummonScope scope(eligible);
    auto result=original_initial(self,out,uid,actor,stage);
    if(blackstar_summon)log_refusal(out,"initial validator");
    return result;
}
void* __fastcall on_common_initial(void* self,void* out,std::uint64_t uid,void* actor,std::uint16_t stage){
    SummonScope scope(guarded_initial(self,uid));
    auto result=original_common_initial(self,out,uid,actor,stage);
    if(blackstar_summon)log_refusal(out,"common/server validator");
    return result;
}
void log_attempt(void* out,const char* stage){
    if(reported_attempts.fetch_add(1)>=24)return;
    __try{char line[160];std::sprintf(line,"ATTEMPT: Blackstar %s returned %u (0x%08x).",stage,
        read_at<std::uint32_t>(out,0),read_at<std::uint32_t>(out,0));log(line);}
    __except(EXCEPTION_EXECUTE_HANDLER){}
}
void* __fastcall on_frame(void* self,void* out,void* a3,void* a4,void* a5){
    FrameSummonScope scope(frame_selector_return,game_base+0x7fbf4f);
    auto result=original_frame(self,out,a3,a4,a5);
    if(scope.selected_key==kBlackstarKey)log_attempt(out,"client frame event");
    return result;
}
void* __fastcall on_reserved_frame(void* self,void* out,void* a3,void* a4,void* a5){
    FrameSummonScope scope(reserved_selector_return,game_base+0x7fd496);
    auto result=original_reserved_frame(self,out,a3,a4,a5);
    if(scope.selected_key==kBlackstarKey)log_attempt(out,"reserved-slot frame event");
    return result;
}
std::uintptr_t __fastcall on_error_display(void* self,std::uint32_t code){
    auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    bool report=false;
    if(code&&caller>=game_base&&caller<game_base+389722112){
        const auto identity=(static_cast<std::uint64_t>(code)<<32)|(caller-game_base);
        std::lock_guard lock(error_mutex);
        bool found=false;for(unsigned i=0;i<error_seen_count;++i)found=found||error_seen[i]==identity;
        if(!found&&error_seen_count<64){error_seen[error_seen_count++]=identity;report=true;}
    }
    if(report){
        char line[640];int used=std::sprintf(line,"GAME_ERROR: code=%u (0x%08x) caller=0x%llx BlackstarScope=%u; stack:",
            code,code,static_cast<unsigned long long>(caller-game_base),blackstar_summon?1:0);
        void* frames[16]{};auto count=RtlCaptureStackBackTrace(0,16,frames,nullptr);
        for(USHORT i=0;i<count;++i){auto p=reinterpret_cast<std::uintptr_t>(frames[i]);
            if(p>=game_base&&p<game_base+389722112)
                used+=std::sprintf(line+used," 0x%llx",static_cast<unsigned long long>(p-game_base));}
        log(line);
    }
    // Diagnostic only: every error, including unrelated UI messages, is passed
    // through exactly once. No error code or success result is fabricated.
    return original_error_display(self,code);
}
void* __fastcall on_gimmick(const std::uint16_t* key){
    auto row=original_gimmick(key);
    thread_local unsigned char copy[0x16a];
    void* result=row;
    const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    __try{result=platform_row(row,copy,caller,platform_return);}__except(EXCEPTION_EXECUTE_HANDLER){return row;}
    if(result!=row&&!reported_platform.exchange(true))log("APPLIED: Blackstar summon platform restriction bypass; original gimmick data untouched.");
    return result;
}
std::uint8_t __fastcall on_town(void* self,void* context,void* a3,void* a4){
    if(guarded_town(self)&&(blackstar_summon||guarded_context(context))){
        if(!reported_town.exchange(true))log("APPLIED: Blackstar town flight restriction bypass.");
        return 0;
    }return original_town(self,context,a3,a4);
}
std::uint8_t __fastcall on_flight_region(void* self,void* context,void* a3,void* a4){
    if(blackstar_summon||guarded_context(context)){
        if(!reported_flight.exchange(true))log("APPLIED: Blackstar regional flight restriction bypass.");
        return 0;
    }return original_flight_region(self,context,a3,a4);
}
std::uint8_t __fastcall on_height(void* self,void* context,void* a3,void* a4){
    if(blackstar_summon||guarded_context(context)){
        if(!reported_height.exchange(true))log("APPLIED: Blackstar flight height restriction bypass.");
        return 0;
    }return original_height(self,context,a3,a4);
}
std::uint8_t __fastcall on_navigation(void* self,void* context,void* a3,void* a4){
    if(blackstar_summon){if(!reported_navigation.exchange(true))log("APPLIED: Blackstar summon navigation gate bypass; original final placement/indoor checks retained.");return 0;}
    return original_navigation(self,context,a3,a4);
}
std::uint8_t __fastcall on_voxel(void* self,void* context,void* a3,void* a4){
    if(blackstar_summon){if(!reported_voxel.exchange(true))log("APPLIED: Blackstar summon voxel gate bypass; original final placement/indoor checks retained.");return 0;}
    return original_voxel(self,context,a3,a4);
}
std::uint8_t __fastcall on_region(void* region,void* categories){
    if(bypass_region(reinterpret_cast<std::uintptr_t>(_ReturnAddress()),region_return)){
        if(!reported_region.exchange(true))log("APPLIED: Blackstar summon region restriction bypass; original indoor/obstacle checks still execute.");
        return 0;
    }
    return original_region(region,categories);
}
DWORD WINAPI initialize(void*){
    wchar_t path[32768];if(!GetModuleFileNameW(nullptr,path,32768))return 0;
    const auto name=wcsrchr(path,L'\\');if(_wcsicmp(name?name+1:path,L"CrimsonDesert.exe"))return 0;
    auto base=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    auto nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    struct Hook{std::uintptr_t rva;const unsigned char* bytes;std::size_t size;void* detour;void** original;};
    Hook hooks[]={
        {kLoaderRva,kLoaderPrologue,sizeof(kLoaderPrologue),reinterpret_cast<void*>(&on_loader),reinterpret_cast<void**>(&original_loader)},
        {kResolverRva,kResolverPrologue,sizeof(kResolverPrologue),reinterpret_cast<void*>(&on_resolver),reinterpret_cast<void**>(&original_resolver)},
        {kSummonRva,kSummonPrologue,sizeof(kSummonPrologue),reinterpret_cast<void*>(&on_summon),reinterpret_cast<void**>(&original_summon)},
        {kRegionRva,kRegionPrologue,sizeof(kRegionPrologue),reinterpret_cast<void*>(&on_region),reinterpret_cast<void**>(&original_region)},
        {kTownRva,kTownPrologue,sizeof(kTownPrologue),reinterpret_cast<void*>(&on_town),reinterpret_cast<void**>(&original_town)},
        {kFlightRegionRva,kFlightRegionPrologue,sizeof(kFlightRegionPrologue),reinterpret_cast<void*>(&on_flight_region),reinterpret_cast<void**>(&original_flight_region)},
        {kNavigationGateRva,kNavigationGatePrologue,sizeof(kNavigationGatePrologue),reinterpret_cast<void*>(&on_navigation),reinterpret_cast<void**>(&original_navigation)},
        {kVoxelGateRva,kVoxelGatePrologue,sizeof(kVoxelGatePrologue),reinterpret_cast<void*>(&on_voxel),reinterpret_cast<void**>(&original_voxel)},
        {kInitialSummonRva,kInitialSummonPrologue,sizeof(kInitialSummonPrologue),reinterpret_cast<void*>(&on_initial),reinterpret_cast<void**>(&original_initial)},
        {kGimmickResolverRva,kGimmickResolverPrologue,sizeof(kGimmickResolverPrologue),reinterpret_cast<void*>(&on_gimmick),reinterpret_cast<void**>(&original_gimmick)},
        {kFrameSummonRva,kFrameSummonPrologue,sizeof(kFrameSummonPrologue),reinterpret_cast<void*>(&on_frame),reinterpret_cast<void**>(&original_frame)},
        {kReservedFrameSummonRva,kReservedFrameSummonPrologue,sizeof(kReservedFrameSummonPrologue),reinterpret_cast<void*>(&on_reserved_frame),reinterpret_cast<void**>(&original_reserved_frame)},
        {kCommonInitialRva,kCommonInitialPrologue,sizeof(kCommonInitialPrologue),reinterpret_cast<void*>(&on_common_initial),reinterpret_cast<void**>(&original_common_initial)},
        {kErrorDisplayRva,kErrorDisplayPrologue,sizeof(kErrorDisplayPrologue),reinterpret_cast<void*>(&on_error_display),reinterpret_cast<void**>(&original_error_display)}};
    bool valid=nt->FileHeader.TimeDateStamp==1790086912&&nt->OptionalHeader.SizeOfImage==389722112&&hash_matches(path);
    for(const auto& h:hooks)valid=valid&&!std::memcmp(reinterpret_cast<void*>(base+h.rva),h.bytes,h.size);
    struct Guard{std::uintptr_t rva;const unsigned char* bytes;std::size_t size;};
    Guard guards[]={
        {kMountedActorRva,kMountedActorPrologue,sizeof(kMountedActorPrologue)},
        {kReleaseActorRefRva,kReleaseActorRefPrologue,sizeof(kReleaseActorRefPrologue)},
        {kMercenaryLookupRva,kMercenaryLookupPrologue,sizeof(kMercenaryLookupPrologue)},
        {kHeightRva,kHeightPrologue,sizeof(kHeightPrologue)},
        {kFlightGuard0Rva,kFlightGuard0,sizeof(kFlightGuard0)},
        {kFlightGuard1Rva,kFlightGuard1,sizeof(kFlightGuard1)},
        {kFlightGuard2Rva,kFlightGuard2,sizeof(kFlightGuard2)},
        {kFlightGuard3Rva,kFlightGuard3,sizeof(kFlightGuard3)},
        {kFlightGuard4Rva,kFlightGuard4,sizeof(kFlightGuard4)},
        {kRequestSelector0Rva,kRequestSelector0,sizeof(kRequestSelector0)},
        {kRequestSelector1Rva,kRequestSelector1,sizeof(kRequestSelector1)},
        {kRequestPrecheck0Rva,kRequestPrecheck0,sizeof(kRequestPrecheck0)},
        {kRequestPrecheck1Rva,kRequestPrecheck1,sizeof(kRequestPrecheck1)},
        {kRequestValidator0Rva,kRequestValidator0,sizeof(kRequestValidator0)},
        {kRequestValidator1Rva,kRequestValidator1,sizeof(kRequestValidator1)},
        {kCommonLookupGuardRva,kCommonLookupGuard,sizeof(kCommonLookupGuard)},
        {kErrorDisplayBodyRva,kErrorDisplayBodyPrologue,sizeof(kErrorDisplayBodyPrologue)}};
    for(const auto& g:guards)valid=valid&&!std::memcmp(reinterpret_cast<void*>(base+g.rva),g.bytes,g.size);
    auto height_slot=reinterpret_cast<void**>(base+kHeightSlotRva);
    valid=valid&&*height_slot==reinterpret_cast<void*>(base+kHeightRva);
    const unsigned char region_call[]={0xe8,0xe9,0xb2,0xd9,0x00,0x84,0xc0,0x74,0x75};
    const unsigned char indoor_call[]={0xe8,0x67,0x09,0,0};
    valid=valid&&!std::memcmp(reinterpret_cast<void*>(base+0x9dd8f2),region_call,sizeof(region_call))&&
        !std::memcmp(reinterpret_cast<void*>(base+0x9ddcf4),indoor_call,sizeof(indoor_call));
    if(!valid){log("DISABLED: unsupported executable/hash or conflicting code. No hooks installed.");return 0;}
    region_return=base+0x9dd8f7;
    platform_return=base+0x9dd69f;
    game_base=base;frame_selector_return=base+0x7fc186;reserved_selector_return=base+0x7fd456;
    original_height=reinterpret_cast<Condition>(base+kHeightRva);
    flight={original_resolver,reinterpret_cast<MountedActor>(base+kMountedActorRva),
            reinterpret_cast<ReleaseActorRef>(base+kReleaseActorRefRva),reinterpret_cast<MercenaryLookup>(base+kMercenaryLookupRva)};
    if(MH_Initialize()!=MH_OK){log("DISABLED: hook initialization failed.");return 0;}
    unsigned created=0;bool ready=true;
    for(auto& h:hooks){if(MH_CreateHook(reinterpret_cast<void*>(base+h.rva),h.detour,h.original)!=MH_OK){ready=false;break;}++created;}
    // MinHook supplies the original resolver only after CreateHook has run.
    flight.character=original_resolver;
    if(ready)for(auto& h:hooks)if(MH_QueueEnableHook(reinterpret_cast<void*>(base+h.rva))!=MH_OK)ready=false;
    if(ready)ready=MH_ApplyQueued()==MH_OK;
    if(ready)ready=replace_vtable_slot(height_slot,reinterpret_cast<void*>(original_height),reinterpret_cast<void*>(&on_height));
    if(!ready){for(unsigned i=0;i<created;++i){auto p=reinterpret_cast<void*>(base+hooks[i].rva);MH_DisableHook(p);MH_RemoveHook(p);}log("DISABLED: complete hook installation failed.");return 0;}
    log("READY: Blackstar early summon action support and bounded refusal diagnostics; 14 hooks and one condition slot, build 2976. Stamina/cooldown, 1000-minute duration and town/region/height flight retained. Original final indoor/placement checks preserved. Outdoor summon completion still requires in-game verification.");
    return 0;
}
void start(){if(started.exchange(true))return;HANDLE t=CreateThread(nullptr,0,initialize,nullptr,0,nullptr);if(t)CloseHandle(t);}
}
extern "C" __declspec(dllexport) void InitializeASI(){start();}
BOOL APIENTRY DllMain(HMODULE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){module=h;DisableThreadLibraryCalls(h);start();}return TRUE;}
