#pragma once
#include "repair_reader.h"

namespace crimson::repair::writer {
// In-process storage interface for the field phase, also implemented by private
// fixtures. It must be the SAME object used by HeldCapture to read the sources.
// Methods do not acquire ownership, change page protections or send events.
class Access : public reader::Memory {
public:
    // True only when captured item/nested addresses are actual owned-process
    // pointers usable by verified native copy methods. Virtual fixture address
    // spaces can still test the field phase but cannot prepare native copies.
    virtual bool native_items() const noexcept { return false; }
    virtual bool writable(std::uintptr_t address, std::size_t size) const noexcept = 0;
    // Compare the current word before storing. False after this call is always
    // treated as uncertain, even if a particular backend made no change.
    virtual bool write_word(std::uintptr_t address, std::uint16_t before, std::uint16_t after) const noexcept = 0;
};
class LocalAccess final : public Access {
    reader::LocalMemory memory_;
public:
    bool native_items() const noexcept override { return true; }
    bool copy(std::uintptr_t, std::span<std::uint8_t>) const noexcept override;
    bool writable(std::uintptr_t, std::size_t) const noexcept override;
    bool write_word(std::uintptr_t, std::uint16_t, std::uint16_t) const noexcept override;
};

enum class Code { fields_written, nothing_to_write, rejected, outcome_unknown, wrong_thread, invalid_state, already_attempted };
enum class Reason { none, read_failed, invalid_plan, read_only, invalid_geometry, aliased_storage,
    source_changed, write_failed, verification_failed, allocation_failed };
struct Outcome {
    Code code = Code::rejected;
    Reason reason = Reason::none;
    reader::Code read = reader::Code::captured;
    action::Code plan = action::Code::prepared;
    std::size_t attempted_words = 0;
    std::size_t written_words = 0;
};
class Batch {
public:
    // Complete preflight of every planned item in BOTH realms, then only the
    // changed +0x40 / socket+2 words. No source address comes from the caller.
    // One attempt per HeldCapture; no rollback/retry on uncertain writes.
    // Retains locks on ordinary success/rejection for the host's next phase.
    // A failed refresh closes the capture by its existing ownership contract.
    //
    // This is NOT action::Commit::applied: native after-copies must be prepared
    // before calling, then dirty marking and all inventory/equipment events must
    // follow in the admitted engine transaction. No notifications are sent here.
    // Host admission (build, engine thread/TLS, manager/world/catalog lifetime)
    // remains mandatory. Tests execute only on their own memory.
    static Outcome apply(reader::HeldCapture&, reader::Frame current, const action::Plan&) noexcept;
};
}
