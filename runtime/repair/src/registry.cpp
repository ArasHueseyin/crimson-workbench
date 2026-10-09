#include "repair_registry.h"

namespace crimson::repair::registry {
reference::Source Source::source(std::uint32_t full_handle) noexcept {
    if (!manager_ || !lookup_ || !destroy_ || !full_handle) return {};
    return {this, full_handle, acquire, release};
}
void Source::acquire(void* context, std::uint32_t handle, reference::Receipt& output) noexcept {
    auto& source = *static_cast<Source*>(context);
    source.lookup_(source.manager_, &output, handle);
}
void Source::release(void* context, reference::Receipt& receipt) noexcept {
    auto& source = *static_cast<Source*>(context);
    source.destroy_(&receipt);
}
}
