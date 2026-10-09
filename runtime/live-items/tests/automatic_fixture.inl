// Actual build2976 task dispatcher + wrapper + MinHook trampoline, in the own
// process arena. Lookup, actor release, definitions and inventory are fixtures.
// This proves ABI/callback/lease ordering, not the real game's synchronization.
namespace {
std::array<unsigned char,0x110> auto_owner{},auto_client{};
std::array<unsigned char,0x200> auto_components{};
std::array<unsigned char,0xe0> auto_possessor{};
std::array<unsigned char,0xe0> auto_client_possessor{};
std::array<unsigned char,0x100> auto_manager{},auto_root{},auto_job{},auto_task{};
std::array<unsigned char,0x40> auto_holder{},auto_bucket{};
std::array<unsigned char,0xc8> auto_slot{};
unsigned char* auto_buckets[]={auto_bucket.data()};
std::array<unsigned char,0x40> auto_metadata{};
std::array<std::uintptr_t,3> auto_task_vtable{};
std::optional<Queue> auto_queue;
Bindings auto_bindings{};TaskDispatch auto_original{};
using Driver=void(__fastcall*)(void*,void*,bool,bool*);
Driver auto_driver{};
unsigned auto_leases{},auto_jobs{},auto_grants{},auto_callbacks{};
int auto_job_error{};bool auto_lookup_valid=true,auto_grant_fail=false;
std::uint64_t auto_now{};
template<class T,std::size_t N>void auto_put(std::array<unsigned char,N>& a,std::size_t at,T value){assert(at+sizeof(T)<=N);memcpy(a.data()+at,&value,sizeof(value));}
bool __fastcall auto_release(void*){assert(auto_leases);--auto_leases;return false;}
void __fastcall auto_job_failed(void* owner){assert(owner==auto_owner.data()&&auto_leases);}
void __fastcall auto_release_receipt(void* p){auto a=static_cast<unsigned char*>(p);if(a[0x10]){assert(auto_leases);--auto_leases;a[0x10]=0;}}
void* __fastcall auto_manager_lookup(){return auto_manager.data();}
void* __fastcall auto_lookup(void* manager,void* out,const std::uint32_t* handle,const std::uint16_t*){
    assert(manager==auto_manager.data()&&*handle==0xa0100001);
    memset(out,0,0x30);auto a=static_cast<unsigned char*>(out);
    *reinterpret_cast<std::uintptr_t*>(a)=fixture_base+0x5725e48;
    if(auto_lookup_valid){*reinterpret_cast<void**>(a+8)=auto_owner.data();a[0x10]=1;++auto_leases;}return out;
}
int* __fastcall auto_task_run(void* task,int* error,void* context){
    assert(task==auto_task.data()&&*reinterpret_cast<void**>(context)==auto_owner.data()&&auto_leases);
    ++auto_jobs;*error=auto_job_error;return error;
}
void auto_after(void* context,int* error,std::uintptr_t caller){
    ++auto_callbacks;assert(auto_leases);
    pump(*auto_queue,auto_bindings,fixture_base,context,error,caller,auto_now);
}
int* __fastcall auto_hook(int* error,void* task,void* context){
    return forward_task(auto_original,auto_after,error,task,context,reinterpret_cast<std::uintptr_t>(_ReturnAddress()));
}
bool __fastcall auto_item_lookup(std::uint16_t* type,std::uint32_t key){*type=41;return key==2200;}
int* __fastcall auto_grant(void* holder,int* error,const void* init){
    assert(holder==auto_holder.data()&&auto_leases==1);++auto_grants;
    // Re-enter the *actual* server driver. Its original task still executes,
    // while the shared production pump suppresses a nested grant.
    bool proceed{};auto_driver(nullptr,auto_job.data(),false,&proceed);assert(proceed&&auto_leases==1);
    *error=auto_grant_fail?55:0;
    if(!auto_grant_fail)auto_put(auto_slot,0x10,get<std::int64_t>(auto_slot,0x10)+*reinterpret_cast<const std::int64_t*>(static_cast<const unsigned char*>(init)+0x10));
    return error;
}
Request auto_request(unsigned id=1){Request r;r.operation=Operation::grant;r.id[0]=static_cast<std::uint8_t>(id);r.key=2200;r.quantity=1;r.epoch=456;return r;}
void auto_tick(){bool proceed{};auto_driver(nullptr,auto_job.data(),false,&proceed);assert(proceed&&!auto_leases);}
}
void automatic_fixture(Source& source,Arena& arena){
    arena.load(source,0x2774730,0x3ed,"f6ad2ead93d7bc7ac743ad7e03a11c3c33f9ab0ba01dd52b3b11459dee9d821e");
    arena.load(source,0x2774b20,0x38,"8ef6c164fd9207e3ec484b894d6544ff3e30b279977535a172fface8eb7cf030");
    arena.redirect(0x2775800,reinterpret_cast<std::uintptr_t>(&auto_manager_lookup));
    arena.redirect(0x2775950,reinterpret_cast<std::uintptr_t>(&auto_lookup));
    arena.redirect(0x1434880,reinterpret_cast<std::uintptr_t>(&auto_release_receipt));
    auto ref=std::array<std::uintptr_t,3>{0,0,fixture_base+0x1434880};
    arena.write(0x5b04520,std::span(reinterpret_cast<const std::uint8_t*>(ref.data()),sizeof(ref)),PAGE_READONLY);
    arena.write(0x5725e48,std::span(reinterpret_cast<const std::uint8_t*>(ref.data()),sizeof(ref)),PAGE_READONLY);
    std::array<std::uintptr_t,20> actor_vtable{};actor_vtable[1]=reinterpret_cast<std::uintptr_t>(&auto_release);actor_vtable[19]=reinterpret_cast<std::uintptr_t>(&auto_job_failed);
    arena.write(0x5b1e2b0,std::span(reinterpret_cast<const std::uint8_t*>(actor_vtable.data()),sizeof(actor_vtable)),PAGE_READONLY);
    auto root=auto_root.data();arena.write(0x6d69190,std::span(reinterpret_cast<const std::uint8_t*>(&root),sizeof(root)),PAGE_READONLY);
    auto_put(auto_root,0x30,auto_manager.data());auto_put(auto_manager,0,fixture_base+0x55b62c8);auto_put(auto_manager,0x50,auto_client.data());
    auto_put(auto_client,0,fixture_base+0x5585c40);auto_put(auto_client,0x60,0xa0100001u);auto_client[0x96]=1;
    auto_put(auto_client,0x88,auto_metadata.data());auto_put(auto_client,0xa0,auto_client_possessor.data());auto_put(auto_client_possessor,0xd0,auto_client.data());
    auto_put(auto_owner,0,fixture_base+0x5b1e2b0);auto_put(auto_owner,0x60,0xa0100001u);
    auto_put(auto_owner,0x68,auto_components.data());auto_put(auto_owner,0x88,auto_metadata.data());auto_put(auto_owner,0xa0,auto_possessor.data());auto_owner[0x96]=1;auto_metadata[1]=1;
    auto_put(auto_possessor,0xd0,auto_owner.data());auto_put(auto_components,0xb8,auto_holder.data());auto_put(auto_holder,8,auto_owner.data());
    auto_put(auto_holder,0x18,auto_buckets);auto_put(auto_holder,0x20,1u);auto_put(auto_bucket,0,auto_slot.data());auto_put(auto_bucket,8,std::uint16_t{1});auto_put(auto_slot,8,std::uint16_t{41});auto_put(auto_slot,0x10,std::int64_t{20});
    auto_task_vtable[2]=reinterpret_cast<std::uintptr_t>(&auto_task_run);auto_put(auto_task,0,auto_task_vtable.data());auto_put(auto_job,0x50,0xa0100001u);auto_put(auto_job,0x60,auto_task.data());
    definition[0x244]=0;definition[0x3f0]=1;auto_put(definition,0x42,std::uint16_t{41});
    auto_bindings={auto_item_lookup,resolve,arena.function<Construct>(0x2409970),arena.function<Convert>(0x2449730),arena.function<Destroy>(0xf493f00),auto_grant};
    auto_driver=arena.function<Driver>(0x2774730);
    auto target=arena.function<void*>(0x2774b20);
    assert(MH_Initialize()==MH_OK&&MH_CreateHook(target,reinterpret_cast<void*>(&auto_hook),reinterpret_cast<void**>(&auto_original))==MH_OK&&MH_EnableHook(target)==MH_OK);
    Request status;auto r=auto_request();
    {
        Tls tls;tls.local[0x1ec]=1;*reinterpret_cast<void**>(tls.local.data()+0x250)=auto_root.data();
        auto_queue.emplace(123,456);auto_tick();assert(auto_jobs==1&&auto_callbacks==1&&auto_queue->handle(status,0).state==State::ready);
        assert(auto_queue->handle(r,1).state==State::queued);auto_now=2;auto_tick();
        assert(auto_grants==1&&auto_jobs==3&&auto_callbacks==3&&get<std::int64_t>(auto_slot,0x10)==21&&auto_queue->handle(r,2).state==State::applied&&allocations.empty());
        // Repeated UUID and repeated ordinary ticks never repeat the write.
        assert(auto_queue->handle(r,3).state==State::applied);auto_now=4;auto_tick();assert(auto_grants==1);
        for(unsigned which=0;which<7;which++){
            auto_queue.emplace(123,456);auto_now=10;
            if(which==0)auto_job_error=77;
            if(which==1)tls.local[0x1ec]=0;
            if(which==2)*reinterpret_cast<void**>(tls.local.data()+0x250)=nullptr;
            if(which==3)auto_put(auto_client,0x60,0xa0100002u);
            if(which==4)auto_owner[0x96]=0;
            if(which==5)auto_metadata[1]=4;
            if(which==6)auto_lookup_valid=false;
            auto_tick();assert(auto_queue->handle(status,10).state==State::starting&&auto_grants==1);
            auto_job_error=0;tls.local[0x1ec]=1;*reinterpret_cast<void**>(tls.local.data()+0x250)=auto_root.data();auto_put(auto_client,0x60,0xa0100001u);auto_owner[0x96]=1;auto_metadata[1]=1;auto_lookup_valid=true;
        }
        auto_queue.emplace(123,456);auto_now=20;auto_tick();assert(auto_queue->handle(r,21).state==State::queued);
        auto_now=5021;auto_tick();assert(auto_grants==1&&auto_queue->handle(r,5021).error==Error::context);
        // Pause: pipe-side pure selection reads keep the SAME observed owner
        // available. Returning to gameplay consumes the request automatically.
        auto_queue.emplace(123,456);auto_now=5100;auto_tick();
        auto_queue->confirm_selection(10000,selected_identity(fixture_base));assert(auto_queue->handle(r,10001).state==State::queued);
        auto_now=10002;auto_tick();assert(auto_grants==2&&auto_queue->handle(r,10002).state==State::applied);
        auto_queue.emplace(123,456);auto_now=11000;auto_tick();assert(auto_queue->handle(r,11001).state==State::queued);
        auto_queue->confirm_selection(11002,0);assert(auto_queue->handle(r,11002).error==Error::context&&auto_queue->handle(status,11002).state==State::starting);
        auto_queue.emplace(123,456);auto_now=6000;auto_tick();assert(auto_queue->handle(r,6001).state==State::queued);
        auto_grant_fail=true;auto_now=6002;auto_tick();auto_grant_fail=false;
        assert(auto_grants==3&&auto_queue->handle(r,6002).state==State::uncertain&&auto_queue->handle(auto_request(2),6003).error==Error::unavailable);
        std::uint32_t identity{};void* context=auto_owner.data();int ok=0;
        assert(!task_player(fixture_base,&context,&ok,fixture_base+1,&identity));
        assert(task_player(fixture_base,&context,&ok,fixture_base+0x27742ae,&identity)==auto_holder.data()&&identity==0xa0100001);
    }
    assert(MH_DisableHook(target)==MH_OK&&MH_RemoveHook(target)==MH_OK&&MH_Uninitialize()==MH_OK);
    std::puts("PASS: actual server dispatcher/wrapper and MinHook; lease held across automatic grant, original ABI/order, nested dispatch, UUID replay, wrong owner/realm/pool, load gap and fault latch. Own process only.");
}
