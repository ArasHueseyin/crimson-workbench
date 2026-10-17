#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace crimson::live_items {
inline constexpr std::uint32_t kTaskObserverAbiV1 = 1;
// Process-local ABI only. Every pointer is borrowed for this synchronous call,
// before the pinned native caller releases its actor lease. Never retain these
// pointers, enqueue them or call game functions from a registration/IPC thread.
// This observation grants no authority over field time, movement or other
// systems: each consumer must acquire and validate its own native subsystem.
struct TaskObservationV1 {
    std::uint32_t size{sizeof(TaskObservationV1)};
    std::uint32_t abi{kTaskObserverAbiV1};
    std::uintptr_t base{};
    void* context{};
    int* native_error{};
    std::uintptr_t return_address{};
    std::uint64_t monotonic_ms{};
    std::uint32_t identity{};
    std::uint32_t reserved{};
};
static_assert(sizeof(void*)==8 && sizeof(TaskObservationV1)==56);
static_assert(std::is_standard_layout_v<TaskObservationV1> && std::is_trivially_copyable_v<TaskObservationV1>);
static_assert(offsetof(TaskObservationV1,base)==8 && offsetof(TaskObservationV1,identity)==48);
using TaskObserverV1 = void(__cdecl*)(const TaskObservationV1*);
// The runtime pins the callback's module before publishing it and retains that
// callback until process exit. Repeated registration is idempotent. There is no
// unregister/unload operation; a consumer gates its own policy when disabled.
using RegisterTaskObserverV1 = bool(__cdecl*)(std::uint32_t abi,std::uint32_t bytes,TaskObserverV1);
inline constexpr char kRegisterTaskObserverExportV1[] = "RegisterWorkbenchTaskObserverV1";
} // namespace crimson::live_items
