#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include "gems.h"
#include "gear.h"

namespace extra {
template<class T> T read(const void* p,std::size_t at){T v;std::memcpy(&v,static_cast<const unsigned char*>(p)+at,sizeof(v));return v;}
template<class T> void write(void* p,std::size_t at,T v){std::memcpy(static_cast<unsigned char*>(p)+at,&v,sizeof(v));}
#pragma pack(push,1)
struct Gem {std::uint16_t handle,endurance;std::uint8_t index,padding;};
struct StoredGem {std::uint32_t key;std::uint16_t endurance;};
struct Record {std::uint64_t uid;std::uint32_t key;std::uint8_t baseline,additional;std::uint16_t reserved;StoredGem gems[10];};
inline constexpr unsigned maxRecords=256;
struct Config {char magic[8];std::uint32_t version,count;unsigned char digest[32];Record records[maxRecords];};
#pragma pack(pop)
static_assert(sizeof(Gem)==6 && sizeof(Record)==76 && sizeof(Config)==48+76*maxRecords);
inline unsigned configSize(const Config& c){return 48+c.count*sizeof(Record);}
struct Vector {Gem* data;std::uint32_t count,capacity;};
struct Target {std::uint64_t uid;std::uint32_t key;std::uint8_t baseline;};
inline constexpr Target targets[]={{1001416,111005,0},{1003059,1000380,1},{1003664,1001062,5}};
inline constexpr std::size_t itemSize=0xc8;
inline constexpr Gem empty{65535,0,255,0};
using Reserve=bool(__fastcall*)(Vector*,std::uint32_t);
using Resolve=const void*(__fastcall*)(const std::uint16_t*);
using Find=std::uint16_t*(__fastcall*)(std::uint16_t*,std::uint32_t);
inline int target(std::uint64_t uid,std::uint32_t key){for(int i=0;i<3;++i)if(targets[i].uid==uid&&targets[i].key==key)return i;return -1;}
inline int target(const Config& c,std::uint64_t uid,std::uint32_t key){for(unsigned i=0;i<c.count;++i)if(c.records[i].uid==uid&&c.records[i].key==key)return i;return -1;}
inline bool gearKey(unsigned key){for(auto k:kAllowedGearKeys)if(k==key)return true;return false;}
inline bool structure(const Config& c){
    if(std::memcmp(c.magic,"CWEXSOCK",8)||c.count>maxRecords||(c.version!=1&&c.version!=2)||(c.version==1&&c.count!=3))return false;
    for(unsigned i=0;i<c.count;++i){const auto& r=c.records[i];
        if(!r.uid||r.uid==UINT64_MAX||!gearKey(r.key)||r.baseline>5||r.additional!=10||r.reserved)return false;
        if(c.version==1){const auto& t=targets[i];if(r.uid!=t.uid||r.key!=t.key||r.baseline!=t.baseline)return false;}
        for(unsigned j=0;j<i;++j)if(c.records[j].uid==r.uid)return false;
        for(auto g:r.gems){if(!g.key){if(g.endurance)return false;}else{bool ok=false;for(auto a:kAllowedGems)if(a.key==g.key&&g.endurance<=a.endurance)ok=true;if(!ok)return false;}}
    }return true;
}
inline bool rowGem(const void* row,std::uint32_t key,std::uint16_t endurance){
    bool allowed=false;for(auto a:kAllowedGems)if(a.key==key&&endurance<=a.endurance)allowed=true;
    return allowed&&row&&read<std::uint32_t>(row,0)==key&&endurance<=read<std::uint16_t>(row,0x400);
}
// Returns false before touching the engine vector if configuration or locked padding differs.
inline bool extend(void* item,const Record& record,Find find,Resolve resolve,Reserve reserve){
    if(read<std::uint64_t>(item,0)!=record.uid||record.baseline>5||record.additional!=10||!gearKey(record.key))return false;
    const auto* row=resolve(static_cast<std::uint16_t*>(static_cast<void*>(static_cast<unsigned char*>(item)+8)));
    if(!row||read<std::uint32_t>(row,0)!=record.key)return false;
    auto* vec=reinterpret_cast<Vector*>(static_cast<unsigned char*>(item)+0x60);
    if(vec->count!=5||vec->capacity<5||!vec->data||read<std::uint8_t>(item,0x70)!=record.baseline)return false;
    for(unsigned i=record.baseline;i<5;++i)if(vec->data[i].handle!=65535)return false;
    std::array<Gem,10> additional{};
    for(unsigned i=0;i<10;++i){auto g=record.gems[i];additional[i]=empty;
        if(g.key){std::uint16_t h=65535;find(&h,g.key);if(h==65535||!rowGem(resolve(&h),g.key,g.endurance))return false;
            additional[i]={h,g.endurance,static_cast<std::uint8_t>(record.baseline+i),0};}
    }
    const unsigned n=record.baseline+10;reserve(vec,n);
    if(!vec->data||vec->capacity<n)return false;
    for(unsigned i=record.baseline;i<n;++i)vec->data[i]=additional[i-record.baseline];
    vec->count=n;write(item,0x70,static_cast<std::uint8_t>(n));return true;
}
struct SaveView {alignas(16) std::array<unsigned char,itemSize> bytes;std::array<Gem,5> gems;std::array<StoredGem,10> extra;int index=-1;};
// A borrowed copy, never destroyed as an engine Item. All non-socket fields are retained.
inline bool saveView(const void* item,const Record& record,Resolve resolve,SaveView& out){
    if(read<std::uint64_t>(item,0)!=record.uid||record.baseline>5||record.additional!=10)return false;
    const unsigned base=record.baseline,n=base+10;
    const auto vec=read<Vector>(item,0x60);
    if(vec.count!=n||vec.capacity<n||!vec.data)return false;
    // Normalize a configured extended vector even if its opened byte drifted;
    // that byte must never become the bound of an inline five-record writer.
    bool valid=true;
    for(unsigned i=0;i<10;++i){const auto g=vec.data[base+i];out.extra[i]={0,0};
        if(g.handle!=65535){auto row=resolve(&g.handle);if(!row){valid=false;continue;}
            auto k=read<std::uint32_t>(row,0);if(!rowGem(row,k,g.endurance)){valid=false;continue;}out.extra[i]={k,g.endurance};}
    }
    std::memcpy(out.bytes.data(),item,itemSize);
    out.gems.fill(empty);for(unsigned i=0;i<base;++i)out.gems[i]=vec.data[i];
    write(out.bytes.data(),0x60,Vector{out.gems.data(),5,5});
    write(out.bytes.data(),0x70,static_cast<std::uint8_t>(base));
    out.index=valid?0:-1;return true;
}
// Legacy fixture interface; the runtime always supplies the checked configured record.
inline bool saveView(const void* item,unsigned key,Resolve resolve,SaveView& out){
    const int i=target(read<std::uint64_t>(item,0),key);if(i<0)return false;
    const auto t=targets[i];return saveView(item,Record{t.uid,t.key,t.baseline,10,0,{}},resolve,out);
}
}
