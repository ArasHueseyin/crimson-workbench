#include "../src/task_observer_registry.h"
#include "../src/dispatch.h"
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
using namespace crimson::live_items;
TaskObservers* current_registry{};
TaskObservationV1 expected{};
unsigned calls[5]{},validations{};
bool changed{},reenter{},throw_callback{};
int error{},original_result{};
unsigned original_calls{};
void observe(unsigned index,const TaskObservationV1* observation) {
    ++calls[index];
    assert(observation && observation->size==56 && observation->abi==1 && !observation->reserved);
    assert(observation->base==expected.base && observation->context==expected.context &&
        observation->native_error==expected.native_error && observation->return_address==expected.return_address &&
        observation->monotonic_ms==expected.monotonic_ms && observation->identity==expected.identity);
    if (index==0) changed=true;
    if (reenter) {
        current_registry->notify(*observation,[](TaskObservationV1&) {
            assert(false && "reentrant observer validation must not run");return true;
        });
    }
    if (throw_callback) throw std::runtime_error("owned fixture callback exception");
}
void __cdecl first(const TaskObservationV1* o) {observe(0,o);}
void __cdecl second(const TaskObservationV1* o) {observe(1,o);}
void __cdecl third(const TaskObservationV1* o) {observe(2,o);}
void __cdecl fourth(const TaskObservationV1* o) {observe(3,o);}
void __cdecl fifth(const TaskObservationV1* o) {observe(4,o);}
int* __fastcall native_original(int* out,void* task,void* context) {
    ++original_calls;
    assert(out==&error && task==reinterpret_cast<void*>(0x7654) && context==expected.context);
    *out=71;return &original_result;
}
void after_native(void* context,int* out,std::uintptr_t caller) {
    assert(original_calls==1 && context==expected.context && out==&error && *out==71 && caller==expected.return_address);
}
} // namespace

int main() {
    using namespace crimson::live_items;
    TaskObservers registry;current_registry=&registry;
    expected.base=0x140000000;expected.context=reinterpret_cast<void*>(0x1234);expected.native_error=&error;
    expected.return_address=expected.base+0x2774a0d;expected.monotonic_ms=123456;expected.identity=0xa0123456;
    unsigned pins{};
    auto pin=[&](TaskObserverV1) {++pins;return true;};
    assert(!registry.add(2,56,first,pin) && !registry.add(1,55,first,pin) &&
        !registry.add(1,56,nullptr,pin) && pins==0);
    assert(!registry.add(1,56,first,[](TaskObserverV1){return false;}));
    std::vector<std::thread> threads;
    for (unsigned i=0;i<24;++i)
        threads.emplace_back([&]{assert(registry.add(1,56,first,pin));});
    for (auto& thread:threads) thread.join();
    assert(pins==1 && !calls[0]); // Registering never executes game work.
    assert(registry.add(1,56,second,pin) && registry.add(1,56,third,pin) && registry.add(1,56,fourth,pin));
    assert(!registry.add(1,56,fifth,pin) && pins==4);
    // A preceding observer changed the selected world: later callbacks must be
    // revalidated and rejected instead of reusing the first observer's lease.
    registry.notify(expected,[](TaskObservationV1& observation) {
        ++validations;observation.identity=expected.identity;return !changed;
    });
    assert(validations==4 && calls[0]==1 && calls[1]==0 && calls[2]==0 && calls[3]==0);
    changed=false;reenter=true;
    registry.notify(expected,[](TaskObservationV1&){++validations;return true;});
    assert(validations==8 && calls[0]==2 && calls[1]==1 && calls[2]==1 && calls[3]==1);
    reenter=false;throw_callback=true;
    bool caught=false;
    try {registry.notify(expected,[](TaskObservationV1&){return true;});}
    catch (const std::runtime_error&) {caught=true;}
    assert(caught && calls[0]==3 && calls[1]==1);
    throw_callback=false;
    registry.notify(expected,[](TaskObservationV1&){return true;});
    assert(calls[0]==4 && calls[1]==2 && calls[2]==2 && calls[3]==2); // TLS guard unwound.
    assert(forward_task(native_original,after_native,&error,reinterpret_cast<void*>(0x7654),
        expected.context,expected.return_address)==&original_result);
    assert(original_calls==1 && error==71);
    std::puts("PASS: versioned borrowed-task ABI; concurrent idempotent/pinned registration, capacity/refusal, fresh validation per observer, recursion exclusion, exception unwinding and unchanged native forwarding.");
}
