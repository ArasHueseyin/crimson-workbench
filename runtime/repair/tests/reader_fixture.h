#pragma once
#include "repair_writer.h"
#include <Windows.h>
#include <algorithm>
#include <cstring>
#include <functional>
#include <map>
#include <stdexcept>

namespace repair_test_fixture {
namespace rd = crimson::repair::reader;
namespace act = crimson::repair::action;
namespace ref = crimson::repair::reference;
class Memory final : public crimson::repair::writer::Access {
    std::uintptr_t next_ = 0x200000000;
    bool native_ = false;
public:
    explicit Memory(bool native = false) : native_(native) {}
    bool native_items() const noexcept override { return native_; }
    mutable std::map<std::uintptr_t, std::vector<std::uint8_t>> blocks;
    mutable std::map<std::uintptr_t, unsigned> reads;
    std::vector<std::pair<std::uintptr_t, std::size_t>> forbidden;
    mutable unsigned forbidden_reads = 0;
    std::uintptr_t change_on_verify = 0;
    unsigned change_after = 2;
    std::uintptr_t guarded_address = 0;
    const unsigned* owner_depth_a = nullptr;
    const unsigned* owner_depth_b = nullptr;
    std::uintptr_t deny_word = 0;
    unsigned fail_write = 0;
    bool store_then_fail = false, ignore_store = false;
    mutable unsigned write_calls = 0, probe_calls = 0;
    std::function<void(unsigned)> probe_callback, write_callback;
    std::uintptr_t allocate(std::size_t size) {
        if (native_) {
            std::vector<std::uint8_t> bytes(size);
            const auto p = reinterpret_cast<std::uintptr_t>(bytes.data());
            blocks.emplace(p, std::move(bytes));
            return p;
        }
        const auto p = next_;
        next_ += ((size + 4095) & ~std::size_t(4095)) + 4096;
        blocks[p] = std::vector<std::uint8_t>(size);
        return p;
    }
    template<class T> void put(std::uintptr_t p, std::size_t at, T value) {
        auto it = blocks.upper_bound(p);
        if (it == blocks.begin()) throw std::runtime_error("Fixture write address");
        --it;
        const auto offset = p - it->first + at;
        if (offset > it->second.size() || sizeof(T) > it->second.size() - offset)
            throw std::runtime_error("Fixture write bounds");
        std::memcpy(it->second.data() + offset, &value, sizeof(T));
    }
    bool copy(std::uintptr_t address, std::span<std::uint8_t> output) const noexcept override {
        for (const auto& [start, size] : forbidden) {
            if ((address >= start && address - start < size) ||
                (address < start && start - address < output.size())) {
                ++forbidden_reads;
                return false;
            }
        }
        if (address == guarded_address && (!owner_depth_a || !owner_depth_b || !*owner_depth_a || !*owner_depth_b))
            return false;
        auto it = blocks.upper_bound(address);
        if (it == blocks.begin()) return false;
        --it;
        const auto offset = address - it->first;
        if (offset > it->second.size() || output.size() > it->second.size() - offset) return false;
        std::copy_n(it->second.begin() + offset, output.size(), output.begin());
        const auto times = ++reads[address];
        if (address == change_on_verify && times >= change_after && !output.empty()) output.back() ^= 1;
        return true;
    }
    bool writable(std::uintptr_t address, std::size_t size) const noexcept override {
        ++probe_calls;
        if (probe_callback) probe_callback(probe_calls);
        if (address == deny_word || (owner_depth_a && (!*owner_depth_a || !*owner_depth_b))) return false;
        auto it = blocks.upper_bound(address);
        if (it == blocks.begin()) return false;
        --it;
        const auto offset = address - it->first;
        return offset <= it->second.size() && size <= it->second.size() - offset;
    }
    bool write_word(std::uintptr_t address, std::uint16_t before, std::uint16_t after) const noexcept override {
        ++write_calls;
        if (write_callback) write_callback(write_calls);
        if (write_calls == fail_write && !store_then_fail) return false;
        auto it = blocks.upper_bound(address);
        if (it == blocks.begin() || address % 2 || (owner_depth_a && (!*owner_depth_a || !*owner_depth_b))) return false;
        --it;
        const auto offset = address - it->first;
        if (offset > it->second.size() || 2 > it->second.size() - offset) return false;
        std::uint16_t current; std::memcpy(&current, it->second.data() + offset, 2);
        if (current != before) return false;
        if (!ignore_store) std::memcpy(it->second.data() + offset, &after, 2);
        return write_calls != fail_write;
    }
};
struct Actor {
    std::uintptr_t actor, possessor, parts, holder, equipment, bag, bag_items, slots, sockets, descriptor;
};
struct Fixture {
    Memory memory;
    rd::Frame frame{0x180000000, 7, 9, rd::Frame::Build::steam_25381195};
    std::uint32_t handle = 0x12345;
    std::uintptr_t client_context, client_manager, server_context, server_manager, hash, nodes, node;
    Actor client, server;
    std::vector<act::Definition> definitions{{10, 100}, {20, 30}};
    void item(std::uintptr_t address, std::uint64_t uid, std::uint16_t key, std::int64_t quantity, std::uint16_t endurance) {
        memory.put(address, 0, uid);
        memory.put(address, 8, key);
        memory.put(address, 0x10, quantity);
        memory.put(address, 0x40, endurance);
    }
    Actor actor() {
        Actor a{};
        a.actor = memory.allocate(0xa8);
        a.possessor = memory.allocate(0xd8);
        a.parts = memory.allocate(0xc0);
        a.descriptor = memory.allocate(2);
        a.holder = memory.allocate(0x28);
        a.equipment = memory.allocate(0x208); // includes the 2949 server dirty-slot table
        a.bag = memory.allocate(0x30);
        a.bag_items = memory.allocate(3 * act::item_size);
        a.slots = memory.allocate(2 * 0xd0);
        a.sockets = memory.allocate(12);
        memory.put(a.actor, 0x60, handle);
        memory.put(a.actor, 0x4a, std::uint8_t(1));
        memory.put(a.actor, 0x68, a.parts);
        memory.put(a.actor, 0x88, a.descriptor);
        memory.put(a.descriptor, 1, std::uint8_t(1));
        memory.put(a.actor, 0xa0, a.possessor);
        memory.put(a.possessor, 0xd0, a.actor);
        memory.put(a.parts, 0xb8, a.holder);
        memory.put(a.parts, 0x38, a.equipment);
        memory.put(a.holder, 8, a.actor);
        memory.put(a.equipment, 8, a.actor);
        const auto buckets = memory.allocate(16);
        const auto storage = memory.allocate(0x30);
        memory.put(buckets, 0, storage);
        memory.put(storage, 0x10, std::uint16_t(8)); // storage item data deliberately absent
        memory.put(buckets, 8, a.bag);
        memory.put(a.holder, 0x18, buckets);
        memory.put(a.holder, 0x20, std::uint32_t(2));
        memory.put(a.bag, 0, a.bag_items);
        memory.put(a.bag, 8, std::uint16_t(1)); // old +8 count would miss/incorrectly bound slots
        memory.put(a.bag, 0xc, std::uint16_t(3));
        memory.put(a.bag, 0x10, rd::carried_container);
        item(a.bag_items, 11, 10, 1, 35);
        item(a.bag_items + act::item_size, UINT64_MAX, act::absent, 0, 0);
        item(a.bag_items + 2 * act::item_size, 12, 20, 0, 0); // consumed
        memory.put(a.bag_items, 0x60, a.sockets);
        memory.put(a.bag_items, 0x68, std::uint32_t(2));
        memory.put(a.bag_items, 0x6c, std::uint32_t(2));
        memory.put(a.bag_items, 0x70, std::uint8_t(2));
        memory.put(a.sockets, 0, std::uint16_t(20));
        memory.put(a.sockets, 2, std::uint16_t(5));
        memory.put(a.sockets, 4, std::uint8_t(0));
        memory.put(a.sockets, 5, std::uint8_t(5));
        memory.put(a.sockets, 6, act::absent);
        memory.put(a.sockets, 10, std::uint8_t(1));
        const auto table = memory.allocate(0x18);
        memory.put(a.equipment, 0x90, table);
        memory.put(table, 8, a.slots);
        memory.put(table, 0x10, std::uint32_t(2));
        item(a.slots, 22, 20, 1, 0);
        memory.put(a.slots, 0xc8, std::uint16_t(3));
        item(a.slots + 0xd0, UINT64_MAX, act::absent, 0, 0);
        memory.put(a.slots + 0xd0, 0xc8, std::uint16_t(4));
        return a;
    }
    explicit Fixture(bool native = false) : memory(native) {
        client_context = memory.allocate(0x38);
        client_manager = memory.allocate(0x58);
        server_context = memory.allocate(0x50);
        server_manager = memory.allocate(0xb8);
        client = actor(); server = actor();
        memory.blocks[frame.image_base + rd::client_context_rva].resize(8);
        memory.blocks[frame.image_base + rd::server_context_rva].resize(8);
        memory.put(frame.image_base + rd::client_context_rva, 0, client_context);
        memory.put(client_context, 0x30, client_manager);
        memory.put(client_manager, 0x50, client.actor);
        memory.put(frame.image_base + rd::server_context_rva, 0, server_context);
        memory.put(server_context, 0x48, server_manager);
        hash = memory.allocate(256); nodes = memory.allocate(16); node = memory.allocate(16);
        memory.put(server_manager, 0x98, std::uint32_t(1));
        memory.put(server_manager, 0x9c, std::uint32_t(1));
        memory.put(server_manager, 0xa8, hash);
        memory.put(server_manager, 0xb0, nodes);
        memory.put(hash, 0, std::uint32_t(1));
        memory.put(hash, 8, handle & 0xfffffu);
        memory.put(hash, 12, std::uint32_t(0));
        memory.put(nodes, 0, node);
        memory.put(node, 4, handle & 0xfffffu);
        memory.put(node, 8, server.actor);
    }
    rd::Code capture(rd::Capture& out) { return rd::capture(memory, frame, definitions, out); }
};
struct OwnerLock {
    unsigned depth = 0, attempts = 0, releases = 0;
    bool blocked = false;
    Fixture* fixture = nullptr;
    std::uintptr_t change_owner = 0;
    const unsigned* reference_depth = nullptr;
};
bool try_owner_lock(void* object, bool shared) noexcept {
    auto& lock = *static_cast<OwnerLock*>(object);
    ++lock.attempts;
    if (shared || lock.blocked || (lock.reference_depth && !*lock.reference_depth)) return false;
    ++lock.depth;
    if (lock.change_owner) lock.fixture->memory.put(lock.change_owner, 8, std::uintptr_t(0));
    return true;
}
void release_owner_lock(void* object, bool) noexcept {
    auto& lock = *static_cast<OwnerLock*>(object);
    --lock.depth; ++lock.releases;
}
struct ReferenceSource {
    std::uintptr_t actor = 0;
    std::uint32_t handle = 0;
    unsigned depth = 0, attempts = 0, cleanups = 0;
    bool available = true, order_ok = true;
    std::array<OwnerLock, 2>* locks = nullptr;
};
void acquire_reference(void* context, std::uint32_t handle, ref::Receipt& receipt) noexcept {
    auto& source = *static_cast<ReferenceSource*>(context);
    ++source.attempts;
    std::memcpy(receipt.bytes.data(), &context, sizeof(context)); // fixture cleanup metadata
    std::memcpy(receipt.bytes.data() + 8, &source.actor, sizeof(source.actor));
    if (source.available && handle == source.handle) { receipt.bytes[0x10] = 1; ++source.depth; }
}
void release_reference(void* context, ref::Receipt& receipt) noexcept {
    ReferenceSource* source = nullptr;
    std::memcpy(&source, receipt.bytes.data(), sizeof(source));
    source->order_ok &= context == source;
    ++source->cleanups;
    source->order_ok &= !(*source->locks)[0].depth && !(*source->locks)[1].depth;
    if (receipt.bytes[0x10]) --source->depth;
    receipt.bytes[0x10] = 0;
}
}
