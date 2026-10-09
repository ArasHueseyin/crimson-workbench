#undef NDEBUG
#include "../src/native.h"
#include "../src/dispatch.h"
#include <cassert>
#include <atomic>
#include <cstdio>
#include <thread>
using namespace crimson::live_items;
namespace {
std::array<unsigned char,0x480> def{};
std::array<unsigned char,0x40> holder{},bucket{};
std::array<unsigned char,0xc8> slot{};
unsigned char* buckets[]={bucket.data()};
unsigned constructs{},converts{},destroys{},grants{};bool fail{},bad_conversion{},bad_delta{};
unsigned forwarded{},after_calls{};int original_error{},original_result{};
int* __fastcall original(void* context,int* error,void* owner,void* label,void* requests,bool a6,std::uint64_t a7,void* a8,std::uint16_t a9,bool a10,std::uint16_t a11,bool a12,bool a13){
    ++forwarded;assert(context==def.data()&&error==&original_error&&owner==holder.data()&&label==bucket.data()&&requests==slot.data()&&a6&&a7==0x123456789abcdef0&&a8==buckets&&a9==0x789a&&!a10&&a11==0xcdef&&a12&&!a13);*error=71;return &original_result;
}
void after(void* context,int* error,void* owner){++after_calls;assert(forwarded==1&&context==def.data()&&error==&original_error&&owner==holder.data()&&*error==71);}
bool __fastcall lookup(std::uint16_t* out,std::uint32_t key){*out=41;return key==2200;}
unsigned char* __fastcall definition(const std::uint16_t*){return def.data();}
void* __fastcall construct(void* out,const std::uint16_t*,std::int64_t qty){++constructs;auto p=static_cast<unsigned char*>(out);*reinterpret_cast<std::int64_t*>(p)=-1;*reinterpret_cast<std::int64_t*>(p+0x10)=qty;return out;}
void* __fastcall convert(void* out,const void* val,std::uint16_t){++converts;auto p=static_cast<unsigned char*>(out);*reinterpret_cast<std::int64_t*>(p)=-1;*reinterpret_cast<std::uint32_t*>(p+8)=bad_conversion?999:2200;*reinterpret_cast<std::int64_t*>(p+0x10)=*reinterpret_cast<const std::int64_t*>(static_cast<const unsigned char*>(val)+0x10);return out;}
void __fastcall destroy(void*){++destroys;}
int* __fastcall grant(void* h,int* err,const void* init){assert(h==holder.data());++grants;*err=fail?55:0;if(!fail&&!bad_delta)*reinterpret_cast<std::int64_t*>(slot.data()+0x10)+=*reinterpret_cast<const std::int64_t*>(static_cast<const unsigned char*>(init)+0x10);return err;}
Request req(unsigned n=1){Request r;r.operation=Operation::grant;r.id[0]=static_cast<unsigned char>(n);r.key=2200;r.quantity=10;r.epoch=456;return r;}
}
int main(){
    assert(forward_inventory(original,after,def.data(),&original_error,holder.data(),bucket.data(),slot.data(),true,0x123456789abcdef0,buckets,0x789a,false,0xcdef,true,false)==&original_result);
    assert(forwarded==1&&after_calls==1&&original_error==71);
    assert(!completed_context(nullptr,&original_error,holder.data()));
    Queue q(123,456);auto r=req();assert(q.handle(r,0).error==Error::unavailable);q.observe(0);
    assert(q.handle(r,0).state==State::queued);assert(q.handle(r,1).state==State::queued);
    auto changed=r;changed.quantity++;assert(q.handle(changed,1).error==Error::invalid_request);
    assert(q.handle(req(2),1).error==Error::busy);
    std::atomic<unsigned> taken{};std::vector<std::thread> racers;
    for(int i=0;i<16;i++)racers.emplace_back([&]{if(q.take(2))taken++;});for(auto& t:racers)t.join();assert(taken==1);
    q.finish(r,State::applied,Error::none);assert(q.handle(r,3).state==State::applied);assert(!q.take(3));
    r=req(2);q.handle(r,4);assert(q.handle(r,30004).state==State::expired);assert(!q.take(30004));
    q.observe(30005);r=req(3);q.handle(r,30005);auto cancel=r;cancel.operation=Operation::cancel;assert(q.handle(cancel,30006).state==State::cancelled);assert(!q.take(30006));
    r=req(4);q.handle(r,30007);assert(q.take(30007));q.finish(r,State::uncertain,Error::native_fault);assert(q.handle(req(5),30008).error==Error::unavailable);
    Request status;assert(q.handle(status,30008).state==State::uncertain);
    auto malformed=req();malformed.magic++;assert(q.handle(malformed,0).error==Error::protocol);
    malformed=req();malformed.quantity=10001;Queue fresh(1,456);fresh.observe(0);assert(fresh.handle(malformed,0).error==Error::invalid_request);
    malformed=req();malformed.epoch++;assert(fresh.handle(malformed,0).error==Error::unavailable);assert(!fresh.take(0));
    malformed=req();malformed.version=1;assert(fresh.handle(malformed,0).error==Error::protocol);
    Queue identity(123,456);identity.observe(10,77);r=req();assert(identity.handle(r,11).state==State::queued);
    assert(!identity.take(12,78));identity.observe(13,78);assert(identity.handle(r,14).error==Error::context);assert(!identity.take(14,78));
    identity.observe(15,78);r=req(2);assert(identity.handle(r,16).state==State::queued);
    assert(identity.handle(status,5016).state==State::starting);assert(!identity.take(5016,78));
    identity.observe(5016,78);assert(identity.handle(r,5017).error==Error::context);
    assert(identity.handle(req(3),5017).state==State::queued);identity.observe(35017,78);assert(identity.handle(req(3),35017).state==State::expired);
    *reinterpret_cast<unsigned char***>(holder.data()+0x18)=buckets;*reinterpret_cast<unsigned*>(holder.data()+0x20)=1;
    *reinterpret_cast<unsigned char**>(bucket.data())=slot.data();*reinterpret_cast<std::uint16_t*>(bucket.data()+8)=1;
    *reinterpret_cast<std::uint16_t*>(slot.data()+8)=41;*reinterpret_cast<std::int64_t*>(slot.data()+0x10)=20;
    *reinterpret_cast<unsigned*>(def.data())=2200;*reinterpret_cast<std::uint16_t*>(def.data()+0x42)=41;def[0x3f0]=1;
    Bindings b{lookup,definition,construct,convert,destroy,grant};r=req();
    auto outcome=execute(b,holder.data(),r);assert(outcome.state==State::applied&&constructs==1&&converts==1&&destroys==1&&grants==1);
    def[0x244]=6;assert(execute(b,holder.data(),r).error==Error::socket_limit);assert(grants==1);def[0x244]=0;
    def[0x3f0]=0;r.quantity=101;assert(execute(b,holder.data(),r).error==Error::quantity_limit);assert(grants==1);def[0x3f0]=1;r.quantity=10;
    bad_conversion=true;assert(execute(b,holder.data(),r).state==State::rejected);assert(grants==1&&destroys==2);bad_conversion=false;
    fail=true;assert(execute(b,holder.data(),r).state==State::uncertain);fail=false;
    bad_delta=true;assert(execute(b,holder.data(),r).error==Error::inventory_mismatch);bad_delta=false;
    // Unset optional +0x42 must not reject otherwise valid items (Bag 6003).
    *reinterpret_cast<std::uint16_t*>(def.data()+0x42)=0xffff;
    auto grants_before=grants;assert(execute(b,holder.data(),r).state==State::applied&&grants==grants_before+1);
    *reinterpret_cast<unsigned*>(def.data()+0x228)=6;assert(execute(b,holder.data(),r).error==Error::socket_limit);
    *reinterpret_cast<unsigned*>(def.data()+0x228)=0;
    *reinterpret_cast<unsigned*>(def.data())=999;assert(execute(b,holder.data(),r).error==Error::definition_mismatch);
    assert(!server_holder(0,nullptr));assert(!server_holder(0,reinterpret_cast<void*>(1)));
    std::int64_t sum{};assert(!total(nullptr,41,&sum));
    std::puts("PASS: concurrent exactly-once dequeue; replay, payload tampering, expiry, cancellation, fault latch; native construction/cleanup order, quantity/socket limits and inventory delta.");
}
