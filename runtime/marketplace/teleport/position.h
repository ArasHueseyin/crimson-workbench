#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace crimson::marketplace::teleport {
// Output of the native current-world transform getter (4f8220). This must not
// be populated from the raw component's local position, which may have a
// non-identity parent transform. The native global decoder uses signed sectors
// at +0xc/+0xe and the pinned 1000.0f sector extent.
struct WorldPosition {
    float x{}, y{}, z{};
    friend bool operator==(const WorldPosition&, const WorldPosition&) = default;
};
struct CompressedWorldPosition {
    float x{}, y{}, z{};
    std::int16_t sector_x{}, sector_z{};
};
static_assert(sizeof(WorldPosition)==12 && sizeof(CompressedWorldPosition)==16);
static_assert(offsetof(CompressedWorldPosition,sector_x)==0xc && offsetof(CompressedWorldPosition,sector_z)==0xe);
inline bool finite(const WorldPosition& value) noexcept {
    return std::isfinite(value.x)&&std::isfinite(value.y)&&std::isfinite(value.z);
}
inline std::optional<WorldPosition> decode_world_position(const CompressedWorldPosition& value) noexcept {
    // Retain the original float operation order, including negative sectors.
    const WorldPosition result{
        static_cast<float>(value.sector_x)*1000.0f+value.x,
        value.y,
        static_cast<float>(value.sector_z)*1000.0f+value.z};
    if(!finite(result))return std::nullopt;
    return result;
}
// Pure persistence payload. A field definition and stage are stable data keys;
// runtime world IDs, process addresses and actor pointers never belong here.
struct SavedPosition {
    std::uint32_t field_definition{};
    std::uint16_t stage{0xffff};
    std::uint16_t reserved{};
    WorldPosition position{};
    float yaw{};
    std::uint32_t version{1};
    std::uint32_t reserved_tail{};
};
static_assert(sizeof(SavedPosition)==32 && offsetof(SavedPosition,position)==8 && offsetof(SavedPosition,yaw)==20);
static_assert(std::is_trivially_copyable_v<SavedPosition> && std::is_standard_layout_v<SavedPosition>);
inline bool valid_saved_position(const SavedPosition& value) noexcept {
    return value.version==1&&value.field_definition&&value.stage!=0xffff&&!value.reserved&&!value.reserved_tail&&
        finite(value.position)&&std::isfinite(value.yaw);
}
} // namespace crimson::marketplace::teleport
