#include "repair_writer.h"
#include <Windows.h>

namespace crimson::repair::writer {
bool LocalAccess::copy(std::uintptr_t address, std::span<std::uint8_t> output) const noexcept {
    return memory_.copy(address, output);
}
bool LocalAccess::writable(std::uintptr_t address, std::size_t size) const noexcept {
    if (address < 65536 || !size || size > 16 * 1024 * 1024 || address > UINTPTR_MAX - size) return false;
    const auto end = address + size;
    auto cursor = address;
    while (cursor < end) {
        MEMORY_BASIC_INFORMATION info{};
        if (VirtualQuery(reinterpret_cast<const void*>(cursor), &info, sizeof(info)) != sizeof(info) ||
            info.State != MEM_COMMIT || (info.Protect & PAGE_GUARD)) return false;
        const auto protection = info.Protect & 0xffu;
        // Do not write executable memory or relax its protections.
        if (protection != PAGE_READWRITE && protection != PAGE_WRITECOPY) return false;
        const auto base = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
        if (base > UINTPTR_MAX - info.RegionSize || base + info.RegionSize <= cursor) return false;
        cursor = base + info.RegionSize;
    }
    return true;
}
namespace {
bool compare_and_store(std::uintptr_t address, std::uint16_t before, std::uint16_t after) noexcept {
    // Own-process, aligned word only; no C++ objects in this SEH frame. This is
    // not synchronization: the HeldCapture's owner locks remain required.
    __try {
        auto* word = reinterpret_cast<volatile std::uint16_t*>(address);
        if (*word != before) return false;
        *word = after;
        return true;
    } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ||
        GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }
}
}
bool LocalAccess::write_word(std::uintptr_t address, std::uint16_t before, std::uint16_t after) const noexcept {
    return address % alignof(std::uint16_t) == 0 && writable(address, 2) && compare_and_store(address, before, after);
}
}
