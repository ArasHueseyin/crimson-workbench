#pragma once
#include <cstddef>
#include <cstdint>

#ifdef CRIMSON_REPAIR_EXPORTS
#define REPAIR_API __declspec(dllexport)
#else
#define REPAIR_API __declspec(dllimport)
#endif

// Development ABI. This is NOT a game loader or an installation interface.
// Buffers are private, non-overlapping caller-owned copies of the two functions.
// No process lookup, memory protection change, game callback or file write occurs.
namespace crimson::repair {
inline constexpr std::uint32_t abi_version = 1;
inline constexpr std::size_t common_size = 437;
inline constexpr std::size_t server_size = 691;
inline constexpr std::uint32_t common_rva = 0x240dfb0;
inline constexpr std::uint32_t server_rva = 0x2be3ee0;
inline constexpr char exe_sha256[] =
    "6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7";

enum class Status : std::uint32_t {
    prepared = 0,
    invalid_argument = 1,
    unknown_code = 2,
    internal_error = 3,
};

extern "C" REPAIR_API Status crimson_repair_prepare(
    std::uint32_t abi,
    const std::uint8_t* common, std::size_t common_length,
    const std::uint8_t* server, std::size_t server_length,
    std::uint8_t* common_output, std::size_t common_capacity,
    std::uint8_t* server_output, std::size_t server_capacity) noexcept;

// Always false in this prototype: the separate own-action library works on
// owned snapshots only. A verified engine transaction, UI and loader are absent.
extern "C" REPAIR_API bool crimson_repair_installable() noexcept;
}
