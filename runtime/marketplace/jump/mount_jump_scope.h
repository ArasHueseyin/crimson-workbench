#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace crimson::marketplace::jump {
using Address = std::uintptr_t;
inline constexpr Address kCharacterInfoManager = 0x6d69a48;
inline constexpr float kMountHeightMultiplier = 1.40f;
inline constexpr std::uint32_t kLion = 1002316;
inline constexpr std::uint32_t kRokade = 31378;

struct Prepared {
    std::array<unsigned char, 16> reference{};
    std::array<unsigned char, 26> data{};
};

template<class Read>
bool mount(Read& read, Address base, Address actor) {
    if (!base || !actor) return false;
    const auto identity = read.template at<std::uint32_t>(actor + 0x60);
    const auto metadata = read.template at<Address>(actor + 0x88);
    if (!identity || !metadata || read.template at<std::uint8_t>(actor + 0x96) != 1) return false;
    const auto kind = read.template at<std::uint8_t>(metadata + 1);
    if (kind != 1 && kind != 3 && kind != 4 && kind != 5 && kind != 6 && kind != 8) return false;
    const auto components = read.template at<Address>(actor + 0x68);
    const auto count = read.template at<std::uint32_t>(actor + 0x70);
    if (!components || count < 9 || count > 57) return false;
    const auto status = read.template at<Address>(components + 0x20);
    const auto control = read.template at<Address>(components + 0x40);
    if (!status || !control || read.template at<Address>(status + 8) != actor ||
        read.template at<Address>(control + 8) != actor) return false;
    const auto state = read.template at<Address>(control + 0xc8);
    if (!state || read.template at<Address>(state + 0x68) != actor) return false;
    const auto index = read.template at<std::uint16_t>(status + 0x30);
    const auto definitions = read.template at<Address>(base + kCharacterInfoManager);
    if (!definitions) return false;
    const auto row_count = read.template at<std::uint32_t>(definitions + 8);
    const auto rows = read.template at<Address>(definitions + 0x58);
    if (!row_count || row_count > 100'000 || index >= row_count || !rows) return false;
    const auto row = read.template at<Address>(rows + Address{index} * 8);
    if (!row) return false;
    const auto key = read.template at<std::uint32_t>(row);
    if (key != kLion && key != kRokade) return false;
    // Neither the native definition handle nor an actor pointer is cached.
    return read.template at<std::uint32_t>(actor + 0x60) == identity &&
        read.template at<std::uint8_t>(actor + 0x96) == 1 &&
        read.template at<Address>(actor + 0x88) == metadata &&
        read.template at<std::uint8_t>(metadata + 1) == kind &&
        read.template at<Address>(actor + 0x68) == components &&
        read.template at<std::uint32_t>(actor + 0x70) == count &&
        read.template at<Address>(components + 0x20) == status &&
        read.template at<Address>(components + 0x40) == control &&
        read.template at<Address>(status + 8) == actor &&
        read.template at<Address>(control + 8) == actor &&
        read.template at<Address>(control + 0xc8) == state &&
        read.template at<Address>(state + 0x68) == actor &&
        read.template at<std::uint16_t>(status + 0x30) == index &&
        read.template at<Address>(base + kCharacterInfoManager) == definitions &&
        read.template at<std::uint32_t>(definitions + 8) == row_count &&
        read.template at<Address>(definitions + 0x58) == rows &&
        read.template at<Address>(rows + Address{index} * 8) == row &&
        read.template at<std::uint32_t>(row) == key;
}

// The pinned native frame-event function reads only reference+08 and the first
// 26 bytes of its Jump data. It copies the parameters and never retains them.
// Give that synchronous invocation a private copy; original action data stays
// unchanged and cannot accumulate multipliers on repeated jumps.
template<class Read>
bool prepare(Read& read, Address base, Address event, Address actor_reference, Prepared& out) {
    if (!event || !actor_reference) return false;
    const auto actor = read.template at<Address>(actor_reference);
    if (!mount(read, base, actor)) return false;
    const auto data = read.template at<Address>(event + 8);
    if (!data) return false;
    read.copy(event, out.reference.data(), out.reference.size());
    read.copy(data, out.data.data(), out.data.size());
    float height{};
    std::memcpy(&height, out.data.data() + 12, sizeof(height));
    const float higher = height * kMountHeightMultiplier;
    if (!std::isfinite(height) || height <= 0 || !std::isfinite(higher) || higher <= height) return false;
    if (read.template at<Address>(actor_reference) != actor ||
        read.template at<Address>(event + 8) != data || !mount(read, base, actor)) return false;
    std::memcpy(out.data.data() + 12, &higher, sizeof(higher));
    const auto local = reinterpret_cast<Address>(out.data.data());
    std::memcpy(out.reference.data() + 8, &local, sizeof(local));
    return true;
}
} // namespace crimson::marketplace::jump
