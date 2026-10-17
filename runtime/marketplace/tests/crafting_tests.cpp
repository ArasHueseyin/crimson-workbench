#include "../crafting/crafting_policy.h"
#include "../crafting/crafting_shadow.h"
#include "../crafting/crafting_hooks.h"
#include <array>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <intrin.h>
#include <thread>
#include <vector>

namespace {
using namespace crimson::marketplace::crafting;
template<class T> void put(void* at,std::size_t offset,T value) { std::memcpy(static_cast<unsigned char*>(at)+offset,&value,sizeof(value)); }
extern "C" void crimson_crafting_fixture_probe(void*,Address);
extern "C" void crimson_crafting_fixture_resume();
extern "C" void crimson_crafting_fixture_alternate();
extern "C" void crimson_crafting_fixture_clobber();
extern "C" std::uint32_t crimson_crafting_fixture_bad_alignment;
extern "C" std::uint32_t crimson_crafting_fixture_stable_pointer;
struct alignas(64) Probe {
    std::array<unsigned char,512> original{},before{},after{},before_ymm{},after_ymm{};
    std::array<std::uint64_t,16> observed{};
    Address expected_rsp{},observed_rsp{};
    std::array<unsigned char,64> saved_state{};
    std::array<unsigned char,48> alignment_padding{};
};
static_assert(offsetof(Probe,before)==512 && offsetof(Probe,after)==1024);
static_assert(offsetof(Probe,observed)==2560 && offsetof(Probe,expected_rsp)==2688 && offsetof(Probe,observed_rsp)==2696);
std::atomic<unsigned> callbacks{},mode{};
void check_probe(Address entry,bool edited=false);
void callback(Context& context,Site site) noexcept {
    assert(static_cast<std::size_t>(site)<kSiteCount && (__readeflags() & 0x400)==0);
    ++callbacks;
    const auto support=state_support();
    const auto stack=(reinterpret_cast<Address>(&context)-(support.bytes+128)) & ~Address{63};
    const auto probe=*reinterpret_cast<Probe**>(context.interrupted_rsp()+160);
    std::memcpy(probe->saved_state.data(),reinterpret_cast<void*>(stack+64),probe->saved_state.size());
    static thread_local bool recursive=false;
    if(mode.load()==2 && !recursive) { recursive=true;check_probe(bridge_entries()[1]);recursive=false; }
    if(mode.load()==3) { Context invalid{};invalid.rbx=0x10000;assert(!guarded_decide(0x10000,Site::Database,invalid).continuation); }
    if (mode.load()==1) {
        const Edit edit{reinterpret_cast<Address>(&crimson_crafting_fixture_alternate),true,0x1357,true,40};
        apply(context,edit);
    }
    crimson_crafting_fixture_clobber();
}
void check_probe(Address entry,bool edited) {
    Probe probe;
    crimson_crafting_fixture_probe(&probe,entry);
    constexpr std::array<unsigned,15> register_order{15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};
    for (std::size_t i=0;i<register_order.size();++i) {
        auto expected=std::uint64_t{0x1111111111111111}*register_order[i];
        if (edited && i==8) expected=40; // EDI changes only when requested.
        if (edited && i==11) expected=(expected & ~std::uint64_t{0xffff})|0x1357;
        assert(probe.observed[i]==expected);
    }
    assert((probe.observed[15]&0xcd5)==0xcd5 && probe.expected_rsp==probe.observed_rsp);
    // FXSAVE reserved bytes are not architectural state. Compare defined x87,
    // MXCSR and complete XMM portions; all AVX upper halves are compared below.
    assert(std::memcmp(probe.before.data(),probe.after.data(),5)==0);
    if(std::memcmp(probe.before.data()+6,probe.after.data()+6,26)!=0) {
        for(unsigned i=0;i<32;++i)if(probe.before[i]!=probe.after[i])std::fprintf(stderr,"FP difference byte %u: %02x -> %02x (saved %02x)\n",i,probe.before[i],probe.after[i],probe.saved_state[i]);
        assert(false);
    }
    for (unsigned i=0;i<8;++i) assert(std::memcmp(probe.before.data()+32+i*16,probe.after.data()+32+i*16,10)==0);
    assert(std::memcmp(probe.before.data()+160,probe.after.data()+160,256)==0);
    if (state_support().avx) assert(probe.before_ymm==probe.after_ymm);
}
struct Fixture {
    Address base{};
    std::array<unsigned char,0x90> manager{},knowledge_manager{};
    std::vector<Address> rows=std::vector<Address>(kRecipeCount);
    std::array<unsigned char,0x100> row{},bucket{};
    std::array<unsigned char,16> knowledge_node{};
    std::array<Address,1> knowledge_nodes{};
    std::array<unsigned char,0xc00> frame{};
    std::array<unsigned char,0x300> ui{};
    std::array<unsigned char,0x40> root{};
    std::array<unsigned char,0x58> player_manager{};
    std::array<unsigned char,0xb0> actor{};
    std::array<unsigned char,0x1d0> components{};
    std::array<unsigned char,2> metadata{};
    std::array<unsigned char,0xd8> possessor{};
    std::array<unsigned char,0x350> crafting{};
    std::array<std::uint64_t,2> references{};
    std::uint16_t handle=42;
    static constexpr std::uint16_t knowledge_handle=17;
    Fixture() {
        base=reinterpret_cast<Address>(VirtualAlloc(nullptr,0x6d6f000,MEM_RESERVE,PAGE_NOACCESS));
        assert(base);
        assert(VirtualAlloc(reinterpret_cast<void*>(base+0x6d69000),4096,MEM_COMMIT,PAGE_READWRITE));
        assert(VirtualAlloc(reinterpret_cast<void*>(base+0x6d6e000),4096,MEM_COMMIT,PAGE_READWRITE));
        put(reinterpret_cast<void*>(base+kRecipeManager),0,manager.data());
        put(manager.data(),8,kRecipeCount); put(manager.data(),0x58,rows.data()); rows[handle]=reinterpret_cast<Address>(row.data());
        put(reinterpret_cast<void*>(base+kKnowledgeManager),0,knowledge_manager.data());
        put(knowledge_manager.data(),8,std::uint32_t{100});
        put(knowledge_manager.data(),0x68,std::uint32_t{1}); put(knowledge_manager.data(),0x6c,std::uint32_t{1});
        put(knowledge_manager.data(),0x78,bucket.data()); put(knowledge_manager.data(),0x80,knowledge_nodes.data());
        knowledge_nodes[0]=reinterpret_cast<Address>(knowledge_node.data());
        put(bucket.data(),0,std::uint32_t{1}); put(bucket.data(),12,std::uint32_t{0});
        put(knowledge_node.data(),8,knowledge_handle);
        put(reinterpret_cast<void*>(base+0x6d69190),0,root.data()); put(root.data(),0x30,player_manager.data());
        put(player_manager.data(),0,base+0x55b62c8); put(player_manager.data(),0x50,actor.data());
        put(actor.data(),0,base+0x5585c40); put(actor.data(),0x60,std::uint32_t{0xa0101234});
        put(actor.data(),0x68,components.data()); put(actor.data(),0x70,std::uint32_t{46});
        put(actor.data(),0x88,metadata.data()); metadata[1]=1; actor[0x96]=1;
        put(actor.data(),0xa0,possessor.data()); put(possessor.data(),0xd0,actor.data());
        put(components.data(),0x168,crafting.data()); put(crafting.data(),8,actor.data()); put(crafting.data(),0x2e8,handle);
        put(frame.data(),0,actor.data());
        references[0]=reinterpret_cast<Address>(&handle); references[1]=1;
        set_recipe(kOrdinaryRecipes[0]);
    }
    ~Fixture() { assert(VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE)); }
    void set_knowledge(std::uint32_t key) {
        put(bucket.data(),8,key); put(knowledge_node.data(),4,key);
    }
    void set_recipe(const KnownRecipe& known) {
        row.fill(0); put(row.data(),0,base+kRecipeVtable); put(row.data(),8,known.key); row[0x1c]=1;
        put(row.data(),0x28,known.conditions); put(row.data(),0xe8,std::uint32_t{1});
        put(row.data(),0x30,known.knowledge ? knowledge_handle : std::uint16_t{0xffff});
        for (std::size_t i=0;i<5;++i) row[0x38+i]=known.flags[i];
        set_knowledge(known.knowledge ? known.knowledge : 1000689);
    }
    Context context(Site site) {
        Context context{}; context.continuation=0x12345678;
        context.rbp=reinterpret_cast<Address>(frame.data()+0x80);
        context.rbx=reinterpret_cast<Address>(&handle); context.rax=reinterpret_cast<Address>(row.data());
        context.r14=reinterpret_cast<Address>(references.data()); context.r15=reinterpret_cast<Address>(ui.data());
        if (site==Site::Client) { context.rax=0; context.rcx=0; put(frame.data(),0x80+0xa80,handle); }
        if (site==Site::KnowledgeA || site==Site::KnowledgeB) context.rbx=0xabcdef123456ffff;
        return context;
    }
    Edit decision(Site site) { return guarded_decide(base,site,context(site)); }
};
struct Backend {
    unsigned operation{},fail{},created{},removed{},disabled{},initialized{},uninitialized{},queued{},applied{};
    bool fail_disable{},fail_remove{},partial_activation{};
    bool step() { return ++operation!=fail; }
    bool initialize() { if(!step())return false; ++initialized; return true; }
    bool create(Address,Address,Address& trampoline) { if(!step())return false; ++created; trampoline=0xfeed0000+created; return true; }
    bool queue_enable(Address) { if(!step())return false; ++queued; return true; }
    bool apply() { ++applied; partial_activation=true; return step(); }
    bool disable(Address) { ++disabled; return !fail_disable; }
    bool remove(Address) { if(fail_remove)return false; ++removed; return true; }
    bool uninitialize() { ++uninitialized; return true; }
};
} // namespace

