#pragma once
#include <array>
#include <atomic>
#include <mutex>
#include "task_observer.h"

namespace crimson::live_items {
class TaskObservers {
    std::array<std::atomic<TaskObserverV1>,4> slots_{};
    std::mutex registration_;
    inline static thread_local bool notifying_{};
public:
    template<class Pin>
    bool add(std::uint32_t abi,std::uint32_t bytes,TaskObserverV1 callback,Pin pin) {
        if (abi!=kTaskObserverAbiV1 || bytes!=sizeof(TaskObservationV1) || !callback) return false;
        std::lock_guard lock(registration_);
        for (const auto& slot:slots_)
            if (slot.load(std::memory_order_acquire)==callback) return true;
        for (auto& slot:slots_) {
            if (slot.load(std::memory_order_relaxed)) continue;
            if (!pin(callback)) return false;
            slot.store(callback,std::memory_order_release);
            return true;
        }
        return false;
    }
    // Revalidate immediately before each observer, including after another
    // observer may have changed the selected world. Registration never invokes
    // a callback, and no engine pointer is stored in this registry.
    template<class Validate>
    void notify(const TaskObservationV1& observation,Validate validate) {
        if (notifying_) return;
        struct Guard {
            Guard() {notifying_=true;}
            ~Guard() {notifying_=false;}
        } guard;
        for (const auto& slot:slots_) {
            const auto callback=slot.load(std::memory_order_acquire);
            if (!callback) continue;
            auto current=observation;
            if (validate(current)) callback(&current);
        }
    }
};
} // namespace crimson::live_items
