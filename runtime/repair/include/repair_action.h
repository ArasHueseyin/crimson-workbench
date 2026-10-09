#pragma once
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>
#include <thread>
#include <vector>

namespace crimson::repair::action {
// An explicit repair command, independent of the game's empty repair rules.
// This library has NO game resolver, hook, process access, save I/O or loader.
// A future game adapter must provide an exclusive engine transaction, not call
// these functions against live game memory from a render/input/worker thread.
inline constexpr std::size_t item_size = 0xc8;
inline constexpr std::size_t socket_size = 6;
inline constexpr std::size_t max_sockets = 64; // implementation budget, not an engine limit
inline constexpr std::size_t max_items = 2048; // implementation budget, not inventory capacity
inline constexpr std::uint16_t absent = 0xffff;

struct Socket {
    std::array<std::uint8_t, socket_size> bytes{};
    bool operator==(const Socket&) const = default;
};
struct Image {
    std::array<std::uint8_t, item_size> bytes{};
    // Detached storage: never dereference the pointer at bytes+0x60.
    // Exactly the initialized vector count at +0x68. The logical slot limit at
    // +0x70 is separate and can exceed both count and allocated capacity.
    std::vector<Socket> sockets;
    bool operator==(const Image&) const = default;
};
struct Definition {
    std::uint16_t key;
    std::uint16_t maximum; // repair target from the verified, admitted vanilla catalog
    std::uint16_t active_maximum;
    bool no_wear_override;
    constexpr Definition(std::uint16_t k, std::uint16_t m) :
        key(k), maximum(m), active_maximum(m), no_wear_override(false) {}
    constexpr Definition(std::uint16_t k, std::uint16_t m, std::uint16_t active, bool no_wear) :
        key(k), maximum(m), active_maximum(active), no_wear_override(no_wear) {}
    bool operator==(const Definition&) const = default;
};
struct Session {
    std::uint64_t generation = 0; // must change on load, character/world transition
    std::uint64_t player = 0;
    std::uint64_t catalog_revision = 0; // must change when definitions/settings change
    bool operator==(const Session&) const = default;
};
enum class Area : std::uint8_t { carried, equipped };
struct Identity {
    std::uint64_t instance = 0;
    Area area = Area::carried;
    std::uint16_t container = 0;
    std::uint16_t slot = 0;
    bool operator==(const Identity&) const = default;
};
struct Item {
    Identity identity;
    Image authority;
    Image presentation;
    bool operator==(const Item&) const = default;
};
struct Snapshot {
    Session session;
    std::vector<Definition> definitions;
    std::vector<Item> items;
    bool operator==(const Snapshot&) const = default;
};
enum class Scope : std::uint8_t { one, carried, equipped, all };
struct Request {
    Session session;
    Scope scope = Scope::all;
    Identity item; // required only for Scope::one
};
enum class Code {
    prepared, queued, applied, nothing_to_repair, busy, no_request,
    invalid_request, invalid_snapshot, missing_definition, unsupported_maximum,
    missing_item, identity_mismatch, mirrors_disagree, stale, wrong_thread,
    expired, cancelled, host_rejected, outcome_unknown, fault_latched,
    awaiting_confirmation, confirmation_mismatch,
};
struct Count {
    std::size_t items = 0;
    std::size_t main_fields = 0;
    std::size_t socket_fields = 0;
    bool operator==(const Count&) const = default;
};
struct Change {
    Item before;
    Item after;
};
class Plan {
    friend Code prepare(const Request&, const Snapshot&, Plan&);
    Session session_;
    std::vector<Definition> definitions_;
    std::vector<Change> changes_;
    Count count_;
    bool prepared_ = false;
public:
    const Session& session() const { return session_; }
    std::span<const Definition> definitions() const { return definitions_; }
    std::span<const Change> changes() const { return changes_; }
    Count count() const { return count_; }
    bool prepared() const { return prepared_; }
};

// Produces an immutable plan; failure clears any earlier plan. Only endurance
// words at item+0x40 / socket+2 may differ. No materials or item counts change.
Code prepare(const Request& request, const Snapshot& snapshot, Plan& output);

// Metadata for the engine's equipment notification, derived BEFORE mutation.
// The engine classifies broken state using the ACTIVE maximum and a signed
// endurance word. A no-wear override must not generate a false repaired event.
// This is neither a dispatch receipt nor permission to access an engine item.
struct EquipmentNotice {
    Session session;
    Identity item;
    bool was_broken = false;
    bool is_broken = false;
    bool operator==(const EquipmentNotice&) const = default;
};
// Only an exact equipped identity in a prepared plan is accepted. Failure clears
// output; carried items require their own notification path. No memory access.
Code equipment_notice(const Plan&, Identity, EquipmentNotice& output) noexcept;

// Concrete copy-store backend used by the sandbox. Preflights the ENTIRE plan,
// then performs allocation-free writes into already sized, owned images.
// It is deliberately not a game-memory writer or persistence claim.
Code apply_to_owned_snapshot(const Plan& plan, Snapshot& current);

// Read-only confirmation of every planned after-image in a freshly captured
// session. This proves fields, not engine events; the host must confirm both.
Code verify_applied(const Plan&, const Snapshot& observed);

enum class Commit { applied, rejected, unknown, pending };
class Transaction {
public:
    virtual ~Transaction() = default;
    // The adapter must hold exclusive ownership until destruction. The snapshot
    // includes only this player's carried/equipped items, never NPCs or storage.
    virtual Snapshot capture() = 0;
    // Revalidate identity and every before-image, then update engine authority,
    // presentation and notifications as ONE engine transaction. If mutation may
    // have occurred without a confirmed outcome, return unknown, never rejected.
    // A rejected commit promises that it made NO changes. No retry on unknown.
    // pending means accepted but not yet confirmed. Release the engine lease
    // normally; do not hold it across frames. Queue::confirm requires a fresh
    // capture and all event receipts. Marshal callbacks after execute returns.
    virtual Commit commit(const Plan& plan) noexcept = 0;
};
struct Outcome {
    Code code = Code::no_request;
    std::uint64_t request_id = 0;
    Count count;
};

// Input only queues a request. Execution is explicitly bound to the host's
// dispatch thread and fresh session. No automatic repair on wear, startup,
// load or retry; repeat key events coalesce while a command is pending/running.
class Queue {
    using Clock = std::chrono::steady_clock;
    std::thread::id thread_;
    std::mutex mutex_;
    Request request_{};
    Clock::time_point deadline_{};
    std::uint64_t next_ = 0;
    std::uint64_t pending_ = 0;
    bool running_ = false;
    bool fault_ = false;
    std::uint64_t confirming_ = 0;
    Plan confirmation_;
    Clock::time_point confirmation_deadline_{};
    Outcome uncertain(); // caller holds mutex_
public:
    explicit Queue(std::thread::id dispatch_thread) : thread_(dispatch_thread) {}
    Outcome submit(const Request&, Clock::time_point now = Clock::now());
    Outcome cancel();
    Outcome execute(Transaction&, Clock::time_point now = Clock::now());
    // Dispatch thread only. Host must associate the receipt with this exact
    // request/session (never infer success from an item pointer or error=0).
    // A failed, partial, late or changed-session completion faults permanently.
    Outcome confirm(std::uint64_t request_id, Session receipt_session, const Snapshot& observed,
        bool notifications_confirmed, Clock::time_point now = Clock::now());
    // Host calls each dispatch tick, including loading/menu frames. Cancels
    // unstarted stale work; faults accepted work after transition/5s timeout.
    Outcome poll(Session current, Clock::time_point now = Clock::now());
    // Fault latch has no reset: the host must destroy/recreate the queue only
    // after reconstructing a new, verified engine session.
};
}
