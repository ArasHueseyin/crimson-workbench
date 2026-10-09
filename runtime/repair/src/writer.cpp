#include "repair_writer.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <new>

namespace crimson::repair::writer {
namespace {
template<class T, class B> T read(const B& bytes, std::size_t at) {
    T out; std::memcpy(&out, bytes.data() + at, sizeof(out)); return out;
}
bool valid(reader::Region r) {
    return r.address >= 65536 && r.size && r.address <= UINTPTR_MAX - r.size;
}
struct Regions {
    std::vector<reader::Region> values;
    std::vector<std::uintptr_t> maximum_end;
    void index() {
        std::sort(values.begin(), values.end(), [](const auto& a, const auto& b) { return a.address < b.address; });
        maximum_end.reserve(values.size());
        std::uintptr_t end = 0;
        for (auto r : values) { end = (std::max)(end, r.address + r.size); maximum_end.push_back(end); }
    }
    std::size_t upper(std::uintptr_t end) const {
        return static_cast<std::size_t>(std::lower_bound(values.begin(), values.end(), end,
            [](const auto& a, auto b) { return a.address < b; }) - values.begin());
    }
    bool overlaps(reader::Region r) const {
        const auto i = upper(r.address + r.size);
        return i && maximum_end[i - 1] > r.address;
    }
    // Every byte of a target word must belong to exactly one observed payload
    // span and must not overlap any other item or structural read, even an empty
    // or excluded slot. Prefix maxima avoid quadratic scans for normal batches.
    bool unique_payload(std::uintptr_t word, reader::Region payload) const {
        if (word < payload.address || payload.size < 2 || word - payload.address > payload.size - 2) return false;
        unsigned matches = 0;
        auto i = upper(word + 2);
        while (i && maximum_end[i - 1] > word) {
            const auto r = values[--i];
            if (r.address + r.size <= word) continue;
            if (r != payload || ++matches != 1) return false;
        }
        return matches == 1;
    }
};
struct Word { std::uintptr_t address; std::uint16_t before, after; reader::Region payload; };
bool geometry(const reader::Capture& capture, Regions& protected_data) {
    if (capture.locations.size() != capture.snapshot.items.size()) return false;
    for (std::size_t i = 0; i < capture.locations.size(); ++i) {
        const auto& location = capture.locations[i];
        const auto& item = capture.snapshot.items[i];
        if (location.identity != item.identity) return false;
        for (const auto& [region, image] : {std::pair{location.authority, &item.authority},
            std::pair{location.presentation, &item.presentation}}) {
            if (!valid(region) || region.address % 8 || region.size !=
                (item.identity.area == action::Area::carried ? action::item_size : 0xd0)) return false;
            for (const auto& [at, stride] : {std::pair{std::size_t(0x60), std::size_t(6)},
                std::pair{std::size_t(0x78), std::size_t(16)}, std::pair{std::size_t(0xa8), std::size_t(6)}}) {
                const auto pointer = read<std::uintptr_t>(image->bytes, at);
                const auto count = read<std::uint32_t>(image->bytes, at + 8);
                const auto allocation = read<std::uint32_t>(image->bytes, at + 12);
                if (count > allocation || (pointer && pointer % 2) || (allocation && !pointer)) return false;
                if (allocation && !valid({pointer, std::size_t(allocation) * stride})) return false;
                if (at == 0x60) {
                    const auto logical = image->bytes[0x70];
                    if (count > logical || image->sockets.size() != count) return false;
                    if (allocation > count)
                        protected_data.values.push_back({pointer + count * stride, (allocation - count) * stride});
                } else if (allocation) protected_data.values.push_back({pointer, allocation * stride});
            }
            for (const auto& [at, size] : {std::pair{std::size_t(0xb8), std::size_t(24)},
                std::pair{std::size_t(0xc0), std::size_t(16)}}) {
                const auto pointer = read<std::uintptr_t>(image->bytes, at);
                if (pointer) {
                    if (!valid({pointer, size})) return false;
                    protected_data.values.push_back({pointer, size});
                }
            }
        }
    }
    return true;
}
void fields(const action::Image& before, const action::Image& after, reader::Region raw, std::vector<Word>& words) {
    const auto a = read<std::uint16_t>(before.bytes, 0x40), b = read<std::uint16_t>(after.bytes, 0x40);
    if (a != b) words.push_back({raw.address + 0x40, a, b, raw});
    const reader::Region sockets{read<std::uintptr_t>(before.bytes, 0x60), before.sockets.size() * action::socket_size};
    for (std::size_t i = 0; i < before.sockets.size(); ++i) {
        const auto x = read<std::uint16_t>(before.sockets[i].bytes, 2), y = read<std::uint16_t>(after.sockets[i].bytes, 2);
        if (x != y) words.push_back({sockets.address + i * action::socket_size + 2, x, y, sockets});
    }
}
}
Outcome Batch::apply(reader::HeldCapture& held, reader::Frame frame, const action::Plan& plan) noexcept {
    Outcome out;
    if (held.thread_ != std::this_thread::get_id()) { out.code = Code::wrong_thread; return out; }
    if (!held.held_) { out.code = Code::invalid_state; return out; }
    if (held.write_attempted_) { out.code = Code::already_attempted; return out; }
    held.write_attempted_ = true;
    const auto* access = dynamic_cast<const Access*>(held.memory_);
    if (!access) { out.reason = Reason::read_only; return out; }
    try {
        out.read = held.refresh(frame);
        if (out.read != reader::Code::captured) { out.reason = Reason::read_failed; return out; }
        const auto before = held.capture_;
        auto expected = before.snapshot;
        out.plan = action::apply_to_owned_snapshot(plan, expected);
        if (out.plan == action::Code::nothing_to_repair) { out.code = Code::nothing_to_write; return out; }
        if (out.plan != action::Code::applied) { out.reason = Reason::invalid_plan; return out; }
        Regions observed{before.observed_regions, {}}, protected_data{before.structural_regions, {}};
        for (const auto& binding : held.owners_.locks)
            protected_data.values.push_back({reinterpret_cast<std::uintptr_t>(binding.object), 0x30});
        // The 2949 server component also owns the dirty-slot table here.
        if (frame.build == reader::Frame::Build::steam_25455892)
            protected_data.values.push_back({before.authority.equipment, 0x208});
        if (!geometry(before, protected_data) ||
            !std::all_of(observed.values.begin(), observed.values.end(), [](auto r) { return valid(r); }) ||
            !std::all_of(protected_data.values.begin(), protected_data.values.end(), [](auto r) { return valid(r); })) {
            out.reason = Reason::invalid_geometry; return out;
        }
        observed.index(); protected_data.index();
        if (before.observed_regions.size() != before.observed_bytes.size()) { out.reason = Reason::invalid_geometry; return out; }
        std::map<std::pair<std::uintptr_t, std::size_t>, std::size_t> read_indices;
        auto expected_reads = before.observed_bytes;
        for (std::size_t i = 0; i < before.observed_regions.size(); ++i) {
            const auto r = before.observed_regions[i];
            if (expected_reads[i].size() != r.size) { out.reason = Reason::invalid_geometry; return out; }
            read_indices.emplace(std::pair{r.address, r.size}, i);
        }
        std::vector<Word> words;
        words.reserve((plan.count().main_fields + plan.count().socket_fields) * 2);
        for (const auto& change : plan.changes()) {
            const auto location = std::find_if(before.locations.begin(), before.locations.end(), [&](const auto& p) {
                return p.identity == change.before.identity;
            });
            if (location == before.locations.end()) { out.reason = Reason::invalid_geometry; return out; }
            fields(change.before.authority, change.after.authority, location->authority, words);
            fields(change.before.presentation, change.after.presentation, location->presentation, words);
        }
        std::sort(words.begin(), words.end(), [](const auto& a, const auto& b) { return a.address < b.address; });
        for (std::size_t i = 0; i < words.size(); ++i) {
            const auto& word = words[i];
            if (word.address % 2 || !valid({word.address, 2})) { out.reason = Reason::invalid_geometry; return out; }
            if ((i && words[i - 1].address + 2 > word.address) ||
                !observed.unique_payload(word.address, word.payload) || protected_data.overlaps({word.address, 2})) {
                out.reason = Reason::aliased_storage; return out;
            }
            if (!access->writable(word.address, 2)) {
                held.evidence_valid_ = false;
                out.reason = Reason::read_only; return out;
            }
            const auto record = read_indices.find({word.payload.address, word.payload.size});
            if (record == read_indices.end()) { out.reason = Reason::invalid_geometry; return out; }
            std::memcpy(expected_reads[record->second].data() + (word.address - word.payload.address), &word.after, 2);
        }
        // Host/backend admission may have called other code. Validate the whole
        // read set again before the first word, not just the first item's value.
        out.read = held.refresh(frame);
        if (out.read != reader::Code::captured) { out.reason = Reason::read_failed; return out; }
        if (held.capture_.snapshot != before.snapshot || held.capture_.locations != before.locations ||
            held.capture_.observed_regions != before.observed_regions || held.capture_.structural_regions != before.structural_regions ||
            held.capture_.observed_bytes != before.observed_bytes) {
            out.reason = Reason::source_changed; return out;
        }
        held.evidence_valid_ = false;
        for (const auto& word : words) {
            ++out.attempted_words;
            if (!access->write_word(word.address, word.before, word.after)) {
                out.code = Code::outcome_unknown; out.reason = Reason::write_failed; return out;
            }
            ++out.written_words;
        }
        out.read = held.refresh(frame);
        if (out.read != reader::Code::captured || held.capture_.snapshot != expected || held.capture_.locations != before.locations ||
            held.capture_.observed_regions != before.observed_regions || held.capture_.structural_regions != before.structural_regions ||
            held.capture_.observed_bytes != expected_reads) {
            out.code = Code::outcome_unknown; out.reason = Reason::verification_failed; return out;
        }
        out.code = Code::fields_written;
        return out;
    } catch (...) {
        // Allocation/read failures after any store attempt cannot promise that
        // nothing changed. No blind rollback, replay or success notification.
        out.code = out.attempted_words ? Code::outcome_unknown : Code::rejected;
        held.evidence_valid_ = false;
        out.reason = Reason::allocation_failed;
        return out;
    }
}
}
