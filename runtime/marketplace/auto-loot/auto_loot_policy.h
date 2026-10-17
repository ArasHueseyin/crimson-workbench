#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <unordered_set>

namespace crimson::marketplace::auto_loot {
using Address=std::uintptr_t;
inline constexpr std::uint32_t kFreeGroundInteraction=1000016;
inline constexpr std::uint32_t kAnimatedGroundInteraction=1000004;
inline constexpr std::array<std::uint32_t,3> kCorpseInteractions{12,1000009,1000337};
inline constexpr Address kNearbyRva=0x9af360;
inline constexpr Address kNearbyCaller=0xad26e1;
inline constexpr std::array<Address,1> kHookRvas{kNearbyRva};

// These records belong to the current native call. They must never be copied
// into a queue or retained after the synchronous native update returns.
struct Interaction {
    Address name{};
    void* serialized_name{};
    std::uint16_t handle{0xffff};
    std::uint16_t padding{};
    std::uint32_t pivot{0xffffffff};
    Address pivot_name{};
    std::uint64_t pivot_value{};
    std::uint32_t option{0xffffffff};
    std::uint8_t state{2};
    std::uint8_t flag{};
    std::uint16_t trailing{};
};
struct InteractionVector {Interaction* data{};std::uint32_t size{},capacity{};};
struct Eligibility {
    std::uint8_t state{2};
    std::uint8_t padding[3]{};
    std::int32_t error{};
    std::uint32_t pivot{0xffffffff};
    std::uint8_t flag{};
    std::uint8_t trailing[3]{};
};
static_assert(sizeof(Interaction)==0x30 && offsetof(Interaction,handle)==0x10 && offsetof(Interaction,state)==0x2c);
static_assert(sizeof(InteractionVector)==16 && sizeof(Eligibility)==16);

using Nearby=void(__fastcall*)(void*,void*);
using Scan=bool(__fastcall*)(void*,void*,InteractionVector*);
using Definition=const unsigned char*(__fastcall*)(const std::uint16_t*);
using Check=void(__fastcall*)(void*,void*,const Interaction*,std::uint8_t,Eligibility*);
using CanPickUp=int*(__fastcall*)(void*,int*,void*,std::uint8_t);
using Submit=int*(__fastcall*)(void*,int*,void*,const Interaction*,bool*,std::uint32_t*);
using FindInteraction=bool(__fastcall*)(void*,bool,const std::uint32_t*,Interaction*);
using ClearInteractions=void(__fastcall*)(InteractionVector*);
using CanLoot=int*(__fastcall*)(void*,int*,void*,void*,bool);
using Relationship=std::uint8_t(__fastcall*)(void*,void*,bool);
using Lock=void*(__fastcall*)(void**,void*);
using Unlock=void(__fastcall*)(void*,bool);
struct ActorLease {
    Address vtable{};void* actor{};std::uint8_t active{},exclusive{};std::uint8_t padding[6]{};void* context{};
};
static_assert(sizeof(ActorLease)==32 && offsetof(ActorLease,active)==0x10);
using Resolve=ActorLease*(__fastcall*)(void*,ActorLease*,std::uint32_t);
using Release=void(__fastcall*)(ActorLease*);
struct Bindings {
    Definition interaction{},gimmick{};Check check{};CanPickUp can_pick_up{};Submit submit{};Lock lock{};
    Scan scan{};Resolve resolve{};Release release{};
    FindInteraction find{};ClearInteractions clear{};CanLoot can_loot{};
    Definition character{},tribe{};Relationship relationship{};
};

struct Frame {
    Frame* previous{};
    void* receiver{};
    void* player{};
    std::uint32_t identity{};
    std::uint64_t now{};
    bool submitted{};
};
inline thread_local Frame* current_frame{};
inline thread_local bool in_policy{};
struct FrameScope {
    Frame value{};
    FrameScope(void* receiver,void* player,std::uint32_t identity,std::uint64_t now) noexcept
        :value{current_frame,receiver,player,identity,now,false} {current_frame=&value;}
    ~FrameScope(){current_frame=value.previous;}
    FrameScope(const FrameScope&)=delete;
    FrameScope& operator=(const FrameScope&)=delete;
};
struct PolicyScope {
    PolicyScope(){in_policy=true;}
    ~PolicyScope(){in_policy=false;}
};

// Only scalar native identities are retained. Once a request entered native
// code, a failed/unknown outcome never authorizes an automatic retry for that
// object. The growing set never evicts an identity during this process.
template<class Allocator=std::allocator<std::uint64_t>> class AttemptSet {
    using Set=std::unordered_set<std::uint64_t,std::hash<std::uint64_t>,std::equal_to<std::uint64_t>,Allocator>;
    // Lazy construction also keeps allocations out of DLL static initialization.
    std::optional<Set> entries_{};
    bool allocation_failed_{};
    std::uint64_t next_{},next_scan_{};
    std::atomic_flag busy_=ATOMIC_FLAG_INIT;
public:
    bool begin_scan(std::uint64_t now) noexcept {
        if(busy_.test_and_set(std::memory_order_acquire))return false;
        struct ReleaseFlag {std::atomic_flag& flag;~ReleaseFlag(){flag.clear(std::memory_order_release);}} release{busy_};
        if(now<next_scan_||allocation_failed_)return false;
        next_scan_=now>UINT64_MAX-250 ? UINT64_MAX : now+250;return true;
    }
    bool claim(std::uint32_t player,std::uint32_t target,std::uint64_t now) noexcept {
        if(!player||!target||busy_.test_and_set(std::memory_order_acquire))return false;
        struct Release {std::atomic_flag& flag;~Release(){flag.clear(std::memory_order_release);}} release{busy_};
        if(now<next_||allocation_failed_)return false;
        const auto identity=(std::uint64_t(player)<<32)|std::uint64_t(target);
        // This catch encloses only our own container construction/insertion,
        // before entering any native request. Engine exceptions propagate.
        try {
            if(!entries_)entries_.emplace();
            if(!entries_->insert(identity).second)return false;
        } catch(const std::bad_alloc&) {allocation_failed_=true;return false;}
        next_=now>UINT64_MAX-500 ? UINT64_MAX : now+500;
        return true;
    }
};
using Attempts=AttemptSet<>;

struct Snapshot {
    void* gimmick_component{};
    std::uint32_t target{};
    std::uint16_t definition{0xffff};
};
struct CorpseSnapshot {
    void* loot_component{};void* interaction_component{};void* state_component{};
    std::uint32_t target{};std::uint16_t definition{0xffff};
    bool operator==(const CorpseSnapshot&) const=default;
};

// A backend implements guarded memory reads and the exact native ownership
// checks. Engine calls are deliberately outside exception-catching read code.
template<class Backend,class Ledger>
bool try_candidate(Backend& backend,Ledger& attempts,void* component,void* target,InteractionVector* output) {
    auto* frame=current_frame;
    if(!frame||frame->submitted||
        !output||!output->data||!output->size||output->size>32||output->size>output->capacity)return false;
    if(!backend.player(*frame,component)||!backend.bindings().scan(component,target,nullptr))return false;
    for(std::uint32_t index=0;index<output->size;++index) {
        const auto* interaction=output->data+index;
        if(!backend.plain_interaction(interaction))continue;
        const auto* definition=backend.bindings().interaction(&interaction->handle);
        if(!backend.free_interaction(definition))continue;
        Snapshot snapshot{};
        if(!backend.ground(*frame,target,snapshot))continue;
        const auto* gimmick=backend.bindings().gimmick(&snapshot.definition);
        if(!backend.free_gimmick(gimmick))continue;
        Eligibility eligibility{};
        backend.bindings().check(component,target,interaction,3,&eligibility);
        if(eligibility.state!=2||eligibility.error||!backend.player(*frame,component))continue;
        int capacity_error=-1;
        const auto capacity=backend.bindings().can_pick_up(snapshot.gimmick_component,&capacity_error,frame->player,0);
        if(capacity!=&capacity_error||capacity_error)continue;
        Snapshot fresh{};
        if(!backend.ground(*frame,target,fresh)||fresh.target!=snapshot.target||
            fresh.definition!=snapshot.definition||fresh.gimmick_component!=snapshot.gimmick_component||
            !backend.player(*frame,component)||!backend.plain_interaction(interaction)||
            !backend.free_interaction(definition)||!backend.free_gimmick(gimmick)||
            !backend.record_current(target,*interaction))continue;
        if(!attempts.claim(frame->identity,snapshot.target,frame->now))return false;
        frame->submitted=true; // publish before the call; nested/failed requests cannot retry
        int error=-1;bool dialog=false;std::uint32_t scratch{};
        backend.bindings().submit(frame->receiver,&error,target,interaction,&dialog,&scratch);
        return true; // the native request/animation/transaction owns completion
    }
    return false;
}

template<class Backend,class Ledger>
bool try_corpse_candidate(Backend& backend,Ledger& attempts,void* component,void* target,const InteractionVector& records) {
    auto* frame=current_frame;
    if(!frame||frame->submitted||!records.data||!records.size||records.size>32||records.size>records.capacity||
        !backend.player(*frame,component))return false;
    for(std::uint32_t index=0;index<records.size;++index){
        const auto* interaction=records.data+index;
        if(!backend.plain_interaction(interaction))continue;
        const auto* definition=backend.bindings().interaction(&interaction->handle);
        if(!backend.free_corpse_interaction(definition))continue;
        CorpseSnapshot snapshot{};
        if(!backend.corpse(*frame,target,snapshot)||!backend.bindings().scan(component,target,nullptr))continue;
        Eligibility eligibility{};backend.bindings().check(component,target,interaction,3,&eligibility);
        if(eligibility.state!=2||eligibility.error||!backend.player(*frame,component))continue;
        int error=-1;
        // This is the ordinary IsLootable/loot-processor gate, including
        // existing drop data and inventory capacity; it creates no items.
        if(backend.bindings().can_loot(snapshot.loot_component,&error,nullptr,frame->player,true)!=&error||error)continue;
        CorpseSnapshot fresh{};
        if(!backend.corpse(*frame,target,fresh)||fresh!=snapshot||!backend.player(*frame,component)||
            !backend.plain_interaction(interaction)||!backend.free_corpse_interaction(definition)||
            !backend.record_current(target,*interaction))continue;
        if(!attempts.claim(frame->identity,snapshot.target,frame->now))return false;
        frame->submitted=true;
        error=-1;bool dialog=false;std::uint32_t scratch{};
        backend.bindings().submit(frame->receiver,&error,target,interaction,&dialog,&scratch);
        return true;
    }
    return false;
}

template<class Backend,class Ledger>
void dispatch_nearby(Backend& backend,Ledger& attempts,Nearby original,bool enabled,Address caller,
    std::uint64_t now,void* field,void* actor) {
    // The original performs and joins its radius query first. Recursive engine
    // updates still run, but cannot enter the policy a second time.
    if(in_policy){original(field,actor);return;}
    PolicyScope policy;
    original(field,actor);
    if(!enabled||caller!=backend.base()+kNearbyCaller||current_frame||!backend.healthy())return;
    const auto identity=backend.identity(actor);
    auto* receiver=backend.receiver();auto* component=backend.component(actor);
    Frame probe{nullptr,receiver,actor,identity,now,false};
    if(!identity||!backend.player(probe,component))return;
    if(!attempts.begin_scan(now))return;
    std::array<std::uint32_t,512> identities{};std::size_t count{};
    if(!backend.nearby(field,identities,count)||count>identities.size())return;
    FrameScope frame(receiver,actor,identity,now);
    for(std::size_t index=0;index<count;++index) {
        if(!backend.player(frame.value,component))return;
        typename Backend::Lease target(backend,identities[index]);
        if(!target.get())continue;
        std::array<Interaction,32> records{};std::uint32_t size{};
        if(!backend.interactions(target.get(),records,size)||!size||size>records.size())continue;
        InteractionVector view{records.data(),size,static_cast<std::uint32_t>(records.size())};
        if(try_candidate(backend,attempts,component,target.get(),&view)||
            try_corpse_candidate(backend,attempts,component,target.get(),view))return;
    }
}
} // namespace crimson::marketplace::auto_loot
