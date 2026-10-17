#pragma once
#include "position.h"
#include <array>

namespace crimson::marketplace::teleport {
// Native move request value (TrocTrMoveActorReq and 21a1950). Its child data
// contains values only, not retained engine references. It is not our IPC ABI.
struct NativeMoveValue {
    WorldPosition position{};
    std::array<float,4> rotation{0,0,0,1};
    std::uint32_t mode{};
    std::array<std::byte,0x28> transform{};
    std::array<std::byte,0x88> motion{};
    std::uint8_t flag_d0{};
    std::array<std::byte,7> padding_d1{};
    std::uint64_t native_time{};
    std::uint8_t time_realm{};
    std::array<std::byte,7> padding_e1{};
};
static_assert(sizeof(NativeMoveValue)==0xe8 && offsetof(NativeMoveValue,rotation)==0xc &&
    offsetof(NativeMoveValue,transform)==0x20 && offsetof(NativeMoveValue,motion)==0x48 &&
    offsetof(NativeMoveValue,flag_d0)==0xd0 && offsetof(NativeMoveValue,native_time)==0xd8);

// Original field-transition value copied by 2bbf510 / 2bc4950 / 2bbf630.
// Keep semantically unproven flags named by their offsets. No caller should
// choose them by guesswork. Native owns the optional copied character list.
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
// Pin vector row, as copied by CreateOrChangePinMarkerAck and server
// ContentsMisc+0xc8. Its Y value has not yet been proven to be a ground height.
struct NativePinMarker {
    std::uint64_t id{};
    WorldPosition position{};
    std::uint8_t shape{},color{};
    std::array<std::byte,2> padding{};
};
static_assert(sizeof(NativePinMarker)==24 && offsetof(NativePinMarker,position)==8 && offsetof(NativePinMarker,shape)==20);
} // namespace crimson::marketplace::teleport
