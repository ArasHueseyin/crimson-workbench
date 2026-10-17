#include "../force-palm/force_palm_policy.h"
#include "../force-palm/force_palm_hooks.h"
#include "../consumables/consumables_targets.h"
#include "../mount-protection/mount_protection_targets.h"
#include "../crafting/crafting_hooks.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace {
using namespace crimson::marketplace::force_palm;
template<class T> void put(void* data,std::size_t offset,T value) {
    std::memcpy(static_cast<unsigned char*>(data)+offset,&value,sizeof(value));
}
struct Fixture {
    Address base{};
    std::array<unsigned char,0x38> root{};
    std::array<unsigned char,0x58> manager{};
    std::array<unsigned char,0xb0> actor{};
    std::array<unsigned char,2> metadata{0,1};
    std::array<unsigned char,0x48> components{};
    std::array<unsigned char,0xd8> possessor{};
    std::array<unsigned char,0x18> control{},descriptor{};
    std::array<unsigned char,0x160> package{};
    std::vector<unsigned char> buffer=std::vector<unsigned char>(kBufferBytes);
    std::vector<unsigned char> finder=std::vector<unsigned char>(kFinderCount*2);
    std::vector<unsigned char> branches=std::vector<unsigned char>(1646*52);
    std::uint32_t cursor=3;
    std::uint8_t negated=0xa5;
    std::array<unsigned char,32> frame{},result{};
    bool latch=false,learned=true,movement=true;
    std::uint32_t spirit=1000000;
    Fixture() {
        using namespace crimson::marketplace::stamina;
        base=reinterpret_cast<Address>(VirtualAlloc(nullptr,0x6d6a000,MEM_RESERVE,PAGE_NOACCESS));
        assert(base && VirtualAlloc(reinterpret_cast<void*>(base+0x6d69000),4096,MEM_COMMIT,PAGE_READWRITE));
        put(reinterpret_cast<void*>(base+kClientRoot),0,root.data());
        put(root.data(),0x30,manager.data());put(manager.data(),0,base+kClientManager);
        put(manager.data(),0x50,actor.data());put(actor.data(),0,base+kClientActor);
        put(actor.data(),0x60,std::uint32_t{0xa0101234});put(actor.data(),0x68,components.data());
        put(actor.data(),0x70,std::uint32_t{24});put(actor.data(),0x88,metadata.data());
        put(actor.data(),0x96,std::uint8_t{1});put(actor.data(),0xa0,possessor.data());
        put(possessor.data(),0xd0,actor.data());put(components.data(),0x40,control.data());
        put(control.data(),0,base+kControlType);put(control.data(),8,actor.data());
        put(descriptor.data(),0,kConditionStart);put(descriptor.data(),4,kConditionCount);
        put(descriptor.data(),8,package.data()+0x110);
        put(package.data(),0x110,buffer.data());put(package.data(),0x118,kBufferBytes);
        put(package.data(),0x120,kFinderCount);put(package.data(),0x128,finder.data());
        put(package.data(),0x130,std::uint32_t{1646});put(package.data(),0x138,branches.data());
        put(buffer.data(),0,std::uint32_t{629});put(buffer.data(),73*4,std::uint32_t{183});
        for (std::size_t i=0;i<5;++i) put(finder.data(),(kConditionStart+i)*2,kFinderWords[i]);
        for (std::size_t i=0;i<kLeafPair.size();++i) put(buffer.data(),2717*4+i*4,kLeafPair[i]);
        std::memcpy(branches.data()+285*52,kEntryBranch.data(),kEntryBranch.size());
    }
    ~Fixture() {assert(VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE));}
    Fixture(const Fixture&)=delete;
};
Fixture* fixture{};
unsigned calls{};
std::uint8_t native_result=1;
unsigned mutate{};
bool throw_native=false;
std::uint8_t __fastcall original(void* descriptor,void* control,void* frame,std::uint8_t flag,
    std::uint32_t* cursor,std::uint8_t* negated,void* result) {
    ++calls;
    assert(descriptor==fixture->descriptor.data() && control==fixture->control.data() &&
        frame==fixture->frame.data() && flag==0x5a && cursor==&fixture->cursor &&
        negated==&fixture->negated && result==fixture->result.data());
    if (throw_native) throw std::runtime_error("native fixture exception");
    if (mutate==1) put(fixture->manager.data(),0x50,Address{0});
    if (mutate==2) put(fixture->package.data(),0x110,Address{0});
    if (mutate==3) put(fixture->actor.data(),0x60,std::uint32_t{0xa0101235});
    if (mutate==4) put(fixture->descriptor.data(),8,Address{0});
    if (mutate==5) put(fixture->buffer.data(),2717*4+4,std::uint32_t{0x8019ee84});
    return native_result;
}
std::uint8_t invoke(Fixture& value,bool enabled=true) {
    fixture=&value;const auto before=calls;
    const auto result=dispatch(original,value.base,enabled,value.descriptor.data(),value.control.data(),
        value.frame.data(),0x5a,&value.cursor,&value.negated,value.result.data());
    assert(calls==before+1 && value.negated==0xa5 && !value.latch);
    return result;
}
// Mirror the verified native evaluator's prefix recursion/short circuit. The
// native skill leaf stays on the right and is never replaced by the adapter.
std::uint8_t __fastcall native_graph(void* d,void* c,void* f,std::uint8_t flag,
    std::uint32_t* cursor,std::uint8_t* n,void* r) {
    for (;;) {
        const auto index=*cursor;
        if (index==0 || index==2) {
            ++*cursor;
            const auto left=dispatch(native_graph,fixture->base,true,d,c,f,flag,cursor,n,r);
            if (left) {++*cursor; if (index==0) *cursor=4;return left;}
            ++*cursor;continue;
        }
        if (index==1) return fixture->movement ? 0 : 1;
        if (index==3) return fixture->latch ? 0 : 1;
        assert(index==4);
        return fixture->learned && fixture->spirit>=5000 ? 0 : 1;
    }
}
struct Backend {
    bool fail_pin{},fail_init{},fail_create{},null_trampoline{},fail_enable{},fail_disable{},fail_remove{},fail_uninit{};
    unsigned pins{},initializations{},creates{},enables{},disables{},removes{},uninitializations{};
    bool pin() {++pins;return !fail_pin;}
    bool initialize() {++initializations;return !fail_init;}
    bool create(Address,Address,Address& trampoline) {++creates;trampoline=null_trampoline ? 0 : 0x12345;return !fail_create;}
    bool enable(Address) {++enables;return !fail_enable;}
    bool disable(Address) {++disables;return !fail_disable;}
    bool remove(Address) {++removes;return !fail_remove;}
    bool uninitialize() {++uninitializations;return !fail_uninit;}
};
void lifecycle() {
    for (unsigned failure=0;failure<9;++failure) {
        Backend backend;std::atomic<bool> enabled=false;Address published{};
        backend.fail_pin=failure==0;backend.fail_init=failure==1;backend.fail_create=failure==2;
        backend.null_trampoline=failure==3;backend.fail_enable=failure==5 || failure==6;
        backend.fail_disable=failure==6;backend.fail_remove=failure==7;backend.fail_uninit=failure==8;
        HookTransaction<Backend> hooks(backend,enabled);
        const bool ok=hooks.install(0x10000000,0x123,[]{return true;},[&](Address value) {
            published=value;return failure!=4 && failure!=7 && failure!=8;
        });
        assert(!ok && !enabled);
        assert(!hooks.install(0x10000000,0x123,[]{return true;},[](Address){return true;}));
        if (failure==5 || failure==6) {
            assert(published==0x12345 && backend.enables==1 && backend.disables==1 &&
                backend.removes==0 && backend.uninitializations==0);
        }
        if (failure==7) assert(backend.removes==1 && backend.uninitializations==0);
        if (failure==8) assert(backend.removes==1 && backend.uninitializations==1);
    }
    Backend backend;std::atomic<bool> enabled=false;Address published{};
    HookTransaction<Backend> hooks(backend,enabled);
    assert(hooks.install(0x10000000,0x123,[]{return true;},[&](Address p){published=p;return true;}));
    assert(enabled && published==0x12345 && hooks.cleanup() && !enabled);
    assert(backend.removes==0 && backend.uninitializations==0); // In-flight original remains valid.
}
} // namespace

