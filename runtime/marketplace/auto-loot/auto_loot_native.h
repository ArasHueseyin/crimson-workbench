#pragma once
#include <Windows.h>
#include <intrin.h>
#include <cstring>
#include <cmath>
#include <exception>
#include "auto_loot_policy.h"

namespace crimson::marketplace::auto_loot {
template<class T> inline T field(const void* object,std::size_t offset) {
    T result{};std::memcpy(&result,static_cast<const unsigned char*>(object)+offset,sizeof(result));return result;
}
inline void* lock_node(void* actor) noexcept {
    __try {return actor ? field<void*>(actor,8) : nullptr;}
    __except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
}
// Matches the native one-pointer guard used at 8f58c8/8f5927. It holds the
// subsystem lock only while reading a snapshot, never while issuing a request.
class ActorGuard {
    void* value_{};
public:
    ActorGuard(Lock lock,void* actor) {if(auto* node=lock_node(actor))lock(&value_,node);}
    ~ActorGuard(){if(value_)reinterpret_cast<Unlock>(field<Address>(field<void*>(value_,0),0x20))(value_,true);}
    explicit operator bool() const noexcept{return value_!=nullptr;}
    ActorGuard(const ActorGuard&)=delete;
    ActorGuard& operator=(const ActorGuard&)=delete;
};

class Native {
    Address base_{};
    Bindings bindings_{};
    mutable std::atomic<bool> cleanup_failed_{};
    bool nearby_locked(void* field_object,std::array<std::uint32_t,512>& ids,std::size_t& count) const noexcept {
        __try {
            auto* root=field<void*>(reinterpret_cast<void*>(base_),0x6d69190);
            auto* actor_manager=root ? field<void*>(root,0x30) : nullptr;
            auto* fields=actor_manager ? field<void*>(actor_manager,0x70) : nullptr;
            auto* client_root=field<void*>(field_object,0x218);
            // ad23a4/ad26d1 select this manager's first field. The original
            // query itself follows field+218 -> ClientRoot+10f8 at 9af4d3.
            if(!actor_manager||field<Address>(actor_manager,0)!=base_+0x55b62c8||!fields||
                field<void*>(fields,0)!=field_object||!client_root||field<void*>(client_root,0x10f8)!=actor_manager||
                field<Address>(field_object,0)!=base_+0x55bcea0||
                !field<std::uint8_t>(field_object,0x318)||!field<std::uint8_t>(field_object,0x31a))return false;
            // Same radius as ac3aa6. Worker 9aef70 stores the squared gap
            // between actor bounds in +20. Its +25 is camera-frustum culling
            // (9af149..9af221), so it must not restrict nearby loot.
            const auto radius=field<float>(reinterpret_cast<void*>(base_),0x6cca8b8);
            if(!std::isfinite(radius)||radius<=0||radius>100000)return false;
            const auto squared=radius*radius;
            auto* entries=field<unsigned char*>(field_object,0x308);
            const auto size=field<std::uint32_t>(field_object,0x310);
            const auto capacity=field<std::uint32_t>(field_object,0x314);
            if(!entries||size>capacity||size>4096)return false;
            for(std::uint32_t index=0;index<size&&count<ids.size();++index) {
                const auto* entry=entries+index*0x28;
                const auto distance=field<float>(entry,0x20);
                if(!std::isfinite(distance)||distance<0||distance>squared)continue;
                if(!entry[0x10]||entry[0x24])continue;
                auto* actor=field<void*>(entry,8);
                if(!actor)continue;
                const auto id=field<std::uint32_t>(actor,0x60);
                if(id)ids[count++]=id;
            }
            return true;
        } __except(EXCEPTION_EXECUTE_HANDLER){count=0;return false;}
    }
    bool base_records_locked(void* target,const unsigned char* definition,std::array<Interaction,32>& output,std::uint32_t& size) const noexcept {
        __try {
            auto* components=field<void*>(target,0x68);
            if(field<std::uint32_t>(target,0x70)<20)return false;
            auto* interaction=components ? field<void*>(components,0x98) : nullptr;
            if(!interaction||field<Address>(interaction,0)!=base_+0x55bdb30||field<void*>(interaction,8)!=target)return false;
            // 2123710 first uses the current +88/+90 cached list. Only
            // default records have no borrowed or reference-counted payload;
            // copy those by value under the target lock, preserving handles.
            const auto cached_count=field<std::uint32_t>(interaction,0x90);
            if(cached_count) {
                const auto* cached=field<const Interaction*>(interaction,0x88);
                if(!cached||cached_count>output.size())return false;
                for(std::uint32_t index=0;index<cached_count;++index)
                    if(plain_interaction(cached+index))output[size++]=cached[index];
                return size!=0;
            }
            auto** rows=field<void**>(definition,0x30);
            const auto count=field<std::uint32_t>(definition,0x38);
            if(!rows||!count||count>output.size())return false;
            for(std::uint32_t index=0;index<count;++index) {
                if(!rows[index])return false;
                output[index]={};output[index].handle=field<std::uint16_t>(rows[index],0);
                output[index].name=output[index].pivot_name=base_+0x6a65940;
            }
            size=count;return true;
        } __except(EXCEPTION_EXECUTE_HANDLER){size=0;return false;}
    }
    bool selected_pair_locked(const Frame& frame,void* component) const noexcept {
        __try {
            auto* root=field<void*>(reinterpret_cast<void*>(base_),0x6d69190);
            auto* manager=root ? field<void*>(root,0x30) : nullptr;
            if(!manager||field<Address>(manager,0)!=base_+0x55b62c8||field<void*>(root,0x58)!=frame.receiver)return false;
            auto* selected=field<void*>(manager,0x50);
            auto* possessor=field<void*>(manager,0x58);
            if(!selected||!possessor||field<Address>(selected,0)!=base_+0x5585c40||
                !field<std::uint8_t>(selected,0x96)||field<void*>(selected,0xa0)!=possessor||
                field<void*>(possessor,0xd0)!=selected||field<void*>(possessor,0xd8)!=frame.player||
                field<std::uint32_t>(selected,0x60)!=frame.identity)return false;
            auto* selected_metadata=field<void*>(selected,0x88);
            if(!selected_metadata||field<std::uint8_t>(selected_metadata,1)!=1)return false;
            auto* actor=frame.player;
            if(!actor||field<Address>(actor,0)!=base_+0x55857b0||
                field<std::uint32_t>(actor,0x60)!=frame.identity)return false;
            auto* metadata=field<void*>(actor,0x88);
            auto* components=field<void*>(actor,0x68);
            if(!metadata||field<std::uint8_t>(metadata,1)!=1||field<void*>(actor,0xa0)!=possessor||
                !components||field<std::uint32_t>(actor,0x70)<20||field<void*>(components,0x98)!=component)return false;
            return component&&field<Address>(component,0)==base_+0x55bdb30&&field<void*>(component,8)==actor&&
                field<std::uint8_t>(component,0x2a)==0&&frame.receiver&&
                !field<std::uint8_t>(frame.receiver,0x2c)&&!field<std::uint32_t>(frame.receiver,0xd8)&&
                !field<std::uint8_t>(frame.receiver,0xdc);
        } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    bool player_locked(const Frame& frame,void* component) const noexcept {
        __try {
            auto** tls=reinterpret_cast<unsigned char**>(__readgsqword(0x58));
            if(!tls||!tls[0]||tls[0][0x1ec]!=0||!field<void*>(tls[0],0x250))return false;
            return selected_pair_locked(frame,component);
        } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    bool ground_locked(void* target,Snapshot& snapshot) const noexcept {
        __try {
            if(!target)return false;
            auto* metadata=field<void*>(target,0x88);
            auto* components=field<void*>(target,0x68);
            if(!metadata||field<std::uint8_t>(metadata,1)!=7||!components||field<std::uint32_t>(target,0x70)<7)return false;
            auto* gimmick=field<void*>(components,0x30);
            auto* state=field<void*>(components,0x20);
            if(!gimmick||field<Address>(gimmick,0)!=base_+0x55b7898||field<void*>(gimmick,8)!=target||
                !state||field<std::uint8_t>(state,0x2d0)==0x1d)return false;
            auto* item=field<void*>(gimmick,0xc0);
            if(!item||item==field<void*>(reinterpret_cast<void*>(base_),0x6cfcd78)||
                field<std::uint16_t>(item,8)==0xffff||field<std::int64_t>(item,0x10)<=0)return false;
            snapshot={gimmick,field<std::uint32_t>(target,0x60),field<std::uint16_t>(gimmick,0x48)};
            return snapshot.target&&snapshot.definition!=0xffff;
        } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    bool corpse_locked(void* target,CorpseSnapshot& snapshot) const noexcept {
        __try {
            if(!target||field<Address>(target,0)!=base_+0x55857b0)return false;
            auto* metadata=field<void*>(target,0x88);auto* components=field<void*>(target,0x68);
            if(!metadata||field<std::uint8_t>(metadata,1)!=1||!components||field<std::uint32_t>(target,0x70)<26)return false;
            auto* state=field<void*>(components,0x20);auto* loot=field<void*>(components,0xc8);auto* interaction=field<void*>(components,0x98);
            // CheckDead224f16d returns condition status 1 for +273==0,
            // not a boolean "dead". AI CheckDead ead9178 and the ordinary
            // DeadBody LootBuff branch 2acd4de require nonzero +273.
            // The native
            // criminal-action predicate at state+2e8 has an owner at +18,
            // conditions at +8/+10 and an unconditional flag at +1c.
            if(!state||field<void*>(state,8)!=target||!field<std::uint8_t>(state,0x273)||
                field<std::uint32_t>(state,0x300)||field<std::uint32_t>(state,0x2f8)||field<std::uint8_t>(state,0x304)||
                !loot||field<Address>(loot,0)!=base_+0x55be5d8||field<void*>(loot,8)!=target||
                !interaction||field<Address>(interaction,0)!=base_+0x55bdb30||field<void*>(interaction,8)!=target)return false;
            snapshot={loot,interaction,state,field<std::uint32_t>(target,0x60),field<std::uint16_t>(state,0x30)};
            return snapshot.target&&snapshot.definition!=0xffff;
        } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    std::uint16_t tribe_handle(const unsigned char* character) const noexcept {
        __try {return character?field<std::uint16_t>(character,0xa4):0xffff;}
        __except(EXCEPTION_EXECUTE_HANDLER){return 0xffff;}
    }
    bool crime_exempt_tribe(const unsigned char* tribe) const noexcept {
        __try {return tribe&&!tribe[0x10]&&tribe[0x50]==7;}
        __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    class OwnedInteraction {
        ClearInteractions clear_;InteractionVector view_;std::atomic<bool>& failed_;int exceptions_;bool closed_{};
    public:
        Interaction value{};
        OwnedInteraction(Address base,ClearInteractions clear,std::atomic<bool>& failed)
            :clear_(clear),view_{&value,1,1},failed_(failed),exceptions_(std::uncaught_exceptions()){value.name=value.pivot_name=base+0x6a65940;}
        void close(){
            closed_=true;
            try{clear_(&view_);}catch(...){failed_.store(true,std::memory_order_release);throw;}
        }
        ~OwnedInteraction() noexcept(false){
            if(closed_)return;
            if(std::uncaught_exceptions()>exceptions_){
                // Preserve the primary native exception. Only a secondary
                // cleanup exception is contained, after latching automation
                // off permanently; this record is never cleaned a second time.
                try{close();}catch(...){}
            }else close();
        }
        OwnedInteraction(const OwnedInteraction&)=delete;OwnedInteraction& operator=(const OwnedInteraction&)=delete;
    };
    bool corpse_interactions(void* target,std::array<Interaction,32>& output,std::uint32_t& size) const {
        const auto* frame=current_frame;CorpseSnapshot snapshot{};
        if(!frame||!corpse(*frame,target,snapshot))return false;
        for(const auto key:kCorpseInteractions){
            OwnedInteraction owned(base_,bindings_.clear,cleanup_failed_);
            // 2124060 owns its temporary native list, locks the target and
            // resolves the actual current record by external key, including
            // overrides. Only a plain value survives this owned record.
            const bool found=bindings_.find(snapshot.interaction_component,true,&key,&owned.value);
            if(found&&plain_interaction(&owned.value)){
                const auto* definition=bindings_.interaction(&owned.value.handle);
                if(free_corpse_interaction(definition)&&external_key(definition)==key)output[size++]=owned.value;
            }
            owned.close();
        }
        return size!=0;
    }
public:
    Native(Address base,Bindings bindings):base_(base),bindings_(bindings){}
    Native(const Native& other):base_(other.base_),bindings_(other.bindings_),cleanup_failed_(!other.healthy()){}
    Native& operator=(const Native& other){base_=other.base_;bindings_=other.bindings_;cleanup_failed_.store(!other.healthy());return *this;}
    Address base() const noexcept{return base_;}
    bool healthy() const noexcept{return !cleanup_failed_.load(std::memory_order_acquire);}
    const Bindings& bindings() const noexcept{return bindings_;}
    // Read-only object-graph proof; it does not grant a native execution context.
    bool selected_pair(const Frame& frame,void* component) const {
        ActorGuard guard(bindings_.lock,frame.player);return guard&&selected_pair_locked(frame,component);
    }
    void* manager() const noexcept {
        __try {auto* root=field<void*>(reinterpret_cast<void*>(base_),0x6d69190);return root ? field<void*>(root,0x30) : nullptr;}
        __except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
    }
    void* receiver() const noexcept {
        __try {auto* root=field<void*>(reinterpret_cast<void*>(base_),0x6d69190);return root ? field<void*>(root,0x58) : nullptr;}
        __except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
    }
    class Lease {
        Native& owner_;ActorLease value_{};bool constructed_{};
    public:
        Lease(Native& owner,std::uint32_t id):owner_(owner) {
            if(auto* source=owner.manager())constructed_=owner.bindings_.resolve(source,&value_,id)==&value_;
        }
        ~Lease(){if(constructed_)owner_.bindings_.release(&value_);}
        void* get() const noexcept{return constructed_&&value_.active ? value_.actor : nullptr;}
        Lease(const Lease&)=delete;Lease& operator=(const Lease&)=delete;
    };
    bool nearby(void* field_object,std::array<std::uint32_t,512>& ids,std::size_t& count) const {
        count=0;ActorGuard guard(bindings_.lock,field_object);
        return guard&&nearby_locked(field_object,ids,count);
    }
    bool interactions(void* target,std::array<Interaction,32>& output,std::uint32_t& size) const {
        size=0;
        if(!healthy())return false;
        {
            ActorGuard guard(bindings_.lock,target);Snapshot snapshot{};
            if(!guard)return false;
            if(ground_locked(target,snapshot)){
                const auto* definition=bindings_.gimmick(&snapshot.definition);
                return free_gimmick(definition)&&base_records_locked(target,definition,output,size);
            }
        }
        return corpse_interactions(target,output,size);
    }
    bool record_current(void* target,const Interaction& wanted) const {
        std::array<Interaction,32> records{};std::uint32_t size{};
        if(!interactions(target,records,size)||!plain_interaction(&wanted))return false;
        for(std::uint32_t index=0;index<size;++index)
            if(records[index].handle==wanted.handle&&plain_interaction(&records[index]))return true;
        return false;
    }
    std::uint32_t identity(void* actor) const noexcept {
        __try {return actor ? field<std::uint32_t>(actor,0x60) : 0;}
        __except(EXCEPTION_EXECUTE_HANDLER){return 0;}
    }
    void* component(void* actor) const noexcept {
        __try {
            if(!actor||field<std::uint32_t>(actor,0x70)<20)return nullptr;
            auto* components=field<void*>(actor,0x68);return components ? field<void*>(components,0x98) : nullptr;
        }
        __except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
    }
    bool player(const Frame& frame,void* component) const {
        ActorGuard guard(bindings_.lock,frame.player);
        return guard&&player_locked(frame,component);
    }
    bool ground(const Frame&,void* target,Snapshot& snapshot) const {
        ActorGuard guard(bindings_.lock,target);
        return guard&&ground_locked(target,snapshot);
    }
    bool corpse(const Frame& frame,void* target,CorpseSnapshot& snapshot) const {
        if(!healthy()||target==frame.player||!bindings_.find||!bindings_.clear||!bindings_.can_loot||!bindings_.character||!bindings_.tribe||!bindings_.relationship)return false;
        {
            ActorGuard guard(bindings_.lock,target);if(!guard||!corpse_locked(target,snapshot))return false;
        }
        const auto* character=bindings_.character(&snapshot.definition);const auto tribe=tribe_handle(character);
        if(tribe==0xffff)return false;
        const auto* tribe_definition=bindings_.tribe(&tribe);
        // Both are direct, independent refusals in native25d8570. Neither
        // relies on a temporary BlockCrime buff, wanted timer or detection.
        if(!crime_exempt_tribe(tribe_definition)&&bindings_.relationship(frame.player,target,true)!=0)return false;
        ActorGuard guard(bindings_.lock,target);CorpseSnapshot fresh{};
        return guard&&corpse_locked(target,fresh)&&fresh==snapshot;
    }
    bool plain_interaction(const Interaction* record) const noexcept {
        __try {
            // Default base records constructed at 2123813..21238b7. Reject
            // serialized names, custom pivots and non-default option payloads.
            return record&&record->name==base_+0x6a65940&&!record->serialized_name&&record->handle!=0xffff&&
                record->pivot==0xffffffff&&record->pivot_name==base_+0x6a65940&&!record->pivot_value&&
                record->option==0xffffffff&&record->state==2&&!record->flag;
        } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    bool free_interaction(const unsigned char* definition) const noexcept {
        __try {
            if(!definition)return false;
            const auto key=field<std::uint32_t>(definition,0);
            const bool pickup=(key==kFreeGroundInteraction&&definition[0x11]==0x0b)||
                (key==kAnimatedGroundInteraction&&definition[0x11]==0);
            // Buy and thief keys are separate native routes. The normal key
            // retains its native animation and PickUpItem frame events.
            return pickup&&!definition[0x10]&&field<std::uint32_t>(definition,0x40)==1&&
                !field<std::uint32_t>(definition,0x78)&&field<std::uint32_t>(definition,0x80)==0xffffffff&&
                !field<std::uint32_t>(definition,0xa0)&&!definition[0xc1];
        } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    bool free_corpse_interaction(const unsigned char* definition) const noexcept {
        __try {
            if(!definition)return false;
            const auto key=field<std::uint32_t>(definition,0);
            const bool ordinary=key==kCorpseInteractions[0]||key==kCorpseInteractions[1]||key==kCorpseInteractions[2];
            return ordinary&&!definition[0x10]&&!definition[0x11]&&field<std::uint32_t>(definition,0x40)==1&&
                !field<std::uint32_t>(definition,0x78)&&field<std::uint32_t>(definition,0x80)==0xffffffff&&
                !field<std::uint32_t>(definition,0xa0)&&!definition[0xc1];
        } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    std::uint32_t external_key(const unsigned char* definition) const noexcept {
        __try {return definition?field<std::uint32_t>(definition,0):0xffffffff;}
        __except(EXCEPTION_EXECUTE_HANDLER){return 0xffffffff;}
    }
    bool free_gimmick(const unsigned char* definition) const noexcept {
        __try {
            return definition&&!definition[0x18]&&!field<void*>(definition,0x288)&&!definition[0x66d]&&
                !definition[0x3c9]&&!definition[0x3ca];
        } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
};
} // namespace crimson::marketplace::auto_loot
