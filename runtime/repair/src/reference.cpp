#include "repair_reference.h"
#include <cstring>
#include <exception>

namespace crimson::repair::reference {
std::uintptr_t Receipt::actor() const noexcept {
    if (!bytes[0x10]) return 0; // an invalid receipt may retain a non-null pointer
    std::uintptr_t result = 0;
    std::memcpy(&result, bytes.data() + 8, sizeof(result));
    return result;
}
Pair::~Pair() {
    if (count_ && release() == Code::wrong_thread) std::terminate();
}
Code Pair::acquire(std::span<const Source> sources) noexcept {
    if (std::this_thread::get_id() != thread_) return Code::wrong_thread;
    if (count_) return Code::already_acquired;
    if (sources.size() != 2) return Code::invalid_source;
    for (const auto& source : sources)
        if (!source.context || !source.handle || !source.acquire || !source.release)
            return Code::invalid_source;
    if (sources[0].handle != sources[1].handle) return Code::invalid_source;
    for (std::size_t i = 0; i < sources.size(); ++i) {
        sources_[i] = sources[i];
        ++count_; // failed native receipts also need their matching cleanup
        sources_[i].acquire(sources_[i].context, sources_[i].handle, receipts_[i]);
        if (!receipts_[i].actor()) { release(); return Code::unavailable; }
    }
    if (receipts_[0].actor() == receipts_[1].actor()) { release(); return Code::unavailable; }
    return Code::acquired;
}
Code Pair::release() noexcept {
    if (std::this_thread::get_id() != thread_) return Code::wrong_thread;
    if (!count_) return Code::empty;
    while (count_) {
        const auto i = --count_;
        sources_[i].release(sources_[i].context, receipts_[i]);
        receipts_[i].bytes = {};
        sources_[i] = {};
    }
    return Code::released;
}
std::uintptr_t Pair::actor(std::size_t i) const noexcept {
    if (std::this_thread::get_id() != thread_ || count_ != 2 || i >= count_) return 0;
    return receipts_[i].actor();
}
}
