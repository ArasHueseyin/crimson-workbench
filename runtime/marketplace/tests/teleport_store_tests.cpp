// Standalone receipt/persistence regressions. No executable bytes, engine
// bindings, game files, or AppData paths are used. All native calls are mocks.
#include "../teleport/teleport_policy.h"
#include "../teleport/teleport_store.h"
#include "../teleport/persistence_revision.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace crimson::marketplace::teleport;
namespace {
unsigned checks{};
void check(bool value,const char* message){++checks;if(!value){std::fprintf(stderr,"FAILED: %s\n",message);std::abort();}}

struct OwnedTemporaryFile {
    std::filesystem::path path;
    OwnedTemporaryFile(){
        wchar_t directory[MAX_PATH]{},file[MAX_PATH]{};
        const auto count=GetTempPathW(MAX_PATH,directory);
        check(count&&count<MAX_PATH,"temporary directory is available");
        check(GetTempFileNameW(directory,L"CWT",0,file)!=0,"reserve a unique owned temporary file");
        path=file;
    }
    OwnedTemporaryFile(const OwnedTemporaryFile&)=delete;
    OwnedTemporaryFile& operator=(const OwnedTemporaryFile&)=delete;
    ~OwnedTemporaryFile(){
        DeleteFileW(path.c_str());auto staging=path;staging+=L".tmp";DeleteFileW(staging.c_str());
    }
};

struct ReceiptFixture {
    static inline ReceiptFixture* current{};
    std::array<std::uint8_t,0x700> actor{},transform{},field{},character{};
    std::array<std::uint8_t,0x200> components{},possessor{};
    std::array<std::uint8_t,2> metadata{0,1};
    std::array<std::uintptr_t,0x150/8> table{};std::array<std::uintptr_t,5> lock_table{};
    std::uintptr_t* lock_object{};void* actor_pointer{};int task_error{};
    NativeBindings bindings{};crimson::live_items::TaskObservationV1 task{};
    CompressedWorldPosition observed{10,20,30,0,0};
    unsigned acquires{},releases{},references{},begins{};
    template<class T>static void put(void* object,std::size_t offset,const T& value){std::memcpy(static_cast<std::uint8_t*>(object)+offset,&value,sizeof(value));}
    static FieldLease* __fastcall acquire(void*,FieldLease* out){
        auto& f=*current;++f.acquires;*out={0,f.field.data(),1,0,{},0,nullptr,f.transform.data()};
        put(f.transform.data(),0x54a,std::int16_t{1});return out;
    }
    static void __fastcall release(void*){
        auto& f=*current;++f.releases;put(f.transform.data(),0x54a,std::int16_t{0});
        f.transform[0x548]=0;put(f.transform.data(),0x550,NativeFieldMoveValue{});
    }
    static void __fastcall reference(FieldLease*){++current->references;}
    static std::uint16_t* __fastcall stage(void*,std::uint16_t* out){*out=17;return out;}
    static const std::byte* __fastcall stage_info(const std::uint16_t*){static const std::uint32_t definition=555;return reinterpret_cast<const std::byte*>(&definition);}
    static CompressedWorldPosition* __fastcall position(void*,CompressedWorldPosition* out){*out=current->observed;return out;}
    static float __fastcall yaw(void*){return 0.25f;}
    static void** __fastcall read_lock(void** out,void* node){*out=node;return out;}
    static void __fastcall unlock(void*,bool read){check(read,"snapshot unlock uses the read-lock ABI");}
    static int* __fastcall validate(void*,int* out){*out=0;return out;}
    static int* __fastcall begin(void* context,int* error,const NativeFieldMoveValue* value){
        ++current->begins;put(context,0x60,*value);static_cast<std::uint8_t*>(context)[0x58]=1;*error=0;return error;
    }
    static std::uint8_t realm(){return 1;}
    static std::uint32_t selected(std::uintptr_t){return 123;}
    ReceiptFixture(){
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
    ~ReceiptFixture(){current=nullptr;}
    void observe(TeleportPolicy& policy,std::uint64_t now){task.monotonic_ms=now;policy.observe(bindings,task);}
    static Request request(const Reply& state,std::uint64_t command,Operation operation,std::uint64_t deadline){
        Request result{};result.session=state.session;result.command=command;result.epoch=state.epoch;result.not_after_ms=deadline;result.operation=operation;return result;
    }
};

void lost_receipt_and_late_arrival(){
    ReceiptFixture f;TeleportPolicy policy(777);f.observe(policy,1000);
    auto state=policy.handle({},1001);auto save=f.request(state,1,Operation::save_current,4000);std::memcpy(save.name.data(),"Owned",6);
    check(policy.handle(save,1002).state==State::pending,"capture is queued");
    check(policy.handle(save,1002).state==State::pending,"lost pending capture receipt does not create another command");
    f.observe(policy,1003);state=policy.handle(save,1004);
    check(state.state==State::captured&&state.bookmarks_revision==1,"capture receipt has exactly one new revision");
    check(policy.handle(save,1004).bookmarks_revision==1,"repeated capture receipt cannot advance persistence revision");
    f.observed.x=9000;f.observe(policy,1005);auto travel=f.request(state,2,Operation::travel_saved,4000);
    check(policy.handle(travel,1006).state==State::pending,"saved travel is queued");
    check(policy.handle(travel,1006).state==State::pending&&!f.begins,"lost pending travel receipt does not call Begin");
    f.observe(policy,1007);state=policy.handle(travel,1008);
    check(state.state==State::native_queued&&f.begins==1,"separate observation attempts one Begin");
    auto altered=travel;altered.not_after_ms=3999;
    check(policy.handle(altered,1009).reason==Reason::command_order&&f.begins==1,"altered same-ID payload cannot replay a command");
    check(policy.handle(save,1009).reason==Reason::command_order,"older command ID is rejected");
    f.observed.x=10;f.observe(policy,61008);state=policy.handle(travel,61009);
    check(state.state==State::uncertain&&state.reason==Reason::timeout&&f.begins==1,"first position observation after deadline is not late success");
    f.observe(policy,61010);state=policy.handle(travel,61011);
    check(state.state==State::uncertain&&f.begins==1,"late arrival and repeated receipt never replay Begin");
    check(f.acquires==f.releases&&f.releases==f.references,"receipt processing balances all completed field borrows");
}

void pending_deadline_does_not_replay(){
    ReceiptFixture f;TeleportPolicy policy(778);f.observe(policy,1000);const auto state=policy.handle({},1001);
    auto save=f.request(state,1,Operation::save_current,1100);std::memcpy(save.name.data(),"Owned",6);
    check(policy.handle(save,1002).state==State::pending,"short-lived capture is initially accepted");
    f.observe(policy,1101);const auto expired=policy.handle(save,1102);
    check(expired.state==State::uncertain&&expired.reason==Reason::timeout&&!expired.bookmarks_revision,"expired pending capture creates no bookmark revision");
    f.observe(policy,1103);check(!policy.handle(save,1104).bookmarks_revision&&!f.begins,"polling an expired capture never executes it later");
}

// Deliberately synthetic build tokens; these are not executable hashes.
constexpr char kBuild[]="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
constexpr char kOtherBuild[]="bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
std::array<Bookmark,kSavedSlots> bookmarks(){
    std::array<Bookmark,kSavedSlots> values{};values[0].occupied=1;
    values[0].position={555,17,0,{10,20,30},0.25f,1,0};std::memcpy(values[0].name.data(),"Owned DPAPI",12);return values;
}
bool equal(const std::array<Bookmark,kSavedSlots>& a,const std::array<Bookmark,kSavedSlots>& b){return !std::memcmp(a.data(),b.data(),sizeof(a));}
void replace_owned_file(const std::filesystem::path& path){std::ofstream file(path,std::ios::binary|std::ios::trunc);check(bool(file<<"owned corrupt replacement"),"write corruption only to the owned temporary file");}

void dpapi_roundtrip_and_explicit_repair(){
    OwnedTemporaryFile temporary;const auto values=bookmarks();BookmarkStore first(temporary.path);
    first.load(kBuild);check(first.save(values,kBuild),"DPAPI writes an owned native-shaped bookmark");
    BookmarkStore loaded(temporary.path);check(equal(loaded.load(kBuild),values),"DPAPI roundtrip preserves the exact schema payload");
    BookmarkStore wrong(temporary.path);check(!wrong.load(kOtherBuild)[0].occupied,"another synthetic build cannot restore the bookmark");
    replace_owned_file(temporary.path);BookmarkStore corrupt(temporary.path);check(!corrupt.load(kBuild)[0].occupied,"corrupt ciphertext fails closed");
    check(DeleteFileW(temporary.path.c_str())!=FALSE,"remove only the reserved owned temporary file");
    check(first.save(values,kBuild)&&std::filesystem::exists(temporary.path),"explicit identical save recreates externally deleted storage");
    BookmarkStore recreated(temporary.path);check(equal(recreated.load(kBuild),values),"recreated storage decrypts as the full payload");
    replace_owned_file(temporary.path);check(first.save(values,kBuild),"explicit identical save replaces external corruption");
    BookmarkStore replaced(temporary.path);check(equal(replaced.load(kBuild),values),"replacement storage decrypts as the full payload");
}

void persistence_revision_write_counts(){
    OwnedTemporaryFile temporary;const auto values=bookmarks();BookmarkStore store(temporary.path);store.load(kBuild);
    PersistenceRevision revision;unsigned writes{};const auto write=[&]{++writes;return store.save(values,kBuild);};
    check(revision.persist(0,write)==1&&!writes,"status before any edit performs no writes");
    check(revision.persist(1,write)==1&&writes==1,"first explicit revision writes once");
    for(unsigned i=0;i<100;++i)revision.persist(1,write);
    check(writes==1,"100 unchanged status polls perform zero additional writes");
    check(DeleteFileW(temporary.path.c_str())!=FALSE,"remove the owned revision-test file");revision.persist(1,write);
    check(writes==1&&!std::filesystem::exists(temporary.path),"unchanged polling does not repair external deletion");
    check(revision.persist(2,write)==1&&writes==2&&std::filesystem::exists(temporary.path),"new explicit revision repairs deleted storage exactly once");
    replace_owned_file(temporary.path);revision.persist(2,write);check(writes==2,"unchanged polling does not write after external replacement");
    check(revision.persist(3,write)==1&&writes==3,"new explicit revision replaces corrupted storage once");
    BookmarkStore final(temporary.path);check(equal(final.load(kBuild),values),"revision-authorized replacement decrypts correctly");
    PersistenceRevision failing;unsigned failed_writes{};const auto fail=[&]{++failed_writes;return false;};
    check(failing.persist(1,fail)==2&&failed_writes==1,"failed explicit revision reports unavailable");
    for(unsigned i=0;i<100;++i)failing.persist(1,fail);
    check(failed_writes==1,"100 unchanged polls never retry a failed write");
    check(failing.persist(2,fail)==2&&failed_writes==2,"a new explicit revision authorizes one further failed-write attempt");
}
}
int main(){
    lost_receipt_and_late_arrival();pending_deadline_does_not_replay();dpapi_roundtrip_and_explicit_repair();persistence_revision_write_counts();
    std::printf("Teleport receipts/DPAPI persistence: %u checks passed; owned temporary files and mock native calls only.\n",checks);
}
