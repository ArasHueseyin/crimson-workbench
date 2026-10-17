#include "../consumables/consumables_policy.h"
#include "../consumables/consumables_hooks.h"
#ifdef CRIMSON_PRIVATE_CONSUMABLE_EVIDENCE_HEADER
#include CRIMSON_PRIVATE_CONSUMABLE_EVIDENCE_HEADER
#endif
#include <vector>
#include <array>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <thread>
#include <intrin.h>

namespace {
using namespace crimson::marketplace::consumables;
namespace stamina=crimson::marketplace::stamina;
template<class T> void put(void* value,std::size_t offset,T data) {std::memcpy(static_cast<unsigned char*>(value)+offset,&data,sizeof(data));}
template<class T> T get(const void* value,std::size_t offset) {T data;std::memcpy(&data,static_cast<const unsigned char*>(value)+offset,sizeof(data));return data;}
template<std::size_t N> using Bytes=std::array<unsigned char,N>;
struct Player {
    Bytes<0xb0> actor{}; Bytes<0x1d0> components{}; Bytes<2> metadata{};
    Bytes<0xd8> possessor{}; Bytes<0x28> inventory{}; Bytes<0x20> status{};
    Bytes<0x68> vital{}; Bytes<0x90> entries{};
    void setup(Address base,bool server) {
        put(actor.data(),0,base+(server?stamina::kServerActor:stamina::kClientActor));
        put(actor.data(),0x60,std::uint32_t{0xa0101234});
        put(actor.data(),0x68,components.data()); put(actor.data(),0x70,std::uint32_t{57});
        put(actor.data(),0x88,metadata.data()); metadata[1]=1; put(actor.data(),0x96,std::uint8_t{1});
        put(actor.data(),0xa0,possessor.data()); put(possessor.data(),0xd0,actor.data());
        put(components.data(),0xb8,inventory.data()); put(components.data(),0x20,status.data());
        put(inventory.data(),0,base+kInventoryType); put(inventory.data(),8,actor.data());
        put(status.data(),0,base+(server?stamina::kServerStatus:stamina::kClientStatus)); put(status.data(),8,actor.data());
        put(status.data(),0x18,vital.data()); put(vital.data(),0,status.data());
        put(vital.data(),0x58,entries.data()); put(vital.data(),0x60,std::uint32_t{1});
    }
};
struct Fixture {
    Address base{}; Player client{},server{};
    Bytes<0x38> cr{}; Bytes<0x50> sr{}; Bytes<0x58> cm{}; Bytes<0xb8> sm{};
    Bytes<0x100> bucket{}; std::array<Address,1> nodes{}; Bytes<0x10> node{};
    std::array<Address,1> inventories{}; Bytes<0x30> inventory{}; Bytes<0xc8> stack{};
    Bytes<0x60> im{},um{}; std::array<Address,2> item_rows{}; Bytes<0x438> item_info{};
    std::array<std::uint32_t,kMaxUses> handles{}, active_handles{}; std::array<Address,kMaxUses> use_rows{};
    std::array<Bytes<0x20>,kMaxUses> use_info{}; std::array<Bytes<0x58>,kMaxUses> use_data{};
    std::array<Bytes<8>,kMaxUses> skills{};
    std::array<Bytes<24>,kMaxUses> conditions{};
    RequestVector vector{}; std::array<Address,2> source{}; Settings selected_settings{};
    const Item* item{}; std::int64_t requested_count=1;
    std::uint64_t extra=0xfedcba9876543210, time=0x123456789abcdef0, scratch=0xfafa0b0b3c3c4d4d;
    std::uint16_t handle=7,definition_handle=1,slot_index=0;
    void* call_actor{}; void* target=reinterpret_cast<void*>(0x12345678);
    int result{}, returned{}; unsigned effects{},cooldowns{},pops{},lookup_calls{},use_calls{};
    bool expect_protection=true, nested=false, nested_protected=false, native_failure=false, throw_native=false;
    bool mutate_selection=false; Address outer_shadow{};
    explicit Fixture(const Item& selected_item) : item(&selected_item) {
        base=reinterpret_cast<Address>(VirtualAlloc(nullptr,0x6d70000,MEM_RESERVE,PAGE_NOACCESS)); assert(base);
        assert(VirtualAlloc(reinterpret_cast<void*>(base+0x6d69000),4096,MEM_COMMIT,PAGE_READWRITE));
        assert(VirtualAlloc(reinterpret_cast<void*>(base+0x6d6f000),4096,MEM_COMMIT,PAGE_READWRITE));
        client.setup(base,false);server.setup(base,true);call_actor=server.actor.data();
        put(reinterpret_cast<void*>(base+stamina::kClientRoot),0,cr.data()); put(cr.data(),0x30,cm.data());
        put(reinterpret_cast<void*>(base+stamina::kServerRoot),0,sr.data()); put(sr.data(),0x48,sm.data());
        put(cm.data(),0,base+stamina::kClientManager);put(cm.data(),0x50,client.actor.data());
        put(sm.data(),0,base+stamina::kServerManager);put(sm.data(),0x98,std::uint32_t{1});
        put(sm.data(),0xa8,bucket.data());put(sm.data(),0xb0,nodes.data());
        put(bucket.data(),0,std::uint32_t{1});put(bucket.data(),8,std::uint32_t{0x1234});put(bucket.data(),12,std::uint32_t{0});
        nodes[0]=reinterpret_cast<Address>(node.data());put(node.data(),4,std::uint32_t{0x1234});put(node.data(),8,server.actor.data());
        inventories[0]=reinterpret_cast<Address>(inventory.data());put(server.inventory.data(),0x18,inventories.data());
        put(server.inventory.data(),0x20,std::uint32_t{1});put(inventory.data(),0,stack.data());
        put(inventory.data(),0xc,std::int16_t{1});put(inventory.data(),0x10,handle);
        put(stack.data(),0,std::uint64_t{0xfeedbeef12345678});put(stack.data(),8,definition_handle);put(stack.data(),0x10,std::int64_t{5});
        put(reinterpret_cast<void*>(base+kItemManager),0,im.data());put(im.data(),8,std::uint32_t{2});
        put(im.data(),0x58,item_rows.data());item_rows[1]=reinterpret_cast<Address>(item_info.data());
        put(item_info.data(),0,item->key);std::memset(item_info.data()+0x11,0xa5,7);
        put(item_info.data(),0xa3,item->native_type);
        put(item_info.data(),0x80,handles.data());put(item_info.data(),0x88,item->count);
        put(reinterpret_cast<void*>(base+kUseManager),0,um.data());put(um.data(),8,item->count);put(um.data(),0x58,use_rows.data());
        for (std::uint32_t i=0;i<item->count;++i) {
            use_rows[i]=reinterpret_cast<Address>(use_info[i].data());put(use_info[i].data(),0,item->uses[i]);
            std::memset(use_info[i].data()+0x11,0x5a,7);put(use_info[i].data(),0x18,use_data[i].data());
            handles[i]=i;active_handles[i]=i;
            const auto& pattern=kUsePatterns[item->patterns[i]];
            auto& data=use_data[i];
            put(data.data(),0,base+use_vtable(pattern.type));
            data[0x18]=pattern.type;data[0x19]=pattern.frame;data[0x24]=pattern.cost;
            data[0x25]=pattern.gate25;data[0x26]=pattern.gate26;
            data[0x27]=pattern.flag27;data[0x28]=pattern.flag28;
            put(data.data(),0x20,pattern.parameter);
            if(pattern.conditions)put(data.data(),8,conditions[i].data());
            put(data.data(),0x10,pattern.conditions);put(data.data(),0x14,pattern.conditions);
            if(pattern.type==0) {
                put(data.data(),0x30,skills[i].data());put(data.data(),0x38,pattern.skills);put(data.data(),0x3c,pattern.skills);
                put(skills[i].data(),0,std::uint32_t{100+i});put(skills[i].data(),4,std::uint32_t{2});
            }

        }
        vector={active_handles.data(),item->count,item->count};source[0]=reinterpret_cast<Address>(&vector);
        selected_settings.count=1;selected_settings.keys[0]=item->key;
    }
    ~Fixture() {assert(VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE));}
    Fixture(const Fixture&)=delete;Fixture& operator=(const Fixture&)=delete;
    std::int64_t amount() const {return get<std::int64_t>(stack.data(),0x10);}
    Bytes<40> request() const {
        Bytes<40> bytes{};put(bytes.data(),0,std::uint16_t{0xffff});put(bytes.data(),8,static_cast<std::uint64_t>(requested_count));
        put(bytes.data(),0x10,handle);put(bytes.data(),0x12,slot_index);put(bytes.data(),0x18,extra);
        put(bytes.data(),0x22,std::uint32_t{0xffff0000});return bytes;
    }
};
thread_local Fixture* fixture{};
std::atomic<unsigned> total_calls=0;
void* __fastcall native_lookup(const std::uint32_t* handle) {
    ++fixture->lookup_calls;assert(handle && *handle<fixture->item->count);return fixture->use_info[*handle].data();
}
void* __fastcall native_pop(void* transaction,void* result,void* actor,const RequestVector* input,std::uint64_t time,
    std::uint8_t type,std::uint16_t reason,std::uint8_t flag,std::uint64_t scratch) {
    auto& f=*fixture;++f.pops;
    assert(transaction==f.inventory.data() && result==&f.result && actor==f.call_actor);
    assert(time==f.time && type==2 && reason==8 && flag==1 && scratch==f.scratch);
    // Match the relocated native consumer's write to the ninth stack home.
    // The forwarding call must reserve it even though its initial value is unused.
    auto* home=static_cast<unsigned char*>(_AddressOfReturnAddress())+0x48;
    assert(get<std::uint64_t>(home,0)==scratch);put(home,0,std::uint64_t{0});
    assert(input && (input->count==0 || input->count==1));
    if (input->count) {
        assert(input->data && get<std::uint64_t>(input->data,8)==static_cast<std::uint64_t>(f.requested_count));
        if (!f.native_failure) put(f.stack.data(),0x10,f.amount()-f.requested_count);
    }
    f.result=f.native_failure?73:0;return &f.returned;
}
void* __fastcall native_use(void*,void*,void*,void*,std::uint16_t,void*,std::uint16_t,void*,std::uint16_t,std::uint64_t);
void* invoke(Fixture& f,const Settings& settings) {
    fixture=&f;
    return dispatch_use(native_use,f.base,settings,f.server.inventory.data(),&f.result,f.call_actor,
        f.target,f.handle,&f.requested_count,f.definition_handle,f.source.data(),f.slot_index,f.extra);
}
void* __fastcall native_use(void* inventory,void* result,void* actor,void* target,std::uint16_t handle,void* amount,
    std::uint16_t definition_handle,void* source,std::uint16_t slot_index,std::uint64_t extra) {
    auto& f=*fixture;++f.use_calls;++total_calls;
    assert(inventory==f.server.inventory.data() && result==&f.result && actor==f.call_actor && target==f.target);
    assert(handle==f.handle && amount==&f.requested_count && definition_handle==f.definition_handle && source==f.source.data() && slot_index==f.slot_index && extra==f.extra);
    if (f.throw_native) throw std::runtime_error("owned callback failure");
    const auto info_before=f.use_info;const auto data_before=f.use_data;
    if (f.mutate_selection) {put(f.cm.data(),0x50,Address{0});f.expect_protection=false;}
    if (f.native_failure) {f.result=73;return &f.returned;}
    for (std::uint32_t i=0;i<f.vector.count;++i) {
        const auto index=static_cast<const std::uint32_t*>(f.vector.data)[i];
        const auto raw=f.use_info[index].data();
        assert(dispatch_lookup(native_lookup,f.base+0x2b49dac,&index)==raw);
        if (f.use_data[index][0x18]==0) {
            auto* info=dispatch_lookup(native_lookup,f.base+kSkillLookupReturn,&index);
            auto* data=reinterpret_cast<const void*>(get<Address>(info,0x18));
            const bool protected_record=f.expect_protection && f.use_data[index][0x24];
            assert((info!=raw)==protected_record);
            if (protected_record) {
                const auto copied_data=static_cast<const unsigned char*>(data);
                for (unsigned j=0;j<0x40;++j) assert(copied_data[j]==(j==0x24?0:f.use_data[index][j]));
                auto copy=f.use_info[index];put(copy.data(),0x18,get<Address>(info,0x18));
                assert(!std::memcmp(info,copy.data(),copy.size()));
                assert(dispatch_lookup(native_lookup,f.base+kSkillLookupReturn,&index)==info);
                if (f.outer_shadow) assert(f.outer_shadow!=reinterpret_cast<Address>(info));
            }
            if (f.nested) {
                f.nested=false;const bool expected=f.expect_protection;
                const auto previous=f.outer_shadow;f.outer_shadow=reinterpret_cast<Address>(info);
                f.expect_protection=f.nested_protected;
                assert(invoke(f,f.nested_protected?f.selected_settings:Settings{})==&f.returned);
                f.outer_shadow=previous;f.expect_protection=expected;
                assert(current && current->valid);
                assert(get<std::uint8_t>(data,0x24)==(protected_record?0:f.use_data[index][0x24]));
            }
            if (get<std::uint8_t>(data,0x24)) put(f.stack.data(),0x10,f.amount()-f.requested_count);
            ++f.effects;
            assert(get<Address>(data,0x30)==reinterpret_cast<Address>(f.skills[index].data()));
            assert(get<std::uint8_t>(data,0x25)==f.use_data[index][0x25] && get<std::uint8_t>(data,0x26)==f.use_data[index][0x26]);
        } else if (f.item->kind==Kind::Arrow && index==2) {
            auto bytes=f.request();const RequestVector input{bytes.data(),1,1};
            assert(dispatch_pop(native_pop,f.base+kArrowPopReturn,f.inventory.data(),&f.result,actor,&input,f.time,2,8,1,f.scratch)==&f.returned);
        } else {
            // These handlers have their own native lookup sites. Even a
            // deliberately spoofed Skill return cannot shadow another type.
            assert(dispatch_lookup(native_lookup,f.base+kSkillLookupReturn,&index)==raw);
            assert(dispatch_lookup(native_lookup,f.base+0x2b50c9f,&index)==raw);
            if(f.use_data[index][0x18]!=16 && f.use_data[index][0x24]) {
                auto bytes=f.request();const RequestVector input{bytes.data(),1,1};
                assert(dispatch_pop(native_pop,f.base+0x2b50f24,f.inventory.data(),&f.result,actor,&input,f.time,2,8,1,f.scratch)==&f.returned);
            }
            ++f.effects;
        }
    }
    ++f.cooldowns;f.result=0;
    assert(f.use_info==info_before && f.use_data==data_before);return &f.returned;
}
void all_supported() {
    unsigned skill_records{},cost_records{},other_records{};
    for (const auto& item:kItems) {
        Fixture f(item);
        if(item.kind==Kind::Arrow || item.kind==Kind::Potion) {
            const auto before=f.stack;assert(invoke(f,f.selected_settings)==&f.returned);
            assert(f.amount()==5 && f.stack==before && f.cooldowns==1 && f.result==0 && !current);
            assert(f.effects==(item.kind==Kind::Potion?item.count:2));
            f.expect_protection=false;assert(invoke(f,Settings{})==&f.returned);
            assert(f.amount()==4 && f.cooldowns==2 && !current);
            continue;
        }
        // Native frame/eligibility filtering selects a subset of the complete
        // registered family. Exercise EVERY row individually, never pretend
        // that eating, feeding and releasing all run in one real request.
        for(std::uint32_t i=0;i<item.count;++i) {
            f.active_handles[0]=i;f.vector.count=1;
            put(f.stack.data(),0x10,std::int64_t{5});f.expect_protection=true;
            const auto effects=f.effects,cooldowns=f.cooldowns;
            const auto cost=f.use_data[i][0x24];const auto type=f.use_data[i][0x18];
            assert(invoke(f,f.selected_settings)==&f.returned && !current);
            assert(f.effects==effects+1 && f.cooldowns==cooldowns+1);
            if(type==0) {
                ++skill_records;cost_records+=cost;
                assert(f.amount()==5);
                f.expect_protection=false;
                assert(invoke(f,Settings{})==&f.returned && !current);
                assert(f.amount()==5-cost);
            } else {
                ++other_records;
                assert(f.amount()==5-(type==16?0:cost));
            }
        }
    }
    assert(skill_records==3537 && cost_records>1500 && other_records==2396);
}
void type_mapping_guards() {
    // These assertions cover the compiled semantic mapping and rejection paths.
    // They do not establish that mapping from an original game constructor.
    assert(use_vtable(16)==0x59bf3c0 && use_vtable(16)!=kProjectileType);
    assert(use_vtable(12)==kProjectileType && use_vtable(8)!=use_vtable(19));
    Fixture f(*supported(50001));f.expect_protection=false;
    put(f.use_data[0].data(),0,f.base+kProjectileType);
    assert(invoke(f,f.selected_settings)==&f.returned && f.amount()==4 && !current);
}
#ifdef CRIMSON_PRIVATE_CONSUMABLE_EVIDENCE_HEADER
// Optional local original-image evidence. Raw executable/resource bytes are
// supplied by a private header and are never needed by the public test suite.
void original_factory_mapping() {
    // Execute only copied constructor arithmetic/stores against owned bytes.
    // This catches the old impossible Type16 + Projectile-vtable combination;
    // seeded C++ objects alone could never establish that mapping.
    const unsigned char prologue[]{0x57,0x41,0x57,0x48,0x8b,0xf9,0x45,0x33,0xff};
    const unsigned char epilogue[]{0x41,0x5f,0x5f,0xc3};
    for(const auto& c:evidence::kFactoryCases) {
        auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));assert(code);
        std::memcpy(code,prologue,sizeof(prologue));std::memcpy(code+sizeof(prologue),c.bytes.data(),c.size);
        std::memcpy(code+sizeof(prologue)+c.size,epilogue,sizeof(epilogue));
        for(std::size_t i=0;i<c.relocations.size();++i) {
            const auto offset=sizeof(prologue)+c.relocations[i];
            const auto target=std::uintptr_t{0x300}+i*0x40;
            const auto displacement=static_cast<std::int32_t>(target-offset-4);
            put(code,offset,displacement);
        }
        DWORD old{};assert(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));
        assert(FlushInstructionCache(GetCurrentProcess(),code,4096));
        Bytes<0x58> data{};data.fill(0xa5);
        reinterpret_cast<void(__fastcall*)(void*)>(code)(data.data());
        assert(get<Address>(data.data(),0)==reinterpret_cast<Address>(code+0x340));
        assert(data[0x18]==c.type && data[0x19]==13 && data[0x24]==0);
        assert(use_vtable(c.type)==c.vtable);
        assert(VirtualFree(code,0,MEM_RELEASE));
    }
    assert(use_vtable(16)==0x59bf3c0 && use_vtable(16)!=kProjectileType);
    assert(use_vtable(12)==kProjectileType && use_vtable(8)!=use_vtable(19));
    Fixture f(*supported(50001));f.expect_protection=false;
    put(f.use_data[0].data(),0,f.base+kProjectileType);
    assert(invoke(f,f.selected_settings)==&f.returned && f.amount()==4 && !current);
}
struct OwnedStream {
    const void* const* vtable{};
    const unsigned char* bytes{};
    std::size_t size{},cursor{};
};
[[noreturn]] void unexpected_decoder_call() {std::abort();}
bool __fastcall read_owned_bytes(OwnedStream* stream,void* destination,std::uint32_t count) {
    assert(stream && destination && stream->cursor<=stream->size && count<=stream->size-stream->cursor);
    std::memcpy(destination,stream->bytes+stream->cursor,count);stream->cursor+=count;return true;
}
std::uint16_t owned_key(std::uint32_t key) {return key?static_cast<std::uint16_t>(1+key%65534):std::uint16_t{0xffff};}
bool __fastcall read_owned_key(OwnedStream* stream,std::uint16_t* destination) {
    std::uint32_t key{};read_owned_bytes(stream,&key,4);*destination=owned_key(key);return true;
}
std::uint16_t* __fastcall map_owned_key(std::uint16_t* destination,std::uint32_t key) {*destination=owned_key(key);return destination;}
void __fastcall reserve_owned_skills(void* array,std::uint32_t desired) {
    assert(array && get<Address>(array,0) && desired<=get<std::uint32_t>(array,12));
}
class OwnedDecoders {
    unsigned char* code{};
public:
    using Decode=bool(__fastcall*)(void*,OwnedStream*);
    OwnedDecoders() {
        code=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x4000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));assert(code);
        const auto common=reinterpret_cast<Address>(code);
        const auto fail=reinterpret_cast<Address>(&unexpected_decoder_call);
        put(code,0x3000,fail);
        std::size_t thunk=0x2000;
        for(std::size_t index=0;index<evidence::kDecoderCases.size();++index) {
            const auto& c=evidence::kDecoderCases[index];auto* body=code+index*0x400;
            std::memcpy(body,c.bytes.data(),c.size);
            for(std::size_t j=0;j<c.call_count;++j) {
                const auto& call=c.calls[j];Address target=fail;
                switch(call.target) {
                case 0x241f230:target=common;break;
                case 0x1523a20:case 0x151ef60:case 0x1522380:target=reinterpret_cast<Address>(&read_owned_key);break;
                case 0x38ac90:target=reinterpret_cast<Address>(&map_owned_key);break;
                case 0x3d4130:target=reinterpret_cast<Address>(&reserve_owned_skills);break;
                }
                // Every external relative call is replaced, including every
                // unexecuted allocation/error path. It can never reach the EXE.
                code[thunk]=0x48;code[thunk+1]=0xb8;put(code,thunk+2,target);
                code[thunk+10]=0xff;code[thunk+11]=0xe0;
                const auto displacement=static_cast<std::int32_t>(thunk-(index*0x400+call.displacement+4));
                put(body,call.displacement,displacement);thunk+=16;
            }
            for(std::size_t j=0;j<c.indirect_count;++j) {
                // Owned fail pointer also replaces dormant imported error calls
                // and diagnostic-string RIP references. No real imports remain.
                const auto offset=c.indirect[j];
                put(body,offset,static_cast<std::int32_t>(0x3000-(index*0x400+offset+4)));
            }
        }
        assert(thunk<0x3000);DWORD old{};assert(VirtualProtect(code,0x4000,PAGE_EXECUTE_READ,&old));
        assert(FlushInstructionCache(GetCurrentProcess(),code,0x4000));
    }
    ~OwnedDecoders(){assert(VirtualFree(code,0,MEM_RELEASE));}
    Decode at(std::uint8_t type) const {
        const auto index=type==0?1:type==8?2:type==16?3:type==12?4:0;
        return reinterpret_cast<Decode>(code+index*0x400);
    }
};
void original_resource_decoder() {
    OwnedDecoders native;
    const void* reader_vtable[]{reinterpret_cast<const void*>(&unexpected_decoder_call),reinterpret_cast<const void*>(&read_owned_bytes)};
    for(const auto& c:evidence::kRawUseCases) {
        assert(c.pattern<kUsePatterns.size());const auto& expected=kUsePatterns[c.pattern];
        Bytes<0x58> data{};Bytes<24> conditions{};Bytes<8> skill{};
        data[0x18]=expected.type;data[0x19]=13;
        put(data.data(),8,conditions.data());put(data.data(),0x14,std::uint32_t{4});
        if(expected.type==0) {put(data.data(),0x30,skill.data());put(data.data(),0x3c,std::uint32_t{1});}
        OwnedStream stream{reader_vtable,c.bytes.data(),c.size,0};
        assert(native.at(expected.type)(data.data(),&stream));
        const bool derived=expected.type==0 || expected.type==8 || expected.type==16;
        assert(stream.cursor==(derived?c.size:c.common_size));
        assert(data[0x18]==expected.type && data[0x19]==expected.frame);
        assert(data[0x24]==expected.cost && data[0x25]==expected.gate25 && data[0x26]==expected.gate26);
        assert(data[0x27]==expected.flag27 && data[0x28]==expected.flag28);
        assert(get<std::uint32_t>(data.data(),0x20)==expected.parameter);
        assert(get<std::uint32_t>(data.data(),0x10)==expected.conditions);
        for(std::uint32_t i=0;i<expected.conditions;++i) {
            const auto serialized=std::size_t{22}+i*9;
            assert(get<std::uint16_t>(conditions.data(),i*6)==owned_key(get<std::uint32_t>(c.bytes.data(),serialized)));
            assert(conditions[i*6+2]==c.bytes[serialized+4]);
            assert(get<std::uint16_t>(conditions.data(),i*6+4)==owned_key(get<std::uint32_t>(c.bytes.data(),serialized+5)));
        }
        if(expected.type==0) {
            assert(get<Address>(data.data(),0x30)==reinterpret_cast<Address>(skill.data()));
            assert(get<std::uint32_t>(data.data(),0x38)==1);
            assert(get<std::uint16_t>(skill.data(),0)==owned_key(get<std::uint32_t>(c.bytes.data(),c.common_size+4)));
            assert(get<std::uint32_t>(skill.data(),4)==get<std::uint32_t>(c.bytes.data(),c.common_size+8));
        } else if(expected.type==8) {
            assert(get<std::uint16_t>(data.data(),0x30)==owned_key(get<std::uint32_t>(c.bytes.data(),c.common_size)));
            assert(get<std::uint32_t>(data.data(),0x34)==get<std::uint32_t>(c.bytes.data(),c.common_size+4));
            assert(data[0x38]==c.bytes[c.common_size+8]);
        } else if(expected.type==16) {
            assert(get<std::uint16_t>(data.data(),0x30)==owned_key(get<std::uint32_t>(c.bytes.data(),c.common_size)));
            assert(data[0x32]==c.bytes[c.common_size+4]);
        }
    }
}
#endif
void healing_food_guards() {
    for(unsigned test=0;test<19;++test) {
        Fixture f(*supported(22009));fixture=&f;f.active_handles[0]=5;f.vector.count=1;
        Context c{};
        const auto begin_context=[&] {return guarded_begin(c,f.base,f.selected_settings,f.server.inventory.data(),f.call_actor,f.handle,&f.requested_count,f.definition_handle,f.source.data(),f.slot_index,f.extra);};
        assert(begin_context());
        switch(test) {
        case 0:put(f.item_info.data(),0xa3,std::uint8_t{19});break;
        case 1:put(f.item_info.data(),0x88,std::uint32_t{18});break;
        case 2:put(f.use_info[7].data(),0,std::uint32_t{1});break;
        case 3:put(f.use_data[0].data(),0,f.base+kProjectileType);break;
        case 4:put(f.use_data[3].data(),0,f.base+use_vtable(19));break;
        case 5:f.use_data[5][0x18]=19;break;
        case 6:f.use_data[5][0x19]^=1;break;
        case 7:f.use_data[5][0x24]^=1;break;
        case 8:f.use_data[5][0x25]^=1;break;
        case 9:f.use_data[5][0x26]^=1;break;
        case 10:f.use_data[5][0x27]^=1;break;
        case 11:f.use_data[5][0x28]^=1;break;
        case 12:put(f.use_data[5].data(),0x20,std::uint32_t{1});break;
        case 13:put(f.use_data[5].data(),0x10,std::uint32_t{0});break;
        case 14:put(f.use_data[5].data(),8,Address{0});break;
        case 15:put(f.use_data[5].data(),0x30,Address{0});break;
        case 16:put(f.use_data[5].data(),0x38,std::uint32_t{2});break;
        case 17:f.vector.count=18;break;
        case 18:f.active_handles[1]=5;f.vector.count=2;break;
        }
        assert(!begin_context());
    }
    // Stable per-record shadow addresses survive more than three records.
    const auto* item=supported(756513);assert(item); // Wild Insam: four independent Skill self-cost rows.
    Fixture f(*item);fixture=&f;Context c{};
    c.valid=guarded_begin(c,f.base,f.selected_settings,f.server.inventory.data(),f.call_actor,f.handle,&f.requested_count,f.definition_handle,f.source.data(),f.slot_index,f.extra);assert(c.valid);
    std::vector<std::pair<void*,unsigned>> views;
    for(std::uint32_t i=0;i<f.item->count;++i) {
        if(f.use_data[i][0x18] || !f.use_data[i][0x24])continue;
        auto* view=guarded_lookup(c,f.base+kSkillLookupReturn,&i,f.use_info[i].data());assert(view!=f.use_info[i].data());
        views.emplace_back(view,i);
    }
    assert(views.size()>3 && c.shadow_count==views.size());
    for(const auto& [view,index]:views) {
        assert(get<std::uint32_t>(view,0)==f.item->uses[index]);
        const auto* data=reinterpret_cast<const unsigned char*>(get<Address>(view,0x18));
        assert(!data[0x24]);
        for(std::size_t i=0;i<0x40;++i)assert(data[i]==(i==0x24?0:f.use_data[index][i]));
    }
    // A changed unconsumed family row invalidates a retained receipt too.
    f.use_data[0][0x28]^=1;
    const auto index=static_cast<std::uint32_t>(views[0].second);
    assert(guarded_lookup(c,f.base+kSkillLookupReturn,&index,f.use_info[index].data())==f.use_info[index].data());
}
void settings_tests() {
    Bytes<8+kItems.size()*4> bytes{};std::memcpy(bytes.data(),"CWC1",4);put(bytes.data(),4,static_cast<std::uint32_t>(kItems.size()));
    for (std::size_t i=0;i<kItems.size();++i) put(bytes.data(),8+i*4,kItems[i].key);
    assert(setting(bytes.data(),bytes.size()).count==kItems.size());
    assert(!setting(nullptr,bytes.size()).count && !setting(bytes.data(),bytes.size()-1).count && !setting(bytes.data(),bytes.size()+1).count);
    bytes[0]='X';assert(!setting(bytes.data(),bytes.size()).count);bytes[0]='C';
    put(bytes.data(),4,static_cast<std::uint32_t>(kItems.size()+1));assert(!setting(bytes.data(),bytes.size()).count);
    put(bytes.data(),4,static_cast<std::uint32_t>(kItems.size()));
    put(bytes.data(),8,kItems[1].key);assert(!setting(bytes.data(),bytes.size()).count);
    put(bytes.data(),8,std::uint32_t{50057});assert(!setting(bytes.data(),bytes.size()).count);
    const unsigned char empty[]{'C','W','C','1',0,0,0,0};assert(!setting(empty,8).count);
    Settings invalid{};invalid.count=1000;assert(!invalid.contains(1));
}
void scope_tests() {
    for (unsigned test=0;test<24;++test) {
        Fixture f(*supported(1000028));f.expect_protection=false;
        switch(test) {
        case 0:put(f.cm.data(),0x50,Address{0});break;
        case 1:put(f.server.actor.data(),0x60,std::uint32_t{0xb0101234});break;
        case 2:put(f.server.inventory.data(),8,f.client.actor.data());break;
        case 3:put(f.client.components.data(),0xb8,Address{0});break;
        case 4:f.server.metadata[1]=6;break;
        case 5:put(f.server.actor.data(),0x96,std::uint8_t{0});break;
        case 6:put(f.sm.data(),0x98,std::uint32_t{0});break;
        case 7:put(f.bucket.data(),0,std::uint32_t{32});break;
        case 8:put(f.node.data(),4,std::uint32_t{0x1235});break;
        case 9:put(f.inventory.data(),0x10,std::uint16_t{8});break;
        case 10:put(f.inventory.data(),0xc,std::int16_t{0});break;
        case 11:put(f.stack.data(),0,std::uint64_t{0});break;
        case 12:put(f.item_info.data(),0x10,std::uint8_t{1});break;
        case 13:put(f.item_info.data(),0,std::uint32_t{50057});break;
        case 14:put(f.item_info.data(),0xa3,std::uint8_t{0});break;
        case 15:put(f.item_info.data(),0x88,std::uint32_t{2});break;
        case 16:put(f.use_info[0].data(),0x10,std::uint8_t{1});break;
        case 17:put(f.use_info[0].data(),0,std::uint32_t{1006680});break;
        case 18:put(f.use_data[0].data(),0,Address{0});break;
        case 19:put(f.use_data[0].data(),0x19,std::uint8_t{8});break;
        case 20:put(f.use_data[0].data(),0x38,std::uint32_t{2});break;
        case 21:f.call_actor=f.client.actor.data();break;
        case 22:put(f.server.possessor.data(),0xd0,f.client.actor.data());break;
        case 23:put(f.server.components.data(),0xb8,f.client.inventory.data());break;
        }
        assert(invoke(f,f.selected_settings)==&f.returned && f.amount()==4 && !current);
    }
    {Fixture f(*supported(1000028));f.requested_count=6;f.expect_protection=false;assert(invoke(f,f.selected_settings)==&f.returned && !current);}
    {Fixture f(*supported(1000028));f.mutate_selection=true;assert(invoke(f,f.selected_settings)==&f.returned && f.amount()==4);}
    {Fixture f(*supported(1000034));f.nested=true;f.nested_protected=false;assert(invoke(f,f.selected_settings)==&f.returned && f.amount()==4 && f.effects==6 && !current);}
    {Fixture f(*supported(1000028));f.nested=true;f.nested_protected=true;assert(invoke(f,f.selected_settings)==&f.returned && f.amount()==5 && f.effects==2 && !current);}
    {Fixture f(*supported(1000028));f.native_failure=true;assert(invoke(f,f.selected_settings)==&f.returned && f.result==73 && f.amount()==5 && !current);}
    {Fixture f(*supported(1000028));f.throw_native=true;try {invoke(f,f.selected_settings);assert(false);}catch(const std::runtime_error&){}assert(!current);}
    {Context c{};Settings s{};s.count=1;s.keys[0]=1000028;assert(!guarded_begin(c,1,s,reinterpret_cast<void*>(1),reinterpret_cast<void*>(1),1,reinterpret_cast<void*>(1),1,reinterpret_cast<void*>(1),1,1));}
}
void arrow_guards() {
    Fixture f(*supported(50001));fixture=&f;Context c{};
    c.valid=guarded_begin(c,f.base,f.selected_settings,f.server.inventory.data(),f.server.actor.data(),f.handle,&f.requested_count,f.definition_handle,f.source.data(),f.slot_index,f.extra);assert(c.valid);
    auto bytes=f.request();RequestVector input{bytes.data(),1,1};
    const auto check=[&](Address caller,void* actor,const RequestVector* vector,std::uint8_t type,std::uint16_t reason,std::uint8_t flag) {return guarded_arrow(c,caller,actor,vector,type,reason,flag);};
    assert(check(f.base+kArrowPopReturn,f.call_actor,&input,2,8,1));
    assert(!check(f.base+kArrowPopReturn+1,f.call_actor,&input,2,8,1));
    assert(!check(f.base+kArrowPopReturn,f.client.actor.data(),&input,2,8,1));
    assert(!check(f.base+kArrowPopReturn,f.call_actor,&input,1,8,1));
    assert(!check(f.base+kArrowPopReturn,f.call_actor,&input,2,7,1));
    assert(!check(f.base+kArrowPopReturn,f.call_actor,&input,2,8,0));
    assert(!check(f.base+kArrowPopReturn,f.call_actor,reinterpret_cast<RequestVector*>(1),2,8,1));
    for (unsigned test=0;test<8;++test) {
        bytes=f.request();input={bytes.data(),1,1};
        switch(test) {
        case 0:put(bytes.data(),0,std::uint16_t{1});break;
        case 1:put(bytes.data(),8,std::uint64_t{2});break;
        case 2:put(bytes.data(),0x10,std::uint16_t{8});break;
        case 3:put(bytes.data(),0x12,std::uint16_t{1});break;
        case 4:put(bytes.data(),0x18,std::uint64_t{0});break;
        case 5:put(bytes.data(),0x22,std::uint32_t{0});break;
        case 6:input.count=2;break;
        case 7:input.data=nullptr;break;
        }
        assert(!check(f.base+kArrowPopReturn,f.call_actor,&input,2,8,1));
    }
    bytes=f.request();input={bytes.data(),1,1};
    current=&c;
    // Crafting/shop/quest callers retain the original cost even with an active context.
    assert(dispatch_pop(native_pop,f.base+0x1,f.inventory.data(),&f.result,f.call_actor,&input,f.time,2,8,1,f.scratch)==&f.returned && f.amount()==4);
    f.native_failure=true;
    assert(dispatch_pop(native_pop,f.base+kArrowPopReturn,f.inventory.data(),&f.result,f.call_actor,&input,f.time,2,8,1,f.scratch)==&f.returned && f.result==73 && f.amount()==4);
    put(f.stack.data(),0,std::uint64_t{123});assert(!check(f.base+kArrowPopReturn,f.call_actor,&input,2,8,1));
    current=nullptr;
}
void pointer_guards() {
    Fixture f(*supported(1000028));fixture=&f;Context c{};
    c.valid=guarded_begin(c,f.base,f.selected_settings,f.server.inventory.data(),f.call_actor,f.handle,&f.requested_count,f.definition_handle,f.source.data(),f.slot_index,f.extra);assert(c.valid);
    const std::uint32_t index=0;auto* raw=f.use_info[0].data();
    auto* shadow=guarded_lookup(c,f.base+kSkillLookupReturn,&index,raw);assert(shadow!=raw);
    f.use_data[0][0x28]^=1;assert(guarded_lookup(c,f.base+kSkillLookupReturn,&index,raw)==raw);f.use_data[0][0x28]^=1;
    assert(guarded_lookup(c,f.base+kSkillLookupReturn,reinterpret_cast<const std::uint32_t*>(1),raw)==raw);
    assert(guarded_lookup(c,f.base+kSkillLookupReturn+1,&index,raw)==raw);
    put(f.stack.data(),0x10,std::int64_t{0});assert(guarded_lookup(c,f.base+kSkillLookupReturn,&index,raw)==raw);
}
struct HookBackend {
    std::atomic<bool>& enabled;
    unsigned operation{},fail{},created{},removed{},disabled{},initialized{},uninitialized{},pins{},applied{};
    bool fail_disable{},fail_remove{};
    bool step() {assert(!enabled.load());return ++operation!=fail;}
    bool pin() {if(!step())return false;++pins;return true;}
    bool initialize() {if(!step())return false;++initialized;return true;}
    bool create(Address,Address,Address& trampoline) {if(!step())return false;++created;trampoline=0xfeed0000+created;return true;}
    bool queue_enable(Address) {return step();}
    bool apply() {++applied;return step();}
    bool disable(Address) {assert(!enabled.load());++disabled;return !fail_disable;}
    bool remove(Address) {assert(!enabled.load());if(fail_remove)return false;++removed;return true;}
    bool uninitialize() {++uninitialized;return true;}
};
void lifecycle_tests() {
    const std::array<Address,3> entries{11,22,33};
    for(unsigned fail=1;fail<=10;++fail) {
        std::atomic<bool> enabled=false;HookBackend backend{enabled};backend.fail=fail;
        HookTransaction transaction(backend,enabled);
        assert(!transaction.install(0x140000000,entries,[](Address,std::size_t){return true;},[&](const std::array<Address,3>& original) {
            for(auto value:original)assert(value);return backend.step();
        }));
        assert(!enabled.load());
        if(backend.applied) {assert(backend.created==3 && backend.disabled==3 && backend.removed==0 && backend.uninitialized==0);}
        else {assert(backend.created==backend.removed && backend.initialized==backend.uninitialized);}
    }
    {std::atomic<bool> enabled=false;HookBackend backend{enabled};HookTransaction transaction(backend,enabled);
        assert(!transaction.install(1,entries,[](Address,std::size_t i){return i!=1;},[](auto){return true;}));assert(!backend.operation);}
    {std::atomic<bool> enabled=false;HookBackend backend{enabled};backend.fail=9;backend.fail_disable=true;
        HookTransaction transaction(backend,enabled);
        assert(!transaction.install(1,entries,[](Address,std::size_t){return true;},[&](auto){return backend.step();}));
        assert(!enabled.load() && backend.created==3 && backend.disabled==3 && backend.removed==0 && !backend.uninitialized);
        backend.fail_disable=false;backend.fail_remove=true;assert(!transaction.cleanup() && !backend.removed && !backend.uninitialized);
        backend.fail_remove=false;assert(transaction.cleanup() && backend.removed==3 && backend.uninitialized==1);}
    {std::atomic<bool> enabled=false;HookBackend backend{enabled};HookTransaction transaction(backend,enabled);
        assert(transaction.install(1,entries,[](Address,std::size_t){return true;},[&](auto){return backend.step();}));
        assert(enabled.load() && transaction.active());backend.fail_disable=true;assert(!transaction.cleanup());
        assert(!enabled.load() && backend.removed==0 && backend.uninitialized==0);
        backend.fail_disable=false;assert(transaction.cleanup() && backend.removed==0 && backend.uninitialized==0);
        assert(!transaction.install(1,entries,[](Address,std::size_t){return true;},[](auto){return true;}));}
}
} // namespace
int main() {
    type_mapping_guards();
#ifdef CRIMSON_PRIVATE_CONSUMABLE_EVIDENCE_HEADER
    original_factory_mapping();original_resource_decoder();
#endif
    settings_tests();all_supported();healing_food_guards();scope_tests();arrow_guards();pointer_guards();lifecycle_tests();
    std::thread a([] {for(unsigned i=0;i<50;++i){Fixture f(*supported(50001));assert(invoke(f,f.selected_settings)==&f.returned && f.amount()==5 && !current);}});
    std::thread b([] {for(unsigned i=0;i<50;++i){Fixture f(*supported(1000034));assert(invoke(f,f.selected_settings)==&f.returned && f.amount()==5 && !current);}});
    a.join();b.join();
    std::printf("PASS: 561 selected item families, every mixed healing/food row, owned-stack scope, exact native ABI/results, unmodified definitions/effects/cooldowns, nested and concurrent contexts, fault fallback (%u owned callback calls).\n",total_calls.load());
}
