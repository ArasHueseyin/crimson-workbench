// Only selected, hash-pinned game functions execute in private test memory.
// The game process is never opened and its EXE entry point never executes.
#undef NDEBUG
#include <Windows.h>
#include <intrin.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <span>
#include <vector>
#include <cstdlib>
#include <MinHook.h>
#include "hash.h"
#include "../src/native.h"
#include "../src/pump.h"
#include "../src/dispatch.h"
using namespace crimson::live_items;
namespace {
std::uintptr_t fixture_base{};const char* phase="load";volatile unsigned last_type{},test_cap{};
LONG CALLBACK crash(EXCEPTION_POINTERS* info){if(info->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION)fprintf(stderr,"Fixture fault: phase=%s cap=%u type=%u RIP=%llx RVA=%llx access=%llx\n",phase,test_cap,last_type,info->ContextRecord->Rip,info->ContextRecord->Rip-fixture_base,info->ExceptionRecord->ExceptionInformation[1]);return EXCEPTION_CONTINUE_SEARCH;}
class Source {
    std::ifstream file_;std::vector<IMAGE_SECTION_HEADER> sections_;
    template<class T>T read(std::size_t offset){T out{};file_.clear();file_.seekg(offset);file_.read(reinterpret_cast<char*>(&out),sizeof(out));assert(file_);return out;}
public:
    explicit Source(const wchar_t* path):file_(std::filesystem::path(path),std::ios::binary){
        assert(file_&&crimson::repair::hash_stream(file_)=="57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7");
        auto dos=read<IMAGE_DOS_HEADER>(0);auto nt=read<IMAGE_NT_HEADERS64>(dos.e_lfanew);assert(dos.e_magic==IMAGE_DOS_SIGNATURE&&nt.Signature==IMAGE_NT_SIGNATURE);
        for(unsigned i=0;i<nt.FileHeader.NumberOfSections;i++)sections_.push_back(read<IMAGE_SECTION_HEADER>(dos.e_lfanew+sizeof(nt)+i*sizeof(IMAGE_SECTION_HEADER)));
    }
    std::vector<std::uint8_t> bytes(std::uint32_t rva,std::size_t count){
        for(auto s:sections_)if(rva>=s.VirtualAddress&&rva-s.VirtualAddress+count<=s.SizeOfRawData){std::vector<std::uint8_t> out(count);file_.clear();file_.seekg(s.PointerToRawData+rva-s.VirtualAddress);file_.read(reinterpret_cast<char*>(out.data()),count);assert(file_);return out;}
        assert(false);return {};
    }
};
class Arena {
    unsigned char* base_=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x18000000,MEM_RESERVE,PAGE_NOACCESS));std::map<std::uintptr_t,DWORD> pages_;
