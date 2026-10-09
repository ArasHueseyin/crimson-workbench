#pragma once
#include "repair_action.h"
#include "repair_lease.h"
#include "repair_reference.h"
#include <span>

namespace crimson::repair::writer { class Batch; }
namespace crimson::repair::reader {
// Read-only layout adapter for Steam 25381195 / 25455892 (EXE 2944 / 2949).
// Both builds have independently checked field/accessor evidence. Loading this
// adapter into the game still requires the missing verified startup/lease host.
// It cannot write item memory, attach to a process or install. Optional locked
// capture uses host-supplied bindings. Verified acquisition, context/manager
// lifetime and engine dispatch/TLS admission remain the host's job.
inline constexpr std::uintptr_t client_context_rva = 0x6d691b0;
inline constexpr std::uintptr_t server_context_rva = 0x6d696c0;
inline constexpr std::uint16_t carried_container = 2; // verified InventoryInfo Character
inline constexpr std::size_t max_buckets = 128;
inline constexpr std::size_t max_equipment = 64;
inline constexpr std::size_t max_registry_buckets = 65536;
inline constexpr std::size_t max_registry_nodes = 65536;

class Memory {
public:
    virtual ~Memory() = default;
    // Exact bounded read or false. No partial success; destination is private.
    virtual bool copy(std::uintptr_t address, std::span<std::uint8_t> output) const noexcept = 0;
};
// Reads ONLY the calling process. No PID, process lookup, OpenProcess, writer or
// remote access. Tests use this exclusively in their own executable.
class LocalMemory final : public Memory {
public:
    bool copy(std::uintptr_t, std::span<std::uint8_t>) const noexcept override;
};
enum class Code {
    captured, no_player, no_authority, unreadable, invalid_layout, identity_mismatch,
    ambiguous, changed_during_read, budget_exceeded, invalid_frame,
    lock_busy, invalid_lock_binding, owner_unavailable, invalid_reference_source,
    invalid_state, wrong_thread, released,
};
struct Frame {
    std::uintptr_t image_base = 0;
    // Supplied by the future engine host, which must increment this on EVERY
    // load/transition, including address/handle reuse. Pointer equality is not
    // a world-generation signal. This adapter cannot create such a signal.
    std::uint64_t world_epoch = 0;
    std::uint64_t catalog_revision = 0;
    enum class Build { unverified, steam_25381195, steam_25455892 };
    // The host must verify the complete EXE/tables first. Merely setting this
    // selector cannot authorize a game process or replace that admission.
    Build build = Build::unverified;
    bool operator==(const Frame&) const = default;
};
struct Character {
    std::uintptr_t actor = 0, possessor = 0, components = 0, holder = 0, equipment = 0;
    std::uint32_t handle = 0;
    bool operator==(const Character&) const = default;
};
struct Region {
    std::uintptr_t address = 0;
    std::size_t size = 0;
    bool operator==(const Region&) const = default;
};
struct Location {
    action::Identity identity;
    Region authority, presentation;
    bool operator==(const Location&) const = default;
};
struct Capture {
    action::Snapshot snapshot;
    Character authority, presentation;
    std::size_t bytes_read = 0, reads = 0;
    // Diagnostic source geometry, not ownership or write permission. Only a
    // fresh capture inside a HeldCapture may supply addresses to the writer.
    std::vector<Location> locations;
    std::vector<Region> observed_regions;
    std::vector<std::vector<std::uint8_t>> observed_bytes;
    std::vector<Region> structural_regions;
};
// Follows the current client-manager anchor and a key-directed server registry
// lookup. Diagnostic/offline capture only: registry and object lifetimes are
// NOT protected here. Never use this route as a live acquisition source.
// Re-reads its full read set; this detects change, but IS NOT an engine lock.
Code capture(const Memory&, Frame, std::span<const action::Definition>, Capture&);

// The future engine host supplies independently pinned owner lifetimes and
// verified lock methods. Order: client, server. Lock acquisition itself does
// not establish lifetime or authorize an arbitrary actor/object pointer.
struct PinnedOwners {
    std::array<std::uintptr_t, 2> actors{};
    std::array<lease::Binding, 2> locks{};
    // Required: both actors must be alive/not-destroying and retain this full
    // handle before and after capture under locks. Zero is rejected.
    std::uint32_t expected_handle = 0;
};
// Acquire both owner write locks without blocking and capture only the pinned
// pair. Check the current-client anchor; never traverse a registry again.
// The host must keep context/manager alive and prevent world transitions during
// this call; actor references do not pin their manager. All locks are released
// on return. Output is read-only evidence, never a continuing write permission.
// Caller must be on the verified engine dispatch thread with initialized TLS.
Code capture_with_locks(const Memory&, Frame, std::span<const action::Definition>,
    const PinnedOwners&, Capture&);

// Own both native references for the full locked capture; release locks BEFORE
// releasing references, including errors/exceptions. Sources and lock bindings
// must be resolved by the verified engine host. Output is a private copy only.
Code capture_with_references(const Memory&, Frame, std::span<const action::Definition>,
    const std::array<reference::Source, 2>&, const std::array<lease::Binding, 2>&, Capture&);

// A single synchronous engine-dispatch scope. Unlike a detached Capture, this
// retains both native references AND exclusive owner locks after open/refresh.
// The host must still pin context/manager/catalog, exclude world transitions and
// preserve initialized TLS. This is not a frame-spanning lease or game host.
// After release_locks(), no further reading is allowed; references alone remain
// alive for notifications until close/destruction. Never reacquire this scope.
class HeldCapture {
    friend class crimson::repair::writer::Batch;
    std::thread::id thread_ = std::this_thread::get_id();
    reference::Pair references_; // destruction order: locks before references
    lease::Group locks_;
    const Memory* memory_ = nullptr;
    Frame frame_;
    std::vector<action::Definition> definitions_;
    PinnedOwners owners_;
    Capture capture_;
    bool attempted_ = false, held_ = false;
    bool evidence_valid_ = false;
    bool write_attempted_ = false;
public:
    HeldCapture() = default;
    HeldCapture(const HeldCapture&) = delete;
    HeldCapture& operator=(const HeldCapture&) = delete;
    HeldCapture(HeldCapture&&) = delete;
    HeldCapture& operator=(HeldCapture&&) = delete;
    Code open(const Memory&, Frame, std::span<const action::Definition>,
        const std::array<reference::Source, 2>&, const std::array<lease::Binding, 2>&);
    // A caller's independently observed current frame is mandatory. A failed
    // refresh clears evidence and closes ownership; it never leaves stale data.
    Code refresh(Frame);
    const Capture* view() const noexcept;
    Code release_locks() noexcept;
    Code close() noexcept;
};

// Engine-equivalent inventory lookup for an already owned/locked holder.
// Empty/default/filtered entries return address 0; malformed input is rejected.
// This does not establish that a caller-supplied holder belongs to the player.
Code inventory_slot(const Memory&, std::uintptr_t holder, std::uint16_t type,
    std::int16_t slot, std::uintptr_t& address);
}
