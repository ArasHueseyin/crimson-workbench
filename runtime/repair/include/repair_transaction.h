#pragma once
#include "repair_item.h"
#include "repair_writer.h"
#include <memory>

namespace crimson::repair::transaction {
inline constexpr std::size_t copy_budget = 16 * 1024 * 1024; // batch budget, not an engine limit
struct Scene {
    reader::Frame frame;
    reader::Character authority, presentation;
};
struct Notice {
    const action::Change& change;
    const void* authority_after;
    const void* presentation_after;
    action::EquipmentNotice equipment; // meaningful only for equipped items
};
struct Route {
    void* context = nullptr;
    // Read-only admission of the complete route before source stores. Resolve
    // and reserve any route-specific resources here. Source ownership is held.
    bool (*admit)(void*, const Scene&, const action::Change&) noexcept = nullptr;
    bool (*mark_locked)(void*, const Scene&, const Notice&) noexcept = nullptr;
    // Called after both owner locks are released, with actor references and
    // native copies still alive. Consume/copy data synchronously; retain no
    // borrowed pointers. True means submitted, NOT acknowledged/persisted.
    bool (*notify_unlocked)(void*, const Scene&, const Notice&) noexcept = nullptr;
};
struct Environment {
    void* context = nullptr;
    reader::Frame (*current_frame)(void*) noexcept = nullptr;
    std::array<Route, 2> routes; // carried, equipped; missing route rejects ALL
    // One batch-level completion after all per-item notices. Consume marked
    // changes/reserved resources before returning; no borrowed pointer retention.
    // True confirms submission only, not persistence or request acknowledgement.
    bool (*finish_unlocked)(void*, const Scene&) noexcept = nullptr;
};
enum class Stage { idle, captured, admitted, copies_ready, fields_written, marked, unlocked, submitted, refused, uncertain };
struct Report {
    Stage stage = Stage::idle;
    reader::Code read = reader::Code::captured;
    item::Code copy = item::Code::empty;
    writer::Outcome fields;
    std::size_t prepared_items = 0, marked_items = 0, submitted_items = 0;
    bool finalized = false;
};
// Concrete action transaction with host-bound native lifecycle and per-area
// event routes. It is not a resolver, loader or live-game admission mechanism.
// The host must pin context/manager/catalog/world and provide verified methods
// on the engine thread with stable TLS throughout this synchronous operation.
class Adapter final : public action::Transaction {
    const std::thread::id thread_ = std::this_thread::get_id();
    const writer::Access& memory_;
    reader::Frame frame_;
    std::vector<action::Definition> definitions_;
    std::array<reference::Source, 2> sources_;
    std::array<lease::Binding, 2> locks_;
    item::Binding item_;
    Environment environment_;
    reader::HeldCapture held_;
    bool captured_ = false, attempted_ = false;
    Report report_;
    bool current() const noexcept;
public:
    Adapter(const writer::Access&, reader::Frame, std::span<const action::Definition>,
        std::array<reference::Source, 2>, std::array<lease::Binding, 2>, item::Binding, Environment);
    action::Snapshot capture() override;
    // Always pending after successful submission; never reports applied. Queue
    // confirmation still needs independently correlated receipts and fresh data.
    action::Commit commit(const action::Plan&) noexcept override;
    const Report& report() const noexcept { return report_; }
};
}
