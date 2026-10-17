#include "../teleport/position.h"
#include "../teleport/teleport_protocol.h"
#include "../teleport/persistence_revision.h"
#ifdef _WIN32
#include "../teleport/teleport_policy.h"
#endif
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

using namespace crimson::marketplace::teleport;
namespace {unsigned checks{};
void check(bool value,const char* message) {
    ++checks;if(!value){std::fprintf(stderr,"FAILED: %s\n",message);std::abort();}
}
#ifdef _WIN32
struct NativeFixture {
    std::array<std::uint8_t,0x700> actor{},transform{},field{},character{};
    std::array<std::uint8_t,0x200> components{},possessor{},misc{},navigation{};
    std::array<NativePinMarker,1> pins{},waypoints{};
    std::array<std::uint8_t,2> metadata{0,1};
    std::array<std::uintptr_t,0x150/8> table{};
    std::array<std::uintptr_t,5> lock_table{};
    std::uintptr_t* lock_object{};
    NativeBindings bindings{};MarkerBindings marker_bindings{};void* actor_pointer{};int task_error{};
    NativeFieldMoveValue last_begin{};
    std::uint32_t definition{555};float marker_height{1500};std::array<std::byte,0xf0> marker_filter{};
    crimson::live_items::TaskObservationV1 task{};
    std::uint32_t selected{123};unsigned acquired{},released{},cleaned{},locks{},unlocks{},begins{};
    bool fail_read{},fail_begin{},bad_publication{},geometry_missing{};int begin_error{};
    static inline NativeFixture* current{};
    template<class T>static void put(void* value,std::size_t at,const T& data){std::memcpy(static_cast<std::uint8_t*>(value)+at,&data,sizeof(data));}
    static FieldLease* __fastcall acquire(void*,FieldLease* lease){auto& f=*current;++f.acquired;*lease={0,f.field.data(),1,0,{},0,nullptr,f.transform.data()};put(f.transform.data(),0x54a,std::int16_t{1});return lease;}
    static void __fastcall release(void*){auto& f=*current;++f.released;put(f.transform.data(),0x54a,std::int16_t{0});f.transform[0x548]=0;put(f.transform.data(),0x550,NativeFieldMoveValue{});}
    static void __fastcall clean(FieldLease*){++current->cleaned;}
    static std::uint16_t* __fastcall stage(void*,std::uint16_t* out){*out=17;return out;}
    static const std::byte* __fastcall info(const std::uint16_t*){return reinterpret_cast<const std::byte*>(&current->definition);}
    static CompressedWorldPosition* __fastcall position(void* value,CompressedWorldPosition* out){std::memcpy(out,static_cast<std::uint8_t*>(value)+0xb4,sizeof(*out));return out;}
    static float __fastcall yaw(void*){return 0.25f;}
    static void** __fastcall read_lock(void** guard,void* lock){++current->locks;*guard=lock;return guard;}
    static void __fastcall unlock(void*,bool read){check(read,"snapshot lock released as read lock");++current->unlocks;}
    static int* __fastcall validate(void*,int* error){*error=0;return error;}
    static int* __fastcall begin(void* context,int* error,const NativeFieldMoveValue* value){
        auto& f=*current;++f.begins;
        if(f.fail_begin)RaiseException(0xe0123456,0,0,nullptr);
        *error=f.begin_error;if(*error)return error;
        f.last_begin=*value;put(context,0x60,*value);static_cast<std::uint8_t*>(context)[0x58]=1;
        if(f.bad_publication)static_cast<std::uint8_t*>(context)[0x68]^=1;
        return error;
    }
    static std::uint8_t realm(){return 1;}
    static std::uint32_t selected_identity(std::uintptr_t){return current->selected;}
    static NavigationContext* __fastcall marker_context(NavigationContext* value,void*,void*,std::uint32_t mode,std::uint16_t,float){*value={};marker_store(value->bytes.data()+0x10c,mode);marker_store(value->bytes.data()+0x12c,std::uint8_t{1});return value;}
    static void* __fastcall marker_copy(void* out,const void*){std::memset(out,0,0xf0);return out;}
    static void __fastcall marker_clean(void*){}
    static void marker_free(std::uintptr_t,void*){check(false,"fixture ray must not allocate native outputs");}
    static bool __fastcall marker_ray(NavigationContext*,GroundRayQuery* q,GroundRayResult* out){out->distance=marker_load<WorldPosition>(q->bytes.data()).y-25.0f;return !current->geometry_missing;}
    static PlacementQuery* __fastcall marker_query(PlacementQuery* out,const WorldPosition* p){*out={};marker_store(out->bytes.data(),*p);return out;}
    static bool __fastcall marker_placement(NavigationContext*,PlacementQuery* query,PlacementResult* out,void*){const auto p=marker_load<WorldPosition>(query->bytes.data());marker_store(out->bytes.data(),CompressedWorldPosition{p.x,p.y,p.z,0,0});return true;}
    static bool __fastcall marker_identity(const void*){return true;}
    static CompressedWorldPosition* __fastcall marker_compose(const void*,CompressedWorldPosition*,const CompressedWorldPosition*){check(false,"fixture parent transform must be identity");return nullptr;}
    NativeFixture(){
        current=this;actor_pointer=actor.data();lock_table[4]=reinterpret_cast<std::uintptr_t>(&unlock);lock_object=lock_table.data();
        table[0x140/8]=reinterpret_cast<std::uintptr_t>(&acquire);table[0x148/8]=reinterpret_cast<std::uintptr_t>(&release);
        bindings={true,0x100000,reinterpret_cast<std::uintptr_t>(actor.data()),reinterpret_cast<std::uintptr_t>(table.data()),reinterpret_cast<std::uintptr_t>(field.data()),
            acquire,release,clean,stage,info,position,yaw,read_lock,validate,begin,realm,selected_identity};
        put(actor.data(),0,bindings.actor_type);put(actor.data(),8,&lock_object);put(actor.data(),0x60,selected);put(actor.data(),0x68,components.data());put(actor.data(),0x70,std::uint32_t{53});put(actor.data(),0x88,metadata.data());actor[0x96]=1;put(actor.data(),0xa0,possessor.data());
        put(possessor.data(),0xd0,actor.data());put(possessor.data(),0x60,std::uint32_t{789});
        put(components.data(),0x1a0,transform.data());put(components.data(),0x20,character.data());
        put(transform.data(),0,bindings.transform_type);put(transform.data(),8,actor.data());put(transform.data(),0x28,std::uint16_t{17});put(transform.data(),0xb4,CompressedWorldPosition{10,20,30,0,0});
        put(character.data(),8,actor.data());put(character.data(),0x30,std::uint16_t{33});
        put(field.data(),0,bindings.field_type);put(field.data(),0x180,std::uint32_t{789});
        task={sizeof(task),crimson::live_items::kTaskObserverAbiV1,bindings.base,&actor_pointer,&task_error,bindings.base+0x2774a0d,1000,selected,0};
    }
    void enable_markers(){
        definition=1;put(actor.data(),0x70,std::uint32_t{54});put(components.data(),0x168,misc.data());put(components.data(),0x1a8,navigation.data());
        marker_bindings.verified=true;marker_bindings.base=bindings.base;marker_bindings.misc_type=reinterpret_cast<std::uintptr_t>(misc.data());marker_bindings.navigation_type=reinterpret_cast<std::uintptr_t>(navigation.data());
        marker_bindings.layer_height=&marker_height;marker_bindings.default_filter=marker_filter.data();marker_bindings.ground_filter=marker_filter.data();
        marker_bindings.context=marker_context;marker_bindings.copy_filter=marker_copy;marker_bindings.destroy_filter=marker_clean;marker_bindings.ray=marker_ray;
        marker_bindings.placement_query=marker_query;marker_bindings.placement=marker_placement;marker_bindings.release_reference=marker_clean;marker_bindings.release_locked_reference=marker_clean;
        marker_bindings.free_allocation=marker_free;marker_bindings.identity_srt=marker_identity;marker_bindings.compose_position=marker_compose;
        put(misc.data(),0,marker_bindings.misc_type);put(misc.data(),8,actor.data());put(misc.data(),0xc8,pins.data());put(misc.data(),0xd0,std::uint32_t{1});put(misc.data(),0xd4,std::uint32_t{1});
        put(misc.data(),0xd8,waypoints.data());put(misc.data(),0xe0,std::uint32_t{1});put(misc.data(),0xe4,std::uint32_t{1});
        put(navigation.data(),0,marker_bindings.navigation_type);put(navigation.data(),8,actor.data());put(navigation.data(),0x28,navigation.data());
        pins[0].id=0;pins[0].position={500,0,600};waypoints[0].id=0;waypoints[0].position={700,0,800};
    }
    void observe(TeleportPolicy& policy,std::uint64_t at){task.monotonic_ms=at;policy.observe(bindings,task,marker_bindings.verified?&marker_bindings:nullptr);}
    Request request(const Reply& reply,std::uint64_t id,Operation operation,std::uint32_t slot=0){Request r{};r.session=reply.session;r.command=id;r.epoch=reply.epoch;r.not_after_ms=reply.observed_ms+2500;r.operation=operation;r.slot=slot;return r;}
};
void native_policy_checks(){
    NativeFixture f;TeleportPolicy policy(8001);f.observe(policy,1000);
    auto reply=policy.handle({},1001);check(reply.state==State::ready&&reply.current.position==WorldPosition{10,20,30},"leased task captures a global position");
    check(f.acquired==1&&f.released==1&&f.cleaned==1&&f.locks==f.unlocks,"field and read locks balanced");
    auto save=f.request(reply,1,Operation::save_current);std::memcpy(save.name.data(),"Home",5);
    check(policy.handle(save,1002).state==State::pending,"capture queues value-only request");f.observe(policy,1003);
    reply=policy.handle({},1004);check(reply.state==State::captured&&reply.bookmarks[0].occupied&&f.begins==0,"saving captures on the native thread without mutation");
    NativeFixture::put(f.transform.data(),0xb4,CompressedWorldPosition{9000,250,2000,0,0});f.observe(policy,1005);
    auto travel=f.request(reply,2,Operation::travel_saved);check(policy.handle(travel,1006).state==State::pending,"saved destination queues");f.observe(policy,1007);
    check(f.begins==1&&policy.handle({},1008).state==State::native_queued,"native acceptance does not claim arrival");
    for(unsigned i=0;i<3;++i){check(policy.handle(travel,1009+i).state==State::native_queued,"same command queries its receipt");f.observe(policy,1010+i);}
    check(f.begins==1,"lost/repeated replies never resubmit native travel");
    NativeFixture::put(f.transform.data(),0xb4,CompressedWorldPosition{10,20,30,0,0});f.observe(policy,1020);
    reply=policy.handle({},1021);check(reply.state==State::position_observed,"later independent position observation has separate receipt");
    auto marker=f.request(reply,3,Operation::travel_marker);marker.marker=0x8000000100000002;
    check(policy.handle(marker,1022).reason==Reason::marker_height_or_world_unproven&&f.begins==1,"unproven marker height/world fails closed");
    auto stale=f.request(reply,3,Operation::travel_saved);check(policy.handle(stale,8000).reason==Reason::stale_context,"stale task context cannot queue movement");
    f.observe(policy,8001);reply=policy.handle({},8002);auto uncertain=f.request(reply,3,Operation::travel_saved);policy.handle(uncertain,8003);f.bad_publication=true;f.observe(policy,8004);
    check(policy.handle({},8005).state==State::uncertain&&f.begins==2,"native publication mismatch yields uncertainty once");
    policy.handle(uncertain,8006);f.observe(policy,8007);check(f.begins==2,"uncertain duplicate is not retried");
    NativeFixture fault;TeleportPolicy latched(9001);fault.observe(latched,1000);reply=latched.handle({},1001);save=fault.request(reply,1,Operation::save_current);std::memcpy(save.name.data(),"Home",5);latched.handle(save,1002);fault.observe(latched,1003);
    reply=latched.handle({},1004);travel=fault.request(reply,2,Operation::travel_saved);latched.handle(travel,1005);fault.fail_begin=true;fault.observe(latched,1006);
    check(latched.handle({},1007).state==State::faulted,"native exception latches module");
    const auto acquisitions=fault.acquired;fault.observe(latched,1008);check(fault.acquired==acquisitions&&fault.begins==1,"latched module makes no more engine calls");
    NativeFixture markers;markers.enable_markers();TeleportPolicy marker_policy(9101);markers.observe(marker_policy,1000);reply=marker_policy.handle({},1001);
    check(reply.marker_count==2&&reply.marker_status==MarkerCode::ready&&reply.markers_observed_ms==1000&&reply.markers[0].id==0&&reply.markers[1].source==MarkerSource::waypoint,"server-owned source/ID snapshot crosses the pointer-free policy");
    auto waypoint=markers.request(reply,1,Operation::travel_marker);waypoint.marker_source=1;
    check(marker_policy.handle(waypoint,1002).state==State::pending&&markers.begins==0,"marker request queues without worker engine calls");markers.observe(marker_policy,1003);
    reply=marker_policy.handle({},1004);
    check(reply.state==State::native_queued&&markers.begins==1&&markers.last_begin.position==WorldPosition{700,25,800},"resolved native waypoint queues its source-specific grounded target once");
    NativeFixture::put(markers.transform.data(),0xb4,CompressedWorldPosition{700,25,800,0,0});markers.observe(marker_policy,1005);
    check(marker_policy.handle({},1006).state==State::position_observed,"separate observation confirms the chosen source, not equal-ID map pin");
    marker_policy.handle(waypoint,1007);markers.observe(marker_policy,1008);check(markers.begins==1,"duplicate marker command never replays native queue");
    markers.observe(marker_policy,1600);reply=marker_policy.handle({},1601);auto map_pin=markers.request(reply,2,Operation::travel_marker);
    marker_policy.handle(map_pin,1602);markers.geometry_missing=true;markers.observe(marker_policy,1603);
    reply=marker_policy.handle({},1604);check(reply.state==State::rejected&&reply.reason==Reason::marker_geometry&&markers.begins==1,"missing target geometry rejects before native movement");
    check(reply.marker_count==0&&!reply.markers_observed_ms,"failed marker context does not publish a fresh list");
    markers.geometry_missing=false;markers.observe(marker_policy,2200);reply=marker_policy.handle({},2201);check(reply.marker_count==2,"later native capture can refresh the list");
    reply=marker_policy.handle({},6000);check(!reply.marker_count&&!reply.markers_observed_ms&&reply.marker_status==MarkerCode::context,"worker polling expires marker snapshot without native calls");
    check(markers.acquired==markers.released&&markers.released==markers.cleaned&&markers.locks==markers.unlocks,"marker policy releases native leases and read locks normally");
}
#endif
}
int main() {
    const auto decoded=decode_world_position({17.5f,72.25f,-22.75f,-3,4});
    check(decoded&&*decoded==WorldPosition{-2982.5f,72.25f,3977.25f},"signed sectors and global axes");
    const auto limits=decode_world_position({0,0,0,std::numeric_limits<std::int16_t>::min(),std::numeric_limits<std::int16_t>::max()});
    check(limits&&limits->x==-32768000.0f&&limits->z==32767000.0f,"signed 16-bit sector endpoints");
    for(const auto invalid:{std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        check(!decode_world_position({invalid,0,0,0,0}),"invalid X rejected");
        check(!decode_world_position({0,invalid,0,0,0}),"invalid Y rejected");
        check(!decode_world_position({0,0,invalid,0,0}),"invalid Z rejected");
    }
    SavedPosition saved{1234,17,0,{17.5f,72.25f,-22.75f},0.25f,1,0};
    check(valid_saved_position(saved),"valid stable-key persistence payload");
    auto other=saved;other.field_definition=0;check(!valid_saved_position(other),"missing field rejected");
    other=saved;other.stage=0xffff;check(!valid_saved_position(other),"invalid stage rejected");
    other=saved;other.version=2;check(!valid_saved_position(other),"unknown schema rejected");
    other=saved;other.reserved=1;check(!valid_saved_position(other),"reserved field rejected");
    other=saved;other.reserved_tail=1;check(!valid_saved_position(other),"reserved tail rejected");
    other=saved;other.yaw=std::numeric_limits<float>::infinity();check(!valid_saved_position(other),"nonfinite yaw rejected");
    // Finite coordinates alone are deliberately not called safe, grounded or
    // teleport-ready. Native field bounds/height eligibility is separate.
    other=saved;other.position.y=-900000.0f;check(valid_saved_position(other),"schema validation is not a ground-validity claim");
    Request request{};check(valid_request(request),"exact status request accepted");
    request.bytes=0;check(!valid_request(request),"truncated protocol rejected");
    request={};request.operation=Operation::save_current;request.command=1;request.epoch=1;request.not_after_ms=1000;std::memcpy(request.name.data(),"Camp",5);check(valid_request(request),"named slot capture request has no coordinates");
    request.name[47]='x';check(!valid_request(request),"nonzero bytes after name terminator rejected");
    request={};request.operation=Operation::travel_marker;request.command=1;request.epoch=1;request.not_after_ms=1000;
    check(valid_request(request),"native map pin ID zero is valid");
    request.marker_source=1;check(valid_request(request),"waypoint ID zero preserves its separate source");
    request.marker_source=2;check(!valid_request(request),"unknown marker sources are rejected");
    request.marker_source=0;request.marker=UINT64_MAX;check(!valid_request(request),"native missing-ID sentinel is rejected");
    auto name=[](std::initializer_list<unsigned char> bytes){std::array<char,48> value{};std::size_t at=0;for(auto byte:bytes)value[at++]=static_cast<char>(byte);return value;};
    check(valid_name(name({'C','a','f',0xc3,0xa9,' ',0xf0,0x9f,0x8f,0x95})),"UTF-8 accented and supplementary characters round trip");
    check(!valid_name(name({0xc0,0x80})),"overlong UTF-8 is rejected");
    check(!valid_name(name({0xc2,0x85})),"Unicode C1 controls are rejected");
    check(!valid_name(name({0xed,0xa0,0x80})),"UTF-8 surrogate is rejected");
    check(!valid_name(name({0xf4,0x90,0x80,0x80})),"out-of-range Unicode is rejected");
    check(!valid_name(name({0xe2,0x82})),"truncated UTF-8 is rejected");
    check(!valid_name(name({0xe2,0x28,0xa1})),"invalid UTF-8 continuation is rejected");
    PersistenceRevision persistence;unsigned writes{};
    check(persistence.persist(0,[&]{++writes;return true;})==1&&writes==0,"restored or empty state is not rewritten");
    check(persistence.persist(1,[&]{++writes;return true;})==1&&writes==1,"completed explicit capture persists once");
    for(unsigned i=0;i<20;++i)check(persistence.persist(1,[&]{++writes;return true;})==1&&writes==1,"unchanged status/travel/rejection does not invoke storage");
    check(persistence.persist(2,[&]{++writes;return false;})==2&&writes==2,"failed new revision exposes unpersisted state");
    for(unsigned i=0;i<20;++i)check(persistence.persist(2,[&]{++writes;return true;})==2&&writes==2,"failed persistence is not retried by polling");
    check(persistence.persist(3,[&]{++writes;return true;})==1&&writes==3,"new explicit edit allows one further persistence attempt");
#ifdef _WIN32
    native_policy_checks();
#endif
    std::printf("teleport position/persistence policy: %u checks passed\n",checks);
}
