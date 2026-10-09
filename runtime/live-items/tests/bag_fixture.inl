// Extra-large Bag 6003 scalar defaults observed read-only in build2976.
// Actual constructor, default attributes, InitData conversion and destructor;
// only tables, clock, allocator and the final inventory writer are fixtures.
namespace {
std::array<unsigned char,0x210> bag_stats{};
std::array<unsigned char,0x30> bag_holder{};
std::array<unsigned char,0x10> bag_bucket{};
std::array<unsigned char,0xc8> bag_slot{};
unsigned char* bag_buckets[]{bag_bucket.data()};unsigned bag_grants{};unsigned bag_key=6003;std::uint16_t bag_stat_key=0x21c3;
unsigned char* __fastcall bag_duration(const std::uint16_t* key){assert(*key==bag_stat_key);return bag_stats.data();}
std::int64_t __fastcall bag_clock(){return 1000000;}
bool __fastcall bag_lookup(std::uint16_t* type,std::uint32_t key){*type=41;return key==bag_key;}
int* __fastcall bag_grant(void* holder,int* error,const void* init){
    assert(holder==bag_holder.data());auto p=static_cast<const unsigned char*>(init);
    assert(*reinterpret_cast<const std::int64_t*>(p)==-1&&*reinterpret_cast<const std::uint32_t*>(p+8)==bag_key&&p[0x5e]==0);
    assert(*reinterpret_cast<const std::uint16_t*>(p+0x2a)==0xffff);++bag_grants;
    *reinterpret_cast<std::int64_t*>(bag_slot.data()+0x10)+=*reinterpret_cast<const std::int64_t*>(p+0x10);*error=0;return error;
}
void bag_fixture(Source& source,Arena& arena){
    auto previous=definition;definition.fill(0);
    *reinterpret_cast<unsigned*>(definition.data())=6003;
    *reinterpret_cast<std::uint16_t*>(definition.data()+0x42)=0xffff;
    *reinterpret_cast<std::uint16_t*>(definition.data()+0x128)=0x21c3;
    *reinterpret_cast<std::uint16_t*>(definition.data()+0x400)=0xffff;
    definition[0x240]=0x12;
    for(unsigned n=0;n<3;n++)*reinterpret_cast<unsigned*>(definition.data()+0x330+n*4)=1;
    arena.load(source,0x2409240,5,"3b4cabd96df2f8775519e39bc2c1bde5c6bb765c33ba6399d42e53b0466b3f2f");
    arena.load(source,0xf48ff10,0x60,"fb1733fd822f2ad94f46b7201386f698289dab15a0d315188abdc1e02d211c30");
    arena.load(source,0x24096d0,0xc2,"41446658e551723c22c6572f57d2a26527dfbba4ac3024eb85935bd0abd9acfa");
    const std::array<std::uint8_t,8> zero{};arena.write(0x6d69438,zero,PAGE_READONLY);arena.write(0x6cef5b8,zero,PAGE_READONLY);
    arena.redirect(0x3885b0,reinterpret_cast<std::uintptr_t>(&bag_duration));arena.redirect(0x1416b80,reinterpret_cast<std::uintptr_t>(&bag_clock));
    *reinterpret_cast<unsigned char***>(bag_holder.data()+0x18)=bag_buckets;*reinterpret_cast<unsigned*>(bag_holder.data()+0x20)=1;
    *reinterpret_cast<unsigned char**>(bag_bucket.data())=bag_slot.data();*reinterpret_cast<std::uint16_t*>(bag_bucket.data()+8)=1;
    *reinterpret_cast<std::uint16_t*>(bag_slot.data()+8)=41;
    Bindings b{bag_lookup,resolve,arena.function<Construct>(0x2409970),arena.function<Convert>(0x2449730),arena.function<Destroy>(0xf493f00),bag_grant};
    Request request;request.key=6003;
    {Tls tls;tls.local[0x1ec]=1;for(unsigned quantity:{1,50}) {request.quantity=quantity;phase="bag execute";auto result=execute(b,bag_holder.data(),request);assert(result.state==State::applied&&result.error==Error::none&&allocations.empty());}}
    assert(bag_grants==2&&*reinterpret_cast<std::int64_t*>(bag_slot.data()+0x10)==51);
    // Lightning Arrow has the same unset field and native scalar defaults,
    // but a different stats-definition key. Exercise its complete path too.
    bag_key=1001315;bag_stat_key=0x21cb;*reinterpret_cast<unsigned*>(definition.data())=bag_key;
    *reinterpret_cast<std::uint16_t*>(definition.data()+0x128)=bag_stat_key;request.key=bag_key;
    {Tls tls;tls.local[0x1ec]=1;for(unsigned quantity:{1,100}){request.quantity=quantity;phase="arrow execute";assert(execute(b,bag_holder.data(),request).state==State::applied&&allocations.empty());}}
    assert(bag_grants==4&&*reinterpret_cast<std::int64_t*>(bag_slot.data()+0x10)==152);definition=previous;
    std::cout<<"PASS: bag6003 and lightning-arrow1001315 actual native defaults/constructor/conversion/destructor; optional0xffff, no sockets, quantities1/50/100; mocked table, clock and final inventory writer.\n";
}
}
