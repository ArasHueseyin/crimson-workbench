#include "../photo-mode/photo_policy.h"
#include "../photo-mode/photo_hooks.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <vector>

using namespace crimson::marketplace::photo;
namespace {
template<class T> void put(void* object,std::size_t offset,T value){std::memcpy(static_cast<unsigned char*>(object)+offset,&value,sizeof(value));}
struct Fixture {
    static inline Fixture* current{};
    void* image{};Address base{};
    std::vector<unsigned char> view=std::vector<unsigned char>(0x31400);
    std::array<unsigned char,0x100> panel{},panel_info{},instances{},info{},root{},delta{},registry{};
    std::array<unsigned char,0x140> panel_body{};
    std::array<unsigned char,0xe0> instance{};
    std::array<unsigned char,0x220> control{};
    std::array<unsigned char,0x270> body{};
    std::array<unsigned char,0x170> manager{};
    std::array<unsigned char,0x130> camera{};
    std::array<std::array<unsigned char,16>,3> descriptors{};
    std::array<void*,3> records{};
    void* instance_entry{};void* view_entry{};
    int lookup_calls{},get_calls{},enter_calls{},hide_calls{},show_calls{},leave_calls{},original_calls{};
    bool throw_lookup{},throw_enter{},panel_missing{};float scale=1.0f;void* other_view{};
    Fixture() {
        current=this;
        image=VirtualAlloc(nullptr,0x7000000,MEM_RESERVE,PAGE_NOACCESS);assert(image);base=reinterpret_cast<Address>(image);
        auto page=reinterpret_cast<void*>(base+(0x6d691e8&~Address(4095)));
        assert(VirtualAlloc(page,4096,MEM_COMMIT,PAGE_READWRITE)==page);
        put(image,0x6d691e8,root.data());put(root.data(),0x50,manager.data());put(root.data(),0x98,info.data());
        put(root.data(),0x58,delta.data());put(root.data(),0x68,registry.data());view_entry=view.data();
        put(registry.data(),0x78,&view_entry);put(registry.data(),0x80,std::uint32_t(1));
        put(delta.data(),0x64,0.016f);put(delta.data(),0x6c,0.016f);
        put(view.data(),0x31298,root.data());put(view.data(),0x31339,std::uint8_t(1));
        put(panel.data(),0xa0,panel_info.data());put(panel_info.data(),0x10,panel_body.data());
        put(panel_body.data(),0x120,instances.data());instance_entry=instance.data();
        put(instances.data(),0x40,&instance_entry);put(instances.data(),0x48,std::uint32_t(1));
        put(instance.data(),0xa0,info.data());put(instance.data(),0xd8,control.data());put(info.data(),0x10,body.data());
        put(control.data(),0,base+0x56f1968);put(control.data(),8,instance.data());
        put(control.data(),0x168,root.data());put(control.data(),0x170,panel.data());put(control.data(),0x200,instances.data());
        put(control.data(),0x219,std::uint8_t(1));put(body.data(),0x118,control.data());
        put(manager.data(),0,base+0x55ca3b8);put(manager.data(),0x10,camera.data());put(manager.data(),0x20,records.data());
        put(manager.data(),0x2c,std::uint32_t(3));put(camera.data(),0,base+0x55ca780);
        for(std::size_t i=0;i<3;++i){records[i]=descriptors[i].data();put(descriptors[i].data(),0,static_cast<std::uint8_t>(i));}
        put(descriptors[1].data(),8,camera.data());
    }
    ~Fixture(){assert(VirtualFree(image,0,MEM_RELEASE));current=nullptr;}
    void photo(bool active){put(manager.data(),0x28,std::uint32_t(active?2:1));scale=active?0.0f:1.0f;put(delta.data(),0x64,active?0.0f:0.016f);}
    static void* __fastcall lookup(void* view,const char* key){auto& f=*current;++f.lookup_calls;assert(view==f.view.data()&&reinterpret_cast<Address>(key)==f.base+0x56f13d0);if(f.throw_lookup)throw std::runtime_error("native lookup");return f.panel_missing?nullptr:f.panel.data();}
    static void* __fastcall get(void* control){auto& f=*current;++f.get_calls;assert(control==f.control.data());return f.other_view?f.other_view:f.view.data();}
    static float __fastcall timescale(void* view){assert(view==current->view.data());return current->scale;}
    static void __fastcall enter(void* view,bool value){auto& f=*current;++f.enter_calls;assert(view==f.view.data()&&value);if(f.throw_enter)throw std::runtime_error("native enter");}
    static void __fastcall hide(void* control){auto& f=*current;++f.hide_calls;assert(control==f.control.data());put(control,0x218,std::uint8_t(1));}
    static void __fastcall show(void* control){auto& f=*current;++f.show_calls;assert(control==f.control.data());put(control,0x218,std::uint8_t(0));}
    static void __fastcall input(void* control,void* from,void* clicked,bool cancelled,const void* payload){auto& f=*current;++f.leave_calls;assert(control==f.control.data()&&!from&&clicked==f.panel.data()&&!cancelled&&!payload);}
    static void __fastcall original(void* view){assert(view==current->view.data());++current->original_calls;}
    Native native(){return Native(base,{lookup,get,timescale,enter,hide,show,input});}
};
Reply status(Policy& p,std::uint64_t now=100){return p.handle(Request{},now);}
Request command(Policy& p,Operation op,std::uint64_t id,std::uint64_t now=100){auto s=status(p,now);Request r{};r.session=s.session;r.command=id;r.epoch=s.epoch;r.not_after_ms=now+1000;r.operation=op;return r;}
void native_shape_and_guards() {
    Fixture f;auto native=f.native();Frame frame{};
    assert(native.snapshot(f.view.data(),frame)&&!frame.photo&&!frame.frozen&&frame.token);
    f.photo(true);assert(native.snapshot(f.view.data(),frame)&&frame.photo&&frame.frozen);
    put(f.delta.data(),0x64,0.001f);assert(native.snapshot(f.view.data(),frame)&&frame.photo&&!frame.frozen);put(f.delta.data(),0x64,0.0f);
    put(f.delta.data(),0x6c,0.0f);assert(native.snapshot(f.view.data(),frame)&&!frame.frozen);put(f.delta.data(),0x6c,0.016f);
    f.view_entry=f.root.data();assert(!native.snapshot(f.view.data(),frame));f.view_entry=f.view.data();
    assert(native.snapshot(f.view.data(),frame));
    native.enter(frame);native.hide(frame);native.show(frame);native.leave(frame);
    assert(f.enter_calls==1&&f.hide_calls==1&&f.show_calls==1&&f.leave_calls==1);
    f.scale=0.25f;assert(native.snapshot(f.view.data(),frame)&&!frame.frozen);
    f.scale=0;put(f.manager.data(),0x28,std::uint32_t(3));
    assert(native.snapshot(f.view.data(),frame)&&!frame.photo&&frame.camera_busy);
    put(f.manager.data(),0x28,std::uint32_t(4));assert(!native.snapshot(f.view.data(),frame));
    put(f.manager.data(),0x28,std::uint32_t(2));
    put(f.camera.data(),0,f.base+0x55ca438);assert(!native.snapshot(f.view.data(),frame));put(f.camera.data(),0,f.base+0x55ca780);
    put(f.instances.data(),0x48,std::uint32_t(2));assert(native.snapshot(f.view.data(),frame)&&!frame.controls);put(f.instances.data(),0x48,std::uint32_t(1));
    f.other_view=f.root.data();assert(native.snapshot(f.view.data(),frame)&&!frame.controls);f.other_view=nullptr;
    put(f.body.data(),0x23c,1.0f);assert(native.snapshot(f.view.data(),frame)&&!frame.controls);put(f.body.data(),0x23c,0.0f);
    put(f.body.data(),0x118,f.root.data());assert(native.snapshot(f.view.data(),frame)&&!frame.controls);put(f.body.data(),0x118,f.control.data());
    auto page=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);assert(page);
    put(f.panel_info.data(),0x10,page);assert(native.snapshot(f.view.data(),frame)&&!frame.controls);put(f.panel_info.data(),0x10,f.panel_body.data());
    assert(!native.snapshot(page,frame));assert(VirtualFree(page,0,MEM_RELEASE));
    f.throw_lookup=true;bool propagated=false;try{native.snapshot(f.view.data(),frame);}catch(const std::runtime_error&){propagated=true;}assert(propagated);
}
void normal_lifecycle() {
    Fixture f;auto native=f.native();Policy policy(77);policy.observe(native,f.view.data(),100);
    auto enter=command(policy,Operation::enter,1);assert(policy.handle(enter,100).state==State::pending);
    assert(f.enter_calls==0);policy.observe(native,f.view.data(),110);assert(f.enter_calls==1&&status(policy,110).state==State::native_queued);
    for(int i=0;i<5;++i)policy.observe(native,f.view.data(),120+i);assert(f.enter_calls==1);
    f.photo(true);f.scale=0.5f;policy.observe(native,f.view.data(),130);assert(!status(policy,130).owned&&f.hide_calls==0);
    f.scale=0;policy.observe(native,f.view.data(),140);assert(f.hide_calls==1&&status(policy,140).owned);
    policy.observe(native,f.view.data(),150);auto s=status(policy,150);assert(s.native_photo&&s.owned&&s.hud_hidden&&s.scene_frozen&&s.state==State::hud_hidden);
    assert(policy.handle(enter,150).command==1&&f.enter_calls==1);
    auto show=command(policy,Operation::show_hud,2,150);policy.handle(show,150);policy.observe(native,f.view.data(),160);assert(f.show_calls==1);
    policy.observe(native,f.view.data(),170);assert(status(policy,170).state==State::hud_visible);
    auto leave=command(policy,Operation::leave,3,170);policy.handle(leave,170);policy.observe(native,f.view.data(),180);assert(f.leave_calls==1);
    policy.observe(native,f.view.data(),190);assert(status(policy,190).owned);f.photo(false);policy.observe(native,f.view.data(),200);
    s=status(policy,200);assert(!s.owned&&!s.native_photo&&!s.scene_frozen&&s.state==State::left);
    auto hide=command(policy,Operation::hide_hud,4,200);policy.handle(hide,200);policy.observe(native,f.view.data(),210);assert(status(policy,210).reason==Reason::not_owned&&f.hide_calls==1);
}
void stale_foreign_context_and_no_retry() {
    Fixture f;auto native=f.native();Policy policy(77);policy.observe(native,f.view.data(),100);
    auto bad=command(policy,Operation::enter,1);bad.session=78;assert(policy.handle(bad,100).reason==Reason::session);
    bad.session=77;bad.epoch++;assert(policy.handle(bad,100).reason==Reason::epoch);
    auto request=command(policy,Operation::enter,1);f.photo(true);policy.handle(request,100);policy.observe(native,f.view.data(),110);assert(status(policy,110).reason==Reason::foreign_photo&&f.enter_calls==0);
    f.photo(false);request=command(policy,Operation::enter,2,110);policy.handle(request,110);policy.observe(native,f.view.data(),120);assert(f.enter_calls==1);
    policy.observe(native,f.view.data(),1200);assert(status(policy,1200).state==State::uncertain);policy.observe(native,f.view.data(),1210);assert(f.enter_calls==1);
    f.photo(true);policy.observe(native,f.view.data(),1220);assert(!status(policy,1220).owned);
    f.photo(false);request=command(policy,Operation::enter,3,1220);policy.handle(request,1220);
    put(f.view.data(),0x312c4,std::uint8_t(1));policy.observe(native,f.view.data(),1230);put(f.view.data(),0x312c4,std::uint8_t(0));
    put(f.root.data(),0x98,f.panel_info.data()); // positively different native phase context
    policy.observe(native,f.view.data(),1240);assert(status(policy,1240).epoch!=request.epoch&&f.enter_calls==1);
    assert(policy.handle(request,1240).command==3); // idempotency never replays it
    request=command(policy,Operation::enter,4,1240);request.hide_on_enter=0;policy.handle(request,1240);f.throw_enter=true;
    bool propagated=false;try{policy.observe(native,f.view.data(),1250);}catch(const std::runtime_error&){propagated=true;}assert(propagated&&f.enter_calls==2);
    f.throw_enter=false;policy.observe(native,f.view.data(),1260);assert(f.enter_calls==2);
}
void missing_panel_and_unavailable_transition() {
    Fixture f;auto native=f.native();Policy policy(99);f.panel_missing=true;
    policy.observe(native,f.view.data(),100);auto r=command(policy,Operation::enter,1);auto epoch=r.epoch;
    policy.handle(r,100);policy.observe(native,f.view.data(),110);assert(f.enter_calls==1&&status(policy,110).state==State::native_queued);
    put(f.view.data(),0x312c4,std::uint8_t(1));policy.observe(native,f.view.data(),120);
    auto s=status(policy,120);assert(s.state==State::uncertain&&s.command==1&&s.epoch==epoch&&f.enter_calls==1);
    put(f.view.data(),0x312c4,std::uint8_t(0));f.photo(true);policy.observe(native,f.view.data(),130);
    s=status(policy,130);assert(s.owned&&s.native_photo&&s.scene_frozen&&s.epoch==epoch&&f.hide_calls==0);
    f.panel_missing=false;policy.observe(native,f.view.data(),140);assert(f.hide_calls==1&&f.enter_calls==1);
    policy.observe(native,f.view.data(),150);assert(status(policy,150).state==State::hud_hidden);
    auto leave=command(policy,Operation::leave,2,150);policy.handle(leave,150);f.panel_missing=true;
    policy.observe(native,f.view.data(),160);assert(f.leave_calls==0&&status(policy,160).state==State::pending);
    f.panel_missing=false;policy.observe(native,f.view.data(),170);assert(f.leave_calls==1);
    put(f.view.data(),0x312c4,std::uint8_t(1));policy.observe(native,f.view.data(),180);
    assert(status(policy,180).owned&&status(policy,180).command==2);
    put(f.view.data(),0x312c4,std::uint8_t(0));f.photo(false);policy.observe(native,f.view.data(),190);
    assert(status(policy,190).state==State::left&&!status(policy,190).owned&&f.enter_calls==1&&f.leave_calls==1);
}
void late_observations_cannot_complete_expired_commands() {
    {
        Fixture f;auto native=f.native();Policy policy(101);policy.observe(native,f.view.data(),100);
        auto enter=command(policy,Operation::enter,1);policy.handle(enter,100);policy.observe(native,f.view.data(),110);
        f.photo(true);policy.observe(native,f.view.data(),enter.not_after_ms+1);
        auto s=status(policy,enter.not_after_ms+1);
        assert(s.state==State::uncertain&&s.reason==Reason::timeout&&!s.owned&&s.native_photo&&s.scene_frozen);
        assert(f.enter_calls==1&&f.hide_calls==0);
        policy.observe(native,f.view.data(),enter.not_after_ms+2);s=policy.handle(enter,enter.not_after_ms+2);
        assert(s.state==State::uncertain&&s.reason==Reason::timeout&&!s.owned&&f.enter_calls==1&&f.hide_calls==0);
    }
    {
        Fixture f;auto native=f.native();Policy policy(102);policy.observe(native,f.view.data(),100);
        auto enter=command(policy,Operation::enter,1);enter.hide_on_enter=0;
        policy.handle(enter,100);policy.observe(native,f.view.data(),110);f.photo(true);policy.observe(native,f.view.data(),120);
        assert(status(policy,120).owned&&f.hide_calls==0);
        auto hide=command(policy,Operation::hide_hud,2,130);policy.handle(hide,130);policy.observe(native,f.view.data(),140);
        policy.observe(native,f.view.data(),hide.not_after_ms+1);auto s=status(policy,hide.not_after_ms+1);
        assert(s.state==State::uncertain&&s.reason==Reason::timeout&&s.owned&&s.hud_hidden&&f.hide_calls==1);
        policy.observe(native,f.view.data(),hide.not_after_ms+2);assert(policy.handle(hide,hide.not_after_ms+2).state==State::uncertain&&f.hide_calls==1);
        auto show=command(policy,Operation::show_hud,3,1140);policy.handle(show,1140);policy.observe(native,f.view.data(),1150);
        policy.observe(native,f.view.data(),show.not_after_ms+1);s=status(policy,show.not_after_ms+1);
        assert(s.state==State::uncertain&&s.reason==Reason::timeout&&s.owned&&!s.hud_hidden&&f.show_calls==1);
        policy.observe(native,f.view.data(),show.not_after_ms+2);assert(policy.handle(show,show.not_after_ms+2).state==State::uncertain&&f.show_calls==1);
        auto leave=command(policy,Operation::leave,4,2150);policy.handle(leave,2150);policy.observe(native,f.view.data(),2160);
        f.photo(false);policy.observe(native,f.view.data(),leave.not_after_ms+1);s=status(policy,leave.not_after_ms+1);
        assert(s.state==State::uncertain&&s.reason==Reason::timeout&&!s.owned&&!s.native_photo&&f.leave_calls==1);
        policy.observe(native,f.view.data(),leave.not_after_ms+2);s=policy.handle(leave,leave.not_after_ms+2);
        assert(s.state==State::uncertain&&s.reason==Reason::timeout&&!s.owned&&f.leave_calls==1);
    }
    {
        Fixture f;auto native=f.native();Policy policy(103);f.panel_missing=true;policy.observe(native,f.view.data(),100);
        auto enter=command(policy,Operation::enter,1);policy.handle(enter,100);policy.observe(native,f.view.data(),110);
        f.photo(true);policy.observe(native,f.view.data(),120);assert(status(policy,120).owned&&f.hide_calls==0);
        f.panel_missing=false;policy.observe(native,f.view.data(),enter.not_after_ms+1);auto s=status(policy,enter.not_after_ms+1);
        assert(s.state==State::uncertain&&s.reason==Reason::timeout&&s.owned&&f.hide_calls==0);
        policy.observe(native,f.view.data(),enter.not_after_ms+2);assert(f.enter_calls==1&&f.hide_calls==0);
    }
}
void exact_lease_dispatch() {
    Fixture f;auto native=f.native();Policy policy(88);
    dispatch(native,policy,Fixture::original,true,f.base,f.base+0xad2516,100,f.view.data());assert(f.original_calls==1&&f.lookup_calls==0);
    dispatch(native,policy,Fixture::original,false,f.base,f.base+0xad2502,100,f.view.data());assert(f.original_calls==2&&f.lookup_calls==0);
    dispatch(native,policy,Fixture::original,true,f.base,f.base+0xad2502,100,f.view.data());assert(f.original_calls==3&&f.lookup_calls==1);
}
struct Backend {
    bool fail_apply{},fail_disable{},pinned{},published{};int create_count{},remove_count{};
    bool pin(){pinned=true;return true;}bool initialize(){return true;}
    bool create(Address,Address,Address& value){assert(pinned);++create_count;value=12;return true;}
    bool queue_enable(Address){assert(published);return true;}bool apply(){return !fail_apply;}
    bool disable(Address){return !fail_disable;}bool remove(Address){++remove_count;return true;}bool uninitialize(){return true;}
};
void hook_transaction() {
    Backend b;std::atomic<bool> enabled{};HookTransaction hooks(b,enabled);
    assert(hooks.install(100,{10},[]{return true;},[&](const auto& v){b.published=true;return v[0]==12;}));assert(enabled&&b.create_count==1);
    assert(hooks.cleanup()&&!enabled&&b.remove_count==0);
    Backend fail;fail.fail_apply=true;std::atomic<bool> inactive{};HookTransaction failed(fail,inactive);
    assert(!failed.install(100,{10},[]{return true;},[&](const auto&){fail.published=true;return true;}));assert(!inactive&&fail.remove_count==0);
}
}
int main(){native_shape_and_guards();normal_lifecycle();stale_foreign_context_and_no_retry();missing_panel_and_unavailable_transition();late_observations_cannot_complete_expired_commands();exact_lease_dispatch();hook_transaction();std::puts("Photo mode native adapter, lifecycle, ownership, lease, stale-command and passthrough tests passed.");}
