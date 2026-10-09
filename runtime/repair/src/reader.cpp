#include "repair_reader.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <tuple>

namespace crimson::repair::reader {
namespace {
constexpr std::size_t byte_budget = 16 * 1024 * 1024;
constexpr std::size_t read_budget = 65536;
struct Refusal { Code code; };
std::uintptr_t add(std::uintptr_t p, std::size_t offset) {
    if (p < 65536 || p > UINTPTR_MAX - offset) throw Refusal{Code::unreadable};
    return p + offset;
}
template<class T> T value(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || sizeof(T) > bytes.size() - offset) throw Refusal{Code::invalid_layout};
    T result;
    std::memcpy(&result, bytes.data() + offset, sizeof(T));
    return result;
}
class Reads {
    struct Record { std::uintptr_t address; std::vector<std::uint8_t> bytes; };
    const Memory& memory_;
    std::vector<Record> records_;
    std::vector<Region> structures_;
    std::size_t bytes_ = 0, count_ = 0;
    void charge(std::size_t size) {
        if (size > byte_budget - bytes_ || count_ == read_budget) throw Refusal{Code::budget_exceeded};
        bytes_ += size;
        ++count_;
    }
public:
    explicit Reads(const Memory& m) : memory_(m) {}
    void structural(std::uintptr_t address, std::size_t size) {
        add(address, size);
        structures_.push_back({address, size});
    }
    const std::vector<Region>& structures() const { return structures_; }
    std::vector<std::uint8_t> bytes(std::uintptr_t address, std::size_t size) {
        add(address, size);
        charge(size);
        std::vector<std::uint8_t> result(size);
        if (!memory_.copy(address, result)) throw Refusal{Code::unreadable};
        records_.push_back({address, result});
        return result;
    }
    template<class T> T at(std::uintptr_t address, std::size_t offset = 0) {
        return value<T>(bytes(add(address, offset), sizeof(T)), 0);
    }
    std::uintptr_t pointer(std::uintptr_t address, std::size_t offset = 0, Code absent = Code::unreadable) {
        const auto p = at<std::uintptr_t>(address, offset);
        if (!p) throw Refusal{absent};
        add(p, 0);
        return p;
    }
    void verify() {
        for (const auto& record : records_) {
            charge(record.bytes.size());
            std::vector<std::uint8_t> now(record.bytes.size());
            if (!memory_.copy(record.address, now)) throw Refusal{Code::unreadable};
            if (now != record.bytes) throw Refusal{Code::changed_during_read};
        }
    }
    std::size_t byte_count() const { return bytes_; }
    std::size_t read_count() const { return count_; }
    std::vector<Region> regions() const {
        std::vector<Region> result;
        result.reserve(records_.size());
        for (const auto& record : records_) result.push_back({record.address, record.bytes.size()});
        return result;
    }
    std::vector<std::vector<std::uint8_t>> images() const {
        std::vector<std::vector<std::uint8_t>> result;
        result.reserve(records_.size());
        for (const auto& record : records_) result.push_back(record.bytes);
        return result;
    }
};

bool usable_handle(std::uint32_t h) {
    return h && (!(h & 0x40000000u) || (h & 0x30000000u) == 0x20000000u);
}
bool valid_frame(Frame frame, std::size_t definitions) {
    return frame.image_base >= 65536 && frame.world_epoch && frame.catalog_revision && definitions <= action::absent &&
        (frame.build == Frame::Build::steam_25381195 || frame.build == Frame::Build::steam_25455892);
}
Character character(Reads& r, std::uintptr_t actor) {
    Character result;
    r.structural(actor, 0xa8);
    result.actor = actor;
    result.handle = r.at<std::uint32_t>(actor, 0x60);
    if (!usable_handle(result.handle)) throw Refusal{Code::identity_mismatch};
    const auto descriptor = r.pointer(actor, 0x88);
    const auto type = r.at<std::uint8_t>(descriptor, 1);
    // Only the anchored controlled protagonist / alternate protagonist classes.
    // Type alone never selects a player; the global and possessor must agree.
    if (type != 1 && type != 4 && type != 9) throw Refusal{Code::identity_mismatch};
    result.possessor = r.pointer(actor, 0xa0, Code::no_player);
    r.structural(result.possessor, 0xd8);
    if (r.pointer(result.possessor, 0xd0) != actor) throw Refusal{Code::identity_mismatch};
    result.components = r.pointer(actor, 0x68);
    r.structural(result.components, 0xc0);
    result.holder = r.pointer(result.components, 0xb8);
    result.equipment = r.pointer(result.components, 0x38);
    r.structural(result.holder, 0x28);
    r.structural(result.equipment, 0x98);
    if (r.pointer(result.holder, 8) != actor || r.pointer(result.equipment, 8) != actor)
        throw Refusal{Code::identity_mismatch};
    return result;
}

std::uintptr_t authority_actor(Reads& r, std::uintptr_t manager, std::uint32_t handle) {
    // Hash registry used by RVA 0x2a82730. The generation/type bits are checked
    // again against the resulting actor's FULL handle; low 20-bit collisions
    // alone never establish identity. No entire-registry or heap scan.
    if (!usable_handle(handle)) throw Refusal{Code::identity_mismatch};
    const auto buckets = r.at<std::uint32_t>(manager, 0x98);
    const auto used = r.at<std::uint32_t>(manager, 0x9c);
    if (!buckets || !used) throw Refusal{Code::no_authority};
    if (buckets > max_registry_buckets || used > max_registry_nodes) throw Refusal{Code::invalid_layout};
    const auto data = r.pointer(manager, 0xa8);
    const auto nodes = r.pointer(manager, 0xb0);
    const auto key = handle & 0xfffffu;
    const auto bucket = r.bytes(add(data, (key % buckets) * std::size_t(256)), 256);
    const auto count = value<std::uint32_t>(bucket, 0);
    if (count > 31) throw Refusal{Code::invalid_layout};
    std::uintptr_t result = 0;
    for (std::size_t i = 0; i < count; ++i) {
        if (value<std::uint32_t>(bucket, 8 + i * 8) != key) continue;
        const auto index = value<std::uint32_t>(bucket, 12 + i * 8);
        if (index >= max_registry_nodes) throw Refusal{Code::invalid_layout};
        const auto node = r.pointer(nodes, index * sizeof(std::uintptr_t));
        if (r.at<std::uint32_t>(node, 4) != key) throw Refusal{Code::identity_mismatch};
        const auto actor = r.pointer(node, 8);
        if (r.at<std::uint32_t>(actor, 0x60) != handle) throw Refusal{Code::identity_mismatch};
        if (result) throw Refusal{Code::ambiguous};
        result = actor;
    }
    if (!result) throw Refusal{Code::no_authority};
    return result;
}
struct Bucket {
    std::uintptr_t address, items;
    std::uint16_t slots;
    std::set<std::uint16_t> excluded;
};
Bucket bucket(Reads& r, std::uintptr_t holder, std::uint16_t wanted) {
    const auto count = r.at<std::uint32_t>(holder, 0x20);
    if (count > max_buckets) throw Refusal{Code::invalid_layout};
    if (!count) return {};
    const auto pointers = r.bytes(r.pointer(holder, 0x18), count * sizeof(std::uintptr_t));
    Bucket result{};
    for (std::size_t i = 0; i < count; ++i) {
        const auto p = value<std::uintptr_t>(pointers, i * sizeof(std::uintptr_t));
        r.structural(p, 0x30);
        if (r.at<std::uint16_t>(p, 0x10) != wanted) continue;
        if (result.address) throw Refusal{Code::ambiguous};
        result.address = p;
        const auto size = r.at<std::int16_t>(p, 0xc);
        if (size < 0 || static_cast<std::size_t>(size) > action::max_items) throw Refusal{Code::invalid_layout};
        result.slots = static_cast<std::uint16_t>(size);
        if (size) result.items = r.pointer(p);
        const auto excluded = r.at<std::uint32_t>(p, 0x28);
        if (excluded > action::max_items) throw Refusal{Code::invalid_layout};
        if (excluded) {
            const auto list = r.bytes(r.pointer(p, 0x20), excluded * std::size_t(12));
            for (std::size_t e = 0; e < excluded; ++e)
                result.excluded.insert(value<std::uint16_t>(list, e * 12));
        }
    }
    return result;
}
bool present(std::span<const std::uint8_t> bytes) {
    return value<std::uint16_t>(bytes, 8) != action::absent && value<std::int64_t>(bytes, 0x10) > 0;
}
action::Image image(Reads& r, std::span<const std::uint8_t> raw) {
    action::Image result;
    std::copy_n(raw.begin(), action::item_size, result.bytes.begin());
    const auto count = value<std::uint32_t>(raw, 0x68);
    const auto capacity = raw[0x70];
    const auto allocated = value<std::uint32_t>(raw, 0x6c);
    if (count > capacity || count > allocated || capacity > action::max_sockets) throw Refusal{Code::invalid_layout};
    if (count) {
        const auto sockets = r.bytes(value<std::uintptr_t>(raw, 0x60), count * action::socket_size);
        result.sockets.resize(count);
        for (std::size_t i = 0; i < count; ++i)
            std::copy_n(sockets.begin() + i * action::socket_size, action::socket_size, result.sockets[i].bytes.begin());
    }
    return result;
}
using Position = std::tuple<action::Area, std::uint16_t, std::uint16_t>;
struct Entry { std::uint64_t uid; action::Image image; Region region; };
using Items = std::map<Position, Entry>;
void append(Reads& r, Items& result, std::span<const std::uint8_t> raw, Position pos, std::uintptr_t address) {
    if (!present(raw)) return;
    const auto uid = value<std::uint64_t>(raw, 0);
    if (!uid || uid == UINT64_MAX || std::get<2>(pos) == action::absent) throw Refusal{Code::identity_mismatch};
    if (result.size() >= action::max_items) throw Refusal{Code::budget_exceeded};
    for (const auto& [_, e] : result) if (e.uid == uid) throw Refusal{Code::ambiguous};
    if (!result.emplace(pos, Entry{uid, image(r, raw), {address, raw.size()}}).second) throw Refusal{Code::ambiguous};
}
Items items(Reads& r, const Character& actor) {
    Items result;
    const auto bag = bucket(r, actor.holder, carried_container);
    if (!bag.address) throw Refusal{Code::no_player};
    for (std::uint16_t slot = 0; slot < bag.slots; ++slot) {
        const auto address = add(bag.items, slot * action::item_size);
        const auto raw = r.bytes(address, action::item_size);
        if (bag.excluded.contains(value<std::uint16_t>(raw, 8))) continue;
        append(r, result, raw, {action::Area::carried, carried_container, slot}, address);
    }
    const auto table = r.pointer(actor.equipment, 0x90);
    r.structural(table, 0x18);
    const auto count = r.at<std::uint32_t>(table, 0x10);
    if (count > max_equipment) throw Refusal{Code::invalid_layout};
    if (count) {
        const auto data = r.pointer(table, 8);
        for (std::size_t i = 0; i < count; ++i) {
            const auto address = add(data, i * std::size_t(0xd0));
            const auto raw = r.bytes(address, 0xd0);
            append(r, result, raw, {action::Area::equipped, std::uint16_t(0), value<std::uint16_t>(raw, 0xc8)}, address);
        }
    }
    return result;
}
std::uintptr_t current_client(Reads& r, Frame frame) {
    const auto context = r.pointer(frame.image_base, client_context_rva, Code::no_player);
    r.structural(context, 0x38);
    const auto manager = r.pointer(context, 0x30, Code::no_player);
    r.structural(manager, 0x58);
    return r.pointer(manager, 0x50, Code::no_player);
}
Capture capture_pair(Reads& r, Frame frame, std::span<const action::Definition> definitions,
    std::uintptr_t client, std::uintptr_t server) {
    Capture candidate;
    candidate.presentation = character(r, client);
    candidate.authority = character(r, server);
    const auto& a = candidate.authority;
    const auto& p = candidate.presentation;
    if (a.handle != p.handle || a.actor == p.actor || a.holder == p.holder ||
        a.equipment == p.equipment || a.possessor == p.possessor)
        throw Refusal{Code::identity_mismatch};
    const auto authority = items(r, a);
    const auto presentation = items(r, p);
    if (authority.size() != presentation.size()) throw Refusal{Code::identity_mismatch};
    candidate.snapshot.session = {frame.world_epoch, p.handle, frame.catalog_revision};
    candidate.snapshot.definitions.assign(definitions.begin(), definitions.end());
    for (const auto& [pos, entry] : authority) {
        const auto peer = presentation.find(pos);
        if (peer == presentation.end() || entry.uid != peer->second.uid) throw Refusal{Code::identity_mismatch};
        candidate.snapshot.items.push_back({{entry.uid, std::get<0>(pos), std::get<1>(pos), std::get<2>(pos)},
            entry.image, peer->second.image});
        candidate.locations.push_back({candidate.snapshot.items.back().identity, entry.region, peer->second.region});
    }
    // Validation only. This reader cannot repair anything as a side effect.
    action::Plan check;
    const auto status = action::prepare({candidate.snapshot.session, action::Scope::all, {}}, candidate.snapshot, check);
    if (status != action::Code::prepared && status != action::Code::nothing_to_repair)
        throw Refusal{Code::invalid_layout};
    r.verify();
    candidate.bytes_read = r.byte_count();
    candidate.reads = r.read_count();
    candidate.observed_regions = r.regions();
    candidate.observed_bytes = r.images();
    candidate.structural_regions = r.structures();
    return candidate;
}
bool bound(const Memory& memory, const PinnedOwners& owners) {
    for (std::size_t i = 0; i < owners.actors.size(); ++i) {
        const auto actor = owners.actors[i];
        if (actor < 65536 || actor > UINTPTR_MAX - 16) return false;
        std::uintptr_t lock = 0;
        if (!memory.copy(actor + 8, {reinterpret_cast<std::uint8_t*>(&lock), sizeof(lock)}) ||
            !lock || lock != reinterpret_cast<std::uintptr_t>(owners.locks[i].object)) return false;
    }
    return true;
}
bool current(const Memory& memory, const PinnedOwners& owners) {
    for (const auto actor : owners.actors) {
        std::array<std::uint8_t, 2> state{};
        std::uint32_t handle = 0;
        if (actor > UINTPTR_MAX - 0x64 || !memory.copy(actor + 0x4a, state) ||
            !memory.copy(actor + 0x60, {reinterpret_cast<std::uint8_t*>(&handle), sizeof(handle)}) ||
            !state[0] || state[1] || handle != owners.expected_handle) return false;
    }
    return true;
}
Code capture_held(const Memory& memory, Frame frame, std::span<const action::Definition> definitions,
    const PinnedOwners& owners, Capture& output) {
    if (!bound(memory, owners)) return Code::invalid_lock_binding;
    if (!current(memory, owners)) return Code::owner_unavailable;
    try {
        Reads r(memory);
        // Read only the referenced pair, never a second registry traversal.
        if (current_client(r, frame) != owners.actors[0]) return Code::identity_mismatch;
        auto candidate = capture_pair(r, frame, definitions, owners.actors[0], owners.actors[1]);
        if (!bound(memory, owners)) return Code::invalid_lock_binding;
        if (!current(memory, owners)) return Code::owner_unavailable;
        output = std::move(candidate);
        return Code::captured;
    } catch (const Refusal& error) { return error.code; }
}
}

