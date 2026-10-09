#include "repair_registry.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace reg = crimson::repair::registry;
namespace ref = crimson::repair::reference;
namespace {
unsigned checks = 0;
void require(bool value, const char* message) { ++checks; if (!value) throw std::runtime_error(message); }
struct Trace {
    std::array<int, 8> events{};
    std::size_t used = 0;
    void add(int event) noexcept { if (used < events.size()) events[used++] = event; }
};
struct Manager {
    Trace* trace = nullptr;
    int realm = 0;
    std::uintptr_t actor = 0;
    std::uint32_t handle = 0;
    unsigned active_refs = 0, acquires = 0, destroys = 0;
    bool allow = true, correct_destroy = true;
};
void* lookup(void* context, ref::Receipt* output, std::uint32_t handle) noexcept {
    auto* manager = static_cast<Manager*>(context);
    ++manager->acquires; manager->trace->add(manager->realm);
    std::memcpy(output->bytes.data(), &manager, sizeof(manager));
    std::memcpy(output->bytes.data() + 8, &manager->actor, sizeof(manager->actor));
    if (manager->allow && handle == manager->handle) {
        ++manager->active_refs; output->bytes[0x10] = 1;
    }
    return output;
}
void destroy(ref::Receipt* receipt, int expected_realm) noexcept {
    Manager* manager = nullptr;
    std::memcpy(&manager, receipt->bytes.data(), sizeof(manager));
    manager->correct_destroy &= manager->realm == expected_realm;
    ++manager->destroys; manager->trace->add(-manager->realm);
    if (receipt->bytes[0x10]) --manager->active_refs;
    receipt->bytes[0x10] = 0;
}
void destroy_client(ref::Receipt* receipt) noexcept { destroy(receipt, 1); }
void destroy_server(ref::Receipt* receipt) noexcept { destroy(receipt, 2); }

void adapter_tests() {
    constexpr std::uint32_t handle = 0x1234567;
    for (int scenario = 0; scenario < 9; ++scenario) {
        Trace trace;
        Manager client{&trace, 1, 0x100000, handle}, server{&trace, 2, 0x200000, handle};
        reg::Source a(scenario == 5 ? nullptr : &client, lookup, destroy_client);
        reg::Source b(&server, scenario == 6 ? nullptr : lookup, scenario == 7 ? nullptr : destroy_server);
        if (scenario == 1) client.allow = false;
        if (scenario == 2) server.allow = false;
        if (scenario == 3) server.handle ^= 0x100000; // same registry key is not same full handle
        if (scenario == 4) server.actor = client.actor;
        const std::array sources{a.source(scenario == 8 ? 0 : handle), b.source(handle)};
        const auto expected = scenario == 0 ? ref::Code::acquired :
            scenario >= 5 ? ref::Code::invalid_source : ref::Code::unavailable;
        {
            ref::Pair pair;
            require(pair.acquire(sources) == expected, "Registry source forwards full handle and rejects incomplete bindings");
            require(pair.actor(0) == (scenario == 0 ? client.actor : 0), "No actor exposed on failed pair");
            if (scenario == 0) require(client.active_refs == 1 && server.active_refs == 1,
                "Adapter retains native receipts after protected lookup returns");
        }
        require(!client.active_refs && !server.active_refs, "All acquired references released");
        require(client.acquires == client.destroys && server.acquires == server.destroys,
            "Valid and invalid native receipts cleaned up exactly once");
        require(client.correct_destroy && server.correct_destroy, "Each realm uses its bound destroy method");
        if (scenario >= 5) require(trace.used == 0, "Invalid binding set never calls either registry");
        else if (scenario == 1) require(trace.used == 2 && trace.events[0] == 1 && trace.events[1] == -1,
            "First lookup failure stops before second registry");
        else require(trace.used == 4 && trace.events[0] == 1 && trace.events[1] == 2 &&
            trace.events[2] == -2 && trace.events[3] == -1, "Reverse-order cleanup after pair completion/failure");
    }
}
}
int main() {
    static_assert(!std::is_copy_constructible_v<reg::Source> && !std::is_move_constructible_v<reg::Source>);
    try {
        adapter_tests();
        std::cout << "{\"ok\":true,\"checks\":" << checks
            << ",\"source\":\"synthetic_protected_registry_callbacks\",\"native_registry_execution\":false,"
            << "\"game_process_access\":false,\"save_access\":false}\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
