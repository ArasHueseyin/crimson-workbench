#include "repair_plan.h"
#include "repair_action.h"
#include "repair_reader.h"
#include "repair_lease.h"
#include "hash.h"
#include "pe.h"
#include <Windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <iostream>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

using namespace crimson::repair;
namespace {
unsigned checks = 0;
std::atomic<unsigned> native_calls{0};
void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
template<class T, std::size_t N>
void put(std::array<std::uint8_t, N>& bytes, std::size_t offset, T value) {
    if (offset > N || sizeof(T) > N - offset) throw std::runtime_error("Fixture write bounds");
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}
template<class T, std::size_t N>
T get(const std::array<std::uint8_t, N>& bytes, std::size_t offset) {
    if (offset > N || sizeof(T) > N - offset) throw std::runtime_error("Fixture read bounds");
    T value;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

// Own process, private allocation, no imported game image and no entry point.
// Commit selected helpers/updater, their unwind info, fixture stubs and errors.
// No page is ever writable and executable at the same time.
class Arena {
    static constexpr std::size_t size = 0x6d70000;
    std::uint8_t* base_ = nullptr;
    std::map<std::size_t, DWORD> pages_;
    std::array<RUNTIME_FUNCTION, 21> functions_{};
    bool registered_ = false;
public:
    Arena() {
        base_ = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS));
        if (!base_) throw std::runtime_error("Cannot reserve private fixture arena");
    }
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    ~Arena() {
        if (registered_) RtlDeleteFunctionTable(functions_.data());
        VirtualFree(base_, 0, MEM_RELEASE);
    }
    void write(std::uint32_t rva, std::span<const std::uint8_t> bytes, DWORD final_protection) {
        if (rva > size || bytes.size() > size - rva || bytes.empty())
            throw std::runtime_error("Fixture arena bounds");
        const auto first = rva & ~std::size_t(4095);
        const auto end = (rva + bytes.size() + 4095) & ~std::size_t(4095);
        for (auto page = first; page < end; page += 4096) {
            if (!pages_.contains(page)) {
                if (!VirtualAlloc(base_ + page, 4096, MEM_COMMIT, PAGE_READWRITE))
                    throw std::runtime_error("Cannot commit fixture page");
            } else {
                DWORD old;
                if (!VirtualProtect(base_ + page, 4096, PAGE_READWRITE, &old))
                    throw std::runtime_error("Cannot reopen fixture page");
                if (pages_.at(page) != final_protection)
                    throw std::runtime_error("Unexpected mixed code/data fixture page");
            }
            pages_[page] = final_protection;
        }
        std::memcpy(base_ + rva, bytes.data(), bytes.size());
        for (auto page = first; page < end; page += 4096) {
            DWORD old;
            if (!VirtualProtect(base_ + page, 4096, final_protection, &old))
                throw std::runtime_error("Cannot seal fixture page");
        }
        if (final_protection == PAGE_EXECUTE_READ &&
            !FlushInstructionCache(GetCurrentProcess(), base_ + rva, bytes.size()))
            throw std::runtime_error("Cannot flush private instruction cache");
    }
    void constant(std::uint32_t rva, std::uint64_t value) {
        std::array<std::uint8_t, 11> code{0x48, 0xb8}; // mov rax,imm64; ret
        std::memcpy(code.data() + 2, &value, 8);
        code[10] = 0xc3;
        write(rva, code, PAGE_EXECUTE_READ);
    }
    void service_error(std::uint32_t error) {
        // Fixture 0x141acde60: fill result pointed to by RDX and return RDX.
        std::array<std::uint8_t, 10> code{0xc7, 0x02, 0, 0, 0, 0, 0x48, 0x89, 0xd0, 0xc3};
        std::memcpy(code.data() + 2, &error, 4);
        write(0x1acde60, code, PAGE_EXECUTE_READ);
    }
    void redirect(std::uint32_t rva, std::uintptr_t callback) {
        std::array<std::uint8_t, 12> code{0x48, 0xb8}; // mov rax,callback; jmp rax
        std::memcpy(code.data() + 2, &callback, 8);
        code[10] = 0xff; code[11] = 0xe0;
        write(rva, code, PAGE_EXECUTE_READ);
    }
    void register_unwind(Pe& pe) {
        functions_ = {pe.function(0x27b04c0, 0x118), pe.function(0x27b05e0, 0x276),
            pe.function(common_rva, common_size), pe.function(server_rva, server_size),
            pe.function(0x240d650, 0x195), pe.function(0x212a0f0, 0x7f),
            pe.function(0x212f4a0, 0x44), pe.function(0x212f4e4, 0x7a),
            pe.function(0x212f55e, 0x11), pe.function(0x212f56f, 0x13),
            pe.function(0x2a82730, 0x21e), pe.function(0x393650, 0x68), pe.function(0x98fb70, 0x305),
            pe.function(0x240e3e0, 0x28), pe.function(0x240e408, 0x19e), pe.function(0x240e5a6, 0xb),
            pe.function(0x1371c20, 0xae), pe.function(0x1434900, 0x9c),
            pe.function(0x1436690, 0x2f5), pe.function(0x1436bf0, 0xf6), pe.function(0x1436990, 0x98)};
        std::sort(functions_.begin(), functions_.end(), [](const auto& a, const auto& b) {
            return a.BeginAddress < b.BeginAddress;
        });
        for (const auto& entry : functions_) write(entry.UnwindData, pe.unwind(entry), PAGE_READONLY);
        if (!RtlAddFunctionTable(functions_.data(), static_cast<DWORD>(functions_.size()),
            reinterpret_cast<DWORD64>(base_)))
            throw std::runtime_error("Cannot register fixture unwind data");
        registered_ = true;
    }
    template<class F> F function(std::uint32_t rva) {
        return reinterpret_cast<F>(base_ + rva);
    }
    void check_protection() {
        for (const auto& [page, expected] : pages_) {
            MEMORY_BASIC_INFORMATION info{};
            require(VirtualQuery(base_ + page, &info, sizeof(info)) == sizeof(info), "Query fixture page");
            require(info.Type == MEM_PRIVATE && info.Protect == expected, "Fixture page protection");
        }
    }
};

// ABI reconstructed from the hash-pinned helpers. Dependencies are mocks;
// these calls therefore prove helper arithmetic, not in-game repair routing.
using Common = std::uint32_t* (*)(void*, std::uint32_t*, std::uint16_t, std::uint8_t,
    const std::int64_t*, std::int64_t*, std::uint16_t*);
using Server = std::uint32_t* (*)(void*, std::uint32_t*, void*, void*, std::uint16_t,
    std::int64_t, std::uint8_t, void*, void*, std::uint16_t*);
using DebitPrepare = std::uint32_t* (*)(void*, std::uint32_t*, void*, void*, std::uint64_t,
    std::uint8_t, std::uint16_t, void*, bool, bool, bool);
using DebitCommit = std::uint32_t* (*)(void*, std::uint32_t*);