Code inventory_slot(const Memory& memory, std::uintptr_t holder, std::uint16_t type,
    std::int16_t slot, std::uintptr_t& address) {
    address = 0;
    try {
        Reads r(memory);
        if (slot < 0) return Code::captured;
        const auto bag = bucket(r, holder, type);
        if (!bag.address || slot >= bag.slots) { r.verify(); return Code::captured; }
        const auto candidate = add(bag.items, static_cast<std::size_t>(slot) * action::item_size);
        const auto raw = r.bytes(candidate, action::item_size);
        const bool exists = present(raw) && !bag.excluded.contains(value<std::uint16_t>(raw, 8));
        r.verify();
        if (exists) address = candidate;
        return Code::captured;
    } catch (const Refusal& error) { return error.code; }
}

Code capture_with_locks(const Memory& memory, Frame frame, std::span<const action::Definition> definitions,
    const PinnedOwners& owners, Capture& output) {
    output = {};
    if (!valid_frame(frame, definitions.size()))
        return Code::invalid_frame;
    if (owners.actors[0] == owners.actors[1] || !usable_handle(owners.expected_handle))
        return Code::invalid_lock_binding;
    if (!bound(memory, owners)) return Code::invalid_lock_binding;
    lease::Group locks;
    const auto acquired = locks.try_acquire(owners.locks);
    if (acquired == lease::Code::busy) return Code::lock_busy;
    if (acquired != lease::Code::acquired) return Code::invalid_lock_binding;
    return capture_held(memory, frame, definitions, owners, output);
}

