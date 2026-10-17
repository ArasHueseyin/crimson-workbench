#include "../world-controls/time_units.h"
#include "../world-controls/field_time.h"
#include "../world-controls/clock_native.h"
#include "../world-controls/control_queue.h"
#include "../world-controls/world_hooks.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <limits>

namespace {
using namespace crimson::marketplace::world_controls;
// Independent minute-by-minute timeline: native night advances twice as fast
// in displayed clock units, without assuming a 24-hour raw day.
std::uint64_t minute_oracle(std::uint32_t minute) {
    std::uint64_t total=0;
    for (std::uint32_t i=0;i<minute;++i) total+=(i<180 || i>=1260)?30'000:60'000;
    return total;
}
struct ClockFixture {
    std::array<std::uint8_t,0x200> actor{},components{},possessor{};
    std::array<std::uint8_t,0x600> transform{};
    std::array<std::uint8_t,0x300> field{};
    std::array<std::uint8_t,0x80> stage{};
    std::array<std::uintptr_t,0x150/8> transform_table{};
    FieldTime clock{42,12,0,0,0,0,0,12,1000};
    void* owner{};int error{};
    unsigned acquire{},release{},destroy{},publish{},send{};
    bool ignore_write{},fault_write{};
    std::uint64_t tick{2000};
    std::uint8_t realm{1};
    ClockBindings bindings{};
    crimson::live_items::TaskObservationV1 task{};
    template<class T,std::size_t N> static void put(std::array<std::uint8_t,N>& data,std::size_t at,T value) {std::memcpy(data.data()+at,&value,sizeof(value));}
    static ClockFixture* active;
    static FieldLease* __fastcall take(void* transform,FieldLease* lease) {
        auto& f=*active;assert(transform==f.transform.data());++f.acquire;
        *lease={0,f.field.data(),1,0,{},0,nullptr,transform};return lease;
    }
    static void __fastcall drop(void* transform) {auto& f=*active;assert(transform==f.transform.data());++f.release;}
    static void __fastcall destroy_ref(FieldLease* lease) {auto& f=*active;assert(lease->valid);++f.destroy;lease->valid=0;}
    static std::uint16_t* __fastcall stage_key(void* field,std::uint16_t* key) {assert(field==active->field.data());*key=7;return key;}
    static const std::uint8_t* __fastcall stage_info(const std::uint16_t* key) {assert(*key==7);return active->stage.data();}
    static FieldTime* __fastcall read(void* field,FieldTime* time) {assert(field==active->field.data());*time=active->clock;return time;}
    static std::uint64_t __fastcall now() {return active->tick;}
    static void __fastcall write(void* transform,const FieldTime* time) {
        auto& f=*active;assert(transform==f.transform.data());++f.publish;
        if(f.fault_write)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,nullptr);
        if(!f.ignore_write){put(f.transform,0x450,*time);f.clock=*time;}
    }
    static void __fastcall ack(void* actor,const NearSendTarget* target,const FieldTime* time) {
        auto& f=*active;assert(actor==f.actor.data()&&target->actor==actor&&target->vtable==f.bindings.near_target_vtable&&target->enabled==1&&*time==f.clock);++f.send;
    }
    static std::uint8_t current_realm(){return active->realm;}
    ClockFixture() {
        active=this;owner=actor.data();
        bindings={true,0x10000000,0x11110000,reinterpret_cast<std::uintptr_t>(transform_table.data()),0x33330000,0x44440000,
            &clock,take,drop,destroy_ref,stage_key,stage_info,read,now,write,ack,current_realm};
        transform_table[0x140/8]=reinterpret_cast<std::uintptr_t>(take);transform_table[0x148/8]=reinterpret_cast<std::uintptr_t>(drop);
        put(actor,0,bindings.actor_vtable);put(actor,0x60,std::uint32_t{123});actor[0x96]=1;
        put(actor,0x68,components.data());put(actor,0x70,std::uint32_t{57});put(actor,0xa0,possessor.data());put(possessor,0xd0,actor.data());put(possessor,0x60,std::uint32_t{456});
        put(components,0x1a0,transform.data());put(transform,0,bindings.transform_vtable);put(transform,8,actor.data());put(transform,0x28,std::uint16_t{7});
        put(transform,0x450,clock);put(field,0,bindings.field_vtable);put(field,0x180,std::uint32_t{456});
        task.base=bindings.base;task.context=&owner;task.native_error=&error;task.return_address=task.base+0x2774a0d;task.identity=123;task.monotonic_ms=2000;
    }
};
ClockFixture* ClockFixture::active{};
unsigned native_clock_fixture_checks() {
    unsigned checks{};
    {
        ClockFixture f;ClockFrame frame;
        assert(open_clock_frame(f.bindings,f.task,frame)==ClockNativeReason::ready&&frame.identity==123&&frame.world==456&&frame.stage==7&&frame.native_now==2000);++checks;
        const auto frozen=freeze_time(frame.time,frame.native_now);assert(frozen);
        assert(publish_clock_frame(f.bindings,frame,*frozen)==ClockNativeReason::published&&f.publish==1&&f.send==1&&f.clock.rate==0);++checks;
        assert(close_clock_frame(f.bindings,frame)&&f.acquire==1&&f.release==1&&f.destroy==1&&!frame.actor&&!frame.transform);++checks;
        assert(close_clock_frame(f.bindings,frame)&&f.release==1&&f.destroy==1);++checks;
        assert(publish_clock_frame(f.bindings,frame,*frozen)==ClockNativeReason::invalid_clock&&f.publish==1);++checks;
    }
    for(unsigned scenario=0;scenario<12;++scenario) {
        ClockFixture f;ClockFrame frame;ClockNativeReason expected{};
        switch(scenario) {
        case 0:f.bindings.verified=false;expected=ClockNativeReason::unsupported_build;break;
        case 1:f.realm=0;expected=ClockNativeReason::task_context;break;
        case 2:f.task.return_address=0;expected=ClockNativeReason::task_context;break;
        case 3:f.task.identity=321;expected=ClockNativeReason::actor_identity;break;
        case 4:f.transform[0x3dc]=1;expected=ClockNativeReason::transform_owner;break;
        case 5:ClockFixture::put(f.field,0x180,std::uint32_t{789});expected=ClockNativeReason::world_changed;break;
        case 6:ClockFixture::put(f.transform,0x28,std::uint16_t{8});expected=ClockNativeReason::stage_unavailable;break;
        case 7:f.stage[0x6a]=1;expected=ClockNativeReason::stage_fixed;break;
        case 8:f.field[0x17c]=1;expected=ClockNativeReason::field_clock_stopped;break;
        case 9:f.clock.hour=24;expected=ClockNativeReason::invalid_clock;break;
        case 10:ClockFixture::put(f.actor,0x70,std::uint32_t{52});expected=ClockNativeReason::actor_identity;break;
        default:ClockFixture::put(f.actor,0x70,std::uint32_t{58});expected=ClockNativeReason::actor_identity;break;
        }
        assert(open_clock_frame(f.bindings,f.task,frame)==expected);
        assert(publish_clock_frame(f.bindings,frame,f.clock)==ClockNativeReason::invalid_clock&&f.publish==0&&f.send==0);
        assert(close_clock_frame(f.bindings,frame)&&f.release==f.acquire&&f.destroy==f.acquire);++checks;
    }
    {
        ClockFixture f;ClockFrame frame;f.ignore_write=true;
        assert(open_clock_frame(f.bindings,f.task,frame)==ClockNativeReason::ready);
        const auto frozen=freeze_time(frame.time,frame.native_now);assert(frozen);
        assert(publish_clock_frame(f.bindings,frame,*frozen)==ClockNativeReason::publish_mismatch&&f.publish==1&&f.send==0);
        assert(release_owned_clock(f.bindings,frame,*frozen,12)==ClockNativeReason::invalid_clock&&publish_clock_frame(f.bindings,frame,*frozen)==ClockNativeReason::invalid_clock&&f.publish==1);++checks;
        assert(close_clock_frame(f.bindings,frame)&&f.release==1&&f.destroy==1);++checks;
    }
    return checks;
}
unsigned queue_fixture_checks() {
    unsigned checks{};
    auto status=[](ControlQueue& q,std::uint64_t now=2000){return q.handle(Request{},now);};
    auto request=[&](ControlQueue& q,std::uint64_t id,ControlOperation op,std::uint32_t enabled=0){
        const auto s=status(q);Request r{};r.id=id;r.session=s.session;r.generation=s.generation;r.not_after_ms=10'000;r.operation=op;r.enabled=enabled;return r;
    };
    {
        ClockFixture f;ControlQueue q(123456);q.observe(f.bindings,f.task);
        auto s=status(q);assert(s.code==ControlCode::ready&&(s.flags&time_available)&&s.generation==1);++checks;
        assert(!(status(q,12001).flags&(time_available|weather_available))&&!(status(q,1999).flags&(time_available|weather_available)));++checks;
        auto r=request(q,1,ControlOperation::freeze_time,1);assert(q.handle(r,2000).code==ControlCode::queued&&f.publish==0);++checks;
        q.observe(f.bindings,f.task);s=status(q);assert(s.code==ControlCode::published&&s.time_receipt==1&&(s.flags&time_frozen)&&f.clock.rate==0);++checks;
        const auto held=f.clock;f.tick=9000;f.task.monotonic_ms=9000;q.observe(f.bindings,f.task);assert(f.clock==held&&f.publish==1);++checks;
        r=request(q,2,ControlOperation::choose_time);r.time={23,59,59,999};assert(q.handle(r,9000).code==ControlCode::queued);q.observe(f.bindings,f.task);
        assert(f.clock.rate==0&&f.clock.hour==23&&f.clock.millisecond==998&&(status(q,9000).flags&time_frozen));++checks;
        r=request(q,3,ControlOperation::freeze_time,0);assert(q.handle(r,9000).code==ControlCode::queued);q.observe(f.bindings,f.task);s=status(q,9000);
        assert(s.code==ControlCode::released&&!(s.flags&time_frozen)&&f.clock.rate==12&&f.clock.anchor==9000&&f.clock.hour==23&&f.acquire==f.release&&f.release==f.destroy);++checks;
        assert(q.handle(r,9000).reason==ControlReason::replay&&f.publish==3);++checks;
    }
    for(unsigned scenario=0;scenario<5;++scenario) {
        ClockFixture f;ControlQueue q(654321);q.observe(f.bindings,f.task);
        auto r=request(q,1,ControlOperation::freeze_time,1);assert(q.handle(r,2000).code==ControlCode::queued);q.observe(f.bindings,f.task);assert(f.clock.rate==0);
        if(scenario==0){ClockFixture::put(f.field,0x180,std::uint32_t{789});ClockFixture::put(f.possessor,0x60,std::uint32_t{789});}
        else if(scenario==1){f.stage[0x6a]=1;}
        else if(scenario==2){f.field[0x17c]=1;}
        else if(scenario==3){f.tick=20001;f.task.monotonic_ms=20001;}
        else {f.clock.anchor=3333;}
        q.observe(f.bindings,f.task);const auto s=status(q,f.task.monotonic_ms);
        if(scenario==4)assert(f.clock.rate==0&&f.clock.anchor==3333&&f.publish==1&&s.reason==ControlReason::foreign_change);
        else assert(f.clock.rate==12&&f.publish==2&&s.code==ControlCode::released);
        assert(!(s.flags&time_frozen)&&f.acquire==f.release&&f.release==f.destroy);++checks;
    }
    for(unsigned fault=0;fault<2;++fault) {
        ClockFixture f;ControlQueue q(321);q.observe(f.bindings,f.task);f.ignore_write=!fault;f.fault_write=fault!=0;
        auto r=request(q,1,ControlOperation::freeze_time,1);assert(q.handle(r,2000).code==ControlCode::queued);q.observe(f.bindings,f.task);
        const auto acquires=f.acquire,publishes=f.publish;const auto s=status(q);
        assert(s.code==(fault?ControlCode::fault_latched:ControlCode::uncertain)&&(s.flags&time_uncertain)&&!(s.flags&time_available));
        q.observe(f.bindings,f.task);q.observe(f.bindings,f.task);assert(f.acquire==acquires&&f.publish==publishes&&q.handle(request(q,2,ControlOperation::release_time),2000).code==s.code);++checks;
    }
    {
        ClockFixture f;ControlQueue q(11);q.observe(f.bindings,f.task);auto r=request(q,1,ControlOperation::freeze_time,1);
        assert(q.handle(r,2000).code==ControlCode::queued);f.task.monotonic_ms=10001;q.observe(f.bindings,f.task);
        assert(status(q,10001).code==ControlCode::expired&&f.publish==0);++checks;
        r=request(q,2,ControlOperation::choose_time);r.generation=0;assert(q.handle(r,2000).reason==ControlReason::generation);++checks;
    }
    {
        ClockFixture f;ControlQueue q(12);q.observe(f.bindings,f.task);auto r=request(q,1,ControlOperation::freeze_time,1);
        assert(q.handle(r,2000).code==ControlCode::queued);assert(status(q,10001).code==ControlCode::expired);f.task.monotonic_ms=10001;q.observe(f.bindings,f.task);assert(f.publish==0);++checks;
    }
    return checks;
}
struct WeatherFixture : ClockFixture {
    std::array<std::uint8_t,0x300> client_field{};
    std::array<std::uint8_t,0x60> service{},holder{},buffers{};
    std::array<std::uint8_t,0x20> active_buffer{};
    std::array<std::uint8_t,0x100> manager{};
    std::array<std::uint8_t,0x1000> render_world{};
    std::array<std::uint8_t,0x58> resource{};
    std::array<std::uint8_t,0x1d8> constant{};
    std::array<std::uint8_t,0x158> atmosphere{};
    void* root{};WeatherBindings weather{};unsigned automatic{},derivations{},setters{};bool ignore_float{},fault_float{};
    static WeatherFixture* current;
    static void __fastcall set_manual(void* value,const std::uint8_t* enabled){assert(value==current->manager.data());current->manager[0x31]=*enabled;}
    static void scalar(void* value,const float* number,std::size_t offset,bool atmospheric=false) {
        auto& f=*current;assert(value==(atmospheric?f.atmosphere.data():f.constant.data()));++f.setters;
        if(f.fault_float)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,nullptr);
        if(!f.ignore_float)std::memcpy(static_cast<std::uint8_t*>(value)+offset,number,4);
    }
    static void __fastcall precip(void* p,const float* v){scalar(p,v,0x154);}
    static void __fastcall cloud(void* p,const float* v){scalar(p,v,0x158);}
    static void __fastcall humidity(void* p,const float* v){scalar(p,v,0x15c);}
    static void __fastcall wind(void* p,const float* v){scalar(p,v,0x160);}
    static void __fastcall fog(void* p,const float* v){scalar(p,v,0xc4,true);}
    static void __fastcall density(void* p,const float* v){scalar(p,v,0xd0,true);}
    static void __fastcall derive(void* p) {
        auto& f=*current;assert(p==f.resource.data());++f.derivations;
        const auto input=*reinterpret_cast<float*>(f.constant.data()+0x154);
        const auto rain=*reinterpret_cast<float*>(f.constant.data()+0x1c0)>0;
        put(f.constant,0x194,rain?input:0);put(f.constant,0x190,rain?0:input);
        put(f.constant,0x188,rain?input:0);put(f.constant,0x18c,rain?std::min(1.0f,input*10):0);
    }
    WeatherFixture() {
        current=this;root=service.data();weather={true,bindings.base,0x55550000,0x66660000,0x77770000,0x88880000,0x99990000,0xaaaa0000,&root,set_manual,precip,cloud,humidity,wind,fog,density,derive};
        put(service,0x38,holder.data());put(holder,0x38,manager.data());put(manager,0,weather.manager_vtable);put(manager,0xe8,render_world.data());put(manager,0x60,resource.data()+0x28);put(render_world,0,weather.world_vtable);put(render_world,0xf68,manager.data());
        put(resource,0,weather.resource_vtable);put(resource,0x40,constant.data()+0x28);put(resource,0x48,atmosphere.data()+0x28);
        put(constant,0,weather.constant_vtable);put(constant,0x10,resource.data());put(atmosphere,0,weather.atmosphere_vtable);put(atmosphere,0x10,resource.data());
        put(constant,0x154,0.0f);put(constant,0x158,0.25f);put(constant,0x15c,0.5f);put(constant,0x160,2.0f);put(constant,0x1c0,1.0f);put(atmosphere,0xc4,0.1f);put(atmosphere,0xd0,0.25f);
        put(client_field,0,weather.client_field_vtable);put(client_field,0x180,std::uint32_t{456});put(client_field,0x2b2,std::uint16_t{7});put(client_field,0x210,buffers.data());
        // A live game selects ClientPhase_LivePlay in slot 5. Exercising only
        // slot 0 previously hid an incorrect three-slot bound in the adapter.
        buffers[0x48]=5;put(buffers,0x10+5*8,active_buffer.data());active_buffer[9]=1;
    }
    void client(ControlQueue& q) {
        const auto rain=*reinterpret_cast<float*>(constant.data()+0x194),snow=*reinterpret_cast<float*>(constant.data()+0x190);
        put(client_field,0x178,snow>0?2u:rain>0?1u:32u);
        q.weather_client(weather,client_field.data(),bindings.base+0xa8aed9,task.monotonic_ms);
    }
    void producer(ControlQueue& q,bool primary=true) {
        // Only fixture-owned emulation. The real original is called by hooks.
        if(primary) {
            if(!manager[0x31]){++automatic;put(constant,0x154,0.0f);put(constant,0x158,0.25f);put(atmosphere,0xd0,0.25f);}
            derive(resource.data());put(constant,0x1c0,-25.0f);put(constant,0x1c4,33.0f);put(constant,0x1cc,7.0f);
        } else {
            // 3dc50e0 calls 3dc08d0: its five climate writes reach the owned
            // result before this frame's client consumer, despite no blend.
            put(constant,0x1c0,12.0f);put(constant,0x1c4,44.0f);put(constant,0x1c8,3.0f);put(constant,0x1cc,8.0f);put(constant,0x1d0,9.0f);
        }
        auto animation=*reinterpret_cast<unsigned*>(manager.data()+0x80);put(manager,0x80,animation+1);
        q.weather_producer(weather,manager.data(),bindings.base+(primary?0x3b1073c:0x3b4c180),task.monotonic_ms);
    }
    void ready(ControlQueue& q){q.observe(bindings,task);client(q);producer(q);}
    Request command(ControlQueue& q,std::uint64_t id,ControlOperation op,std::uint32_t enabled=0) {
        const auto s=q.handle(Request{},task.monotonic_ms);Request r{};r.id=id;r.session=s.session;r.generation=s.generation;r.not_after_ms=task.monotonic_ms+5000;r.operation=op;r.enabled=enabled;return r;
    }
};
WeatherFixture* WeatherFixture::current{};
unsigned weather_fixture_checks() {
    unsigned checks{};
    for(unsigned phase=0;phase<7;++phase) {
        WeatherFixture f;ControlQueue q(776);
        f.buffers[0x48]=static_cast<std::uint8_t>(phase);
        ClockFixture::put(f.buffers,0x10+phase*8,f.active_buffer.data());
        WeatherClientObservation observed{};
        assert(read_weather_client(f.weather,f.client_field.data(),f.bindings.base+0xa8aed9,776,observed)==WeatherNativeReason::ready);
        f.ready(q);assert(q.handle(Request{},2000).flags&weather_available);++checks;
    }
    for(const auto phase:{7u,8u,255u}) {
        WeatherFixture f;WeatherClientObservation observed{};
        f.buffers[0x48]=static_cast<std::uint8_t>(phase);
        assert(read_weather_client(f.weather,f.client_field.data(),f.bindings.base+0xa8aed9,776,observed)==WeatherNativeReason::client_field);++checks;
    }
    for(unsigned invalid=0;invalid<2;++invalid) {
        WeatherFixture f;WeatherClientObservation observed{};
        if(invalid)f.active_buffer[9]=0;else ClockFixture::put(f.buffers,0x10+5*8,static_cast<void*>(nullptr));
        assert(read_weather_client(f.weather,f.client_field.data(),f.bindings.base+0xa8aed9,776,observed)==WeatherNativeReason::client_field);++checks;
    }
    {
        WeatherFixture f;ControlQueue q(777);f.ready(q);assert(q.handle(Request{},2000).flags&weather_available);++checks;
        auto r=f.command(q,1,ControlOperation::choose_weather);r.weather={0.75f,0.9f,0.6f,20.0f,2.0f,PrecipitationPhase::snow};
        assert(q.handle(r,2000).code==ControlCode::queued&&f.setters==0);f.producer(q);assert(q.handle(Request{},2000).code==ControlCode::queued&&f.manager[0x31]);f.client(q);
        auto s=q.handle(Request{},2000);assert(s.code==ControlCode::published&&s.weather_receipt==1&&s.weather.phase==PrecipitationPhase::snow&&*reinterpret_cast<float*>(f.constant.data()+0x190)==0.75f);++checks;
        r=f.command(q,2,ControlOperation::freeze_weather,1);assert(q.handle(r,2000).code==ControlCode::queued);f.producer(q);f.client(q);s=q.handle(Request{},2000);assert(s.flags&weather_frozen);++checks;
        WeatherFrame frame{};WeatherSnapshot held{},after{};WeatherParameters p{};
        assert(open_weather_frame(f.weather,f.manager.data(),f.bindings.base+0x3b1073c,777,frame)==WeatherNativeReason::ready&&read_weather(frame,held,p)==WeatherNativeReason::ready);
        const auto animation=*reinterpret_cast<unsigned*>(f.manager.data()+0x80);
        for(unsigned i=0;i<3;++i){f.producer(q);f.producer(q,false);}
        assert(read_weather(frame,after,p)==WeatherNativeReason::ready&&after==held&&*reinterpret_cast<unsigned*>(f.manager.data()+0x80)==animation+6);++checks;
        r=f.command(q,3,ControlOperation::freeze_weather,0);assert(q.handle(r,2000).code==ControlCode::queued);f.producer(q);s=q.handle(Request{},2000);
        assert(!(s.flags&weather_frozen)&&f.manager[0x31]&&s.code==ControlCode::published);++checks;
        r=f.command(q,4,ControlOperation::release_weather);assert(q.handle(r,2000).code==ControlCode::queued);f.producer(q);assert(f.manager[0x31]==0&&q.handle(Request{},2000).weather_result==ControlCode::queued);f.producer(q);
        s=q.handle(Request{},2000);assert(s.code==ControlCode::released&&s.weather_receipt==4&&f.automatic>=2&&f.clock.rate==12);++checks;
    }
    for(unsigned mode=0;mode<4;++mode) {
        WeatherFixture f;ControlQueue q(888);f.ready(q);
        auto r=f.command(q,1,ControlOperation::freeze_weather,1);assert(q.handle(r,2000).code==ControlCode::queued);f.producer(q);f.client(q);
        if(mode==0){f.task.monotonic_ms=12001;}
        else if(mode==1){f.manager[0x31]=0;}
        else if(mode==2){ClockFixture::put(f.field,0x180,std::uint32_t{789});ClockFixture::put(f.possessor,0x60,std::uint32_t{789});q.observe(f.bindings,f.task);}
        else {ClockFixture::put(f.constant,0x10,static_cast<void*>(f.manager.data()));}
        f.producer(q);auto s=q.handle(Request{},f.task.monotonic_ms);
        if(mode==3)assert(!(s.flags&weather_available)&&f.manager[0x31]==1);
        else {assert(!(s.flags&weather_frozen)&&f.manager[0x31]==0);f.producer(q);}
        ++checks;
    }
    for(unsigned fault=0;fault<2;++fault) {
        WeatherFixture f;ControlQueue q(999);f.ready(q);f.ignore_float=!fault;f.fault_float=fault!=0;
        auto r=f.command(q,1,ControlOperation::choose_weather);r.weather={1,1,1,60,10,PrecipitationPhase::rain};assert(q.handle(r,2000).code==ControlCode::queued);f.producer(q);
        const auto writes=f.setters;const auto s=q.handle(Request{},2000);assert(s.flags&weather_uncertain);
        f.producer(q);f.client(q);assert(f.setters==writes&&q.handle(Request{},2000).code==s.code);++checks;
    }
    {
        WeatherFixture f;ControlQueue q(1000);f.ready(q);
        auto r=f.command(q,1,ControlOperation::freeze_weather,1);assert(q.handle(r,2000).code==ControlCode::queued);f.producer(q);f.client(q);
        r=f.command(q,2,ControlOperation::freeze_time,1);assert(q.handle(r,2000).code==ControlCode::queued);q.observe(f.bindings,f.task);
        assert((q.handle(Request{},2000).flags&(time_frozen|weather_frozen))==(time_frozen|weather_frozen));++checks;
        r=f.command(q,3,ControlOperation::freeze_weather,0);assert(q.handle(r,2000).code==ControlCode::queued);f.producer(q);assert(f.manager[0x31]==0&&f.clock.rate==0);f.producer(q);
        assert(q.handle(Request{},2000).code==ControlCode::released&&q.handle(Request{},2000).weather_receipt==3&&(q.handle(Request{},2000).flags&time_frozen));++checks;
        r=f.command(q,4,ControlOperation::release_time);assert(q.handle(r,2000).code==ControlCode::queued);q.observe(f.bindings,f.task);assert(f.clock.rate==12&&f.manager[0x31]==0);++checks;
    }
    {
        WeatherFixture f;f.manager[0x31]=1;ControlQueue q(1001);f.ready(q);
        WeatherFrame frame{};WeatherSnapshot before{},after{};WeatherParameters params{};
        assert(open_weather_frame(f.weather,f.manager.data(),f.bindings.base+0x3b1073c,1001,frame)==WeatherNativeReason::ready&&read_weather(frame,before,params)==WeatherNativeReason::ready);
        auto r=f.command(q,1,ControlOperation::choose_weather);r.weather={1,1,1,60,10,PrecipitationPhase::rain};assert(q.handle(r,2000).code==ControlCode::queued);f.producer(q);f.client(q);
        r=f.command(q,2,ControlOperation::release_weather);assert(q.handle(r,2000).code==ControlCode::queued);f.producer(q);
        assert(f.manager[0x31]==1&&read_weather(frame,after,params)==WeatherNativeReason::ready&&after==before&&q.handle(Request{},2000).code==ControlCode::released);++checks;
    }
    {
        WeatherFixture f;ControlQueue q(1002);f.ready(q);auto r=f.command(q,1,ControlOperation::choose_weather);r.weather={1,1,1,60,10,PrecipitationPhase::rain};assert(q.handle(r,2000).code==ControlCode::queued);f.producer(q);f.client(q);
        ClockFixture::put(f.constant,0x154,0.25f);const auto writes=f.setters;f.producer(q);
        assert(f.setters==writes&&f.manager[0x31]==1&&*reinterpret_cast<float*>(f.constant.data()+0x154)==0.25f&&q.handle(Request{},2000).reason==ControlReason::foreign_change);++checks;
    }
    {
        WeatherFixture f;ControlQueue q(1004);f.ready(q);
        auto r=f.command(q,1,ControlOperation::freeze_time,1);assert(q.handle(r,2000).code==ControlCode::queued);q.observe(f.bindings,f.task);
        const auto held_time=f.clock;
        r=f.command(q,2,ControlOperation::choose_weather);r.weather={0.8f,0.9f,0.6f,20,2,PrecipitationPhase::snow};assert(q.handle(r,2000).code==ControlCode::queued);f.producer(q);f.client(q);
        r=f.command(q,3,ControlOperation::freeze_weather,1);assert(q.handle(r,2000).code==ControlCode::queued);f.producer(q);f.client(q);
        WeatherFrame frame{};WeatherSnapshot held{},after{};WeatherParameters params{};
        assert(open_weather_frame(f.weather,f.manager.data(),f.bindings.base+0x3b1073c,1004,frame)==WeatherNativeReason::ready&&read_weather(frame,held,params)==WeatherNativeReason::ready);
        // Exact frame order: secondary climate writes + restore, client read,
        // main weather job + restore, completion, next frame. A main-only
        // restore would allow the secondary climate outputs to escape here.
        f.producer(q,false);assert(read_weather(frame,after,params)==WeatherNativeReason::ready&&after==held);f.client(q);
        f.producer(q);assert(read_weather(frame,after,params)==WeatherNativeReason::ready&&after==held&&f.clock==held_time);++checks;
        const auto automatic=f.automatic;
        r=f.command(q,4,ControlOperation::release_weather);assert(q.handle(r,2000).code==ControlCode::queued);f.producer(q,false);f.client(q);
        auto s=q.handle(Request{},2000);assert(!f.manager[0x31]&&s.weather_result==ControlCode::queued&&!(s.flags&weather_frozen)&&(s.flags&time_frozen)&&f.clock==held_time);++checks;
        f.producer(q);s=q.handle(Request{},2000);
        assert(s.weather_receipt==4&&s.weather_result==ControlCode::released&&f.automatic==automatic+1&&!f.manager[0x31]&&s.time_receipt==1&&s.time_result==ControlCode::published&&(s.flags&time_frozen)&&f.clock==held_time&&f.publish==1);++checks;
    }
    constexpr std::array<std::size_t,27> float_offsets{0x154,0x158,0x15c,0x160,0x164,0x168,0x16c,0x170,0x174,0x178,0x17c,0x180,
        0x188,0x18c,0x190,0x194,0x1a8,0x1ac,0x1b0,0x1b4,0x1b8,0x1bc,0x1c0,0x1c4,0x1c8,0x1cc,0x1d0};
    for(unsigned index=0;index<30;++index)for(const auto invalid:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
        WeatherFixture f;WeatherFrame frame{};WeatherSnapshot snapshot{};WeatherParameters params{};
        assert(open_weather_frame(f.weather,f.manager.data(),f.bindings.base+0x3b1073c,1005,frame)==WeatherNativeReason::ready);
        if(index<float_offsets.size())ClockFixture::put(f.constant,float_offsets[index],invalid);
        else ClockFixture::put(f.atmosphere,index==27?0xb0:index==28?0xc4:0xd0,invalid);
        assert(read_weather(frame,snapshot,params)==WeatherNativeReason::invalid_value&&f.setters==0&&!f.manager[0x31]);++checks;
    }
    {
        WeatherFixture f;WeatherFrame frame{};ClockFixture::put(f.render_world,0,std::uintptr_t{0xdeadbeef});
        assert(open_weather_frame(f.weather,f.manager.data(),f.bindings.base+0x3b1073c,1006,frame)==WeatherNativeReason::current_manager&&!frame.ready&&f.setters==0);++checks;
    }
    {
        WeatherFixture f;WeatherFrame frame{};WeatherSnapshot snapshot{};WeatherParameters params{};
        // Padding can have arbitrary bits. It is never decoded as a scalar,
        // captured, compared for ownership, or overwritten during restore.
        for(const auto offset:{0x185,0x186,0x187,0x1a5,0x1a6,0x1a7})f.constant[offset]=0xff;
        assert(open_weather_frame(f.weather,f.manager.data(),f.bindings.base+0x3b1073c,1007,frame)==WeatherNativeReason::ready&&read_weather(frame,snapshot,params)==WeatherNativeReason::ready);
        for(const auto offset:{0x185,0x186,0x187,0x1a5,0x1a6,0x1a7})f.constant[offset]=0xa5;
        assert(restore_weather(frame,snapshot)==WeatherNativeReason::published);
        for(const auto offset:{0x185,0x186,0x187,0x1a5,0x1a6,0x1a7})assert(f.constant[offset]==0xa5);++checks;
    }
    {
        WeatherFixture f;ControlQueue q(1008);f.ready(q);
        auto r=f.command(q,1,ControlOperation::freeze_weather,1);assert(q.handle(r,2000).code==ControlCode::queued);
        ClockFixture::put(f.constant,0x160,std::numeric_limits<float>::quiet_NaN());f.producer(q);
        const auto s=q.handle(Request{},2000);assert((s.flags&weather_uncertain)&&!(s.flags&weather_frozen)&&f.setters==0&&!f.manager[0x31]);++checks;
    }
    return checks;
}
struct HookFixture {
    unsigned creates{},enables{},disables{},removes{},uninitializes{},publishes{};
    unsigned fail_create{},null_create{},fail_enable{},fail_disable{};bool published{};
    bool pin(){return true;}bool initialize(){return true;}
    bool create(std::uintptr_t,std::uintptr_t,std::uintptr_t& trampoline) {
        ++creates;if(creates==fail_create)return false;trampoline=creates==null_create?0:0xabc000+creates;return true;
    }
    bool enable(std::uintptr_t){assert(published&&publishes==1);++enables;return enables!=fail_enable;}
    bool disable(std::uintptr_t){++disables;return disables!=fail_disable;}
    bool remove(std::uintptr_t){++removes;assert(!enables);return true;}
    bool uninitialize(){++uninitializes;assert(!enables);return true;}
};
unsigned hook_fixture_checks() {
    unsigned checks{};
    for(unsigned scenario=0;scenario<12;++scenario) {
        HookFixture b;std::atomic<bool> ready{};WorldHookTransaction hooks(b,ready);
        if(scenario>=1&&scenario<=3)b.null_create=scenario;
        if(scenario>=4&&scenario<=6)b.fail_create=scenario-3;
        if(scenario>=8&&scenario<=10)b.fail_enable=scenario-7;
        if(scenario==11){b.fail_enable=2;b.fail_disable=1;}
        const auto publish=[&](const auto& values){for(auto value:values)assert(value);++b.publishes;b.published=true;assert(!b.enables);return scenario!=7;};
        const bool installed=hooks.install(0x10000000,{1,2,3},[]{return true;},publish);
        assert(installed==(scenario==0));
        if(scenario==0){assert(ready&&b.enables==3&&!b.removes);assert(hooks.cleanup()&&!ready&&b.disables==3&&!b.removes&&!b.uninitializes);}
        else if(scenario<=3)assert(!b.publishes&&!b.enables&&b.removes==3&&b.uninitializes==1);
        else if(scenario<=6)assert(!b.publishes&&!b.enables&&b.removes==scenario-4&&b.uninitializes==1);
        else if(scenario==7)assert(b.publishes==1&&!b.enables&&b.removes==3&&b.uninitializes==1);
        else assert(b.publishes==1&&b.enables&&!b.removes&&!b.uninitializes&&!ready);
        const auto creates=b.creates,enables=b.enables;assert(!hooks.install(0x10000000,{1,2,3},[]{return true;},publish)&&b.creates==creates&&b.enables==enables);++checks;
    }
    bool entered=false;
    try {WeatherEntryGuard outer(entered);assert(outer.entered&&entered);{WeatherEntryGuard inner(entered);assert(!inner.entered&&entered);}assert(entered);throw 42;}
    catch(int value){assert(value==42&&!entered);++checks;}
    return checks;
}
unsigned receipt_fixture_checks() {
    unsigned checks{};WeatherFixture f;ControlQueue q(1003);f.ready(q);
    auto time=f.command(q,1,ControlOperation::freeze_time,1),weather=f.command(q,2,ControlOperation::freeze_weather,1);
    assert(q.handle(time,2000).code==ControlCode::queued&&q.handle(weather,2000).code==ControlCode::queued);f.producer(q);f.client(q);
    auto s=q.handle(Request{},2000);assert(s.time_receipt==1&&s.time_result==ControlCode::queued&&s.weather_receipt==2&&s.weather_result==ControlCode::published);++checks;
    q.observe(f.bindings,f.task);s=q.handle(Request{},2000);assert(s.time_result==ControlCode::published&&s.weather_result==ControlCode::published);++checks;
    time=f.command(q,3,ControlOperation::choose_time);time.time={16,0,0,0};assert(q.handle(time,2000).code==ControlCode::queued);
    f.manager[0x31]=0;f.producer(q);s=q.handle(Request{},2000);
    assert(s.id==0&&s.code==ControlCode::released&&s.time_receipt==3&&s.time_result==ControlCode::queued&&s.weather_receipt==2&&s.weather_result==ControlCode::published);++checks;
    f.stage[0x6a]=1;q.observe(f.bindings,f.task);s=q.handle(Request{},2000);
    assert(s.time_receipt==3&&s.time_result==ControlCode::rejected&&s.time_result_reason==ControlReason::stage_fixed&&!(s.flags&time_frozen)&&f.clock.rate==12);++checks;
    return checks;
}
unsigned advance_time_fixture_checks() {
    unsigned checks{};
    for(const auto rate:{std::uint16_t{0},std::uint16_t{12}})for(std::uint32_t hour=0;hour<24;++hour)for(const auto hours:{6u,24u,48u,10'000u}) {
        const FieldTime source{6,hour,15,16,998,6,0,rate,1000};const auto value=advance_time(source,hours,1000);assert(value);
        const auto day=6+(hour+hours)/24;
        assert(value->day==day&&value->weekday==day%7&&value->hour==(hour+hours)%24&&value->minute==15&&value->second==16&&value->millisecond==998&&value->rate==rate&&value->anchor==1000);++checks;
    }
    {
        const FieldTime source{6,22,15,16,998,6,0,12,1000};
        const auto six=advance_time(source,6,1000),day=advance_time(source,24,1000),week=advance_time(source,48,1000);
        assert(six&&six->day==7&&six->weekday==0&&six->hour==4&&six->minute==15&&six->second==16&&six->millisecond==998);++checks;
        assert(day&&day->day==7&&day->weekday==0&&day->hour==22&&week&&week->day==8&&week->weekday==1&&week->hour==22);++checks;
        const auto elapsed=advance_time(source,6,2000);assert(elapsed&&elapsed->hour==4&&elapsed->second==40&&elapsed->millisecond==998&&elapsed->anchor==2000&&elapsed->rate==12);++checks;
    }
    {
        const FieldTime source{42,0,30,0,0,0,0,0,1000};const auto value=advance_time(source,6,9000);assert(value);
        const auto before=encode_clock(fields(source)),after=encode_clock(fields(*value));assert(before&&after);
        assert(value->day==42&&value->hour==6&&value->minute==30&&*after-*before==17'100'000&&*after-*before!=6*3'600'000&&value->anchor==9000&&value->rate==0);++checks;
        // Native nighttime resolution cannot represent an odd displayed ms.
        const auto quantized=advance_time(FieldTime{42,20,30,1,999,0,0,0,1000},6,9000);
        assert(quantized&&quantized->day==43&&quantized->hour==2&&quantized->minute==30&&quantized->second==1&&quantized->millisecond==998);++checks;
    }
    {
        constexpr auto max_day=std::numeric_limits<std::uint32_t>::max();
        const auto final_day=advance_time(FieldTime{max_day-416,0,0,0,0,0,0,0,1000},10'000,1000);
        assert(final_day&&final_day->day==max_day&&final_day->hour==16);++checks;
        assert(!advance_time(FieldTime{max_day-415,0,0,0,0,0,0,0,1000},10'000,1000));++checks;
        assert(!advance_time(FieldTime{max_day,23,0,0,0,0,0,0,1000},1,1000));++checks;
        assert(!advance_time(FieldTime{},0,1000)&&!advance_time(FieldTime{},10'001,1000));++checks;
    }
    {
        WeatherFixture f;ControlQueue q(1010);q.observe(f.bindings,f.task);
        assert(!(q.handle(Request{},2000).flags&weather_available));
        auto request=f.command(q,1,ControlOperation::advance_time);request.time.hour=48;
        assert(!is_weather_operation(request.operation)&&q.handle(request,2000).code==ControlCode::queued&&f.publish==0&&f.setters==0);
        q.observe(f.bindings,f.task);auto s=q.handle(Request{},2000);
        assert(s.time_receipt==1&&s.time_result==ControlCode::published&&!s.weather_receipt&&s.weather_result==ControlCode::unavailable&&f.clock.day==44&&f.clock.weekday==2&&f.clock.hour==12&&f.clock.second==12&&f.clock.rate==12&&f.publish==1&&f.send==1&&!f.setters);++checks;
        const auto held=f.clock;q.observe(f.bindings,f.task);assert(q.handle(request,2000).reason==ControlReason::replay&&f.clock==held&&f.publish==1&&f.send==1);++checks;
    }
    for(unsigned invalid=0;invalid<12;++invalid) {
        WeatherFixture f;ControlQueue q(1011);f.ready(q);auto r=f.command(q,1,ControlOperation::advance_time);r.time.hour=6;
        switch(invalid) {
        case 0:r.time.hour=0;break;case 1:r.time.hour=10'001;break;case 2:r.time.minute=1;break;case 3:r.time.second=1;break;case 4:r.time.millisecond=1;break;
        case 5:r.enabled=1;break;case 6:r.weather.precipitation=1;break;case 7:r.weather.cloudiness=-0.0f;break;
        case 8:r.weather.humidity=1;break;case 9:r.weather.wind_speed=1;break;case 10:r.weather.height_fog=1;break;default:r.weather.phase=PrecipitationPhase::snow;break;
        }
        assert(q.handle(r,2000).reason==ControlReason::invalid_value);q.observe(f.bindings,f.task);assert(f.publish==0&&f.send==0&&!f.setters);++checks;
    }
    {
        WeatherFixture f;ControlQueue q(1014);f.ready(q);auto r=f.command(q,1,static_cast<ControlOperation>(8));
        assert(q.handle(r,2000).reason==ControlReason::protocol&&f.publish==0&&!f.setters);++checks;
    }
    {
        WeatherFixture f;ControlQueue q(1012);f.clock={0xffffffffu,22,0,0,0,3,0,0,2000};ClockFixture::put(f.transform,0x450,f.clock);f.ready(q);
        auto r=f.command(q,1,ControlOperation::advance_time);r.time.hour=10'000;assert(q.handle(r,2000).code==ControlCode::queued);q.observe(f.bindings,f.task);
        const auto s=q.handle(Request{},2000);assert(s.time_receipt==1&&s.time_result==ControlCode::rejected&&s.time_result_reason==ControlReason::invalid_value&&f.publish==0&&f.send==0);++checks;
    }
    {
        WeatherFixture f;ControlQueue q(1013);f.ready(q);
        auto r=f.command(q,1,ControlOperation::freeze_time,1);assert(q.handle(r,2000).code==ControlCode::queued);q.observe(f.bindings,f.task);
        r=f.command(q,2,ControlOperation::choose_weather);r.weather={0.8f,0.9f,0.6f,20,2,PrecipitationPhase::snow};assert(q.handle(r,2000).code==ControlCode::queued);f.producer(q);f.client(q);
        r=f.command(q,3,ControlOperation::freeze_weather,1);assert(q.handle(r,2000).code==ControlCode::queued);f.producer(q);f.client(q);
        WeatherFrame frame{};WeatherSnapshot weather_before{},weather_after{};WeatherParameters params{};
        assert(open_weather_frame(f.weather,f.manager.data(),f.bindings.base+0x3b1073c,1013,frame)==WeatherNativeReason::ready&&read_weather(frame,weather_before,params)==WeatherNativeReason::ready);
        r=f.command(q,4,ControlOperation::advance_time);r.time.hour=48;assert(q.handle(r,2000).code==ControlCode::queued);q.observe(f.bindings,f.task);
        auto s=q.handle(Request{},2000);const auto held=f.clock;
        assert(s.time_receipt==4&&s.time_result==ControlCode::published&&s.weather_receipt==3&&s.weather_result==ControlCode::published&&f.clock.day==44&&f.clock.hour==12&&f.clock.second==12&&f.clock.rate==0&&(s.flags&(time_frozen|weather_frozen))==(time_frozen|weather_frozen));
        assert(read_weather(frame,weather_after,params)==WeatherNativeReason::ready&&weather_after==weather_before);++checks;
        q.observe(f.bindings,f.task);assert(f.clock==held&&(q.handle(Request{},2000).flags&time_frozen)&&f.publish==2&&q.handle(r,2000).reason==ControlReason::replay);++checks;
        r=f.command(q,5,ControlOperation::release_weather);assert(q.handle(r,2000).code==ControlCode::queued);f.producer(q,false);f.producer(q);s=q.handle(Request{},2000);
        assert(s.weather_receipt==5&&s.weather_result==ControlCode::released&&!f.manager[0x31]&&f.clock==held&&(s.flags&time_frozen)&&!(s.flags&weather_frozen));++checks;
        r=f.command(q,6,ControlOperation::release_time);assert(q.handle(r,2000).code==ControlCode::queued);f.tick=3000;q.observe(f.bindings,f.task);
        assert(f.clock.day==44&&f.clock.rate==12&&f.clock.hour==12&&f.clock.second==12&&f.clock.anchor==3000&&f.publish==3);++checks;
    }
    return checks;
}
}
int main() {
    unsigned checks=0;
    for (std::uint32_t day:{0u,1u,6u,7u,42u,365u,0xffffffffu}) {
        for (std::uint32_t minute=0;minute<1440;++minute) {
            ClockFields fields{day,minute/60,minute%60,0,0,static_cast<std::uint8_t>(day%7)};
            const auto raw=encode_clock(fields);assert(raw);
            assert(*raw==std::uint64_t{day}*kDayUnits+minute_oracle(minute));
            assert(decode_clock(*raw)==fields);++checks;
        }
    }
    for (std::uint64_t raw=0;raw<kDayUnits;raw+=997) {
        const auto fields=decode_clock(raw);assert(fields && encode_clock(*fields)==raw);++checks;
    }
    for (std::uint64_t raw:{kEarlyNightEnd-1,kEarlyNightEnd,kEarlyNightEnd+1,
            kLateNightStart-1,kLateNightStart,kLateNightStart+1,kDayUnits-1,kDayUnits,kDayUnits+1}) {
        const auto fields=decode_clock(raw);assert(fields && encode_clock(*fields)==raw);++checks;
    }
    for (auto fields:std::array<ClockFields,5>{{{0,24,0,0,0,0},{0,0,60,0,0,0},{0,0,0,60,0,0},
            {0,0,0,0,1000,0},{0,0xffffffffu,0,0,0,0}}}) assert(!encode_clock(fields));
    assert(!decode_clock(std::numeric_limits<std::uint64_t>::max()));
    assert(!decode_clock((std::uint64_t{0xffffffffu}+1)*kDayUnits));
    // A native nighttime encode quantizes odd display milliseconds downward.
    const ClockFields odd{0,1,2,3,999,0};const auto value=encode_clock(odd);assert(value);
    const auto canonical=decode_clock(*value);assert(canonical && canonical->millisecond==998);
    const ClockFields noon{42,12,0,0,0,0};assert(encode_clock(noon)==std::uint64_t{42}*kDayUnits+37'800'000);
    // Native rate is a multiplier in encoded milliseconds, including zero.
    // At night, 12 encoded units advance the displayed clock by 24 units.
    const FieldTime night{42,1,0,0,0,0,0,12,1000};
    auto advanced=at_native_time(night,2000);assert(advanced && advanced->hour==1 && advanced->second==24 && advanced->anchor==2000);++checks;
    const FieldTime day{42,12,0,0,0,0,0,12,1000};
    advanced=at_native_time(day,2000);assert(advanced && advanced->hour==12 && advanced->second==12);++checks;
    auto stopped=freeze_time(day,2000);assert(stopped && stopped->second==12 && stopped->rate==0);++checks;
    for(std::uint64_t now:{2000ull,5000ull,1000000000000ull,std::numeric_limits<std::uint64_t>::max()}) {
        const auto held=at_native_time(*stopped,now);assert(held && fields(*held)==fields(*stopped));++checks;
    }
    auto resumed=resume_time(*stopped,12,5000);assert(resumed && resumed->anchor==5000 && resumed->second==12 && resumed->rate==12);++checks;
    advanced=at_native_time(*resumed,6000);assert(advanced && advanced->second==24);++checks;
    auto selected=choose_time(day,{21,30,15,999},2000);
    assert(selected && selected->day==42 && selected->hour==21 && selected->minute==30 && selected->second==15 && selected->millisecond==998 && selected->anchor==2000 && selected->rate==12);++checks;
    selected=choose_time(*stopped,{3,0,0,0},3000);assert(selected && selected->rate==0 && selected->hour==3);++checks;
    advanced=at_native_time(FieldTime{6,23,59,59,998,6,0,1,1000},1001);assert(advanced && advanced->day==7 && advanced->hour==0 && advanced->weekday==0);++checks;
    advanced=at_native_time(day,500);assert(advanced && advanced->anchor==1000 && advanced->second==0);++checks;
    assert(!choose_time(day,{24,0,0,0},2000));++checks;
    assert(!resume_time(day,12,2000));++checks;
    assert(!at_native_time(FieldTime{0xffffffffu,23,59,59,998,0,0,1,0},1));++checks;
    assert(!at_native_time(FieldTime{42,12,0,0,0,0,0,65535,0},std::numeric_limits<std::uint64_t>::max()));++checks;
    checks+=native_clock_fixture_checks();
    checks+=queue_fixture_checks();
    checks+=weather_fixture_checks();
    checks+=hook_fixture_checks();checks+=receipt_fixture_checks();
    checks+=advance_time_fixture_checks();
    std::printf("PASS: %u native-clock conversion/anchor/adapter checks. Owned fixtures only; no game code or memory was used.\n",checks);
}