public:
    Arena(){assert(base_);fixture_base=reinterpret_cast<std::uintptr_t>(base_);}~Arena(){VirtualFree(base_,0,MEM_RELEASE);}
    void write(std::uint32_t rva,std::span<const std::uint8_t> bytes,DWORD protection=PAGE_EXECUTE_READ){
        auto first=rva&~std::uint32_t(4095);auto end=(rva+bytes.size()+4095)&~std::size_t(4095);
        for(auto p=first;p<end;p+=4096){DWORD old{};if(!pages_.contains(p))assert(VirtualAlloc(base_+p,4096,MEM_COMMIT,PAGE_READWRITE));else assert(VirtualProtect(base_+p,4096,PAGE_READWRITE,&old));pages_[p]=protection;}
        memcpy(base_+rva,bytes.data(),bytes.size());for(auto p=first;p<end;p+=4096){DWORD old{};assert(VirtualProtect(base_+p,4096,protection,&old));}FlushInstructionCache(GetCurrentProcess(),base_+rva,bytes.size());
    }
    void load(Source& source,std::uint32_t rva,std::size_t count,const char* hash){auto bytes=source.bytes(rva,count);assert(crimson::repair::hash(bytes)==hash);write(rva,bytes);}
    void redirect(std::uint32_t rva,std::uintptr_t target){std::array<std::uint8_t,12> bytes{0x48,0xb8};memcpy(bytes.data()+2,&target,8);bytes[10]=0xff;bytes[11]=0xe0;write(rva,bytes);}
    template<class T>T function(std::uint32_t rva){return reinterpret_cast<T>(base_+rva);}
    void check(){for(auto[p,protection]:pages_){MEMORY_BASIC_INFORMATION info{};assert(VirtualQuery(base_+p,&info,sizeof(info))==sizeof(info)&&info.Type==MEM_PRIVATE&&info.Protect==protection);}}
};
std::array<unsigned char,0x480> definition{},empty{};std::vector<void*> allocations;
unsigned char* __fastcall resolve(const std::uint16_t* type){last_type=*type;return *type==41?definition.data():empty.data();}
void __fastcall reserve(void* vector,unsigned size){assert(size==5);auto p=static_cast<unsigned char*>(vector);auto data=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,size*6);assert(data);allocations.push_back(data);*reinterpret_cast<void**>(p)=data;*reinterpret_cast<unsigned*>(p+12)=size;}
void __fastcall free_value(void* p){auto at=std::find(allocations.begin(),allocations.end(),p);assert(at!=allocations.end());assert(HeapFree(GetProcessHeap(),0,p));allocations.erase(at);}
void* __fastcall fill(void* out,int value,std::size_t size){return memset(out,value,size);}
struct Tls {unsigned char** slots=reinterpret_cast<unsigned char**>(__readgsqword(0x58));unsigned char* previous=slots[0];std::array<unsigned char,0x400> local{};Tls(){slots[0]=local.data();}~Tls(){slots[0]=previous;}};
template<class T,std::size_t N>T get(const std::array<unsigned char,N>& a,std::size_t off){T result;memcpy(&result,a.data()+off,sizeof(result));return result;}
}
#include "automatic_fixture.inl"
#include "bag_fixture.inl"
int wmain(int argc,wchar_t** argv){
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);_set_error_mode(_OUT_TO_STDERR);
    AddVectoredExceptionHandler(1,crash);
    assert(argc==2);Source source(argv[1]);Arena arena;
    arena.load(source,0x2409970,0x3ac,"928af4217c4b9ee80492678b940a7b1a21b6b25cbc5651760ae4ae84a6dd3d71");
    arena.load(source,0x2449730,0xfa,"6ba3a7e768e6f0d9892b47185f15165a2b8160f5371d963418198e1e02f0076e");
    arena.load(source,0x240cac0,0x4cb,"568144d0c69c494c4b189a19e8e3a8f2ffb847091b25275816a101ac33a80708");
    arena.load(source,0x244c4b0,0x87,"861d6452f3e40fe0176bbc5d6cd26c86aa3b79335cb954945cbfb009edee89a5");
    arena.load(source,0x2449830,0x111,"963124fb473a3fafe4c853da2dbba3a7cb8c3ad4abc9c66ba53db324a3557457");
    arena.load(source,0xf493f00,0x109,"70f165fcc4b5e446009040c339357debe2071bf7b6432e870ee9872c0eb327e1");
    arena.load(source,0x3858c0,0x56,"04a7b7234d97c771f773b9e0f468f6425997bbb1842bf03f900880dfba467067");
    arena.load(source,0x240d850,0x1cc,"86eefa30bc5ee72604989adeeb2c4eecfb661d4c4431c409a6db0e79bc61a344");
    const std::array<std::uint8_t,6> empty_socket{0xff,0xff,0,0,0xff,0};
    arena.write(0x6cf7230,empty_socket,PAGE_READONLY);
    arena.redirect(0x38ab60,reinterpret_cast<std::uintptr_t>(&resolve));arena.redirect(0xc22f70,reinterpret_cast<std::uintptr_t>(&reserve));
    arena.redirect(0x47f02bc,reinterpret_cast<std::uintptr_t>(&free_value));arena.redirect(0x47f03bc,reinterpret_cast<std::uintptr_t>(&free_value));arena.redirect(0x4898e24,reinterpret_cast<std::uintptr_t>(&fill));
    auto construct=arena.function<Construct>(0x2409970);auto convert=arena.function<Convert>(0x2449730);auto destroy=arena.function<Destroy>(0xf493f00);
    *reinterpret_cast<unsigned*>(definition.data())=2200;*reinterpret_cast<unsigned*>(definition.data()+0x390)=1;*reinterpret_cast<std::uint16_t*>(definition.data()+0x128)=0xffff;definition[0x240]=0x12;*reinterpret_cast<std::uint16_t*>(definition.data()+0x400)=87;
    unsigned cases=0;
    for(unsigned cap=0;cap<=5;cap++)for(std::int64_t qty:{1,1000}) {
        test_cap=cap;
        alignas(16) std::array<unsigned char,0xc8+32> value{};alignas(16) std::array<unsigned char,0x1c0+32> init{};
        value.fill(0xd7);init.fill(0xd7);definition[0x244]=static_cast<unsigned char>(cap);std::uint16_t type=41;
        {Tls tls;phase="construct";construct(value.data()+16,&type,qty);phase="convert";convert(init.data()+16,value.data()+16,0xffff);
            assert(get<std::int64_t>(init,16)==-1&&get<unsigned>(init,24)==2200&&get<std::int64_t>(init,32)==qty);assert(get<std::uint16_t>(init,16+0x2a)==87&&init[16+0x5e]==cap);
            for(unsigned n=0;n<16;n++)assert(value[n]==0xd7&&value[value.size()-1-n]==0xd7&&init[n]==0xd7&&init[init.size()-1-n]==0xd7);
            phase="destroy";destroy(value.data()+16);assert(allocations.empty());}++cases;
    }
    bag_fixture(source,arena);
    automatic_fixture(source,arena);
    arena.check();std::cout<<"PASS: "<<cases<<" actual build2976 constructor/InitData/destructor cases; 0..5 sockets, quantity1/1000, endurance, new UID and guards; mocked definition/allocator. No game writes.\n";
}
