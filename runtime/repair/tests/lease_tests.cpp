#include "repair_lease.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace ls = crimson::repair::lease;
namespace {
unsigned checks = 0;
void require(bool value, const char* message) { ++checks; if (!value) throw std::runtime_error(message); }
struct Trace {
    std::array<int, 16> calls{};
    std::size_t size = 0;
    bool exclusive = true;
    void add(int value, bool shared) noexcept { exclusive &= !shared; if (size < calls.size()) calls[size++] = value; }
};
struct Lock { Trace* trace; int id; unsigned depth = 0; bool blocked = false; };
bool attempt(void* object, bool shared) noexcept {
    auto& lock = *static_cast<Lock*>(object);
    lock.trace->add(lock.id, shared);
    if (lock.blocked) return false;
    ++lock.depth; return true;
}
void release(void* object, bool shared) noexcept {
    auto& lock = *static_cast<Lock*>(object);
    lock.trace->add(-lock.id, shared); --lock.depth;
}
void wrong_release(void*, bool) noexcept {}
ls::Binding bind(Lock& lock) { return {&lock, attempt, release}; }
void ordered_and_rollback() {
    for (int blocked = 0; blocked < 3; ++blocked) {
        Trace trace;
        std::array<Lock, 2> locks{{{&trace, 1}, {&trace, 2}}};
        if (blocked) locks[blocked - 1].blocked = true;
        const std::array bindings{bind(locks[1]), bind(locks[0])}; // caller order deliberately reversed
        {
            ls::Group group;
            require(group.try_acquire(bindings) == (blocked ? ls::Code::busy : ls::Code::acquired), "All-or-none acquisition");
            require(group.size() == (blocked ? 0u : 2u), "No retained subset after contention");
            if (!blocked) require(group.try_acquire(bindings) == ls::Code::already_acquired, "No accidental repeated lock nesting");
        }
        require(!locks[0].depth && !locks[1].depth && trace.exclusive, "All locks released exclusively");
        const std::array<int, 4> success{1, 2, -2, -1};
        const std::array<int, 3> partial{1, 2, -1};
        if (!blocked) require(trace.size == 4 && std::equal(success.begin(), success.end(), trace.calls.begin()), "Sorted acquire, reverse release");
        if (blocked == 1) require(trace.size == 1 && trace.calls[0] == 1, "First failure never calls second lock");
        if (blocked == 2) require(trace.size == 3 && std::equal(partial.begin(), partial.end(), trace.calls.begin()), "Second failure immediately releases first");
    }
}
void validation_and_thread() {
    Trace trace;
    Lock lock{&trace, 1};
    const auto binding = bind(lock);
    ls::Group group;
    require(group.try_acquire({}) == ls::Code::invalid_binding, "Empty lock set refused");
    for (const auto bad : {ls::Binding{}, ls::Binding{&lock, nullptr, release}, ls::Binding{&lock, attempt, nullptr}}) {
        const std::array entries{binding, bad};
        require(group.try_acquire(entries) == ls::Code::invalid_binding && trace.size == 0, "Validate whole set before callbacks");
    }
    const std::array conflict{binding, ls::Binding{&lock, attempt, wrong_release}};
    require(group.try_acquire(conflict) == ls::Code::invalid_binding && trace.size == 0, "Conflicting duplicate binding refused");
    const std::array too_many{binding, binding, binding};
    require(group.try_acquire(too_many) == ls::Code::invalid_binding, "Two-owner budget");
    const std::array duplicate{binding, binding};
    require(group.try_acquire(duplicate) == ls::Code::acquired && group.size() == 1 && lock.depth == 1,
        "Shared synchronization object acquired once");
    ls::Code acquired{}, released{};
    std::thread worker([&] { acquired = group.try_acquire(duplicate); released = group.release(); });
    worker.join();
    require(acquired == ls::Code::wrong_thread && released == ls::Code::wrong_thread && lock.depth == 1,
        "Wrong thread cannot acquire or release dispatcher locks");
    require(group.release() == ls::Code::released && !lock.depth, "Dispatcher can still release");
    require(group.release() == ls::Code::empty, "No double release");
    lock.depth = 1; // an existing outer engine lease
    require(group.try_acquire(duplicate) == ls::Code::acquired && lock.depth == 2, "Nested external ownership supported by binding");
    group.release();
    require(lock.depth == 1, "Release only this group's ownership level");
}
}
int main() {
    static_assert(!std::is_copy_constructible_v<ls::Group> && !std::is_move_constructible_v<ls::Group>);
    try {
        ordered_and_rollback(); validation_and_thread();
        std::cout << "{\"ok\":true,\"checks\":" << checks
            << ",\"scope\":\"private_lock_fixtures\",\"game_process_access\":false,\"engine_owner_lifetimes\":false}\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
