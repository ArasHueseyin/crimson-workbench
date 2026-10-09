#include "repair_action.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <set>
#include <tuple>
#include <type_traits>

namespace crimson::repair::action {
namespace {
template<class T, std::size_t N> T read(const std::array<std::uint8_t, N>& bytes, std::size_t at) {
    T value;
    std::memcpy(&value, bytes.data() + at, sizeof(value));
    return value;
}
template<std::size_t N> void word(std::array<std::uint8_t, N>& bytes, std::size_t at, std::uint16_t value) {
    std::memcpy(bytes.data() + at, &value, sizeof(value));
}
bool valid(Session s) { return s.generation && s.player && s.catalog_revision; }
bool valid(Identity id) {
    return id.instance && id.instance != UINT64_MAX &&
        (id.area == Area::carried || id.area == Area::equipped) && id.slot != absent;
}
bool valid(const Request& r) {
    return valid(r.session) && (r.scope == Scope::all || r.scope == Scope::carried ||
        r.scope == Scope::equipped || (r.scope == Scope::one && valid(r.item)));
}
Code geometry(const Image& image, const Identity& id) {
    if (read<std::uint64_t>(image.bytes, 0) != id.instance ||
        read<std::uint16_t>(image.bytes, 8) == absent ||
        read<std::int64_t>(image.bytes, 0x10) <= 0) return Code::identity_mismatch;
    const auto count = read<std::uint32_t>(image.bytes, 0x68);
    const auto capacity = image.bytes[0x70];
    if (count > capacity || capacity > max_sockets || image.sockets.size() != count)
        return Code::invalid_snapshot;
    std::set<std::uint8_t> indices;
    for (std::size_t i = 0; i < count; ++i) {
        const auto& s = image.sockets[i].bytes;
        if (read<std::uint16_t>(s, 0) == absent) continue;
        // Locked/unknown socket geometry must not silently become repairable.
        if (s[4] >= capacity || !indices.insert(s[4]).second) return Code::invalid_snapshot;
    }
    return Code::prepared;
}
Code check_snapshot(const Snapshot& snapshot) {
    if (!valid(snapshot.session) || snapshot.items.size() > max_items ||
        snapshot.definitions.size() > absent) return Code::invalid_snapshot;
    std::set<std::uint16_t> definitions;
    for (const auto& d : snapshot.definitions)
        if (d.key == absent || !definitions.insert(d.key).second) return Code::invalid_snapshot;
    std::set<std::uint64_t> instances;
    std::set<std::tuple<Area, std::uint16_t, std::uint16_t>> locations;
    for (const auto& item : snapshot.items) {
        if (!valid(item.identity) || !instances.insert(item.identity.instance).second ||
            !locations.insert({item.identity.area, item.identity.container, item.identity.slot}).second)
            return Code::identity_mismatch;
    }
    return Code::prepared;
}
bool selected(const Request& request, const Identity& identity) {
    switch (request.scope) {
    case Scope::one: return request.item == identity;
    case Scope::all: return true;
    case Scope::carried: return identity.area == Area::carried;
    case Scope::equipped: return identity.area == Area::equipped;
    }
    return false;
}
Code target(std::uint16_t key, std::uint16_t current, std::span<const Definition> catalog,
    std::uint16_t& result) {
    const auto it = std::lower_bound(catalog.begin(), catalog.end(), key,
        [](const Definition& d, std::uint16_t k) { return d.key < k; });
    if (it == catalog.end() || it->key != key) return Code::missing_definition;
    const auto maximum = it->maximum;
    result = current;
    if (maximum > INT16_MAX && maximum != absent) return Code::unsupported_maximum;
    // Combining repair with Workbench's no-wear table override must still heal
    // existing finite items to their VANILLA maximum, not assign 65535. Only a
    // known no-wear override may explain this active/original mismatch. The
    // future adapter must obtain both values from verified catalogs/settings.
    if (it->active_maximum != maximum && !(maximum > 0 && maximum < absent &&
        it->active_maximum == absent && it->no_wear_override)) return Code::invalid_snapshot;
    // Preserve indestructible/non-durability items and sentinels. The native
    // updater uses signed 16-bit comparisons, so reject other high-bit maxima.
    if (maximum == absent || maximum == 0) return Code::prepared;
    if (current == absent) return Code::prepared;
    if (current > maximum) return Code::invalid_snapshot;
    result = maximum;
    return Code::prepared;
}
Code repair(const Image& source, std::span<const Definition> definitions, Image& after, Count& count) {
    after = source;
    std::uint16_t desired = 0;
    auto status = target(read<std::uint16_t>(source.bytes, 8),
        read<std::uint16_t>(source.bytes, 0x40), definitions, desired);
    if (status != Code::prepared) return status;
    if (desired != read<std::uint16_t>(source.bytes, 0x40)) {
        word(after.bytes, 0x40, desired);
        ++count.main_fields;
    }
    const auto size = read<std::uint32_t>(source.bytes, 0x68);
    for (std::size_t i = 0; i < size; ++i) {
        const auto& s = source.sockets[i].bytes;
        const auto key = read<std::uint16_t>(s, 0);
        if (key == absent) continue;
        status = target(key, read<std::uint16_t>(s, 2), definitions, desired);
        if (status != Code::prepared) return status;
        if (desired != read<std::uint16_t>(s, 2)) {
            word(after.sockets[i].bytes, 2, desired);
            ++count.socket_fields;
        }
    }
    return Code::prepared;
}
bool mirrors_match(const Item& item) {
    const auto& a = item.authority;
    const auto& b = item.presentation;
    return read<std::uint16_t>(a.bytes, 8) == read<std::uint16_t>(b.bytes, 8) &&
        read<std::int64_t>(a.bytes, 0x10) == read<std::int64_t>(b.bytes, 0x10) &&
        read<std::uint16_t>(a.bytes, 0x40) == read<std::uint16_t>(b.bytes, 0x40) &&
        read<std::uint32_t>(a.bytes, 0x68) == read<std::uint32_t>(b.bytes, 0x68) &&
        a.sockets == b.sockets;
}
}

Code equipment_notice(const Plan& plan, Identity identity, EquipmentNotice& output) noexcept {
    output = {};
    if (!plan.prepared() || !valid(identity) || identity.area != Area::equipped)
        return Code::invalid_request;
    const auto changes = plan.changes();
    const auto change = std::find_if(changes.begin(), changes.end(), [&](const Change& c) {
        return c.before.identity == identity;
    });
    if (change == changes.end()) return Code::missing_item;
    const auto key = read<std::uint16_t>(change->before.authority.bytes, 8);
    const auto definitions = plan.definitions();
    const auto definition = std::find_if(definitions.begin(), definitions.end(), [&](const Definition& d) {
        return d.key == key;
    });
    if (definition == definitions.end()) return Code::missing_definition;
    const auto broken = [&](const Image& image) {
        return definition->active_maximum != absent && read<std::int16_t>(image.bytes, 0x40) <= 0;
    };
    output = {plan.session(), identity, broken(change->before.authority), broken(change->after.authority)};
    return Code::prepared;
}

Code prepare(const Request& request, const Snapshot& snapshot, Plan& output) {
    output = {};
    if (!valid(request)) return Code::invalid_request;
    if (request.session != snapshot.session) return Code::stale;
    const auto checked = check_snapshot(snapshot);
    if (checked != Code::prepared) return checked;
    Plan candidate;
    candidate.session_ = snapshot.session;
    candidate.definitions_ = snapshot.definitions;
    auto catalog = snapshot.definitions;
    std::sort(catalog.begin(), catalog.end(), [](const Definition& a, const Definition& b) { return a.key < b.key; });
    bool found = false;
    for (const auto& item : snapshot.items) {
        if (!selected(request, item.identity)) continue;
        found = true;
        for (const auto* image : {&item.authority, &item.presentation}) {
            const auto status = geometry(*image, item.identity);
            if (status != Code::prepared) return status;
        }
        if (!mirrors_match(item)) return Code::mirrors_disagree;
        Change change{item, item};
        Count a, b;
        auto status = repair(item.authority, catalog, change.after.authority, a);
        if (status != Code::prepared) return status;
        status = repair(item.presentation, catalog, change.after.presentation, b);
        if (status != Code::prepared) return status;
        if (a != b) return Code::mirrors_disagree;
        if (a.main_fields || a.socket_fields) {
            ++candidate.count_.items;
            candidate.count_.main_fields += a.main_fields;
            candidate.count_.socket_fields += a.socket_fields;
            candidate.changes_.push_back(std::move(change));
        }
    }
    if (!found && request.scope == Scope::one) return Code::missing_item;
    candidate.prepared_ = true;
    output = std::move(candidate);
    return output.changes().empty() ? Code::nothing_to_repair : Code::prepared;
}

Code apply_to_owned_snapshot(const Plan& plan, Snapshot& current) {
    if (!plan.prepared()) return Code::invalid_request;
    if (plan.session() != current.session || !std::ranges::equal(plan.definitions(), current.definitions))
        return Code::stale;
    const auto checked = check_snapshot(current);
    if (checked != Code::prepared) return checked;
    // Allocate/find everything before the first write. One changed or missing
    // item rejects the entire batch, including items validated earlier.
    std::vector<Item*> destinations;
    destinations.reserve(plan.changes().size());
    for (const auto& change : plan.changes()) {
        auto it = std::find_if(current.items.begin(), current.items.end(), [&](const Item& item) {
            return item.identity == change.before.identity;
        });
        if (it == current.items.end() || *it != change.before) return Code::stale;
        destinations.push_back(&*it);
    }
    for (std::size_t i = 0; i < destinations.size(); ++i) {
        const auto& after = plan.changes()[i].after;
        auto& out = *destinations[i];
        // Write only the whitelist, never pointer/quantity/identity/flags.
        for (const auto& [image, target_image] : {
            std::pair{&after.authority, &out.authority},
            std::pair{&after.presentation, &out.presentation}}) {
            word(target_image->bytes, 0x40, read<std::uint16_t>(image->bytes, 0x40));
            for (std::size_t s = 0; s < image->sockets.size(); ++s)
                word(target_image->sockets[s].bytes, 2, read<std::uint16_t>(image->sockets[s].bytes, 2));
        }
    }
    return destinations.empty() ? Code::nothing_to_repair : Code::applied;
}

Code verify_applied(const Plan& plan, const Snapshot& observed) {
    if (!plan.prepared()) return Code::invalid_request;
    if (plan.session() != observed.session || !std::ranges::equal(plan.definitions(), observed.definitions))
        return Code::stale;
    const auto checked = check_snapshot(observed);
    if (checked != Code::prepared) return checked;
    for (const auto& change : plan.changes()) {
        const auto found = std::find_if(observed.items.begin(), observed.items.end(), [&](const Item& item) {
            return item.identity == change.after.identity;
        });
        if (found == observed.items.end() || *found != change.after) return Code::stale;
    }
    return plan.changes().empty() ? Code::nothing_to_repair : Code::applied;
}

Outcome Queue::uncertain() {
    const auto id = confirming_;
    confirming_ = 0;
    confirmation_ = {};
    fault_ = true;
    return {Code::outcome_unknown, id};
}
static_assert(std::is_nothrow_move_assignable_v<Plan>, "Pending plan storage must not throw after commit acceptance");
Outcome Queue::submit(const Request& request, Clock::time_point now) {
    std::lock_guard lock(mutex_);
    if (fault_) return {Code::fault_latched};
    if (pending_ || running_ || confirming_) return {Code::busy};
    if (!valid(request) || next_ == UINT64_MAX) return {Code::invalid_request};
    request_ = request;
    deadline_ = now + std::chrono::seconds(5);
    pending_ = ++next_;
    return {Code::queued, pending_};
}
Outcome Queue::cancel() {
    std::lock_guard lock(mutex_);
    if (running_ || confirming_) return {Code::busy};
    if (!pending_) return {Code::no_request};
    const auto id = pending_;
    pending_ = 0;
    return {Code::cancelled, id};
}
Outcome Queue::execute(Transaction& transaction, Clock::time_point now) {
    if (std::this_thread::get_id() != thread_) return {Code::wrong_thread};
    Request request;
    std::uint64_t id;
    {
        std::lock_guard lock(mutex_);
        if (fault_) return {Code::fault_latched};
        if (running_) return {Code::busy};
        if (confirming_) {
            if (now >= confirmation_deadline_) return uncertain();
            return {Code::awaiting_confirmation, confirming_};
        }
        if (!pending_) return {Code::no_request};
        id = pending_;
        pending_ = 0;
        if (now >= deadline_) return {Code::expired, id};
        request = request_;
        running_ = true;
    }
    Outcome result{Code::host_rejected, id};
    Plan confirmation;
    try {
        Plan plan;
        result.code = prepare(request, transaction.capture(), plan);
        if (result.code == Code::prepared) {
            switch (transaction.commit(plan)) {
            case Commit::applied:
                result.code = Code::applied;
                result.count = plan.count();
                break;
            case Commit::rejected: result.code = Code::host_rejected; break;
            case Commit::pending:
                confirmation = std::move(plan);
                result.code = Code::awaiting_confirmation;
                break;
            default: result.code = Code::outcome_unknown; break;
            }
        }
    } catch (...) {
        // capture/prepare only; commit is noexcept and must report uncertainty.
        result.code = Code::host_rejected;
    }
    {
        std::lock_guard lock(mutex_);
        running_ = false;
        if (result.code == Code::awaiting_confirmation) {
            confirming_ = id;
            confirmation_ = std::move(confirmation);
            confirmation_deadline_ = now + std::chrono::seconds(5);
        }
        if (result.code == Code::outcome_unknown) fault_ = true;
    }
    return result;
}
Outcome Queue::confirm(std::uint64_t request_id, Session receipt_session, const Snapshot& observed,
    bool notifications_confirmed, Clock::time_point now) {
    if (std::this_thread::get_id() != thread_) return {Code::wrong_thread};
    std::lock_guard lock(mutex_);
    if (fault_) return {Code::fault_latched};
    if (running_) return {Code::busy};
    if (!confirming_) return {Code::no_request};
    if (now >= confirmation_deadline_) return uncertain();
    if (request_id != confirming_ || receipt_session != confirmation_.session())
        return {Code::confirmation_mismatch, request_id};
    if (!notifications_confirmed) return uncertain();
    try {
        if (verify_applied(confirmation_, observed) != Code::applied) return uncertain();
    } catch (...) { return uncertain(); }
    const Outcome result{Code::applied, confirming_, confirmation_.count()};
    confirming_ = 0;
    confirmation_ = {};
    return result;
}
Outcome Queue::poll(Session current, Clock::time_point now) {
    if (std::this_thread::get_id() != thread_) return {Code::wrong_thread};
    std::lock_guard lock(mutex_);
    if (fault_) return {Code::fault_latched};
    if (running_) return {Code::busy};
    if (confirming_) {
        if (current != confirmation_.session() || now >= confirmation_deadline_) return uncertain();
        return {Code::awaiting_confirmation, confirming_};
    }
    if (!pending_) return {Code::no_request};
    if (current == request_.session && now < deadline_) return {Code::queued, pending_};
    const auto id = pending_;
    pending_ = 0;
    return {current != request_.session ? Code::stale : Code::expired, id};
}
}
