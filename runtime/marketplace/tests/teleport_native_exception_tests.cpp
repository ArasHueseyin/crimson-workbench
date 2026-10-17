#include "../teleport/teleport_policy.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

using namespace crimson::marketplace::teleport;
namespace {
unsigned checks{};
void check(bool value,const char* label){++checks;if(!value){std::fprintf(stderr,"FAILED: %s\n",label);std::abort();}}
enum Failure:unsigned {acquire_failure=1,stage_failure=2,position_failure=4,yaw_failure=8,validate_failure=16,
    begin_failure=32,begin_seh=64,release_failure=128,reference_failure=256,read_lock_failure=512,unlock_failure=1024};
struct NativeException:std::runtime_error {
    Failure source;
    explicit NativeException(Failure at):std::runtime_error("owned native exception"),source(at){}
};
struct Fixture {
    static inline Fixture* current{};
    std::array<std::uint8_t,0x700> actor{},transform{},field{},character{};
    std::array<std::uint8_t,0x200> components{},possessor{};
    std::array<std::uint8_t,2> metadata{0,1};
    std::array<std::uintptr_t,0x150/8> table{};std::array<std::uintptr_t,5> lock_table{};
    std::uintptr_t* lock_object{};void* actor_pointer{};int task_error{};
    NativeBindings bindings{};crimson::live_items::TaskObservationV1 task{};
    unsigned failures{},acquires{},releases{},references{},read_locks{},unlocks{},begins{};
    template<class T>static void put(void* object,std::size_t offset,const T& value){std::memcpy(static_cast<std::uint8_t*>(object)+offset,&value,sizeof(value));}
    static void fail(Failure point){if(current->failures&point)throw NativeException(point);}
    static FieldLease* __fastcall acquire(void*,FieldLease* out){
        auto& f=*current;++f.acquires;*out={0,f.field.data(),1,0,{},0,nullptr,f.transform.data()};
        put(f.transform.data(),0x54a,std::int16_t{1});fail(acquire_failure);return out;
    }
    static void __fastcall release(void*){auto& f=*current;++f.releases;fail(release_failure);put(f.transform.data(),0x54a,std::int16_t{0});}
    static void __fastcall reference(FieldLease*){++current->references;fail(reference_failure);}
    static std::uint16_t* __fastcall stage(void*,std::uint16_t* out){fail(stage_failure);*out=17;return out;}
    static const std::byte* __fastcall stage_info(const std::uint16_t*){static const std::uint32_t value=555;return reinterpret_cast<const std::byte*>(&value);}
    static CompressedWorldPosition* __fastcall position(void*,CompressedWorldPosition* out){fail(position_failure);*out={10,20,30,0,0};return out;}
    static float __fastcall yaw(void*){fail(yaw_failure);return 0.25f;}
    static void** __fastcall read_lock(void** out,void* node){++current->read_locks;*out=node;fail(read_lock_failure);return out;}
    static void __fastcall unlock(void*,bool read){check(read,"native snapshot cleanup keeps read-lock ABI");++current->unlocks;fail(unlock_failure);}
    static int* __fastcall validate(void*,int* error){fail(validate_failure);*error=0;return error;}
    static int* __fastcall begin(void* context,int* error,const NativeFieldMoveValue* value){
        ++current->begins;put(context,0x60,*value);static_cast<std::uint8_t*>(context)[0x58]=1;
        fail(begin_failure);if(current->failures&begin_seh)RaiseException(0xe0123456,0,0,nullptr);
        *error=0;return error;
    }
    static std::uint8_t realm(){return 1;}
    static std::uint32_t selected(std::uintptr_t){return 123;}
    Fixture(){
        current=this;actor_pointer=actor.data();lock_table[4]=reinterpret_cast<std::uintptr_t>(&unlock);lock_object=lock_table.data();
        table[0x140/8]=reinterpret_cast<std::uintptr_t>(&acquire);table[0x148/8]=reinterpret_cast<std::uintptr_t>(&release);
        bindings={true,0x100000,reinterpret_cast<std::uintptr_t>(actor.data()),reinterpret_cast<std::uintptr_t>(table.data()),reinterpret_cast<std::uintptr_t>(field.data()),
            acquire,release,reference,stage,stage_info,position,yaw,read_lock,validate,begin,realm,selected};
        put(actor.data(),0,bindings.actor_type);put(actor.data(),8,&lock_object);put(actor.data(),0x60,std::uint32_t{123});
        put(actor.data(),0x68,components.data());put(actor.data(),0x70,std::uint32_t{53});put(actor.data(),0x88,metadata.data());actor[0x96]=1;put(actor.data(),0xa0,possessor.data());
        put(possessor.data(),0xd0,actor.data());put(possessor.data(),0x60,std::uint32_t{789});
        put(components.data(),0x1a0,transform.data());put(components.data(),0x20,character.data());
        put(transform.data(),0,bindings.transform_type);put(transform.data(),8,actor.data());put(transform.data(),0x28,std::uint16_t{17});
        put(character.data(),8,actor.data());put(character.data(),0x30,std::uint16_t{33});
        put(field.data(),0,bindings.field_type);put(field.data(),0x180,std::uint32_t{789});
        task={sizeof(task),crimson::live_items::kTaskObserverAbiV1,bindings.base,&actor_pointer,&task_error,bindings.base+0x2774a0d,1000,123,0};
    }
    ~Fixture(){current=nullptr;}
};
void pure_getter_exceptions_preserve_known_lease(){
    for(const auto failure:{stage_failure,validate_failure,position_failure,yaw_failure}){
        Fixture f;f.failures=failure;NativeFrame frame{};bool propagated=false;
        try {open_frame(f.bindings,f.task,frame);}catch(const NativeException& e){propagated=e.source==failure;}
        check(propagated,"native getter C++ exception preserves its type/payload");
        check(frame.acquired&&!frame.faulted&&!frame.attempted,"completed pre-mutation field lease remains safely closable");
        check(close_frame(f.bindings,frame)&&!frame.acquired,"known pre-mutation lease closes once on caller unwind");
        check(f.acquires==1&&f.releases==1&&f.references==1&&!f.begins&&f.read_locks==f.unlocks,"getter failure leaves no field/read borrow behind");
    }
}
void mutation_exceptions_never_guess_cleanup(){
    for(const auto failure:{begin_failure,begin_seh}){
        Fixture f;NativeFrame frame{};check(open_frame(f.bindings,f.task,frame)==NativeCode::ready,"baseline frame opens");
        f.failures=failure;bool propagated=false;NativeCode code{};
        try {code=begin_saved_move(f.bindings,frame,frame.current);}catch(const NativeException& e){propagated=e.source==begin_failure;}
        check(failure==begin_failure?propagated:!propagated&&code==NativeCode::native_fault,"C++ Begin escapes; non-C++ SEH becomes native fault");
        check(frame.attempted&&frame.faulted&&!frame.ready,"uncertain native mutation is latched before returning/unwinding");
        check(!close_frame(f.bindings,frame)&&!f.releases&&!f.references,"uncertain mutation performs no speculative cleanup");
        check(begin_saved_move(f.bindings,frame,frame.current)==NativeCode::position&&f.begins==1,"uncertain Begin is never replayed");
    }
}
void cleanup_exceptions_do_not_become_success(){
    for(const auto failure:{release_failure,reference_failure}){
        Fixture f;NativeFrame frame{};check(open_frame(f.bindings,f.task,frame)==NativeCode::ready,"cleanup baseline frame opens");
        f.failures=failure;bool propagated=false;
        try {close_frame(f.bindings,frame);}catch(const NativeException& e){propagated=e.source==failure;}
        check(propagated&&frame.faulted&&frame.acquired,"native cleanup C++ failure propagates without a false closed frame");
        const auto releases=f.releases,references=f.references;
        check(!close_frame(f.bindings,frame)&&f.releases==releases&&f.references==references,"uncertain native cleanup is not repeated");
    }
}
void partial_acquire_and_read_lock_are_uncertain(){
    for(const auto failure:{acquire_failure,read_lock_failure,unlock_failure}){
        Fixture f;f.failures=failure;NativeFrame frame{};bool propagated=false;
        try {open_frame(f.bindings,f.task,frame);}catch(const NativeException& e){propagated=e.source==failure;}
        check(propagated&&frame.faulted,"partial acquisition/read cleanup preserves native C++ exception and uncertainty");
        check(!close_frame(f.bindings,frame)&&!f.releases&&!f.references,"unknown partial native acquisition/cleanup is not guessed");
    }
}
void policy_unwind_releases_known_lease_and_preserves_primary(){
    for(const bool secondary_cleanup_failure:{false,true}){
        Fixture f;TeleportPolicy policy(1234);f.failures=stage_failure|(secondary_cleanup_failure?release_failure:0);
        bool propagated=false;try {policy.observe(f.bindings,f.task);}catch(const NativeException& e){propagated=e.source==stage_failure;}
        check(propagated,"policy unwind preserves primary native getter C++ exception");
        check(f.acquires==1&&f.releases==1&&f.references==(secondary_cleanup_failure?0U:1U),"policy guard closes only the known pre-Begin lease once");
        const auto reply=policy.handle({},1001);check(reply.state==State::faulted,"escaped or secondary cleanup exception latches policy fault");
        f.failures=0;policy.observe(f.bindings,f.task);check(f.acquires==1,"faulted policy makes no more native calls");
    }
}
void policy_mutation_and_normal_cleanup_faults_latch_once(){
    {
        Fixture f;TeleportPolicy policy(1234);policy.observe(f.bindings,f.task);
        const auto state=policy.handle({},1001);Request save{};save.session=state.session;save.command=1;save.epoch=state.epoch;
        save.not_after_ms=4000;save.operation=Operation::save_current;std::memcpy(save.name.data(),"Owned",6);
        check(policy.handle(save,1002).state==State::pending,"policy capture command is accepted");
        f.task.monotonic_ms=1003;policy.observe(f.bindings,f.task);
        Request travel=save;travel.command=2;travel.operation=Operation::travel_saved;travel.name={};
        check(policy.handle(travel,1004).state==State::pending,"policy travel command is accepted");
        const auto releases=f.releases,references=f.references;f.failures=begin_failure;f.task.monotonic_ms=1005;
        bool propagated=false;try{policy.observe(f.bindings,f.task);}catch(const NativeException& e){propagated=e.source==begin_failure;}
        check(propagated&&f.begins==1&&f.releases==releases&&f.references==references,"policy preserves Begin exception without speculative field release");
        check(policy.handle(travel,1006).state==State::faulted,"duplicate receipt reports the latched mutation fault");
        f.failures=0;policy.observe(f.bindings,f.task);check(f.begins==1,"policy never replays an escaped Begin");
    }
    for(const auto failure:{release_failure,reference_failure}){
        Fixture f;TeleportPolicy policy(1234);f.failures=failure;bool propagated=false;
        try{policy.observe(f.bindings,f.task);}catch(const NativeException& e){propagated=e.source==failure;}
        check(propagated&&policy.handle({},1001).state==State::faulted,"normal policy cleanup failure propagates and cannot report success");
        const auto releases=f.releases,references=f.references;f.failures=0;policy.observe(f.bindings,f.task);
        check(f.releases==releases&&f.references==references,"finished scope does not retry failed native cleanup");
    }
}
}
int main(){
    pure_getter_exceptions_preserve_known_lease();mutation_exceptions_never_guess_cleanup();cleanup_exceptions_do_not_become_success();
    partial_acquire_and_read_lock_are_uncertain();policy_unwind_releases_known_lease_and_preserves_primary();policy_mutation_and_normal_cleanup_faults_latch_once();
    std::printf("Teleport native exception transparency/lifecycle: %u checks passed; owned mocks only.\n",checks);
}