Code capture_with_references(const Memory& memory, Frame frame, std::span<const action::Definition> definitions,
    const std::array<reference::Source, 2>& sources, const std::array<lease::Binding, 2>& locks, Capture& output) {
    output = {};
    HeldCapture held;
    const auto result = held.open(memory, frame, definitions, sources, locks);
    if (result == Code::captured) output = *held.view();
    return result;
}

const Capture* HeldCapture::view() const noexcept {
    return held_ && evidence_valid_ && std::this_thread::get_id() == thread_ ? &capture_ : nullptr;
}
Code HeldCapture::release_locks() noexcept {
    if (std::this_thread::get_id() != thread_) return Code::wrong_thread;
    if (!held_) return Code::invalid_state;
    capture_ = {};
    evidence_valid_ = false;
    held_ = false;
    locks_.release();
    return Code::released;
}
Code HeldCapture::close() noexcept {
    if (std::this_thread::get_id() != thread_) return Code::wrong_thread;
    attempted_ = true;
    held_ = false;
    evidence_valid_ = false;
    capture_ = {};
    locks_.release();
    references_.release();
    memory_ = nullptr;
    owners_ = {};
    return Code::released;
}
Code HeldCapture::refresh(Frame frame) {
    if (std::this_thread::get_id() != thread_) return Code::wrong_thread;
    if (!held_) return Code::invalid_state;
    evidence_valid_ = false;
    capture_ = {};
    if (frame != frame_) { close(); return Code::invalid_frame; }
    try {
        const auto result = capture_held(*memory_, frame_, definitions_, owners_, capture_);
        if (result != Code::captured) close();
        else evidence_valid_ = true;
        return result;
    } catch (...) { close(); throw; }
}
Code HeldCapture::open(const Memory& memory, Frame frame, std::span<const action::Definition> definitions,
    const std::array<reference::Source, 2>& sources, const std::array<lease::Binding, 2>& bindings) {
    if (std::this_thread::get_id() != thread_) return Code::wrong_thread;
    if (attempted_) return Code::invalid_state;
    attempted_ = true;
    if (!valid_frame(frame, definitions.size())) return Code::invalid_frame;
    definitions_.assign(definitions.begin(), definitions.end());
    frame_ = frame; memory_ = &memory;
    const auto acquired = references_.acquire(sources);
    if (acquired != reference::Code::acquired) {
        close();
        return acquired == reference::Code::invalid_source ? Code::invalid_reference_source : Code::owner_unavailable;
    }
    owners_ = {{references_.actor(0), references_.actor(1)}, bindings, sources[0].handle};
    if (!usable_handle(owners_.expected_handle)) { close(); return Code::invalid_lock_binding; }
    if (!bound(memory, owners_)) { close(); return Code::invalid_lock_binding; }
    const auto locked = locks_.try_acquire(bindings);
    if (locked != lease::Code::acquired) {
        close();
        return locked == lease::Code::busy ? Code::lock_busy : Code::invalid_lock_binding;
    }
    held_ = true;
    return refresh(frame);
}

Code capture(const Memory& memory, Frame frame, std::span<const action::Definition> definitions, Capture& output) {
    output = {};
    if (!valid_frame(frame, definitions.size()))
        return Code::invalid_frame;
    try {
        Reads r(memory);
        const auto client = current_client(r, frame);
        const auto handle = r.at<std::uint32_t>(client, 0x60);
        const auto server_context = r.pointer(frame.image_base, server_context_rva, Code::no_authority);
        const auto server_manager = r.pointer(server_context, 0x48, Code::no_authority);
        const auto server = authority_actor(r, server_manager, handle);
        output = capture_pair(r, frame, definitions, client, server);
        return Code::captured;
    } catch (const Refusal& error) { return error.code; }
}
}