struct Fixture {
    std::array<std::uint8_t, 0x440> info{};
    std::array<std::uint8_t, 0x80> item{}, resource{};
    std::array<std::uint8_t, 16> rule{}, requests{};
    std::array<std::uint8_t, 160> request_storage{};
    std::uint32_t error = 0xcccccccc;
    std::int64_t available = 5, consumed = -77;
    std::uint16_t repaired = 0xbeef;
    Fixture(std::uint16_t maximum, std::uint16_t current, std::uint16_t unit,
        std::int64_t quantity, bool has_rule = true, std::uint8_t style = 2) : available(quantity) {
        put(info, 0x400, maximum);
        put(info, 0x408, rule.data());
        put(info, 0x410, std::uint32_t(has_rule ? 1 : 0));
        put(info, 0x428, std::uint16_t(7));
        put(item, 8, std::uint16_t(23));
        put(item, 0x40, current);
        put(resource, 8, std::uint16_t(77));
        put(resource, 0x10, quantity);
        put(rule, 0, std::uint16_t(77));
        put(rule, 2, unit);
        put(rule, 4, style);
        put(rule, 8, std::int64_t(3));
        put(requests, 0, request_storage.data());
        put(requests, 8, std::uint32_t(0));
        put(requests, 12, std::uint32_t(4));
    }
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
};

