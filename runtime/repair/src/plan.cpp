#include "repair_plan.h"
#include "hash.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <span>

namespace crimson::repair {
namespace {
struct Range { std::uintptr_t begin; std::size_t size; };
bool valid(Range a) {
    return a.begin != 0 && a.size <= std::numeric_limits<std::uintptr_t>::max() - a.begin;
}
bool overlaps(Range a, Range b) {
    return a.begin < b.begin + b.size && b.begin < a.begin + a.size;
}
}
Status crimson_repair_prepare(std::uint32_t abi,
    const std::uint8_t* common, std::size_t common_length,
    const std::uint8_t* server, std::size_t server_length,
    std::uint8_t* common_output, std::size_t common_capacity,
    std::uint8_t* server_output, std::size_t server_capacity) noexcept {
    if (abi != abi_version || common_length != common_size || server_length != server_size ||
        common_capacity != common_size || server_capacity != server_size)
        return Status::invalid_argument;
    const std::array<Range, 4> ranges{{
        {reinterpret_cast<std::uintptr_t>(common), common_length},
        {reinterpret_cast<std::uintptr_t>(server), server_length},
        {reinterpret_cast<std::uintptr_t>(common_output), common_capacity},
        {reinterpret_cast<std::uintptr_t>(server_output), server_capacity}}};
    for (std::size_t i = 0; i < ranges.size(); ++i) {
        if (!valid(ranges[i])) return Status::invalid_argument;
        for (std::size_t j = 0; j < i; ++j)
            if (overlaps(ranges[i], ranges[j])) return Status::invalid_argument;
    }
    try {
        // Both complete function bodies must match before either output changes.
        if (hash({common, common_length}) !=
                "61e7429f0415e591df434017a4500a6d93a4e059b11155741fd18ef5d4733aaf" ||
            hash({server, server_length}) !=
                "3f90895c1cb5e7fcdaa2599f74b18a0e6eba0b6b867f7668de2708e491d13bd5")
            return Status::unknown_code;

        std::array<std::uint8_t, common_size> staged_common;
        std::array<std::uint8_t, server_size> staged_server;
        std::copy_n(common, common_size, staged_common.begin());
        std::copy_n(server, server_size, staged_server.begin());
        // Own replacement code, no original game instructions embedded here.
        // After the original cost call: set cost to zero AND bypass its divisor
        // and material cap. Keep the precomputed repair amount and normal clamp.
        // Common: reject negative cost/quantity (except the original -1 quantity
        // sentinel), zero EBX and jump to RVA 0x240e0ef. Never divide by the cost.
        constexpr std::size_t common_begin = 0x240e0d1 - common_rva;
        constexpr std::size_t common_end = 0x240e0ef - common_rva;
        std::fill(staged_common.begin() + common_begin, staged_common.begin() + common_end, std::uint8_t(0x90));
        constexpr std::array<std::uint8_t, 19> common_code{
            0x48, 0x85, 0xc0, 0x78, 0x67,
            0x31, 0xdb, 0x49, 0x8b, 0x45, 0x00,
            0x48, 0x83, 0xf8, 0xff, 0x7c, 0x5b, 0xeb, 0x0b};
        std::copy(common_code.begin(), common_code.end(), staged_common.begin() + common_begin);
        // Server: same validation; error RVA 0x2be3f65, success 0x2be4077.
        constexpr std::size_t server_begin = 0x2be4052 - server_rva;
        constexpr std::size_t server_end = 0x2be4077 - server_rva;
        std::fill(staged_server.begin() + server_begin, staged_server.begin() + server_end, std::uint8_t(0x90));
        constexpr std::array<std::uint8_t, 24> server_code{
            0x48, 0x85, 0xc0, 0x0f, 0x88, 0x0a, 0xff, 0xff, 0xff,
            0x45, 0x31, 0xf6, 0x48, 0x83, 0xfd, 0xff,
            0x0f, 0x8c, 0xfd, 0xfe, 0xff, 0xff, 0xeb, 0x0d};
        std::copy(server_code.begin(), server_code.end(), staged_server.begin() + server_begin);
        // Leave the request vector untouched instead of appending a zero-count
        // withdrawal. Success uses the original epilogue; error paths stay intact.
        constexpr std::size_t debit_begin = 0x2be40bf - server_rva;
        constexpr std::size_t debit_end = 0x2be4162 - server_rva;
        std::fill(staged_server.begin() + debit_begin, staged_server.begin() + debit_end, std::uint8_t(0x90));
        // xor eax,eax; mov [rdi],eax; jmp RVA 0x2be4162.
        constexpr std::array<std::uint8_t, 9> debit_code{0x31, 0xc0, 0x89, 0x07, 0xe9, 0x9a, 0, 0, 0};
        std::copy(debit_code.begin(), debit_code.end(), staged_server.begin() + debit_begin);
        std::memcpy(common_output, staged_common.data(), common_size);
        std::memcpy(server_output, staged_server.data(), server_size);
        return Status::prepared;
    } catch (...) {
        return Status::internal_error;
    }
}
bool crimson_repair_installable() noexcept { return false; }
}