int main(int argc,char** argv) {
    using namespace crimson::marketplace::crafting;
    const auto entries=bridge_entries();
    for(unsigned failure=1;failure<=15;++failure) {
        Backend backend; backend.fail=failure;
        HookTransaction transaction(backend);
        unsigned verified{};
        const auto installed=transaction.install(0x140000000,entries,entries[0],[&](Address,std::size_t){++verified;return true;},[&](BridgeConfiguration configuration,Address lookup){
            assert(lookup);for(auto target:configuration.passthrough)assert(target); return backend.step(); });
        assert(!installed && verified==6);
        if (backend.applied) {
            assert(backend.created==6 && backend.removed==0 && backend.uninitialized==0);
            assert(!transaction.cleanup());
        } else {
            assert(backend.created==backend.removed);
            assert(backend.initialized==backend.uninitialized);
        }
    }
    { Backend backend; HookTransaction transaction(backend);
      assert(!transaction.install(0x140000000,entries,entries[0],[](Address,std::size_t index){return index!=3;},[](BridgeConfiguration,Address){return true;}));
      assert(!backend.operation && !backend.created); }
    { Backend backend; HookTransaction transaction(backend);
      assert(transaction.install(0x140000000,entries,entries[0],[](Address,std::size_t){return true;},[](BridgeConfiguration,Address){return true;}));
      assert(transaction.active() && backend.queued==6 && backend.applied==1);
      backend.fail_disable=true; assert(!transaction.cleanup() && backend.removed==0 && backend.uninitialized==0);
      backend.fail_disable=false;
      assert(!transaction.cleanup() && backend.removed==0 && backend.uninitialized==0); }
    { Backend backend; backend.fail=9; backend.fail_remove=true; HookTransaction transaction(backend);
      assert(!transaction.install(0x140000000,entries,entries[0],[](Address,std::size_t){return true;},[&](BridgeConfiguration,Address){return backend.step();}));
      assert(!backend.applied && backend.created==6 && backend.removed==0);
      backend.fail_remove=false;
      assert(transaction.cleanup() && backend.removed==6 && backend.uninitialized==1); }
    BridgeConfiguration configuration; configuration.passthrough.fill(reinterpret_cast<Address>(&crimson_crafting_fixture_resume)); configuration.callback=callback;
    auto invalid_configuration=configuration; invalid_configuration.passthrough[3]=1;
    assert(!bridge_ready() && !configure_bridge(invalid_configuration) && !bridge_ready());
    assert(configure_bridge(configuration) && bridge_ready() && !configure_bridge(configuration));
    const auto support=state_support(); assert(support.bytes>=512 && support.bytes<=65536);
    if(argc==2 && (std::strcmp(argv[1],"--baseline")==0 || std::strcmp(argv[1],"--native-fip-baseline")==0)) {
        if(std::strcmp(argv[1],"--native-fip-baseline")==0)crimson_crafting_fixture_stable_pointer=0;
        for(unsigned repeat=0;repeat<100000;++repeat)check_probe(reinterpret_cast<Address>(&crimson_crafting_fixture_resume));
        std::puts("100000 direct no-bridge baseline probes passed"); return 0;
    }
    for (auto entry:entries) for(unsigned repeat=0;repeat<8;++repeat) check_probe(entry);
    mode=1; for(auto entry:entries)check_probe(entry,true); mode=0;
    mode=2;check_probe(entries[0]);mode=3;check_probe(entries[0]);mode=0;
    std::vector<std::thread> threads;
    for(unsigned thread=0;thread<4;++thread)threads.emplace_back([&]{for(unsigned repeat=0;repeat<8;++repeat)check_probe(entries[repeat%5]);});
    for(auto& thread:threads)thread.join();
    assert(callbacks==80 && crimson_crafting_fixture_bad_alignment==0);
    // Compose the existing native register policies with the new original-row
    // view, using the same selected-actor fixtures and actual owned memory.
    for (const auto key:{kOrdinaryRecipes.front().key,std::uint32_t{1000085}}) {
        Fixture f;f.set_recipe(*known_recipe(key));
        const auto counts=source_cost_counts(key);
        std::array<FixedBytes,5> materials{};
        std::array<unsigned char,4*0x18> material_groups{};
        put(f.row.data(),0x40,reinterpret_cast<Address>(materials.data()));
        put(f.row.data(),0x48,static_cast<std::uint32_t>(counts>>4));
        put(f.row.data(),0x4c,std::uint32_t{5});
        put(f.row.data(),0x50,reinterpret_cast<Address>(material_groups.data()));
        put(f.row.data(),0x58,static_cast<std::uint32_t>(counts&15));
        put(f.row.data(),0x5c,std::uint32_t{4});
        const auto before=f.row;
        Memory memory;ShadowPool pool;ShadowSource source;
        assert(capture_shadow_source(memory,f.base,f.handle,reinterpret_cast<Address>(f.row.data()),source));
        const auto view=pool.view(source);assert(view);
        ShadowRead effective(memory,pool,f.base);
        const auto quantity=decide(effective,f.base,Site::Quantity,f.context(Site::Quantity));
        assert(quantity.continuation && quantity.change_edi && quantity.edi==40);
        assert(decide(effective,f.base,Site::Client,f.context(Site::Client)).continuation);
        if (key==1000085) {
            for (const auto site:{Site::KnowledgeA,Site::KnowledgeB}) {
                auto context=f.context(site);context.rax=view;
                const auto display=decide(effective,f.base,site,context);
                assert(display.continuation && display.change_bx && display.bx==f.knowledge_handle);
            }
            assert(*reinterpret_cast<const std::uint16_t*>(view+0x30)==0xffff);
        }
        assert(f.row==before && f.rows[f.handle]==reinterpret_cast<Address>(f.row.data()));
    }
    unsigned policies{};
    { Fixture f;
      for(const auto& known:kOrdinaryRecipes) { f.set_recipe(known);
          for(auto site:{Site::Database,Site::Client,Site::Quantity}) { const auto edit=f.decision(site); assert(edit.continuation==f.base+kTargets[static_cast<std::size_t>(site)].continuation); ++policies; }
          if(known.knowledge==0) for(auto site:{Site::KnowledgeA,Site::KnowledgeB}) {
              const auto fallback=knowledge_fallback(known.key); assert(fallback); f.set_knowledge(fallback->knowledge);
              const auto original=f.row; const auto edit=f.decision(site); assert(edit.change_bx && edit.bx==Fixture::knowledge_handle && f.row==original); ++policies;
          }
      }
    }
    for(unsigned invalid_case=0;invalid_case<12;++invalid_case) {
        Fixture f;
        switch(invalid_case) {
        case 0:put(f.row.data(),8,std::uint32_t{0xffffffff});break;
        case 1:f.row[0x18]=1;break;
        case 2:f.row[0x1c]=3;break;
        case 3:put(f.row.data(),0x48,std::uint32_t{1});break; // Retained currency or any fixed material.
        case 4:put(f.row.data(),0x58,std::uint32_t{1});break;
        case 5:f.row[0x38]^=1;break;
        case 6:put(f.row.data(),0x28,std::uint32_t{9999});break;
        case 7:put(f.row.data(),0x30,std::uint16_t{0xffff});break;
        case 8:put(f.row.data(),0xe8,std::uint32_t{0});break;
        case 9:put(f.row.data(),0,Address{0});break;
        case 10:put(f.manager.data(),8,std::uint32_t{18575});break;
        case 11:f.rows[f.handle]=0;break;
        }
        for(auto site:{Site::Database,Site::Client,Site::Quantity})assert(!f.decision(site).continuation);
    }
    { Fixture f; auto context=f.context(Site::Client); context.rcx=1; assert(!guarded_decide(f.base,Site::Client,context).continuation);
      context=f.context(Site::Client); context.rax=1; assert(!guarded_decide(f.base,Site::Client,context).continuation);
      put(f.frame.data(),0x80+0x38,std::uint32_t{1});assert(!f.decision(Site::Client).continuation); }
    { Fixture f;f.set_recipe(*known_recipe(1010292));
      for(auto site:{Site::Database,Site::Client,Site::Quantity})assert(f.decision(site).continuation);
      put(f.row.data(),0x48,std::uint32_t{1});
      for(auto site:{Site::Database,Site::Client,Site::Quantity})assert(!f.decision(site).continuation); }
    { Fixture f; auto context=f.context(Site::Quantity);const auto original=f.row;const auto edit=guarded_decide(f.base,Site::Quantity,context);
      assert(edit.change_edi && edit.edi==40 && f.row==original);apply(context,edit);assert(context.rdi==40); }
    for(unsigned invalid_case=0;invalid_case<11;++invalid_case) {
        Fixture f;
        switch(invalid_case) {
        case 0:put(f.ui.data(),0x190,std::uint32_t{1});break;
        case 1:put(f.player_manager.data(),0x50,Address{0});break;
        case 2:f.metadata[1]=6;break;
        case 3:f.actor[0x96]=0;break;
        case 4:put(f.actor.data(),0x60,std::uint32_t{0});break;
        case 5:put(f.actor.data(),0x70,std::uint32_t{45});break;
        case 6:put(f.possessor.data(),0xd0,Address{0});break;
        case 7:put(f.crafting.data(),8,Address{0});break;
        case 8:put(f.crafting.data(),0x2f8,std::uint32_t{1});break;
        case 9:put(f.crafting.data(),0x308,std::uint32_t{1});break;
        case 10:put(f.frame.data(),0,Address{0});break;
        }
        assert(!f.decision(Site::Quantity).continuation);
    }
    { Fixture f; f.set_recipe(*known_recipe(1000085));
      const auto original=f.row; put(f.bucket.data(),0,std::uint32_t{0});
      for(auto site:{Site::KnowledgeA,Site::KnowledgeB}) { const auto edit=f.decision(site); assert(edit.continuation && edit.change_bx && edit.bx==0xffff); }
      assert(f.row==original && f.row[0x30]==0xff && f.row[0x31]==0xff); }
    Context fault{};fault.rbx=1;fault.rbp=1;fault.r15=1;fault.r14=1;
    for(unsigned site=0;site<5;++site)assert(!guarded_decide(1,static_cast<Site>(site),fault).continuation);
    for(unsigned site=0;site<5;++site) { Fixture f;put(f.manager.data(),0x58,Address{0x10000});assert(!f.decision(static_cast<Site>(site)).continuation); }
    std::printf("Crafting staged native fixtures passed: %u recipe policies; %u x64 bridge probes; XSAVE=%u bytes=%u AVX=%u. No game functions called.\n",policies,callbacks.load(),support.xsave,support.bytes,support.avx);
}
