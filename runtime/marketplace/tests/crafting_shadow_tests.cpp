#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include "../crafting/crafting_shadow.h"
#include "../crafting/crafting_shadow_callers.h"
#include "../crafting/crafting_policy.h"

namespace {
using namespace crimson::marketplace::crafting;
struct Read {
    struct Block { Address start; const void* bytes; std::size_t size; };
    std::vector<Block> blocks;
    template<class T> void add(Address start, const T& object) { blocks.push_back({start,&object,sizeof(object)}); }
    template<class T> T at(Address start) {
        for (const auto& block:blocks)
            if (start>=block.start && start-block.start<=block.size && sizeof(T)<=block.size-(start-block.start)) {
                T value; std::memcpy(&value,static_cast<const unsigned char*>(block.bytes)+(start-block.start),sizeof(T)); return value;
            }
        throw std::runtime_error("fixture read outside registered original bytes");
    }
};
struct Fixture {
    static constexpr Address base=0x140000000;
    std::array<std::uint8_t,0x90> manager{},knowledge_manager{},item_manager{};
    std::array<Address,kRecipeCount> rows{};
    std::array<Address,5> item_rows{};
    std::array<std::array<std::uint8_t,0x20>,5> items{};
    RecipeBytes original{};
    std::array<FixedBytes,5> fixed{};
    std::array<std::uint8_t,4*0x18> groups{};
    std::array<std::uint8_t,0x100> knowledge_bucket{};
    std::array<std::uint8_t,16> knowledge_node{};
    std::array<Address,1> knowledge_nodes{};
    Address manager_pointer=reinterpret_cast<Address>(manager.data());
    Address knowledge_pointer=reinterpret_cast<Address>(knowledge_manager.data());
    Address item_pointer=reinterpret_cast<Address>(item_manager.data());
    std::uint16_t handle=71;
    Read read;
    Fixture() {
        read.add(base+kRecipeManager,manager_pointer); read.add(base+kKnowledgeManager,knowledge_pointer);
        read.add(base+0x6d69aa8,item_pointer);
        read.add(manager_pointer,manager); read.add(knowledge_pointer,knowledge_manager); read.add(item_pointer,item_manager);
        read.add(reinterpret_cast<Address>(rows.data()),rows); read.add(reinterpret_cast<Address>(original.data()),original);
        read.add(reinterpret_cast<Address>(fixed.data()),fixed); read.add(reinterpret_cast<Address>(groups.data()),groups);
        read.add(reinterpret_cast<Address>(item_rows.data()),item_rows);
        for (const auto& item:items) read.add(reinterpret_cast<Address>(item.data()),item);
        read.add(reinterpret_cast<Address>(knowledge_bucket.data()),knowledge_bucket);
        read.add(reinterpret_cast<Address>(knowledge_nodes.data()),knowledge_nodes);
        read.add(reinterpret_cast<Address>(knowledge_node.data()),knowledge_node);
        set_field(manager,8,kRecipeCount); set_field(manager,0x58,reinterpret_cast<Address>(rows.data()));
        rows[handle]=reinterpret_cast<Address>(original.data());
        set_field(knowledge_manager,8,std::uint32_t{32});
        set_field(knowledge_manager,0x68,std::uint32_t{1}); set_field(knowledge_manager,0x6c,std::uint32_t{1});
        set_field(knowledge_manager,0x78,reinterpret_cast<Address>(knowledge_bucket.data()));
        set_field(knowledge_manager,0x80,reinterpret_cast<Address>(knowledge_nodes.data()));
        knowledge_nodes[0]=reinterpret_cast<Address>(knowledge_node.data());
        set_field(knowledge_bucket,0,std::uint32_t{1});
        set_field(knowledge_node,8,std::uint16_t{2});
        set_field(item_manager,8,std::uint32_t{5}); set_field(item_manager,0x58,reinterpret_cast<Address>(item_rows.data()));
        for (std::size_t i=0;i<5;++i) item_rows[i]=reinterpret_cast<Address>(items[i].data());
        set(kOrdinaryRecipes.front());
    }
    void set(const KnownRecipe& known, bool fee_removed=false) {
        original.fill(0xa5);
        set_field(original,0,base+kRecipeVtable); set_field(original,8,known.key); original[0x18]=0; original[0x1c]=1;
        set_field(original,0x28,known.conditions); set_field(original,0x30,known.knowledge ? std::uint16_t{2} : std::uint16_t{0xffff});
        set_field(knowledge_node,4,known.knowledge); set_field(knowledge_bucket,8,known.knowledge);
        for (std::size_t i=0;i<5;++i) original[0x38+i]=known.flags[i];
        const auto counts=source_cost_counts(known.key);
        set_field(original,0x40,reinterpret_cast<Address>(fixed.data()));
        set_field(original,0x48,static_cast<std::uint32_t>((counts>>4)-(fee_removed?1:0)));
        set_field(original,0x4c,std::uint32_t{5});
        set_field(original,0x50,reinterpret_cast<Address>(groups.data()));
        set_field(original,0x58,static_cast<std::uint32_t>(counts&15)); set_field(original,0x5c,std::uint32_t{4});
        set_field(original,0xe8,std::uint32_t{1});
        if (known.key==1010292) {
            constexpr std::array<std::uint32_t,5> keys{1002130,1003633,1003827,1000485,1002198};
            constexpr std::array<std::uint64_t,5> amounts{1,3,1,300,200};
            for (std::uint16_t i=0;i<5-(fee_removed?1:0);++i) {
                const auto source=fee_removed && i>=3 ? i+1 : i;
                fixed[i].fill(0x5a); set_field(fixed[i],0,i);
                set_field(fixed[i],2,std::uint16_t{0xffff}); set_field(fixed[i],4,std::uint16_t{0xffff});
                set_field(fixed[i],8,amounts[source]); set_field(fixed[i],0x10,std::uint64_t{0}); set_field(fixed[i],0x18,std::uint16_t{0});
                set_field(items[i],0,keys[source]); items[i][0x10]=0;
            }
        }
    }
    bool capture(ShadowSource& source) {
        return capture_shadow_source(read,base,handle,reinterpret_cast<Address>(original.data()),source);
    }
};
struct MutatingFeeRead {
    Read& original;
    FixedBytes& fee;
    bool first=true;
    template<class T> T at(Address location) {
        auto value=original.template at<T>(location);
        if constexpr (std::is_same_v<T,FixedBytes>)
            if (first && location==reinterpret_cast<Address>(fee.data())) { first=false;fee[8]^=1; }
        return value;
    }
};
}

