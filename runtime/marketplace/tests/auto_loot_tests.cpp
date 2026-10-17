#include <Windows.h>
#include <cassert>
#include <array>
#include <atomic>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>
#include "../auto-loot/auto_loot_native.h"
#include "../auto-loot/auto_loot_hooks.h"

using namespace crimson::marketplace::auto_loot;
namespace {
template<class T,std::size_t N> void put(std::array<unsigned char,N>& bytes,std::size_t offset,T value) {
    assert(offset+sizeof(value)<=N);std::memcpy(bytes.data()+offset,&value,sizeof(value));
}
struct Fixture;
thread_local Fixture* active{};
struct Fixture {
    Address image=0x140000000;
    void* receiver_object=reinterpret_cast<void*>(0x1110);
    void* player_object=reinterpret_cast<void*>(0x2220);
    void* component_object=reinterpret_cast<void*>(0x3330);
    void* target_object=reinterpret_cast<void*>(0x4440);
    void* gimmick_object=reinterpret_cast<void*>(0x5550);
    std::array<unsigned char,0xc8> interaction_definition{};
    std::array<unsigned char,0x680> gimmick_definition{};
    std::array<Interaction,2> records{};
    InteractionVector vector{records.data(),1,2};
    std::uint32_t player_id=71,target_id=91;
    std::uint16_t gimmick_id=27;
    int scans{},checks{},capacities{},submits{},updates{},ground_reads{},player_reads{},leases{},releases{};
    bool cache_valid=true,lease_valid=true,record_valid=true,expired_first=false;std::size_t candidate_count=1;
    bool scan_result=true,selected=true,ground_valid=true,stale_player=false,stale_target=false;
    bool throw_scan=false,throw_submit=false,throw_native_allocation=false;
    bool nested_scan=false;
    bool corpse_mode=false,corpse_valid=true,stale_corpse=false,throw_loot=false;
    int corpse_reads{},loot_checks{},loot_error{};
    Attempts* nested_attempts{};
    std::uint8_t check_state=2;
    int check_error{},capacity_error{},submit_error{};
    bool saw_dialog=true;
    Fixture() {
        active=this;
        for(auto& record:records){record.name=image+0x6a65940;record.pivot_name=image+0x6a65940;record.handle=16;}
        put(interaction_definition,0,kFreeGroundInteraction);
        interaction_definition[0x11]=0x0b;
        put(interaction_definition,0x40,std::uint32_t{1});
        put(interaction_definition,0x80,std::uint32_t{0xffffffff});
    }
    Address base() const{return image;}
    bool healthy() const{return true;}
    void* receiver()const{return receiver_object;}
    struct Lease {
        Fixture& fixture;bool expired{};
        Lease(Fixture& f,std::uint32_t id):fixture(f),expired(f.expired_first&&id==f.target_id+1000){assert(expired||id==f.target_id);++f.leases;}
        ~Lease(){++fixture.releases;}
        void* get()const{return fixture.lease_valid&&!expired?fixture.target_object:nullptr;}
    };
    bool nearby(void* field,std::array<std::uint32_t,512>& ids,std::size_t& count) {
        assert(field==gimmick_object&&updates);count=candidate_count;ids[0]=target_id;
        if(expired_first){ids[0]=target_id+1000;ids[1]=target_id;}
        return cache_valid;
    }
    bool interactions(void* target,std::array<Interaction,32>& out,std::uint32_t& size) {
        assert(target==target_object);size=vector.size;
        for(std::size_t i=0;i<records.size();++i)out[i]=records[i];return vector.size<=vector.capacity;
    }
    bool record_current(void* target,const Interaction& record)const{return record_valid&&target==target_object&&record.handle==16;}
    std::uint32_t identity(void* actor) const{return actor==player_object ? player_id : 0;}
    void* component(void* actor) const{return actor==player_object ? component_object : nullptr;}
    bool player(const Frame& frame,void* component) {
        ++player_reads;
        return selected&&(!stale_player||!checks)&&frame.player==player_object&&frame.receiver==receiver_object&&
            frame.identity==player_id&&component==component_object;
    }
    bool ground(const Frame&,void* target,Snapshot& snapshot) {
        ++ground_reads;
        snapshot={gimmick_object,target_id+(stale_target&&ground_reads>1?1U:0U),gimmick_id};
        return ground_valid&&target==target_object;
    }
    Native filters() const{return Native(image,{});}
    bool plain_interaction(const Interaction* r)const{return filters().plain_interaction(r);}
    bool free_interaction(const unsigned char* p)const{return filters().free_interaction(p);}
    bool free_gimmick(const unsigned char* p)const{return filters().free_gimmick(p);}
    bool free_corpse_interaction(const unsigned char* p)const{return filters().free_corpse_interaction(p);}
    bool corpse(const Frame&,void* target,CorpseSnapshot& snapshot){
        ++corpse_reads;snapshot={gimmick_object,component_object,nullptr,target_id+(stale_corpse&&corpse_reads>1?1U:0U),gimmick_id};
        return corpse_mode&&corpse_valid&&target==target_object;
    }
    static const unsigned char* __fastcall lookup_interaction(const std::uint16_t* handle) {
        assert(handle&&*handle==16);return active->interaction_definition.data();
    }
    static const unsigned char* __fastcall lookup_gimmick(const std::uint16_t* handle) {
        assert(handle&&*handle==active->gimmick_id);return active->gimmick_definition.data();
    }
    static bool __fastcall scan(void* component,void* target,InteractionVector* output) {
        auto& f=*active;++f.scans;
        assert(component==f.component_object&&target==f.target_object&&!output);
        assert(f.leases>f.releases&&current_frame&&in_policy);
        if(f.throw_scan)throw std::runtime_error("native scan");

        return f.scan_result;
    }
    static void __fastcall check(void* component,void* target,const Interaction* record,std::uint8_t mode,Eligibility* output) {
        auto& f=*active;++f.checks;
        assert(component==f.component_object&&target==f.target_object&&record&&record->handle==16&&mode==3);
        assert(output->state==2&&output->error==0&&output->pivot==0xffffffff);
        output->state=f.check_state;output->error=f.check_error;
    }
    static int* __fastcall capacity(void* gimmick,int* error,void* actor,std::uint8_t mode) {
        auto& f=*active;++f.capacities;
        assert(gimmick==f.gimmick_object&&actor==f.player_object&&mode==0&&*error==-1);
        *error=f.capacity_error;return error;
    }
    static int* __fastcall can_loot(void* loot,int* error,void* unused,void* actor,bool distance){
        auto& f=*active;++f.loot_checks;
        assert(loot==f.gimmick_object&&!unused&&actor==f.player_object&&distance&&*error==-1);
        if(f.throw_loot)throw std::runtime_error("native loot eligibility");
        *error=f.loot_error;return error;
    }
    static int* __fastcall submit(void* receiver,int* error,void* target,const Interaction* record,bool* dialog,std::uint32_t* scratch) {
        auto& f=*active;++f.submits;
        assert(receiver==f.receiver_object&&target==f.target_object&&record&&record->handle==16&&*error==-1&&*scratch==0);
        f.saw_dialog=*dialog;
        assert(current_frame&&current_frame->submitted&&in_policy);
        if(f.throw_native_allocation)throw std::bad_alloc{};
        if(f.throw_submit)throw std::runtime_error("native submit");
        *error=f.submit_error;*scratch=123;return error;
    }
    static void __fastcall update(void* field,void* actor) {
        auto& f=*active;++f.updates;assert(field==f.gimmick_object&&actor==f.player_object);
        if(f.nested_scan){f.nested_scan=false;assert(f.nested_attempts);dispatch_nearby(f,*f.nested_attempts,update,true,f.image+kNearbyCaller,1000,field,actor);}
    }
    Bindings bindings()const{Bindings result{lookup_interaction,lookup_gimmick,check,capacity,submit,nullptr,scan,nullptr,nullptr};result.can_loot=can_loot;return result;}
    bool run(Attempts& attempts,std::uint64_t now=1000,bool enabled=true,Address caller=0) {
        dispatch_nearby(*this,attempts,update,enabled,caller?caller:image+kNearbyCaller,now,gimmick_object,player_object);
        return true;
    }
};

void filters() {
    Fixture f;auto native=f.filters();
    assert(native.free_interaction(f.interaction_definition.data()));
    put(f.interaction_definition,0,kAnimatedGroundInteraction);f.interaction_definition[0x11]=0;
    assert(native.free_interaction(f.interaction_definition.data()));f.interaction_definition[0x11]=0x0b;
    assert(!native.free_interaction(f.interaction_definition.data()));
    for(const std::uint32_t key:{1000138U,1000045U,12U,1000034U}) {
        put(f.interaction_definition,0,key);assert(!native.free_interaction(f.interaction_definition.data()));
    }
    put(f.interaction_definition,0,kFreeGroundInteraction);
    for(const auto offset:{0x10,0xc1}) {f.interaction_definition[offset]=1;assert(!native.free_interaction(f.interaction_definition.data()));f.interaction_definition[offset]=0;}
    for(const auto offset:{0x78,0xa0}) {put(f.interaction_definition,offset,std::uint32_t{1});assert(!native.free_interaction(f.interaction_definition.data()));put(f.interaction_definition,offset,std::uint32_t{});}
    put(f.interaction_definition,0x80,std::uint32_t{7});assert(!native.free_interaction(f.interaction_definition.data()));
    put(f.interaction_definition,0x80,std::uint32_t{0xffffffff});
    f.interaction_definition[0x11]=0xc;assert(!native.free_interaction(f.interaction_definition.data()));
    assert(native.free_gimmick(f.gimmick_definition.data()));
    for(const auto offset:{0x18,0x66d,0x3c9,0x3ca}) {f.gimmick_definition[offset]=1;assert(!native.free_gimmick(f.gimmick_definition.data()));f.gimmick_definition[offset]=0;}
    put(f.gimmick_definition,0x288,Address{1});assert(!native.free_gimmick(f.gimmick_definition.data()));
    assert(native.plain_interaction(&f.records[0]));f.records[0].serialized_name=reinterpret_cast<void*>(1);assert(!native.plain_interaction(&f.records[0]));
    assert(!native.free_interaction(nullptr)&&!native.free_gimmick(nullptr)&&!native.plain_interaction(nullptr));
}
void normal_flow() {
    Fixture f;Attempts attempts;
    assert(f.run(attempts));
    assert(f.scans==1&&f.checks==1&&f.capacities==1&&f.submits==1&&!f.saw_dialog&&f.ground_reads==2&&f.player_reads==5&&f.leases==1&&f.releases==1);
    assert(!current_frame&&!in_policy);
    assert(f.run(attempts,2000)&&f.submits==1); // never resubmit the same native identity
    f.target_id=92;assert(f.run(attempts,1200)&&f.submits==1); // bounded request rate
    assert(f.run(attempts,2500)&&f.submits==2);
    assert(f.vector.data==f.records.data()&&f.vector.size==1&&f.vector.capacity==2);
    const auto leases=f.leases;f.run(attempts,2600);assert(f.leases==leases); // scan throttle skips extra native leases
    Fixture several;Attempts multiple;several.candidate_count=2;several.expired_first=true;
    put(several.interaction_definition,0,kAnimatedGroundInteraction);several.interaction_definition[0x11]=0;
    several.run(multiple);assert(several.submits==1&&several.leases==2&&several.releases==2);
}
void corpse_flow(){
    for(const auto key:kCorpseInteractions){
        Fixture f;Attempts attempts;f.corpse_mode=true;f.ground_valid=false;
        put(f.interaction_definition,0,key);f.interaction_definition[0x11]=0;
        assert(f.free_corpse_interaction(f.interaction_definition.data()));
        f.run(attempts);assert(f.submits==1&&f.loot_checks==1&&!f.capacities&&!f.saw_dialog&&f.leases==f.releases);
        f.run(attempts,2000);assert(f.submits==1); // same ledger for all pickup families
    }
    for(unsigned scenario=0;scenario<9;++scenario){
        Fixture f;Attempts attempts;f.corpse_mode=true;f.ground_valid=false;
        put(f.interaction_definition,0,kCorpseInteractions[0]);f.interaction_definition[0x11]=0;
        switch(scenario){
        case 0:f.corpse_valid=false;break;
        case 1:f.stale_corpse=true;break;
        case 2:f.loot_error=123;break;
        case 3:f.check_state=3;break;
        case 4:f.check_error=55;break;
        case 5:f.record_valid=false;break;
        case 6:f.scan_result=false;break;
        case 7:put(f.interaction_definition,0,std::uint32_t{1000076});break;
        case 8:put(f.interaction_definition,0x78,std::uint32_t{1});break;
        }
        f.run(attempts);assert(!f.submits&&!f.capacities&&f.leases==f.releases&&!current_frame&&!in_policy);
    }
    Fixture throwing;Attempts attempts;throwing.corpse_mode=true;throwing.ground_valid=false;throwing.throw_loot=true;
    put(throwing.interaction_definition,0,kCorpseInteractions[0]);throwing.interaction_definition[0x11]=0;
    try{throwing.run(attempts);assert(false);}catch(const std::runtime_error&){}
    assert(!throwing.submits&&!current_frame&&!in_policy&&throwing.leases==throwing.releases);
    throwing.throw_loot=false;throwing.run(attempts,2000);assert(throwing.submits==1); // pre-request refusal does not consume identity
    for(unsigned failure=0;failure<3;++failure){
        Fixture f;Attempts ledger;f.corpse_mode=true;f.ground_valid=false;
        put(f.interaction_definition,0,kCorpseInteractions[0]);f.interaction_definition[0x11]=0;
        f.submit_error=failure==0?71:0;f.throw_submit=failure==1;f.throw_native_allocation=failure==2;
        try{f.run(ledger);assert(failure==0);}catch(const std::bad_alloc&){assert(failure==2);}catch(const std::runtime_error&){assert(failure==1);}
        assert(f.submits==1&&f.leases==f.releases&&!current_frame&&!in_policy);
        f.throw_submit=f.throw_native_allocation=false;f.run(ledger,2000);assert(f.submits==1);
    }
    Fixture f;f.interaction_definition[0x11]=0;
    for(const auto key:{1000076U,1000179U,21U,1000138U}){put(f.interaction_definition,0,key);assert(!f.free_corpse_interaction(f.interaction_definition.data()));}
    put(f.interaction_definition,0,kCorpseInteractions[0]);
    for(const auto offset:{0x10,0x11,0xc1}){f.interaction_definition[offset]=1;assert(!f.free_corpse_interaction(f.interaction_definition.data()));f.interaction_definition[offset]=0;}
    for(const auto offset:{0x78,0xa0}){put(f.interaction_definition,offset,std::uint32_t{1});assert(!f.free_corpse_interaction(f.interaction_definition.data()));put(f.interaction_definition,offset,std::uint32_t{});}
    put(f.interaction_definition,0x40,std::uint32_t{});assert(!f.free_corpse_interaction(f.interaction_definition.data()));
}
void refusals() {
    for(int scenario=0;scenario<20;++scenario) {
        Fixture f;Attempts attempts;bool enabled=true;Address caller{};
        switch(scenario) {
        case 0:enabled=false;break;
        case 1:caller=f.image+kNearbyCaller+1;break;
        case 2:f.scan_result=false;break;
        case 3:f.selected=false;break;
        case 4:f.ground_valid=false;break;
        case 5:f.check_state=3;break;
        case 6:f.check_error=123;break;
        case 7:f.capacity_error=88;break;
        case 8:f.stale_player=true;break;
        case 9:f.stale_target=true;break;
        case 10:put(f.gimmick_definition,0x288,Address{1});break;
        case 11:put(f.interaction_definition,0,std::uint32_t{1000138});break;
        case 12:f.records[0].pivot=0;break;
        case 13:f.vector.size=33;f.vector.capacity=33;break;
        case 14:f.vector.size=2;f.vector.capacity=1;break;
        case 15:f.cache_valid=false;break;
        case 16:f.lease_valid=false;break;
        case 17:f.candidate_count=513;break;
        case 18:f.candidate_count=0;break;
        case 19:f.record_valid=false;break;
        }
        const bool result=f.run(attempts,1000,enabled,caller);
        assert(result&&f.updates==1&&!f.submits&&!current_frame&&!in_policy&&f.leases==f.releases);
    }
    Fixture f;Attempts attempts;
    {FrameScope scope(f.receiver_object,f.player_object,f.player_id,1000);f.run(attempts);assert(!f.submits);}
    {PolicyScope policy;f.run(attempts);assert(!f.submits);}

}
void failures_and_scopes() {
    Fixture nested;Attempts nested_attempts;nested.nested_scan=true;nested.nested_attempts=&nested_attempts;
    assert(nested.run(nested_attempts)&&nested.updates==2&&nested.scans==1&&nested.submits==1&&nested.checks==1&&!in_policy);
    Fixture f;Attempts attempts;f.submit_error=77;
    assert(f.run(attempts)&&f.submits==1);assert(f.run(attempts,2000)&&f.submits==1);
    Fixture throwing;Attempts other;throwing.throw_submit=true;
    try{throwing.run(other);assert(false);}catch(const std::runtime_error&){}
    assert(!current_frame&&!in_policy&&throwing.submits==1);
    throwing.throw_submit=false;assert(throwing.run(other,2000)&&throwing.submits==1);
    Fixture native_allocation;Attempts native_attempts;native_allocation.throw_native_allocation=true;
    try{native_allocation.run(native_attempts);assert(false);}catch(const std::bad_alloc&){}
    assert(!current_frame&&!in_policy&&native_allocation.leases==native_allocation.releases);
    native_allocation.throw_native_allocation=false;native_allocation.run(native_attempts,2000);assert(native_allocation.submits==1);
    Fixture scan_failure;Attempts third;scan_failure.throw_scan=true;
    try{scan_failure.run(third);assert(false);}catch(const std::runtime_error&){}
    assert(!current_frame&&!in_policy&&!scan_failure.submits);
    assert(throwing.leases==throwing.releases&&scan_failure.leases==scan_failure.releases);

}
struct AllocationFailure {inline static bool fail{};inline static std::size_t allocations{};};
template<class T> struct FailingAllocator {
    using value_type=T;
    FailingAllocator() noexcept=default;
    template<class U> FailingAllocator(const FailingAllocator<U>&) noexcept{}
    T* allocate(std::size_t count) {
        ++AllocationFailure::allocations;
        if(AllocationFailure::fail)throw std::bad_alloc{};
        return std::allocator<T>{}.allocate(count);
    }
    void deallocate(T* value,std::size_t count) noexcept{std::allocator<T>{}.deallocate(value,count);}
    template<class U> bool operator==(const FailingAllocator<U>&)const noexcept{return true;}
};
void growing_attempts() {
    Attempts growing;
    constexpr std::uint32_t count=20000;
    for(std::uint32_t id=1;id<=count;++id)assert(growing.claim(1,id,std::uint64_t(id)*500));
    assert(growing.begin_scan(20000000));
    for(std::uint32_t id=1;id<=count;++id)assert(!growing.claim(1,id,20000000));
    // All 32 bits of both native identities distinguish player/target pairs.
    assert(growing.claim(0x80000001U,1,20000000));
    assert(growing.claim(1,0x80000001U,20000500));
    assert(!growing.claim(0x80000001U,1,20001000));
    assert(!growing.claim(1,0x80000001U,20001000));

    using FailingAttempts=AttemptSet<FailingAllocator<std::uint64_t>>;
    AllocationFailure::allocations=0;AllocationFailure::fail=false;
    FailingAttempts failed;assert(!AllocationFailure::allocations); // lazy container construction
    assert(failed.claim(1,1,1000));
    AllocationFailure::fail=true;assert(!failed.claim(1,2,1500));
    AllocationFailure::fail=false;assert(!failed.begin_scan(2000)&&!failed.claim(1,3,2000));
    assert(!failed.claim(1,1,2000));

    Fixture fixture;FailingAttempts before_request;AllocationFailure::fail=true;
    dispatch_nearby(fixture,before_request,Fixture::update,true,fixture.image+kNearbyCaller,1000,fixture.gimmick_object,fixture.player_object);
    assert(fixture.checks==1&&fixture.capacities==1&&!fixture.submits&&fixture.leases==fixture.releases);
    AllocationFailure::fail=false;
    dispatch_nearby(fixture,before_request,Fixture::update,true,fixture.image+kNearbyCaller,2000,fixture.gimmick_object,fixture.player_object);
    assert(!fixture.submits&&fixture.updates==2&&!current_frame&&!in_policy);
}
struct FakeHooks {
    int pins{},initializations{},creates{},queues{},applies{},disables{},removes{},uninitializes{};
    bool pin_ok=true,apply_ok=true,disable_ok=true;int create_fail{};
    bool pin(){++pins;return pin_ok;}
    bool initialize(){++initializations;return true;}
    bool create(Address,Address,Address& out){++creates;out=Address(0x100+creates);return creates!=create_fail;}
    bool queue_enable(Address){++queues;return true;}
    bool apply(){++applies;return apply_ok;}
    bool disable(Address){++disables;return disable_ok;}
    bool remove(Address){++removes;return true;}
    bool uninitialize(){++uninitializes;return true;}
};
struct OwnedNative {
    unsigned char* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x7000000,MEM_RESERVE,PAGE_NOACCESS));
    std::array<unsigned char,0x100> root{},manager{},fields{},actor{},metadata{},components{},gimmick{},child{},possessor{},receiver{};
    std::array<unsigned char,0x100> unrelated_fields{};
    std::array<unsigned char,0x1200> client_root{};
    std::array<unsigned char,0x380> unrelated_field{};
    std::array<unsigned char,0x380> field_object{};
    std::array<unsigned char,0x100> interaction{};
    std::array<unsigned char,0x380> state{};
    std::array<unsigned char,0x80> loot{},tribe_definition{};
    std::array<unsigned char,0x200> character_definition{},corpse_definition{};
    std::array<unsigned char,0x20> item{};
    std::array<unsigned char,0x680> definition{};
    std::array<unsigned char,0x28*3> entries{};
    std::array<Address,5> node_vtable{};
    std::array<Address,1> node{};
    std::array<std::uint16_t,1> row{16};
    std::array<void*,1> rows{row.data()};
    int locks{},unlocks{},resolves{},releases{},finds{},clears{},relationships{};
    std::uint32_t corpse_key=12;std::uint8_t relationship_result=1;
    bool custom_record=false,throw_find=false,throw_clear=false,wrong_key=false;
    inline static OwnedNative* active_owned{};
    static void __fastcall unlock(void* pointer,bool shared) {
        auto& f=*active_owned;assert(pointer==f.node.data()&&shared);++f.unlocks;
    }
    static void* __fastcall lock(void** output,void* pointer) {
        auto& f=*active_owned;assert(pointer==f.node.data());++f.locks;*output=pointer;return output;
    }
    static const unsigned char* __fastcall gimmick_lookup(const std::uint16_t* handle) {
        assert(*handle==27);return active_owned->definition.data();
    }
    static const unsigned char* __fastcall interaction_lookup(const std::uint16_t* handle){
        assert(*handle==16);return active_owned->corpse_definition.data();
    }
    static const unsigned char* __fastcall character_lookup(const std::uint16_t* handle){
        assert(*handle==27);return active_owned->character_definition.data();
    }
    static const unsigned char* __fastcall tribe_lookup(const std::uint16_t* handle){
        assert(*handle==5);return active_owned->tribe_definition.data();
    }
    static std::uint8_t __fastcall relationship(void* source,void* target,bool native_mode){
        auto& f=*active_owned;++f.relationships;
        assert(source==f.child.data()&&target==f.actor.data()&&native_mode&&f.locks==f.unlocks);
        return f.relationship_result;
    }
    static bool __fastcall find_interaction(void* component,bool cached,const std::uint32_t* key,Interaction* output){
        auto& f=*active_owned;++f.finds;
        assert(component==f.interaction.data()&&cached&&key&&output&&f.locks==f.unlocks&&current_frame);
        assert(output->name==reinterpret_cast<Address>(f.image)+0x6a65940&&!output->serialized_name&&output->handle==0xffff);
        if(f.throw_find)throw std::runtime_error("native owned-record lookup");
        if(*key!=f.corpse_key)return false;
        output->handle=16;if(f.custom_record)output->serialized_name=f.item.data();
        put(f.corpse_definition,0,f.wrong_key?std::uint32_t{1000009}:f.corpse_key);
        return true;
    }
    static void __fastcall clear_interactions(InteractionVector* view){
        auto& f=*active_owned;++f.clears;
        assert(view&&view->size==1&&view->capacity==1&&view->data&&f.locks==f.unlocks);
        view->size=0;
        if(f.throw_clear)throw std::runtime_error("native owned-record cleanup");
    }
    static int* __fastcall loot_eligibility(void*,int* error,void*,void*,bool){return error;}
    static ActorLease* __fastcall resolve(void* manager,ActorLease* output,std::uint32_t id) {
        auto& f=*active_owned;assert(manager==f.manager.data()&&id==91);++f.resolves;
        *output={0,f.actor.data(),1,0,{},nullptr};return output;
    }
    static void __fastcall release(ActorLease* value) {
        auto& f=*active_owned;assert(value->actor==f.actor.data()&&value->active);++f.releases;value->active=0;
    }
    void global(std::size_t offset,const void* value,std::size_t size) {
        const auto page=offset&~std::size_t(4095);
        assert(VirtualAlloc(image+page,4096,MEM_COMMIT,PAGE_READWRITE)==image+page);
        std::memcpy(image+offset,value,size);
    }
    OwnedNative() {
        assert(image);active_owned=this;
        node_vtable[4]=reinterpret_cast<Address>(&unlock);node[0]=reinterpret_cast<Address>(node_vtable.data());
        auto* root_pointer=root.data();global(0x6d69190,&root_pointer,sizeof(root_pointer));
        void* null_pointer{};global(0x6cfcd78,&null_pointer,sizeof(null_pointer));
        float radius=3;global(0x6cca8b8,&radius,sizeof(radius));
        put(root,0x30,manager.data());put(manager,0,reinterpret_cast<Address>(image)+0x55b62c8);
        put(manager,0x70,fields.data());put(fields,0,field_object.data());
        put(root,0xc8,unrelated_fields.data());put(unrelated_fields,0,unrelated_field.data());
        put(unrelated_field,0,reinterpret_cast<Address>(image)+0x55bcea0);put(unrelated_field,8,node.data());
        put(field_object,0,reinterpret_cast<Address>(image)+0x55bcea0);put(field_object,8,node.data());
        put(field_object,0x218,client_root.data());put(client_root,0x10f8,manager.data());
        put(field_object,0x308,entries.data());put(field_object,0x310,std::uint32_t{3});put(field_object,0x314,std::uint32_t{3});
        field_object[0x318]=field_object[0x31a]=1;
        put(actor,8,node.data());put(actor,0x60,std::uint32_t{91});put(actor,0x68,components.data());put(actor,0x88,metadata.data());metadata[1]=7;
        put(actor,0x70,std::uint32_t{32});
        put(components,0x20,state.data());put(components,0x30,gimmick.data());put(components,0x98,interaction.data());
        put(gimmick,0,reinterpret_cast<Address>(image)+0x55b7898);put(gimmick,8,actor.data());put(gimmick,0x48,std::uint16_t{27});put(gimmick,0xc0,item.data());
        put(item,8,std::uint16_t{123});put(item,0x10,std::int64_t{2});
        put(interaction,0,reinterpret_cast<Address>(image)+0x55bdb30);put(interaction,8,actor.data());
        put(definition,0x30,rows.data());put(definition,0x38,std::uint32_t{1});
        for(std::size_t index=0;index<3;++index) {
            put(entries,index*0x28+8,actor.data());entries[index*0x28+0x10]=1;
            put(entries,index*0x28+0x20,float(index+1));
        }
    }
    ~OwnedNative(){assert(locks==unlocks);VirtualFree(image,0,MEM_RELEASE);active_owned=nullptr;}
    Native native() {
        Bindings bindings{};bindings.gimmick=gimmick_lookup;bindings.lock=lock;bindings.resolve=resolve;bindings.release=release;
        bindings.interaction=interaction_lookup;bindings.find=find_interaction;bindings.clear=clear_interactions;
        bindings.can_loot=loot_eligibility;bindings.character=character_lookup;bindings.tribe=tribe_lookup;bindings.relationship=relationship;
        return Native(reinterpret_cast<Address>(image),bindings);
    }
    void as_corpse(){
        put(actor,0,reinterpret_cast<Address>(image)+0x55857b0);metadata[1]=1;
        put(state,8,actor.data());put(state,0x30,std::uint16_t{27});state[0x273]=1;
        put(components,0xc8,loot.data());put(loot,0,reinterpret_cast<Address>(image)+0x55be5d8);put(loot,8,actor.data());
        put(character_definition,0xa4,std::uint16_t{5});tribe_definition[0x50]=7;
        put(corpse_definition,0,corpse_key);put(corpse_definition,0x40,std::uint32_t{1});put(corpse_definition,0x80,std::uint32_t{0xffffffff});
    }
};
void native_readers() {
    OwnedNative fixture;auto native=fixture.native();
    const auto image=reinterpret_cast<Address>(fixture.image);
    put(fixture.root,0x58,fixture.receiver.data());put(fixture.manager,0,image+0x55b62c8);
    put(fixture.manager,0x50,fixture.child.data());put(fixture.manager,0x58,fixture.possessor.data());
    put(fixture.child,0,image+0x5585c40);put(fixture.child,0x60,std::uint32_t{91});fixture.child[0x96]=1;
    put(fixture.child,0xa0,fixture.possessor.data());put(fixture.child,0x88,fixture.metadata.data());
    put(fixture.possessor,0xd0,fixture.child.data());put(fixture.possessor,0xd8,fixture.actor.data());
    put(fixture.actor,0,image+0x55857b0);put(fixture.actor,0xa0,fixture.possessor.data());fixture.actor[0x94]=1;fixture.metadata[1]=1;
    Frame selected{nullptr,fixture.receiver.data(),fixture.actor.data(),91,1000,false};
    assert(native.selected_pair(selected,fixture.interaction.data()));
    put(fixture.actor,0x70,std::uint32_t{19});assert(!native.component(fixture.actor.data())&&!native.selected_pair(selected,fixture.interaction.data()));
    put(fixture.actor,0x70,std::uint32_t{20});assert(native.component(fixture.actor.data())==fixture.interaction.data()&&native.selected_pair(selected,fixture.interaction.data()));
    put(fixture.actor,0x70,std::uint32_t{32});
    put(fixture.manager,0x50,fixture.actor.data());assert(!native.selected_pair(selected,fixture.interaction.data()));put(fixture.manager,0x50,fixture.child.data());
    put(fixture.possessor,0xd8,fixture.child.data());assert(!native.selected_pair(selected,fixture.interaction.data()));put(fixture.possessor,0xd8,fixture.actor.data());
    selected.identity=92;assert(!native.selected_pair(selected,fixture.interaction.data()));selected.identity=91;
    fixture.receiver[0x2c]=1;assert(!native.selected_pair(selected,fixture.interaction.data()));fixture.receiver[0x2c]=0;
    assert(native.selected_pair(selected,fixture.interaction.data()));fixture.metadata[1]=7;
    std::array<std::uint32_t,512> ids{};std::size_t count{};
    fixture.entries[0x25]=1; // behind the camera remains a nearby candidate
    fixture.entries[0x28+0x24]=1; // native excluded state
    put(fixture.entries,0x50+0x20,10.0f); // outside the native radius squared
    assert(native.nearby(fixture.field_object.data(),ids,count)&&count==1&&ids[0]==91);
    // root+c8 deliberately refers to another field; only manager+70 and the
    // field's ClientRoot+10f8 backlink authorize the queried field.
    assert(!native.nearby(fixture.unrelated_field.data(),ids,count));
    put(fixture.root,0xc8,fixture.fields.data());put(fixture.manager,0x70,fixture.unrelated_fields.data());
    assert(!native.nearby(fixture.field_object.data(),ids,count));put(fixture.manager,0x70,fixture.fields.data());
    put(fixture.root,0xc8,fixture.unrelated_fields.data());
    put(fixture.client_root,0x10f8,fixture.root.data());assert(!native.nearby(fixture.field_object.data(),ids,count));put(fixture.client_root,0x10f8,fixture.manager.data());
    put(fixture.field_object,0x218,static_cast<void*>(nullptr));assert(!native.nearby(fixture.field_object.data(),ids,count));put(fixture.field_object,0x218,fixture.client_root.data());
    assert(native.nearby(fixture.field_object.data(),ids,count)&&count==1);
    fixture.field_object[0x318]=0;assert(!native.nearby(fixture.field_object.data(),ids,count)&&!count);fixture.field_object[0x318]=1;
    fixture.field_object[0x31a]=0;assert(!native.nearby(fixture.field_object.data(),ids,count));fixture.field_object[0x31a]=1;
    put(fixture.field_object,0x310,std::uint32_t{4097});assert(!native.nearby(fixture.field_object.data(),ids,count));put(fixture.field_object,0x310,std::uint32_t{3});
    put(fixture.field_object,0x314,std::uint32_t{2});assert(!native.nearby(fixture.field_object.data(),ids,count));put(fixture.field_object,0x314,std::uint32_t{3});
    put(fixture.entries,0x20,std::numeric_limits<float>::quiet_NaN());assert(native.nearby(fixture.field_object.data(),ids,count)&&!count);put(fixture.entries,0x20,1.0f);
    Snapshot snapshot{};Frame frame{};
    assert(native.ground(frame,fixture.actor.data(),snapshot)&&snapshot.target==91&&snapshot.definition==27&&snapshot.gimmick_component==fixture.gimmick.data());
    put(fixture.actor,0x70,std::uint32_t{6});assert(!native.ground(frame,fixture.actor.data(),snapshot));
    put(fixture.actor,0x70,std::uint32_t{7});assert(native.ground(frame,fixture.actor.data(),snapshot));put(fixture.actor,0x70,std::uint32_t{32});
    fixture.metadata[1]=1;assert(!native.ground(frame,fixture.actor.data(),snapshot));fixture.metadata[1]=7;
    put(fixture.item,0x10,std::int64_t{});assert(!native.ground(frame,fixture.actor.data(),snapshot));put(fixture.item,0x10,std::int64_t{2});
    fixture.state[0x2d0]=0x1d;assert(!native.ground(frame,fixture.actor.data(),snapshot));fixture.state[0x2d0]=0;
    std::array<Interaction,32> records{};std::uint32_t size{};
    assert(native.interactions(fixture.actor.data(),records,size)&&size==1&&records[0].handle==16&&native.plain_interaction(&records[0]));
    const auto plain=records[0];
    put(fixture.actor,0x70,std::uint32_t{19});assert(!native.interactions(fixture.actor.data(),records,size));
    put(fixture.actor,0x70,std::uint32_t{20});assert(native.interactions(fixture.actor.data(),records,size));put(fixture.actor,0x70,std::uint32_t{32});
    put(fixture.interaction,0x90,std::uint32_t{1});assert(!native.interactions(fixture.actor.data(),records,size)&&!size);
    std::array<Interaction,1> cached{plain};put(fixture.interaction,0x88,cached.data());
    put(fixture.interaction,0x90,std::uint32_t{33});assert(!native.interactions(fixture.actor.data(),records,size));put(fixture.interaction,0x90,std::uint32_t{1});
    assert(native.interactions(fixture.actor.data(),records,size)&&size==1&&native.record_current(fixture.actor.data(),plain));
    cached[0].serialized_name=fixture.item.data();assert(!native.interactions(fixture.actor.data(),records,size)&&!size);
    assert(!native.record_current(fixture.actor.data(),plain));put(fixture.interaction,0x90,std::uint32_t{});
    // The count header bounds are checked first. Even a sufficient declared
    // count cannot authorize access to an unreadable table span.
    auto* short_table=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    assert(short_table);DWORD old_protection{};
    assert(VirtualProtect(short_table+4096,4096,PAGE_NOACCESS,&old_protection));
    put(fixture.actor,0x68,short_table+4096-0x20);
    assert(!native.component(fixture.actor.data()));
    assert(!native.ground(frame,fixture.actor.data(),snapshot));
    assert(!native.interactions(fixture.actor.data(),records,size));
    fixture.metadata[1]=1;assert(!native.selected_pair(selected,fixture.interaction.data()));fixture.metadata[1]=7;
    put(fixture.actor,0x68,fixture.components.data());assert(VirtualFree(short_table,0,MEM_RELEASE));
    {Native::Lease lease(native,91);assert(lease.get()==fixture.actor.data()&&fixture.resolves==1&&!fixture.releases);}
    assert(fixture.releases==1&&fixture.locks==fixture.unlocks);
}
void corpse_readers(){
    OwnedNative f;f.as_corpse();auto native=f.native();
    Frame frame{nullptr,f.receiver.data(),f.child.data(),71,1000,false};CorpseSnapshot snapshot{};
    assert(native.corpse(frame,f.actor.data(),snapshot)&&snapshot.loot_component==f.loot.data()&&snapshot.target==91&&!f.relationships);
    // Original CheckDead224f16d returns a condition status (zero success),
    // while ServerLootBuff2acd4de tests its DeadBody byte and the target's
    // +273 before entering CanLoot. Model those separate native predicates
    // explicitly: interpreting SETE as a boolean "dead" would invert both.
    const auto check_dead_status=[](unsigned char flag){return flag==0?1:0;};
    const auto ordinary_dead_body_loot=[](bool dead_body,unsigned char flag){return !dead_body||flag!=0;};
    for(const auto flag:{0,1,2,255}){
        f.state[0x273]=static_cast<unsigned char>(flag);
        const auto expected=check_dead_status(f.state[0x273])==0&&ordinary_dead_body_loot(true,f.state[0x273]);
        assert(native.corpse(frame,f.actor.data(),snapshot)==expected);
    }
    f.state[0x273]=1;
    // An owner or any custom criminal predicate is never made eligible by
    // tribe or relationship exemptions, even for a confirmed dead body.
    for(const auto offset:{0x300,0x2f8,0x304}){
        f.state[offset]=1;assert(!native.corpse(frame,f.actor.data(),snapshot));f.state[offset]=0;
    }
    f.tribe_definition[0x50]=4;assert(!native.corpse(frame,f.actor.data(),snapshot)&&f.relationships==1);
    f.relationship_result=0;assert(native.corpse(frame,f.actor.data(),snapshot)&&f.relationships==2);
    f.tribe_definition[0x50]=7;f.tribe_definition[0x10]=1;f.relationship_result=1;
    assert(!native.corpse(frame,f.actor.data(),snapshot));f.tribe_definition[0x10]=0;
    put(f.character_definition,0xa4,std::uint16_t{0xffff});assert(!native.corpse(frame,f.actor.data(),snapshot));put(f.character_definition,0xa4,std::uint16_t{5});
    put(f.actor,0x70,std::uint32_t{25});assert(!native.corpse(frame,f.actor.data(),snapshot));
    put(f.actor,0x70,std::uint32_t{26});assert(native.corpse(frame,f.actor.data(),snapshot));
    put(f.loot,8,f.child.data());assert(!native.corpse(frame,f.actor.data(),snapshot));put(f.loot,8,f.actor.data());
    put(f.state,8,f.child.data());assert(!native.corpse(frame,f.actor.data(),snapshot));put(f.state,8,f.actor.data());
    put(f.interaction,8,f.child.data());assert(!native.corpse(frame,f.actor.data(),snapshot));put(f.interaction,8,f.actor.data());
    const auto vt=field<Address>(f.loot.data(),0);put(f.loot,0,Address{});assert(!native.corpse(frame,f.actor.data(),snapshot));put(f.loot,0,vt);
    auto self=frame;self.player=f.actor.data();assert(!native.corpse(self,f.actor.data(),snapshot));
    // A table declaring enough components still cannot authorize an unreadable
    // +c8 slot; the memory reader fails closed without a native getter call.
    auto* pages=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));assert(pages);
    DWORD prior{};assert(VirtualProtect(pages+4096,4096,PAGE_NOACCESS,&prior));
    put(f.actor,0x68,pages+4096-0xc8);assert(!native.corpse(frame,f.actor.data(),snapshot));
    put(f.actor,0x68,f.components.data());assert(VirtualFree(pages,0,MEM_RELEASE));
    std::array<Interaction,32> records{};std::uint32_t size{};
    assert(!native.interactions(f.actor.data(),records,size)); // no borrowed source frame
    FrameScope scope(frame.receiver,frame.player,frame.identity,frame.now);
    for(const auto key:kCorpseInteractions){
        f.corpse_key=key;const auto finds=f.finds,clears=f.clears;
        assert(native.interactions(f.actor.data(),records,size)&&size==1&&records[0].handle==16&&native.plain_interaction(&records[0]));
        assert(f.finds==finds+3&&f.clears==clears+3&&native.record_current(f.actor.data(),records[0]));
    }
    f.custom_record=true;assert(!native.interactions(f.actor.data(),records,size)&&!size);f.custom_record=false;
    f.corpse_key=12;f.wrong_key=true;assert(!native.interactions(f.actor.data(),records,size));f.wrong_key=false;
    const auto cleared=f.clears;f.throw_find=true;
    try{native.interactions(f.actor.data(),records,size);assert(false);}catch(const std::runtime_error&){}
    assert(f.clears==cleared+1&&f.locks==f.unlocks);f.throw_find=false;
    f.throw_find=true;f.throw_clear=true;const auto before=f.clears;
    try{native.interactions(f.actor.data(),records,size);assert(false);}catch(const std::runtime_error& primary){assert(std::strcmp(primary.what(),"native owned-record lookup")==0);}
    assert(f.clears==before+1&&f.locks==f.unlocks&&!native.healthy());
    f.throw_find=f.throw_clear=false;const auto no_retry=f.finds;
    assert(!native.interactions(f.actor.data(),records,size)&&!native.corpse(frame,f.actor.data(),snapshot)&&f.finds==no_retry);
    // Normal cleanup exceptions remain transparent and also disable this
    // native adapter; the failed cleanup is never attempted a second time.
    auto separate=f.native();f.throw_clear=true;const auto normal_before=f.clears;
    try{separate.interactions(f.actor.data(),records,size);assert(false);}catch(const std::runtime_error& cleanup){assert(std::strcmp(cleanup.what(),"native owned-record cleanup")==0);}
    assert(f.clears==normal_before+1&&f.locks==f.unlocks&&!separate.healthy());f.throw_clear=false;
}
void lifecycle() {
    for(int mode=0;mode<5;++mode) {
        FakeHooks fake;std::atomic<bool> enabled=true;HookTransaction<FakeHooks> tx(fake,enabled);bool published=false;
        if(mode==1)fake.pin_ok=false;
        if(mode==2)fake.create_fail=1;
        if(mode==3)fake.apply_ok=false;
        const bool installed=tx.install(0x140000000,{11},[&]{return mode!=4;},[&](const std::array<Address,1>& p){published=p[0]!=0;return published;});
        assert(installed==(mode==0)&&enabled.load()==installed);
        if(mode==0){assert(published&&fake.queues==1&&fake.applies==1);assert(tx.cleanup()&&!enabled&&fake.removes==0&&fake.uninitializes==0);}
        if(mode==1)assert(!fake.initializations&&!fake.creates);
        if(mode==2)assert(fake.removes==0&&fake.uninitializes==1&&!fake.applies);
        if(mode==3)assert(published&&fake.removes==0&&fake.uninitializes==0);
        if(mode==4)assert(!fake.pins&&!fake.creates);
        assert(!tx.install(0x140000000,{11},[]{return true;},[](const auto&){return true;}));
    }
    Attempts attempts;std::atomic<int> accepted{};std::vector<std::thread> threads;
    for(int i=0;i<16;++i)threads.emplace_back([&]{if(attempts.claim(1,2,1000))++accepted;});
    for(auto& thread:threads)thread.join();assert(accepted==1);
}
} // namespace
int main(){filters();normal_flow();corpse_flow();refusals();failures_and_scopes();native_readers();corpse_readers();growing_attempts();lifecycle();}
