#include "../fall/fall_policy.h"
#include <array>
#include <cassert>
#include <cstdio>

namespace {
using namespace crimson::marketplace::fall;
std::array<unsigned char, 8> row{};
unsigned forwarded{};
void* expected_self{};
void* expected_source{};
void* expected_event{};
void* expected_detail{};
float expected_height{};
void* __fastcall resolve(const std::uint16_t* handle) {
    assert(*handle == 42);
    return row.data();
}
void __fastcall original(void* self, void* source, float height, void* event, void* detail) {
    assert(self == expected_self && source == expected_source && event == expected_event && detail == expected_detail);
    assert(height == expected_height);
    ++forwarded;
}
template<class T> void put(void* at, unsigned offset, T value) {
    std::memcpy(static_cast<unsigned char*>(at) + offset, &value, sizeof(value));
}
}

int main() {
    using namespace crimson::marketplace::fall;
    assert(kProtectedKeys.size() == 342 && std::is_sorted(kProtectedKeys.begin(), kProtectedKeys.end()));
    assert(!protected_key(0) && !protected_key(2) && !protected_key(60003));
    std::array<unsigned char, 0x10> component{};
    std::array<unsigned char, 0xa0> actor{};
    std::array<unsigned char, 2> metadata{};
    std::array<unsigned char, 0x28> components{};
    std::array<unsigned char, 0x32> status{};
    put(component.data(), 8, actor.data());
    put(actor.data(), 0x88, metadata.data());
    put(actor.data(), 0x68, components.data());
    put(components.data(), 0x20, status.data());
    put(status.data(), 0x30, std::uint16_t{42});
    expected_self = component.data(); expected_source = &actor;
    expected_event = &metadata; expected_detail = &components; expected_height = 123.25f;
    unsigned checked{};
    for (const auto kind : {1, 3, 4, 5, 6, 8}) {
        metadata[1] = static_cast<unsigned char>(kind);
        for (const auto key : kProtectedKeys) {
            put(row.data(), 0, key);
            assert(dispatch_fall(original, resolve, expected_self, expected_source, expected_height, expected_event, expected_detail));
            ++checked;
        }
        put(row.data(), 0, std::uint32_t{2});
        assert(!dispatch_fall(original, resolve, expected_self, expected_source, expected_height, expected_event, expected_detail));
    }
    assert(forwarded == 6);
    put(row.data(), 0, std::uint32_t{1002316});
    metadata[1] = 0;
    assert(!dispatch_fall(original, resolve, expected_self, expected_source, expected_height, expected_event, expected_detail));
    expected_self = nullptr;
    assert(!dispatch_fall(original, resolve, expected_self, expected_source, expected_height, expected_event, expected_detail));
    assert(forwarded == 8 && checked == 2052);
    std::puts("PASS: all 342 protected definitions across six native actor kinds; unknown, unsupported and unavailable actors forward unchanged. Own objects only; no game writes.");
}