// These tiny SEH frames intentionally have no C++ objects to unwind.
DWORD invoke_common(Common function, Fixture* f) {
    __try {
        function(f->item.data(), &f->error, 77, 2, &f->available, &f->consumed, &f->repaired);
        return 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
DWORD invoke_server(Server function, Fixture* f) {
    __try {
        function(nullptr, &f->error, f->item.data(), f->resource.data(), 9,
            f->available, 2, nullptr, f->requests.data(), &f->repaired);
        return 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
DWORD invoke_empty_debit(DebitPrepare prepare_debit, DebitCommit commit_debit,
    void* transaction, void* requests, std::uint32_t* error) {
    __try {
        prepare_debit(transaction, error, nullptr, requests, 0, 3, 4, nullptr, false, true, true);
        if (*error == 0) commit_debit(transaction, error);
        return 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
DWORD run(Arena& arena, Fixture& f, bool server, std::int64_t unit_cost = 3,
    std::uint32_t service_error = 0) {
    arena.constant(0x38ab60, reinterpret_cast<std::uint64_t>(f.info.data()));
    arena.constant(0x2411f20, static_cast<std::uint64_t>(unit_cost));
    arena.service_error(service_error);
    const auto item = f.item, resource = f.resource;
    const auto rule = f.rule;
    const auto info = f.info;
    ++native_calls;
    const DWORD exception = server ? invoke_server(arena.function<Server>(server_rva), &f)
                                   : invoke_common(arena.function<Common>(common_rva), &f);
    require(f.item == item && f.resource == resource && f.rule == rule && f.info == info,
        "Calculation unexpectedly mutated item, resource or rule");
    return exception;
}

using CommonBytes = std::array<std::uint8_t, common_size>;
using ServerBytes = std::array<std::uint8_t, server_size>;
Status prepare(std::span<const std::uint8_t> common, std::span<const std::uint8_t> server,
    CommonBytes& out_common, ServerBytes& out_server) {
    return crimson_repair_prepare(abi_version, common.data(), common.size(), server.data(), server.size(),
        out_common.data(), out_common.size(), out_server.data(), out_server.size());
}
void contract_tests() {
    require(!crimson_repair_installable(), "Prototype must never advertise installation readiness");
    const std::array<std::uint8_t, 3> abc{'a', 'b', 'c'};
    require(hash(abc) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        "SHA-256 known vector");
    CommonBytes common{}, common_output;
    ServerBytes server{}, server_output;
    common_output.fill(0xa7); server_output.fill(0x3e);
    const auto saved_common = common_output;
    const auto saved_server = server_output;
    require(prepare(common, server, common_output, server_output) == Status::unknown_code, "Unknown code refused");
    require(common_output == saved_common && server_output == saved_server, "Failure changed output");
    require(prepare(std::span(common).first(common.size() - 1), server, common_output, server_output) ==
        Status::invalid_argument, "Truncation refused");
    require(crimson_repair_prepare(99, common.data(), common.size(), server.data(), server.size(),
        common_output.data(), common_output.size(), server_output.data(), server_output.size()) ==
        Status::invalid_argument, "Unknown ABI refused");
    require(crimson_repair_prepare(1, nullptr, common.size(), server.data(), server.size(),
        common_output.data(), common_output.size(), server_output.data(), server_output.size()) ==
        Status::invalid_argument, "Null pointer refused");
    require(crimson_repair_prepare(1, common.data(), common.size(), server.data(), server.size(),
        common.data(), common.size(), server_output.data(), server_output.size()) ==
        Status::invalid_argument, "In-place modification refused");
    require(crimson_repair_prepare(1, common.data(), common.size(), server.data(), server.size(),
        common_output.data(), common_output.size(), common_output.data() + 1, server_output.size()) ==
        Status::invalid_argument, "Overlapping outputs refused");
    require(common_output == saved_common && server_output == saved_server, "Invalid call changed output");
}

using EnduranceDelta = bool (*)(void*, std::int16_t);
DWORD invoke_delta(EnduranceDelta function, void* item, std::int16_t delta, bool* changed) {
    __try { *changed = function(item, delta); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}

void own_action_native_tests(Pe& pe, Arena& arena) {
    namespace act = crimson::repair::action;
    const auto updater = pe.rva(0x240d650, 0x195);
    require(hash(updater) == "cdbf73177286a1465e808e462c5465c06756ab9910183eb09c2321ee5a195a3f",
        "Unexpected endurance updater");
    arena.write(0x240d650, updater, PAGE_EXECUTE_READ);
    const std::array<std::uint8_t, 8> no_default{};
    arena.write(0x6cfcd90, no_default, PAGE_READONLY);
    std::array<std::uint8_t, 0x440> definition{};
    put(definition, 0x400, std::uint16_t(100));
    arena.constant(0x38ab60, reinterpret_cast<std::uint64_t>(definition.data()));
    act::Item item;
    item.identity = {1, act::Area::equipped, 0, 3};
    auto& raw = item.authority.bytes;
    put(raw, 0, std::uint64_t(1));
    put(raw, 8, std::uint16_t(10));
    put(raw, 0x10, std::int64_t(1));
    put(raw, 0x40, std::uint16_t(35));
    put(raw, 0x68, std::uint32_t(2));
    raw[0x70] = 2;
    item.authority.sockets.resize(2);
    for (std::size_t i = 0; i < 2; ++i) {
        auto& socket = item.authority.sockets[i].bytes;
        put(socket, 0, static_cast<std::uint16_t>(20 + i));
        put(socket, 2, static_cast<std::uint16_t>(10 * i));
        socket[4] = static_cast<std::uint8_t>(i);
        socket[5] = 5;
    }
    item.presentation = item.authority;
    const auto before = item;
    // The unmodified engine updater is NOT a general repair operation: +65
    // repairs this main item but sets the 10/100 socket to zero. Prove this only
    // in our private fixture, then use independent per-field repair targets.
    auto native = item.authority;
    put(native.bytes, 0x60, native.sockets.data());
    bool changed = false;
    require(invoke_delta(arena.function<EnduranceDelta>(0x240d650), native.bytes.data(), 65, &changed) == 0 &&
        changed, "Native positive-delta fixture");
    ++native_calls;
    require(get<std::uint16_t>(native.bytes, 0x40) == 100, "Native main endurance layout");
    require(get<std::uint16_t>(native.sockets[0].bytes, 2) == 0 &&
        get<std::uint16_t>(native.sockets[1].bytes, 2) == 0, "Native socket positive-delta behavior");

    act::Snapshot owned{{1, 1, 1}, {{10, 100}, {20, 100}, {21, 100}}, {item}};
    act::Plan plan;
    require(act::prepare({owned.session, act::Scope::all, {}}, owned, plan) == act::Code::prepared &&
        plan.count() == act::Count{1, 1, 2}, "Own action repairs root and each socket independently");
    require(act::apply_to_owned_snapshot(plan, owned) == act::Code::applied, "Own action copied-instance commit");
    require(item == before, "Source fixture was changed");
    for (const auto& image : {owned.items[0].authority, owned.items[0].presentation}) {
        require(get<std::uint16_t>(image.bytes, 0x40) == 100 &&
            get<std::uint16_t>(image.sockets[0].bytes, 2) == 100 &&
            get<std::uint16_t>(image.sockets[1].bytes, 2) == 100, "Own repair results");
    }
    // Round trip through the ORIGINAL updater: zero delta agrees with all three
    // repaired fields, then ordinary wear -1 sees 99 in all three fields. This
    // proves field geometry/consumer compatibility, not UI, save or transaction.
    native = owned.items[0].authority;
    put(native.bytes, 0x60, native.sockets.data());
    require(invoke_delta(arena.function<EnduranceDelta>(0x240d650), native.bytes.data(), 0, &changed) == 0 &&
        !changed, "Original consumer accepts repaired fields");
    ++native_calls;
    require(invoke_delta(arena.function<EnduranceDelta>(0x240d650), native.bytes.data(), -1, &changed) == 0 &&
        changed, "Original wear still operates");
    ++native_calls;
    require(get<std::uint16_t>(native.bytes, 0x40) == 99 &&
        get<std::uint16_t>(native.sockets[0].bytes, 2) == 99 &&
        get<std::uint16_t>(native.sockets[1].bytes, 2) == 99, "Original wear reads the own-action fields");
    native = owned.items[0].authority;
    put(native.bytes, 0x60, native.sockets.data());
    put(definition, 0x400, std::uint16_t(65535)); // simulated admitted no-wear table override
    require(invoke_delta(arena.function<EnduranceDelta>(0x240d650), native.bytes.data(), -1, &changed) == 0 &&
        !changed, "No-wear override preserves the repaired instance");
    ++native_calls;
    require(get<std::uint16_t>(native.bytes, 0x40) == 100 &&
        get<std::uint16_t>(native.sockets[0].bytes, 2) == 100 &&
        get<std::uint16_t>(native.sockets[1].bytes, 2) == 100, "Repair and no-wear cooperate");
}

using InventorySlot = void* (*)(void*, std::uint16_t, std::int16_t);
using InventoryHolder = void* (*)(void*);
using RegistryLookup = void* (*)(void*, void*, std::uint32_t);
DWORD invoke_slot(InventorySlot function, void* holder, std::uint16_t type, std::int16_t slot, void** result) {
    __try { *result = function(holder, type, slot); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
DWORD invoke_holder(InventoryHolder function, void* actor, void** result) {
    __try { *result = function(actor); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
DWORD invoke_registry(RegistryLookup function, void* manager, void* out, std::uint32_t handle) {
    __try { function(manager, out, handle); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}

void inventory_native_tests(Pe& pe, Arena& arena) {
    const auto getter = pe.rva(0x212f4a0, 0xe2);
    const auto holder_getter = pe.rva(0x212a0f0, 0x7f);
    require(hash(getter) == "959437d3eff481d79aa0d6e9bb4ead3bc54ab2f408d80cddcf689c3b33506c9d" &&
        hash(holder_getter) == "e4b46826485c932d72fb7c175b0e4c3943c8dee8e6cd01a5378ab170f17162c5",
        "Pinned inventory accessors");
    arena.write(0x212f4a0, getter, PAGE_EXECUTE_READ);
    arena.write(0x212a0f0, holder_getter, PAGE_EXECUTE_READ);
    std::array<std::uint8_t, 0x28> holder{};
    std::array<std::uint8_t, 0x30> bag{}, storage{};
    std::array<std::uint8_t, 3 * action::item_size> items{};
    std::array<std::uint8_t, action::item_size> empty{};
    std::array<std::uint8_t, 24> exclusions{};
    std::array<void*, 2> bags{storage.data(), bag.data()};
    put(holder, 0x18, bags.data()); put(holder, 0x20, std::uint32_t(2));
    put(storage, 0x10, std::uint16_t(8));
    put(bag, 0, items.data()); put(bag, 8, std::uint16_t(1)); // deliberately differs from actual bound
    put(bag, 0xc, std::uint16_t(3)); put(bag, 0x10, std::uint16_t(2));
    put(empty, 8, std::uint16_t(0xffff));
    const auto default_pointer = reinterpret_cast<std::uintptr_t>(empty.data());
    arena.write(0x6cfcd98, {reinterpret_cast<const std::uint8_t*>(&default_pointer), sizeof(default_pointer)}, PAGE_READONLY);
    for (std::size_t i = 0; i < 3; ++i) {
        put(items, i * action::item_size, std::uint64_t(i + 1));
        put(items, i * action::item_size + 8, static_cast<std::uint16_t>(10 + i));
        put(items, i * action::item_size + 0x10, std::int64_t(1));
    }
    auto compare = [&](std::uint16_t type, std::int16_t slot, std::uintptr_t expected) {
        const auto before_items = items;
        const auto before_empty = empty;
        const auto before_bag = bag, before_storage = storage;
        const auto before_holder = holder;
        const auto before_exclusions = exclusions;
        void* native = nullptr;
        ++native_calls;
        require(invoke_slot(arena.function<InventorySlot>(0x212f4a0), holder.data(), type, slot, &native) == 0,
            "Native inventory lookup exception");
        auto normalized = reinterpret_cast<std::uintptr_t>(native);
        if (native == empty.data()) normalized = 0;
        else {
            const auto start = reinterpret_cast<std::uintptr_t>(items.data());
            require(normalized >= start && normalized < start + items.size() &&
                (normalized - start) % action::item_size == 0, "Native returned a fixture slot");
            const auto offset = normalized - start;
            if (get<std::uint16_t>(items, offset + 8) == 0xffff || get<std::int64_t>(items, offset + 0x10) <= 0)
                normalized = 0; // caller-level presence check, not an engine allocation
        }
        reader::LocalMemory memory;
        std::uintptr_t captured = UINTPTR_MAX;
        require(reader::inventory_slot(memory, reinterpret_cast<std::uintptr_t>(holder.data()), type, slot, captured) ==
            reader::Code::captured && normalized == expected && captured == expected,
            "Reader/native inventory lookup agree");
        require(items == before_items && empty == before_empty && bag == before_bag && storage == before_storage &&
            holder == before_holder && exclusions == before_exclusions, "Accessors preserve all fixture data");
    };
    for (std::int16_t slot = 0; slot < 3; ++slot)
        compare(2, slot, reinterpret_cast<std::uintptr_t>(items.data() + slot * action::item_size));
    for (const auto slot : {std::int16_t(-32768), std::int16_t(-1), std::int16_t(3), std::int16_t(32767)})
        compare(2, slot, 0);
    compare(99, 0, 0);
    put(items, 8, std::uint16_t(0xffff)); compare(2, 0, 0);
    put(items, 8, std::uint16_t(10));
    for (const auto quantity : {std::int64_t(0), std::int64_t(-1)}) {
        put(items, 0x10, quantity); compare(2, 0, 0);
    }
    put(items, 0x10, std::int64_t(1));
    put(bag, 0x20, exclusions.data()); put(bag, 0x28, std::uint32_t(2));
    put(exclusions, 0, std::uint16_t(99)); put(exclusions, 12, std::uint16_t(10));
    compare(2, 0, 0); // second 12-byte filter record
    compare(2, 2, reinterpret_cast<std::uintptr_t>(items.data() + 2 * action::item_size));
    put(bag, 0, arena.function<void*>(0x1000)); // reserved PAGE_NOACCESS in this process
    void* unused = nullptr;
    ++native_calls;
    require(invoke_slot(arena.function<InventorySlot>(0x212f4a0), holder.data(), 2, 0, &unused) ==
        EXCEPTION_ACCESS_VIOLATION, "Chained unwind handles deliberate invalid private slot");
    reader::LocalMemory memory;
    std::uintptr_t missing = 1;
    require(reader::inventory_slot(memory, reinterpret_cast<std::uintptr_t>(holder.data()), 2, 0, missing) ==
        reader::Code::unreadable && missing == 0, "Reader refuses inaccessible slot before dereferencing");

    std::array<std::uint8_t, 0xa8> actor{}, pawn{};
    std::array<std::uint8_t, 0xc0> parts{}, pawn_parts{}, info{};
    std::array<std::uint8_t, 0xd8> possessor{};
    std::array<std::uint8_t, 0x60> mode{};
    std::array<std::uint8_t, 0x32> status{};
    put(actor, 0x68, parts.data()); put(actor, 0xa0, possessor.data());
    put(parts, 0x20, status.data()); put(parts, 0xb8, holder.data());
    put(pawn, 0x68, pawn_parts.data()); put(pawn_parts, 0xb8, storage.data());
    put(possessor, 0xd0, pawn.data());
    arena.constant(0x389570, reinterpret_cast<std::uintptr_t>(info.data()));
    arena.constant(0x3891b0, reinterpret_cast<std::uintptr_t>(mode.data()));
    auto resolve = [&](void* expected) {
        void* actual = nullptr;
        ++native_calls;
        require(invoke_holder(arena.function<InventoryHolder>(0x212a0f0), actor.data(), &actual) == 0 && actual == expected,
            "Current actor/possessor inventory geometry");
    };
    put(info, 0xbe, std::uint16_t(0xffff)); resolve(holder.data());
    put(info, 0xbe, std::uint16_t(1));
    mode[0x5f] = 0; resolve(nullptr);
    mode[0x5f] = 3; resolve(holder.data());
    mode[0x5f] = 1; resolve(storage.data());
    put(possessor, 0xd0, actor.data()); resolve(holder.data()); // reader requires this roundtrip
    put(possessor, 0xd0, std::uintptr_t(0)); resolve(nullptr);
    put(actor, 0xa0, std::uintptr_t(0)); resolve(nullptr);
}

struct RegistryProbe { unsigned acquired = 0, released = 0, accesses = 0; bool allow = true, abi = true; };
RegistryProbe* registry_probe(void* object, std::size_t offset) noexcept {
    RegistryProbe* probe;
    std::memcpy(&probe, static_cast<std::uint8_t*>(object) + offset, sizeof(probe));
    return probe;
}
void fixture_acquire(void* lock, bool exclusive) noexcept {
    auto* p = registry_probe(lock, 0x10);
    ++p->acquired; p->abi = p->abi && exclusive;
}
void fixture_release(void* lock, bool exclusive) noexcept {
    auto* p = registry_probe(lock, 0x10);
    ++p->released; p->abi = p->abi && exclusive;
}
bool fixture_actor_access(void* actor, std::uint32_t mode, std::uint32_t flags, const std::uint32_t* handle) noexcept {
    auto* p = registry_probe(actor, 0x20);
    ++p->accesses;
    p->abi = p->abi && mode == 4 && flags == 0x10 && handle;
    std::uint32_t actual;
    std::memcpy(&actual, static_cast<std::uint8_t*>(actor) + 0x60, sizeof(actual));
    // Mock lease only: equality is a conservative fixture contract, not a
    // reverse-engineered implementation of the real actor virtual method.
    return p->allow && p->abi && *handle == actual;
}
void registry_native_tests(Pe& pe, Arena& arena) {
    const auto lookup = pe.rva(0x2a82730, 0x21e);
    const auto valid = pe.rva(0x466060, 0x58); // leaf + image-relative switch table
    require(hash(lookup) == "df6ba99ef13614b5a8b8cbcd7c200127a176788905134ff9b445a91c6cf43c68" &&
        hash(valid) == "5f31dba7c7e552dc6983e9288297f246d87d5fb9bfb90506e905b121ecc916a8",
        "Pinned registry/actor-type helpers");
    arena.write(0x2a82730, lookup, PAGE_EXECUTE_READ);
    arena.write(0x466060, valid, PAGE_EXECUTE_READ);
    for (int scenario = 0; scenario < 12; ++scenario) {
        std::array<std::uint8_t, 0xb8> manager{};
        std::array<std::uint8_t, 0xa8> actor{};
        std::array<std::uint8_t, 256 * 3> buckets{};
        std::array<std::uint8_t, 16> node{};
        std::array<std::uint8_t, 2> descriptor{};
        std::array<std::uint8_t, 32> out{};
        std::array<void*, 3> nodes{nullptr, nullptr, node.data()};
        std::array<std::uintptr_t, 5> lock_vtable{};
        std::array<std::uintptr_t, 24> actor_vtable{};
        RegistryProbe probe;
        std::uint32_t handle = 0x12346; // nonzero modulo bucket, nonzero node index
        lock_vtable[3] = reinterpret_cast<std::uintptr_t>(&fixture_acquire);
        lock_vtable[4] = reinterpret_cast<std::uintptr_t>(&fixture_release);
        actor_vtable[23] = reinterpret_cast<std::uintptr_t>(&fixture_actor_access);
        put(manager, 0x68, lock_vtable.data()); put(manager, 0x78, &probe);
        put(manager, 0x98, std::uint32_t(3)); put(manager, 0x9c, std::uint32_t(1));
        put(manager, 0xa8, buckets.data()); put(manager, 0xb0, nodes.data());
        const auto bucket = (handle % 3) * std::size_t(256);
        put(buckets, bucket, std::uint32_t(2));
        put(buckets, bucket + 8, std::uint32_t(99));
        put(buckets, bucket + 16, handle); put(buckets, bucket + 20, std::uint32_t(2));
        put(node, 4, handle); put(node, 8, actor.data());
        put(actor, 0, actor_vtable.data()); put(actor, 0x20, &probe);
        put(actor, 0x60, handle); put(actor, 0x88, descriptor.data());
        descriptor[1] = 1;
        bool resolved = scenario == 0 || scenario == 11;
        bool accessed = resolved || scenario == 1 || scenario == 9;
        bool locked = scenario != 2 && scenario != 3;
        if (scenario == 1) handle += 0x100000; // same low key, mock lease refuses generation mismatch
        if (scenario == 2) handle = 0;
        if (scenario == 3) handle |= 0x40000000;
        if (scenario == 4) put(manager, 0x9c, std::uint32_t(0));
        if (scenario == 5) put(manager, 0x98, std::uint32_t(0));
        if (scenario == 6) put(buckets, bucket + 16, std::uint32_t(98));
        if (scenario == 7) put(node, 4, std::uint32_t(98));
        if (scenario == 8) descriptor[1] = 2;
        if (scenario == 9) probe.allow = false;
        if (scenario == 10) put(buckets, bucket, std::uint32_t(0));
        if (scenario == 11) { handle |= 0x60000000; put(actor, 0x60, handle); }
        const auto before_manager = manager;
        const auto before_actor = actor;
        const auto before_buckets = buckets;
        const auto before_node = node;
        ++native_calls;
        require(invoke_registry(arena.function<RegistryLookup>(0x2a82730), manager.data(), out.data(), handle) == 0,
            "Native registry lookup exception");
        require(probe.abi && probe.acquired == unsigned(locked) && probe.released == unsigned(locked) &&
            probe.accesses == unsigned(accessed), "Registry lock/access ABI and balanced release");
        require(out[0x10] == unsigned(resolved) && out[0x11] == 0 && get<std::uintptr_t>(out, 0x18) == 0,
            "Registry result validity geometry");
        require(get<void*>(out, 8) == (accessed ? actor.data() : nullptr),
            "An invalid lease can retain an actor pointer; validity must be checked");
        require(manager == before_manager && actor == before_actor && buckets == before_buckets && node == before_node,
            "Registry fixture changes only output and mock counters");
    }
}

struct AckProbe {
    unsigned acquires = 0, releases = 0, references = 0, appends = 0, frees = 0, sockets = 0, panels = 0;
    bool abi = true;
    std::array<std::uint8_t, 12> removed{};
};
AckProbe* ack_probe = nullptr; // serial fixture callbacks; never a game-global hook
void ack_acquire(void*, bool flag) noexcept { ++ack_probe->acquires; ack_probe->abi &= !flag; }
void ack_release(void*, bool flag) noexcept { ++ack_probe->releases; ack_probe->abi &= !flag; }
void ack_reference(void*, bool first) noexcept { ++ack_probe->references; ack_probe->abi &= first; }
void ack_append(void* list, const void* socket) noexcept {
    auto* bytes = static_cast<std::uint8_t*>(list);
    std::uint32_t count;
    std::memcpy(&count, bytes + 8, 4);
    if (count >= 2) { ack_probe->abi = false; return; }
    std::memcpy(ack_probe->removed.data() + count * 6, socket, 6);
    const auto pointer = ack_probe->removed.data();
    std::memcpy(bytes, &pointer, 8);
    ++count; std::memcpy(bytes + 8, &count, 4);
    ++ack_probe->appends;
}
void ack_free(void* pointer) noexcept {
    ++ack_probe->frees;
    ack_probe->abi &= pointer == ack_probe->removed.data(); // owns no heap allocation
}
void* ack_item_key(void* out, std::uint32_t key) noexcept {
    const auto small = static_cast<std::uint16_t>(key);
    std::memcpy(out, &small, 2);
    return out;
}
void ack_socket_ui(void*, std::uint16_t item, std::uint16_t socket) noexcept {
    ++ack_probe->sockets;
    ack_probe->abi &= item == 10 && (socket == 20 || socket == 21);
}
void ack_panel_ui(void*) noexcept { ++ack_probe->panels; }
using ClientAck = void* (*)(void*, std::uint32_t*, std::uint16_t, std::int16_t, void*);
DWORD invoke_ack(ClientAck function, void* component, std::uint32_t* error, std::uint16_t slot,
    std::int16_t delta, void* removed) {
    __try { function(component, error, slot, delta, removed); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
void acknowledgement_native_tests(Pe& pe, Arena& arena) {
    auto ack = pe.rva(0x98fb70, 0x305);
    const auto lock = pe.rva(0x393650, 0x68);
    const auto collector = pe.rva(0x240e3e0, 0x1d1);
    require(hash(ack) == "d8221b3537464395a14ebbaf63b2bf124d15bbe0167c05e9a585f7338eb87aa1" &&
        hash(lock) == "9361a1820340878d8bc677ec8cd21654fbab72c61102b5529939f23533e18e2f" &&
        hash(collector) == "88087dcf346127a076e9c715e91d78e3ce5b7ef5e8a10e3ca6a71a9afc8f759b",
        "Pinned client acknowledgement, lock constructor and destructive socket collector");
    // Redirect exactly the allocator's TLS-vector load into private fixture
    // data. No GS/TEB/TLS of the host is modified or interpreted as game TLS.
    // Every other instruction of the acknowledgement is copied unchanged.
    constexpr std::size_t tls_at = 0x98fe2a - 0x98fb70;
    const std::array<std::uint8_t, 9> gs_load{0x65,0x48,0x8b,0x04,0x25,0x58,0,0,0};
    require(std::equal(gs_load.begin(), gs_load.end(), ack.begin() + tls_at), "Exact fixture TLS substitution site");
    const auto original_ack = ack;
    std::array<std::uint8_t, 9> lea{0x48,0x8d,0x05,0,0,0,0,0x90,0x90};
    const std::int32_t displacement = 0x6cffe00 - (0x98fe2a + 7);
    std::memcpy(lea.data() + 3, &displacement, 4);
    std::copy(lea.begin(), lea.end(), ack.begin() + tls_at);
    for (std::size_t i = 0; i < ack.size(); ++i)
        if (i < tls_at || i >= tls_at + lea.size()) require(ack[i] == original_ack[i], "Ack changed outside TLS dependency");
    arena.write(0x98fb70, ack, PAGE_EXECUTE_READ);
    arena.write(0x393650, lock, PAGE_EXECUTE_READ);
    arena.write(0x240e3e0, collector, PAGE_EXECUTE_READ);
    arena.redirect(0x10d2b30, reinterpret_cast<std::uintptr_t>(&ack_append));
    arena.redirect(0x47f025c, reinterpret_cast<std::uintptr_t>(&ack_free));
    arena.redirect(0x38aac0, reinterpret_cast<std::uintptr_t>(&ack_item_key));
    arena.redirect(0x4324a0, reinterpret_cast<std::uintptr_t>(&ack_socket_ui));
    arena.redirect(0x423990, reinterpret_cast<std::uintptr_t>(&ack_panel_ui));
    for (const auto [rva, number] : std::array<std::pair<std::uint32_t, std::uint32_t>, 2>{{
        {0x6cf7248, 902}, {0x6cf7d4c, 1}}})
        arena.write(rva, {reinterpret_cast<const std::uint8_t*>(&number), sizeof(number)}, PAGE_READONLY);
    std::array<std::uint8_t, 0x200> tls{};
    const auto tls_pointer = reinterpret_cast<std::uintptr_t>(tls.data());
    arena.write(0x6cffe00, {reinterpret_cast<const std::uint8_t*>(&tls_pointer), 8}, PAGE_READONLY);
    for (int scenario = 0; scenario < 6; ++scenario) {
        AckProbe probe;
        ack_probe = &probe;
        std::array<std::uint8_t, 0x98> component{};
        std::array<std::uint8_t, 0x10> actor{};
        std::array<std::uint8_t, 0x18> mutex{}, table{};
        std::array<std::uintptr_t, 9> vtable{};
        vtable[3] = reinterpret_cast<std::uintptr_t>(&ack_acquire);
        vtable[4] = reinterpret_cast<std::uintptr_t>(&ack_release);
        vtable[6] = reinterpret_cast<std::uintptr_t>(&ack_reference);
        put(mutex, 0, vtable.data()); put(actor, 8, mutex.data()); put(component, 8, actor.data());
        std::array<std::uint8_t, 0xd0> slot{};
        std::array<action::Socket, 2> socket_data{};
        action::Item item;
        item.identity = {1, action::Area::equipped, 0, 3};
        put(item.authority.bytes, 0, std::uint64_t(1)); put(item.authority.bytes, 8, std::uint16_t(10));
        put(item.authority.bytes, 0x10, std::int64_t(1)); put(item.authority.bytes, 0x40, std::uint16_t(35));
        put(item.authority.bytes, 0x60, socket_data.data()); put(item.authority.bytes, 0x68, std::uint32_t(2));
        item.authority.bytes[0x70] = 2;
        for (std::size_t i = 0; i < 2; ++i) {
            put(socket_data[i].bytes, 0, static_cast<std::uint16_t>(20 + i));
            put(socket_data[i].bytes, 2, static_cast<std::uint16_t>(scenario == 4 ? 0 : 10 + i * 10));
            socket_data[i].bytes[4] = static_cast<std::uint8_t>(i);
        }
        item.authority.sockets.assign(socket_data.begin(), socket_data.end());
        item.presentation = item.authority;
        action::Snapshot expected{{1, 77, 1}, {{10, 100}, {20, 100}, {21, 100}}, {item}};
        action::Plan plan;
        require(action::prepare({expected.session, action::Scope::all, {}}, expected, plan) == action::Code::prepared &&
            action::apply_to_owned_snapshot(plan, expected) == action::Code::applied, "Prepare expected confirmed repair");
        const bool repaired_first = scenario == 1 || scenario == 2;
        const auto& client = repaired_first ? expected.items[0].presentation : item.presentation;
        std::copy(client.bytes.begin(), client.bytes.end(), slot.begin());
        std::copy(client.sockets.begin(), client.sockets.end(), socket_data.begin());
        put(slot, 0xc8, std::uint16_t(3));
        put(table, 8, slot.data()); put(table, 0x10, std::uint32_t(1)); put(component, 0x90, table.data());
        std::array<std::uint8_t, 0x440> info{};
        put(info, 0, std::uint32_t(10)); put(info, 0x400, std::uint16_t(100));
        arena.constant(0x38ab60, reinterpret_cast<std::uintptr_t>(info.data()));
        std::array<std::uint8_t, 0x88> context{};
        std::array<std::uint8_t, 0x5d0> ui{};
        put(context, 0x80, ui.data()); put(ui, 0x4e8, &probe); put(ui, 0x5c8, &probe);
        const auto context_pointer = reinterpret_cast<std::uintptr_t>(context.data());
        arena.write(0x6d691b0, {reinterpret_cast<const std::uint8_t*>(&context_pointer), 8}, PAGE_READONLY);
        std::array<std::uint8_t, 16> reported{};
        const auto expected_removed = socket_data;
        put(reported, 0, expected_removed.data());
        put(reported, 8, std::uint32_t(scenario == 4 ? 2 : scenario == 2 ? 1 : 0));
        std::uint32_t error = UINT32_MAX;
        const std::int16_t delta = scenario == 0 ? 65 : scenario == 3 ? -1 : 0;
        ++native_calls;
        require(invoke_ack(arena.function<ClientAck>(0x98fb70), component.data(), &error,
            scenario == 5 ? 99 : 3, delta, reported.data()) == 0, "Client acknowledgement private invocation");
        require(error == ((scenario == 0 || scenario == 2) ? 902u : 0u), "Ack validation error/result");
        require(probe.abi && probe.acquires == 1 && probe.releases == 1 && probe.references == 1 &&
            get<std::uint32_t>(mutex, 8) == 0, "Native scoped lock reference balanced");
        require(probe.panels == unsigned(scenario == 1 || scenario == 3 || scenario == 4) &&
            probe.sockets == (scenario == 4 ? 2u : 0u), "UI notifications follow successful validation only");
        require(probe.appends == (scenario == 0 || scenario == 4 ? 2u : 0u) &&
            probe.frees == (probe.appends ? 1u : 0u), "Broken-socket removal and fixture allocation cleanup");
        if (scenario == 0 || scenario == 4) {
            require(get<std::uint16_t>(socket_data[0].bytes, 0) == action::absent &&
                get<std::uint16_t>(socket_data[1].bytes, 0) == action::absent,
                "Acknowledgement removes depleted sockets even before a later mismatch error");
        }
        auto observed = expected;
        std::copy_n(slot.begin(), action::item_size, observed.items[0].presentation.bytes.begin());
        observed.items[0].presentation.sockets.assign(socket_data.begin(), socket_data.end());
        require(action::verify_applied(plan, observed) == (repaired_first ? action::Code::applied : action::Code::stale),
            "Return code alone never proves the planned repair was applied");
    }
    ack_probe = nullptr;
}

using TryOwnerLock = bool (*)(void*, bool) noexcept;
using ReleaseOwnerLock = void (*)(void*, bool) noexcept;
TryOwnerLock native_owner_try = nullptr;
ReleaseOwnerLock native_owner_release = nullptr;
void* fixture_lock_tls() noexcept {
    struct Tls { std::array<std::uint8_t, 0x200> bytes{}; std::uint8_t* pointer = bytes.data(); };
    thread_local Tls tls;
    return &tls.pointer;
}
bool measured_owner_try(void* object, bool shared) noexcept {
    ++native_calls;
    return native_owner_try(object, shared);
}
void measured_owner_release(void* object, bool shared) noexcept {
    ++native_calls;
    native_owner_release(object, shared);
}
struct alignas(8) OwnerLockFixture {
    std::array<std::uint8_t, 0x30> bytes{};
    OwnerLockFixture() {
        InitializeSRWLock(srw());
        put(bytes, 0x28, UINT32_MAX);
    }
    SRWLOCK* srw() { return reinterpret_cast<SRWLOCK*>(bytes.data() + 0x10); }
    lease::Binding binding() { return {bytes.data(), measured_owner_try, measured_owner_release}; }
    bool available() {
        if (!TryAcquireSRWLockExclusive(srw())) return false;
        ReleaseSRWLockExclusive(srw()); return true;
    }
    std::uint32_t depth() const { return get<std::uint32_t>(bytes, 0x2c); }
    std::uint32_t owner() const { return get<std::uint32_t>(bytes, 0x28); }
};
void owner_lock_native_tests(Pe& pe, Arena& arena) {
    auto attempt = pe.rva(0x1371c20, 0xae);
    const auto release = pe.rva(0x1371bf0, 0x28);
    require(hash(attempt) == "45f3b9499ce28442b126fd0941aa5711d1cc1a58e19a7d3893330d83ac430c90" &&
        hash(release) == "b43c0aebed083eebd4ce7e824ed7acb44bec3f81d92a8ab52cd1be3981a5740a",
        "Pinned WindowsRWLock try/release implementation");
    constexpr std::size_t tls_at = 0x1371c4f - 0x1371c20;
    const std::array<std::uint8_t, 9> gs_load{0x65,0x48,0x8b,0x04,0x25,0x58,0,0,0};
    require(std::equal(gs_load.begin(), gs_load.end(), attempt.begin() + tls_at), "Pinned lock TLS load");
    const auto original = attempt;
    // This point has no live volatile registers except the overwritten RAX.
    // The original stack already provides call alignment and 32-byte shadow
    // space. Obtain a PRIVATE per-thread vector; never replace real TEB/TLS.
    std::array<std::uint8_t, 9> call{0xe8,0,0,0,0,0x90,0x90,0x90,0x90};
    const std::int32_t displacement = 0x6cfe000 - (0x1371c4f + 5);
    std::memcpy(call.data() + 1, &displacement, 4);
    std::copy(call.begin(), call.end(), attempt.begin() + tls_at);
    for (std::size_t i = 0; i < attempt.size(); ++i)
        if (i < tls_at || i >= tls_at + call.size()) require(attempt[i] == original[i], "Lock code changed outside TLS boundary");
    arena.redirect(0x6cfe000, reinterpret_cast<std::uintptr_t>(&fixture_lock_tls));
    arena.write(0x1371c20, attempt, PAGE_EXECUTE_READ);
    arena.write(0x1371bf0, release, PAGE_EXECUTE_READ);
    // Real Windows SRW primitives, applied only to caller-owned initialized
    // locks. Unlike earlier probes these acquisition/release operations are
    // not counter stubs. No process handles, game objects or engine TLS used.
    for (const auto [rva, fn] : std::array<std::pair<std::uint32_t, std::uintptr_t>, 5>{{
        {0x51eb228, reinterpret_cast<std::uintptr_t>(&TryAcquireSRWLockShared)},
        {0x51eb230, reinterpret_cast<std::uintptr_t>(&TryAcquireSRWLockExclusive)},
        {0x51eb218, reinterpret_cast<std::uintptr_t>(&ReleaseSRWLockShared)},
        {0x51eb220, reinterpret_cast<std::uintptr_t>(&ReleaseSRWLockExclusive)},
        {0x51eb2e0, reinterpret_cast<std::uintptr_t>(&GetCurrentThreadId)}}})
        arena.write(rva, {reinterpret_cast<const std::uint8_t*>(&fn), sizeof(fn)}, PAGE_READONLY);
    native_owner_try = arena.function<TryOwnerLock>(0x1371c20);
    native_owner_release = arena.function<ReleaseOwnerLock>(0x1371bf0);
    std::array<OwnerLockFixture, 2> locks;
    const std::array bindings{locks[1].binding(), locks[0].binding()};
    {
        lease::Group group;
        require(group.try_acquire(bindings) == lease::Code::acquired, "Native owner lock pair acquired");
        require(locks[0].depth() == 1 && locks[1].depth() == 1 && locks[0].owner() == GetCurrentThreadId() &&
            locks[1].owner() == GetCurrentThreadId(), "Native thread identity and recursion fields");
        require(!locks[0].available() && !locks[1].available(), "Real Windows locks actually exclude writers");
    }
    require(locks[0].available() && locks[1].available() && locks[0].depth() == 0 && locks[1].depth() == 0 &&
        locks[0].owner() == UINT32_MAX && locks[1].owner() == UINT32_MAX, "Native pair fully released");
    require(measured_owner_try(locks[0].bytes.data(), false), "Existing outer engine write lease");
    {
        lease::Group group;
        require(group.try_acquire(bindings) == lease::Code::acquired && locks[0].depth() == 2,
            "Original recursive writer ownership preserved");
    }
    require(locks[0].depth() == 1 && !locks[0].available() && locks[1].available(), "Inner group never releases outer lease");
    measured_owner_release(locks[0].bytes.data(), false);
    require(measured_owner_try(locks[1].bytes.data(), false), "Main thread owns second native lock");
    lease::Code worker_result{};
    std::thread worker([&] {
        lease::Group group;
        worker_result = group.try_acquire(bindings);
    });
    worker.join();
    require(worker_result == lease::Code::busy && locks[0].available() && locks[0].depth() == 0 &&
        locks[1].depth() == 1 && !locks[1].available(), "Cross-thread contention rolls back first lock without blocking");
    measured_owner_release(locks[1].bytes.data(), false);
    require(measured_owner_try(locks[0].bytes.data(), true), "Native shared-read path");
    {
        lease::Group group;
        require(group.try_acquire(bindings) == lease::Code::busy && group.size() == 0,
            "Shared owner cannot silently upgrade to a writer");
    }
    measured_owner_release(locks[0].bytes.data(), true);
    require(locks[0].available() && locks[1].available() && locks[0].depth() == 0 && locks[1].depth() == 0,
        "All fixture synchronization restored");
    native_owner_try = nullptr; native_owner_release = nullptr;
}

#include "reference_native.inl"

void pinned_tests(const std::filesystem::path& path) {
    if (!IsProcessorFeaturePresent(PF_AVX_INSTRUCTIONS_AVAILABLE))
        throw std::runtime_error("This game's server helper requires AVX; fixture refused on this CPU");
    Pe pe(path);
    const auto original_common = pe.rva(common_rva, common_size);
    const auto original_server = pe.rva(server_rva, server_size);
    CommonBytes patched_common{};
    ServerBytes patched_server{};
    require(prepare(original_common, original_server, patched_common, patched_server) == Status::prepared,
        "Pinned pair rejected");
    const auto expected_common = patched_common;
    const auto expected_server = patched_server;
    auto bad_server = original_server;
    bad_server.back() ^= 1;
    require(prepare(original_common, bad_server, patched_common, patched_server) == Status::unknown_code,
        "Mixed build refused");
    require(patched_common == expected_common && patched_server == expected_server,
        "Pair validation is not all-or-nothing");
    auto bad_common = original_common;
    bad_common.front() ^= 1;
    require(prepare(bad_common, original_server, patched_common, patched_server) == Status::unknown_code,
        "Foreign hook refused");
    CommonBytes second_common{}; ServerBytes second_server{};
    require(prepare(patched_common, patched_server, second_common, second_server) == Status::unknown_code,
        "Stacked patch refused");
    for (std::size_t i = 0; i < common_size; ++i)
        if (i < 0x121 || i >= 0x13f) require(patched_common[i] == original_common[i], "Common outside diff");
    for (std::size_t i = 0; i < server_size; ++i)
        if ((i < 0x172 || i >= 0x197) && (i < 0x1df || i >= 0x282))
            require(patched_server[i] == original_server[i], "Server outside diff");

    Arena arena;
    arena.register_unwind(pe);
    inventory_native_tests(pe, arena);
    registry_native_tests(pe, arena);
    own_action_native_tests(pe, arena);
    acknowledgement_native_tests(pe, arena);
    owner_lock_native_tests(pe, arena);
    reference_native_tests(pe, arena);
    arena.write(0x27b04c0, pe.rva(0x27b04c0, 0x118), PAGE_EXECUTE_READ);
    arena.write(0x27b05e0, pe.rva(0x27b05e0, 0x276), PAGE_EXECUTE_READ);
    // Private symbolic error values; no game globals/data copied.
    for (const auto& [rva, code] : std::array<std::pair<std::uint32_t, std::uint32_t>, 5>{{
        {0x6cf7b5c, 101}, {0x6cf7b64, 102}, {0x6cf7b6c, 103}, {0x6cf7b70, 104}, {0x6cf749c, 105}}}) {
        const std::span bytes(reinterpret_cast<const std::uint8_t*>(&code), sizeof(code));
        arena.write(rva, bytes, PAGE_READONLY);
    }
    auto install = [&](bool patch) {
        arena.write(common_rva, patch ? std::span<const std::uint8_t>(patched_common) : original_common,
            PAGE_EXECUTE_READ);
        arena.write(server_rva, patch ? std::span<const std::uint8_t>(patched_server) : original_server,
            PAGE_EXECUTE_READ);
    };
    for (bool patch : {false, true}) {
        install(patch);
        for (bool server : {false, true}) {
            // Multiple units / remainders / balances, including very large costs.
            for (const auto [maximum, current, unit, quantity, cost] :
                std::array<std::array<std::int64_t, 5>, 8>{{
                    {100, 35, 10, 5, 3}, {100, 35, 1, 5, 3}, {100, 0, 7, 1000, 3},
                    {101, 99, 10, 3, 3}, {32767, 0, 1, 1, 1}, {100, 35, 10, -1, 3},
                    {100, 35, 10, 1, 1000000000}, {100, 35, 10, 1, 0}}}) {
                Fixture f(static_cast<std::uint16_t>(maximum), static_cast<std::uint16_t>(current),
                    static_cast<std::uint16_t>(unit), quantity);
                const auto exception = run(arena, f, server, cost);
                if (!patch && cost == 0) {
                    require(exception == EXCEPTION_INT_DIVIDE_BY_ZERO, "Expected baseline zero-cost division");
                    continue;
                }
                require(exception == 0, "Unexpected native helper exception");
                if (!patch && quantity != -1 && quantity < cost) {
                    require(f.error == 102 && f.repaired == 0xbeef, "Baseline material shortage");
                    continue;
                }
                const auto desired_units = (maximum - current + unit - 1) / unit;
                const auto units = patch || quantity == -1 ? desired_units : std::min(desired_units, quantity / cost);
                require(f.error == 0, "Valid repair calculation rejected");
                require(f.repaired == std::min(maximum, current + units * unit), "Unexpected repair amount");
                const auto expected_cost = patch ? 0 : units * cost;
                if (server) {
                    require(get<std::uint32_t>(f.requests, 8) == (patch ? 0u : 1u), "Server request count");
                    if (patch) {
                        require(std::all_of(f.request_storage.begin(), f.request_storage.end(),
                            [](auto b) { return b == 0; }), "Free repair wrote a debit");
                        std::array<std::uint8_t, 0x80> transaction{};
                        std::uint32_t error = 0xcccccccc;
                        require(invoke_empty_debit(arena.function<DebitPrepare>(0x27b04c0),
                            arena.function<DebitCommit>(0x27b05e0), transaction.data(),
                            f.requests.data(), &error) == 0 && error == 0,
                            "Engine debit preparation/commit rejected empty requests");
                        native_calls += 2;
                        require(get<std::uint32_t>(transaction, 0x58) == 0, "Empty debit acquired items");
                    } else {
                        require(get<std::int64_t>(f.request_storage, 0x18) == expected_cost, "Server material request");
                        require(get<std::uint16_t>(f.request_storage, 0x12) == 9, "Server slot preserved");
                    }
                } else require(f.consumed == expected_cost, "Common material count");
            }
            for (int scenario = 0; scenario < 6; ++scenario) {
                Fixture f(scenario == 4 ? 65535 : 100, scenario == 2 ? 100 : 35, 10,
                    scenario == 0 ? 0 : (scenario == 5 ? -2 : 5), scenario != 1, scenario == 3 ? 3 : 2);
                require(run(arena, f, server) == 0, "Error-case helper exception");
                const std::array<std::uint32_t, 6> errors{102, 101, 103, 104, 103, 102};
                require(f.error == errors[scenario] && f.repaired == 0xbeef, "Original refusal changed");
                if (server) require(get<std::uint32_t>(f.requests, 8) == 0, "Rejected repair queued a debit");
                else require(f.consumed == 0, "Rejected repair has nonzero debit");
            }
            if (server) {
                Fixture f(100, 35, 10, 5);
                require(run(arena, f, true, 3, 777) == 0 && f.error == 777 &&
                    get<std::uint32_t>(f.requests, 8) == 0, "Service error lost");
            }
            if (patch) {
                Fixture invalid_cost(100, 35, 10, 5);
                require(run(arena, invalid_cost, server, -3) == 0 && invalid_cost.error == 102 &&
                    invalid_cost.repaired == 0xbeef, "Negative cost must not grant a repair");
                if (server) {
                    Fixture existing(100, 35, 10, 5);
                    existing.request_storage.fill(0xa7);
                    put(existing.requests, 8, std::uint32_t(1));
                    const auto before = existing.request_storage;
                    require(run(arena, existing, true) == 0 && existing.error == 0 && existing.repaired == 100 &&
                        get<std::uint32_t>(existing.requests, 8) == 1 && existing.request_storage == before,
                        "Free repair changed a preexisting withdrawal request");
                }
            }
        }
    }
    // Restoring the original helper bytes restores its original material cap.
    install(false);
    for (bool server : {false, true}) {
        Fixture f(100, 35, 10, 5);
        require(run(arena, f, server) == 0 && f.error == 0 && f.repaired == 45, "Restore semantics");
    }
    arena.check_protection();
    pe.verify_unchanged();
}
}

int wmain(int argc, wchar_t** argv) {
    try {
        contract_tests();
        bool unknown_refused = false;
        try { Pe self(argv[0]); } catch (const std::runtime_error&) { unknown_refused = true; }
        require(unknown_refused, "Non-game EXE was accepted");
        const bool pinned = argc == 3 && std::wstring(argv[1]) == L"--exe";
        if (argc != 1 && !pinned) throw std::runtime_error("Usage: repair-harness [--exe <read-only game EXE>]");
        if (pinned) pinned_tests(argv[2]);
        std::cout << "{\"ok\":true,\"checks\":" << checks << ",\"native_calls\":" << native_calls.load()
            << ",\"pinned_source_verified_before_after\":" << (pinned ? "true" : "false")
            << ",\"installable\":false,\"game_process_access\":false,\"game_writes\":false,"
            << "\"own_action_field_roundtrip\":" << (pinned ? "true" : "false") << ','
            << "\"inventory_reader_native_comparison\":" << (pinned ? "true" : "false") << ','
            << "\"registry_layout_with_mock_leases\":" << (pinned ? "true" : "false") << ','
            << "\"client_ack_with_fixture_tls_and_callbacks\":" << (pinned ? "true" : "false") << ','
            << "\"client_ack_cases\":" << (pinned ? 6 : 0) << ','
            << "\"client_ack_tls_redirect_bytes\":" << (pinned ? 9 : 0) << ','
            << "\"native_engine_event_delivery\":false,"
            << "\"owner_locks_use_real_windows_srw\":" << (pinned ? "true" : "false") << ','
            << "\"owner_lock_tls_redirect_bytes\":" << (pinned ? 9 : 0) << ','
            << "\"native_reference_lifecycle_fixture\":" << (pinned ? "true" : "false") << ','
            << "\"reference_tls_redirects\":" << (pinned ? 3 : 0) << ','
            << "\"reference_tracking_map\":\"fixture_callbacks\","
            << "\"blockers\":[\"verified_game_transaction_and_player_resolver\","
            << "\"game_input_and_notifications\",\"loader_and_B0_installation\"]}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Repair prototype test failed: " << error.what() << '\n';
        return 1;
    }
}
