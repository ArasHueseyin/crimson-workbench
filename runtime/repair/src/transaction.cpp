#include "repair_transaction.h"
#include <algorithm>
#include <tuple>
#include <cstring>

namespace crimson::repair::transaction {
namespace {
bool copy_source_ready(const writer::Access& memory, const action::Image& image, std::size_t& budget) noexcept {
    const auto field = [&]<class T>(std::size_t at) {
        T value; std::memcpy(&value, image.bytes.data() + at, sizeof(value)); return value;
    };
    const auto readable = [&](std::uintptr_t pointer, std::size_t size) {
        std::array<std::uint8_t, 4096> scratch{};
        while (size) {
            const auto n = (std::min)(size, scratch.size());
            if (!memory.copy(pointer, std::span(scratch).first(n))) return false;
            pointer += n; size -= n;
        }
        return true;
    };
    for (const auto& [at, stride] : {std::pair{std::size_t(0x60), std::size_t(6)},
        std::pair{std::size_t(0x78), std::size_t(16)}, std::pair{std::size_t(0xa8), std::size_t(6)}}) {
        const auto pointer = field.operator()<std::uintptr_t>(at);
        const auto count = field.operator()<std::uint32_t>(at + 8);
        const auto capacity = field.operator()<std::uint32_t>(at + 12);
        const auto allocation = std::size_t(capacity) * stride;
        if (count > capacity || allocation > budget || (pointer && pointer % 2) ||
            (capacity && (pointer < 65536 || pointer > UINTPTR_MAX - allocation))) return false;
        budget -= allocation;
        // Original copiers allocate capacity but read only initialized records.
        if (!readable(pointer, std::size_t(count) * stride)) return false;
    }
    for (const auto& [at, size] : {std::pair{std::size_t(0xb8), std::size_t(24)},
        std::pair{std::size_t(0xc0), std::size_t(16)}}) {
        const auto pointer = field.operator()<std::uintptr_t>(at);
        if (!pointer) continue;
        if (size > budget || pointer < 65536 || pointer > UINTPTR_MAX - size || !readable(pointer, size)) return false;
        budget -= size;
    }
    return true;
}
}
Adapter::Adapter(const writer::Access& memory, reader::Frame frame, std::span<const action::Definition> definitions,
    std::array<reference::Source, 2> sources, std::array<lease::Binding, 2> locks, item::Binding item, Environment environment)
    : memory_(memory), frame_(frame), definitions_(definitions.begin(), definitions.end()), sources_(sources), locks_(locks),
      item_(item), environment_(environment) {}
bool Adapter::current() const noexcept {
    return environment_.context && environment_.current_frame && environment_.current_frame(environment_.context) == frame_;
}
action::Snapshot Adapter::capture() {
    if (thread_ != std::this_thread::get_id() || captured_ || attempted_ || !memory_.native_items() || !current()) return {};
    captured_ = true;
    report_.read = held_.open(memory_, frame_, definitions_, sources_, locks_);
    if (report_.read != reader::Code::captured) { report_.stage = Stage::refused; return {}; }
    report_.stage = Stage::captured;
    return held_.view()->snapshot;
}
action::Commit Adapter::commit(const action::Plan& plan) noexcept {
    if (thread_ != std::this_thread::get_id()) return action::Commit::rejected;
    if (attempted_) return action::Commit::unknown;
    attempted_ = true;
    bool changed = false;
    // Copies declared later are destroyed first; then release all owner state.
    struct Close { reader::HeldCapture& held; ~Close() { held.close(); } } close{held_};
    auto fail = [&] {
        report_.stage = changed ? Stage::uncertain : Stage::refused;
        return changed ? action::Commit::unknown : action::Commit::rejected;
    };
    try {
        if (!captured_ || !held_.view() || !current() || !item_.construct || !item_.assign || !item_.destroy ||
            !environment_.finish_unlocked) return fail();
        report_.read = held_.refresh(frame_);
        if (report_.read != reader::Code::captured) return fail();
        auto expected = held_.view()->snapshot;
        if (action::apply_to_owned_snapshot(plan, expected) != action::Code::applied) return fail();
        const Scene scene{frame_, held_.view()->authority, held_.view()->presentation};
        for (const auto& change : plan.changes()) {
            const auto& route = environment_.routes[static_cast<std::size_t>(change.before.identity.area)];
            if (!route.context || !route.admit || !route.mark_locked || !route.notify_unlocked ||
                !current() || !route.admit(route.context, scene, change)) return fail();
        }
        report_.stage = Stage::admitted;
        report_.read = held_.refresh(frame_);
        if (!current() || report_.read != reader::Code::captured) return fail();
        expected = held_.view()->snapshot;
        if (action::apply_to_owned_snapshot(plan, expected) != action::Code::applied) return fail();
        std::size_t budget = copy_budget;
        for (const auto& change : plan.changes())
            for (const auto* image : {&change.before.authority, &change.before.presentation})
                if (!copy_source_ready(memory_, *image, budget)) { report_.copy = item::Code::invalid_source; return fail(); }
        struct Prepared {
            const action::Change* change;
            item::Value authority, presentation;
            action::EquipmentNotice equipment{};
            Prepared(const action::Change& c, item::Binding b) : change(&c), authority(b), presentation(b) {}
            Notice notice() const { return {*change, authority.data(), presentation.data(), equipment}; }
        };
        std::vector<std::unique_ptr<Prepared>> copies;
        copies.reserve(plan.changes().size());
        for (const auto& change : plan.changes()) {
            if (!current()) return fail();
            const auto& locations = held_.view()->locations;
            const auto source = std::find_if(locations.begin(), locations.end(), [&](const auto& p) { return p.identity == change.before.identity; });
            if (source == locations.end()) return fail();
            auto copy = std::make_unique<Prepared>(change, item_);
            if (change.before.identity.area == action::Area::equipped &&
                action::equipment_notice(plan, change.before.identity, copy->equipment) != action::Code::prepared) return fail();
            for (const auto& [value, address, realm] : {
                std::tuple{&copy->authority, source->authority.address, item::Realm::authority},
                std::tuple{&copy->presentation, source->presentation.address, item::Realm::presentation}}) {
                report_.copy = value->construct();
                if (report_.copy != item::Code::constructed) return fail();
                report_.copy = value->prepare_repair(reinterpret_cast<const void*>(address), plan, change.before.identity, realm);
                if (report_.copy != item::Code::prepared) return fail();
            }
            copies.push_back(std::move(copy));
            ++report_.prepared_items;
        }
        report_.stage = Stage::copies_ready;
        if (!current()) return fail();
        report_.fields = writer::Batch::apply(held_, frame_, plan);
        changed = report_.fields.attempted_words != 0;
        if (report_.fields.code != writer::Code::fields_written) return fail();
        changed = true;
        report_.stage = Stage::fields_written;
        for (const auto& copy : copies) {
            const auto& route = environment_.routes[static_cast<std::size_t>(copy->change->before.identity.area)];
            if (!current() || !route.mark_locked(route.context, scene, copy->notice())) return fail();
            ++report_.marked_items;
        }
        report_.stage = Stage::marked;
        if (!current() || held_.release_locks() != reader::Code::released) return fail();
        report_.stage = Stage::unlocked;
        for (const auto& copy : copies) {
            const auto& route = environment_.routes[static_cast<std::size_t>(copy->change->before.identity.area)];
            if (!current() || !route.notify_unlocked(route.context, scene, copy->notice()) || !current()) return fail();
            ++report_.submitted_items;
        }
        if (!current() || !environment_.finish_unlocked(environment_.context, scene) || !current()) return fail();
        report_.finalized = true;
        report_.stage = Stage::submitted;
        return action::Commit::pending;
    } catch (...) { return fail(); }
}
}
