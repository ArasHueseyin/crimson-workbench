#pragma once
#include "position.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace crimson::marketplace::teleport {
// These are values copied from the selected server actor, never UI pointers or
// coordinates supplied by the pipe client. Pin IDs include their generation.
enum class MarkerSource : std::uint8_t { map_pin, waypoint };
enum class MarkerLayer : std::uint8_t { unknown, mainland, alternate };
enum class MarkerCode : std::uint32_t {
    ready=0,unsupported=1,context=2,world=3,component=4,list=5,missing=6,layer=7,
    geometry_unavailable=8,placement_unavailable=9,placement_shifted=10,native_fault=11
};
struct MarkerValue {
    std::uint64_t id{};
    WorldPosition position{};
    MarkerSource source{};
    MarkerLayer layer{};
    std::uint8_t shape{},color{};
};
static_assert(sizeof(MarkerValue)==24);
inline constexpr std::size_t kMaximumMapPins=500,kMaximumWaypoints=20;
struct MarkerSnapshot {
    std::array<MarkerValue,kMaximumMapPins+kMaximumWaypoints> values{};
    std::uint32_t count{},identity{},world{},field_definition{};
    std::uint16_t field_index{0xffff};
    std::uint16_t reserved{};
    float layer_height{};
};
// Native values have owned members. Only their constructors, normal cleanup,
// and native calls may operate on these; they never cross the pipe boundary.
struct alignas(16) NavigationContext { std::array<std::byte,0x130> bytes{}; };
struct alignas(16) GroundRayQuery { std::array<std::byte,0x130> bytes{}; };
struct GroundRayResult {
    void* crossings{};
    std::uint32_t count{},capacity{},classification{};
    float distance{};
};
static_assert(sizeof(GroundRayResult)==0x18);
// Explicit trailing alignment space is outside the native 1a8/58-byte prefix.
struct alignas(16) PlacementQuery { std::array<std::byte,0x1b0> bytes{}; };
struct alignas(16) PlacementResult { std::array<std::byte,0x60> bytes{}; };
} // namespace crimson::marketplace::teleport