int main() {
    Fixture fixture; ShadowPool pool;
    std::vector<Address> retained;
    unsigned accepted{};
    for (const auto& known:kOrdinaryRecipes) {
        fixture.set(known);
        const auto before=fixture.original; ShadowSource source;
        assert(fixture.capture(source));
        const auto pointer=pool.view(source); assert(pointer && (pointer&15)==0 && pointer!=fixture.rows[fixture.handle]);
        assert(pool.view(source)==pointer);
        const auto& copy=*reinterpret_cast<const RecipeBytes*>(pointer);
        for (std::size_t i=0;i<copy.size();++i) if (i<0x40 || i>=0x60) assert(copy[i]==before[i]);
        assert(field<Address>(copy,0x50)==0 && field<std::uint32_t>(copy,0x58)==0 && field<std::uint32_t>(copy,0x5c)==0);
        if (known.key==1010292) {
            assert(source.retain_fee && field<std::uint32_t>(copy,0x48)==1 && field<std::uint32_t>(copy,0x4c)==1);
            const auto& fee=*reinterpret_cast<const FixedBytes*>(field<Address>(copy,0x40));
            assert(fee==fixture.fixed[3] && field<std::uint64_t>(fee,8)==300);
        } else for (std::size_t i=0x40;i<0x50;++i) assert(copy[i]==0);
        assert(fixture.original==before && fixture.rows[fixture.handle]==reinterpret_cast<Address>(fixture.original.data()));
        retained.push_back(pointer); ++accepted;
    }
    for (std::size_t i=0;i<retained.size();++i)
        assert(field<std::uint32_t>(*reinterpret_cast<const RecipeBytes*>(retained[i]),8)==kOrdinaryRecipes[i].key);
    fixture.set(*known_recipe(1010292),true); ShadowSource removed;
    assert(fixture.capture(removed) && !removed.retain_fee);
    const auto removed_view=pool.view(removed);
    for (std::size_t i=0x40;i<0x60;++i) assert((*reinterpret_cast<const RecipeBytes*>(removed_view))[i]==0);
    for (unsigned bad=0;bad<14;++bad) {
        fixture.set(*known_recipe(1010292));
        switch(bad) {
        case 0:set_field(fixture.original,0,Address{0});break;
        case 1:fixture.original[0x18]=1;break;
        case 2:fixture.original[0x1c]=3;break;
        case 3:set_field(fixture.original,8,std::uint32_t{999999});break;
        case 4:set_field(fixture.original,0x28,std::uint32_t{999});break;
        case 5:set_field(fixture.original,0x30,std::uint16_t{31});break;
        case 6:set_field(fixture.original,0x48,std::uint32_t{3});break;
        case 7:set_field(fixture.original,0x58,std::uint32_t{1});break;
        case 8:set_field(fixture.original,0x4c,std::uint32_t{4});break;
        case 9:set_field(fixture.fixed[3],8,std::uint64_t{301});break;
        case 10:set_field(fixture.fixed[3],0x10,std::uint64_t{1});break;
        case 11:fixture.items[3][0x10]=1;break;
        case 12:set_field(fixture.original,0xe8,std::uint32_t{2});break;
        case 13:fixture.rows[fixture.handle]=0;break;
        }
        ShadowSource out; assert(!fixture.capture(out));
        fixture.rows[fixture.handle]=reinterpret_cast<Address>(fixture.original.data());
    }
    fixture.set(*known_recipe(1000085)); ShadowSource no_knowledge;
    assert(fixture.capture(no_knowledge)); const auto no_knowledge_view=pool.view(no_knowledge);
    assert(field<std::uint16_t>(*reinterpret_cast<const RecipeBytes*>(no_knowledge_view),0x30)==0xffff);
    fixture.set(*known_recipe(1010292));
    MutatingFeeRead changed_fee{fixture.read,fixture.fixed[3]};ShadowSource torn;
    assert(!capture_shadow_source(changed_fee,Fixture::base,fixture.handle,reinterpret_cast<Address>(fixture.original.data()),torn));
    ShadowPool bounded(1); assert(bounded.view(no_knowledge));
    auto next_generation=no_knowledge; ++next_generation.manager;
    assert(!bounded.view(next_generation) && bounded.view(no_knowledge));
    auto changed=no_knowledge; changed.row[0xf]=0x79;
    const auto changed_pointer=pool.view(changed); assert(changed_pointer && changed_pointer!=no_knowledge_view);
    assert((*reinterpret_cast<const RecipeBytes*>(no_knowledge_view))[0xf]==no_knowledge.row[0xf]);
    for (const auto caller:kShadowCallers) assert(shadow_caller(Fixture::base,Fixture::base+caller));
    assert(!shadow_caller(Fixture::base,Fixture::base-1));
    // Database initialization uses the original valid rows, including during a
    // delayed/failed worker startup. Its getter sites must never see a shadow.
    for (const auto caller:{Address{0x2429dec},Address{0x2429dfc},Address{0x2429e11}})
        assert(!shadow_caller(Fixture::base,Fixture::base+caller));
    fixture.set(kOrdinaryRecipes.front()); ShadowSource client_source;
    assert(fixture.capture(client_source)); const auto client_view=pool.view(client_source);
    const auto& client_row=*reinterpret_cast<const RecipeBytes*>(client_view);
    fixture.read.add(client_view,client_row);
    std::array<std::uint8_t,0xb10> frame{};
    fixture.read.add(reinterpret_cast<Address>(frame.data()),frame);
    set_field(frame,0x80+0xa80,fixture.handle);
    Context client{}; client.rbp=reinterpret_cast<Address>(frame.data()+0x80);
    ShadowRead effective(fixture.read,pool,Fixture::base);
    const auto original_fixed=field<std::uint32_t>(fixture.original,0x48);
    assert(original_fixed>0);
    const auto edit=decide(effective,Fixture::base,Site::Client,client);
    assert(edit.continuation==Fixture::base+kTargets[static_cast<std::size_t>(Site::Client)].continuation);
    assert(!decide(fixture.read,Fixture::base,Site::Client,client).continuation);
    assert(field<std::uint32_t>(fixture.original,0x48)==original_fixed);
    Context database{}; database.rbx=reinterpret_cast<Address>(&fixture.handle);
    fixture.read.add(database.rbx,fixture.handle); database.rax=reinterpret_cast<Address>(fixture.original.data());
    assert(!decide(fixture.read,Fixture::base,Site::Database,database).continuation);
    // Neither a stale registration nor a later table change may be hidden by a
    // previously cached view. The ordinary native gate remains in force.
    fixture.rows[fixture.handle]=0;
    assert(!decide(effective,Fixture::base,Site::Client,client).continuation);
    fixture.rows[fixture.handle]=reinterpret_cast<Address>(fixture.original.data());
    fixture.original[0x18]=1;
    assert(!decide(effective,Fixture::base,Site::Client,client).continuation);
    std::vector<std::thread> threads;
    for (unsigned i=0;i<8;++i) threads.emplace_back([&]{for(unsigned j=0;j<100;++j)assert(pool.view(no_knowledge)==no_knowledge_view);});
    for (auto& thread:threads) thread.join();
    std::printf("PASS: %u immutable recipe views, retained 300-unit fee and independent fee removal, exact non-cost bytes, original rows/registry unchanged, missing knowledge, 14 rejected layouts, native client-policy composition, unchanged database path, stale registry/source refusal, 50 approved getter sites, generation retention, bounded allocation and 800 concurrent lookups.\n",accepted);
}
