#pragma once
#include "position.h"
#include <array>
namespace crimson::marketplace::teleport {
struct NativeFieldMoveValue {
    std::uint32_t field_definition{};
    std::uint16_t character_definition{0xffff};
    std::uint16_t padding_06{};
    WorldPosition position{};
    float yaw{};
    std::uint8_t flag_18{},flag_19{},flag_1a{},flag_1b{},flag_1c{};
    std::array<std::byte,3> padding_1d{};
    std::uint32_t key_20{};
    std::uint32_t padding_24{};
    void* native_characters{};
    std::uint32_t native_character_count{},native_character_capacity{};
};
static_assert(sizeof(NativeFieldMoveValue)==0x38 && offsetof(NativeFieldMoveValue,position)==8 &&
    offsetof(NativeFieldMoveValue,yaw)==0x14 && offsetof(NativeFieldMoveValue,flag_18)==0x18 &&
    offsetof(NativeFieldMoveValue,flag_1c)==0x1c && offsetof(NativeFieldMoveValue,key_20)==0x20 &&
    offsetof(NativeFieldMoveValue,native_characters)==0x28);
}