int main() {
    for (auto native:{0u,1u,2u,0xffu}) {
        Fixture f;native_result=static_cast<std::uint8_t>(native);
        assert(invoke(f)==(native==1 ? 0 : native));assert(invoke(f,false)==native);
    }
    native_result=1;
    for (unsigned invalid=0;invalid<25;++invalid) {
        Fixture f;
        switch(invalid) {
        case 0: put(f.descriptor.data(),0,std::uint32_t{4611});break;
        case 1: put(f.descriptor.data(),4,std::uint32_t{4});break;
        case 2: f.cursor=4;break;
        case 3: put(f.control.data(),0,Address{0});break;
        case 4: put(f.control.data(),8,Address{0});break;
        case 5: put(f.manager.data(),0x50,Address{0});break;
        case 6: put(f.actor.data(),0,Address{0});break;
        case 7: put(f.actor.data(),0x60,std::uint32_t{0});break;
        case 8: put(f.actor.data(),0x96,std::uint8_t{0});break;
        case 9: f.metadata[1]=6;break;
        case 10: put(f.components.data(),0x40,Address{0});break;
        case 11: put(f.possessor.data(),0xd0,Address{0});break;
        case 12: put(f.package.data(),0x118,std::uint32_t{10868+39});break;
        case 13: put(f.package.data(),0x120,std::uint32_t{4616});break;
        case 14: put(f.package.data(),0x130,std::uint32_t{285});break;
        case 15: put(f.package.data(),0x110,Address{UINTPTR_MAX-4});break;
        case 16: put(f.package.data(),0x128,Address{UINTPTR_MAX-4});break;
        case 17: put(f.package.data(),0x138,Address{UINTPTR_MAX-4});break;
        case 18: put(f.package.data(),0x110,Address{0});break;
        case 19: put(f.descriptor.data(),8,Address{1});break;
        case 20: put(f.actor.data(),0x70,std::uint32_t{58});break;
        case 21: put(f.actor.data(),0x70,std::uint32_t{8});break;
        case 22: put(f.package.data(),0x128,Address{1});break; // SEH: unreadable buffer.
        case 23: put(f.package.data(),0x138,Address{1});break;
        case 24: put(f.descriptor.data(),8,Address{0});break;
        }
        assert(invoke(f)==1);
    }
    for (std::size_t i=0;i<kLeafPair.size();++i) {
        Fixture f;put(f.buffer.data(),2717*4+i*4,kLeafPair[i]^1);assert(invoke(f)==1);
    }
    for (std::size_t i=0;i<kEntryBranch.size();++i) {
        Fixture f;f.branches[285*52+i]^=1;assert(invoke(f)==1);
    }
    for (std::size_t i=0;i<kFinderWords.size();++i) {
        Fixture f;put(f.finder.data(),(kConditionStart+i)*2,std::uint16_t{0xffff});assert(invoke(f)==1);
    }
    for (mutate=1;mutate<=5;++mutate) {Fixture f;assert(invoke(f)==1);}mutate=0;
    {Fixture f;throw_native=true;bool caught=false;try {invoke(f);}catch(const std::runtime_error&) {caught=true;}
        assert(caught);throw_native=false;}
    for (std::uint8_t kind:{std::uint8_t{1},std::uint8_t{4}}) {
        Fixture f;f.metadata[1]=kind;fixture=&f;
        // Twenty-four moves across eight first/second/third chains. Only the
        // first-stage re-entry leaf is overridden; resources still bound use.
        for (unsigned chain=0;chain<8;++chain) {
            f.cursor=0;
            assert(dispatch(native_graph,f.base,true,f.descriptor.data(),f.control.data(),f.frame.data(),0,
                &f.cursor,&f.negated,f.result.data())==0);
            for (unsigned stage=0;stage<3;++stage) {f.latch=false;f.spirit-=5000;}
        }
        assert(f.spirit==880000 && !f.latch);
        f.cursor=0;f.learned=false;
        assert(dispatch(native_graph,f.base,true,f.descriptor.data(),f.control.data(),f.frame.data(),0,
            &f.cursor,&f.negated,f.result.data())==1);
        f.learned=true;f.spirit=4999;f.cursor=0;
        assert(dispatch(native_graph,f.base,true,f.descriptor.data(),f.control.data(),f.frame.data(),0,
            &f.cursor,&f.negated,f.result.data())==1);
        f.spirit=1000000;f.movement=false;f.cursor=0;
        assert(dispatch(native_graph,f.base,true,f.descriptor.data(),f.control.data(),f.frame.data(),0,
            &f.cursor,&f.negated,f.result.data())==1);
    }
    lifecycle();
    // New sibling modules' guard spans must not overlap this sole entry patch;
    // its witnesses must not overlap any of their entry patches either.
    using namespace crimson::marketplace;
    constexpr auto overlaps=[](Address a,std::size_t n,Address b) {return a<b+5 && b<a+n;};
    for (const auto& pin:consumables::kPins) assert(!overlaps(pin.rva,pin.length,kEvaluateRva));
    for (const auto& pin:mount_protection::kCodeSpans) assert(!overlaps(pin.rva,pin.count,kEvaluateRva));
    for (const auto& pin:crafting::kFingerprints) assert(!overlaps(pin.rva,pin.length,kEvaluateRva));
    for (const auto& pin:kPins) {
        for (auto hook:{consumables::kUseRva,consumables::kLookupRva,consumables::kPopRva,mount_protection::kDeltaRva,crafting::kLookupRva})
            assert(!overlaps(pin.rva,pin.length,hook));
        for (const auto& target:crafting::kTargets) assert(!overlaps(pin.rva,pin.length,target.hook));
    }
    std::printf("PASS: Force Palm exact native graph/leaf scope, native original forwarding, 24-move graph fixture, resources/negative cases, reload/selection guards, exception forwarding, lifetime and sibling guards.\n");
    return 0;
}
