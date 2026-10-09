#include "repair_reader.h"
#include <Windows.h>
#include <cstring>

namespace crimson::repair::reader {
namespace {
// This SEH frame has no C++ destructors. The reader neither changes protections
// nor suppresses faults originating from any unrelated code.
bool copy_guarded(void* out, const void* source, std::size_t size) noexcept {
    __try { std::memcpy(out, source, size); return true; }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ||
        GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }
}
}
bool LocalMemory::copy(std::uintptr_t address, std::span<std::uint8_t> output) const noexcept {
    if (address < 65536 || output.empty() || output.size() > 16 * 1024 * 1024 ||
        address > UINTPTR_MAX - output.size()) return false;
    const auto end = address + output.size();
    auto cursor = address;
    while (cursor < end) {
        MEMORY_BASIC_INFORMATION info{};
        if (VirtualQuery(reinterpret_cast<const void*>(cursor), &info, sizeof(info)) != sizeof(info) ||
            info.State != MEM_COMMIT || (info.Protect & PAGE_GUARD)) return false;
        const auto protection = info.Protect & 0xffu;
        if (protection != PAGE_READONLY && protection != PAGE_READWRITE && protection != PAGE_WRITECOPY &&
            protection != PAGE_EXECUTE_READ && protection != PAGE_EXECUTE_READWRITE &&
            protection != PAGE_EXECUTE_WRITECOPY) return false;
        const auto region = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
        if (region > UINTPTR_MAX - info.RegionSize || region + info.RegionSize <= cursor) return false;
        cursor = region + info.RegionSize;
    }
    return copy_guarded(output.data(), reinterpret_cast<const void*>(address), output.size());
}
}
