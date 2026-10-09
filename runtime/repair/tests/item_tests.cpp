#include "repair_item.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <type_traits>

using namespace crimson::repair;
namespace {
unsigned checks = 0, creates = 0, copies = 0, destroys = 0, alternate_destroys = 0;
bool bad_construct = false, bad_assign = false;
void require(bool ok, const char* message) { ++checks; if (!ok) throw std::runtime_error(message); }
void* construct(void* p) noexcept {
    ++creates; std::memset(p, 0, action::item_size);
    return bad_construct ? nullptr : p;
}
void* assign(void* p, const void* source) noexcept {
    ++copies; if (p != source) std::memcpy(p, source, action::item_size);
    return bad_assign ? nullptr : p;
}
void destroy(void*) noexcept { ++destroys; }
void destroy_alternate(void*) noexcept { ++alternate_destroys; }
const item::Binding binding{construct, assign, destroy};
void tests() {
    static_assert(!std::is_copy_constructible_v<item::Value> && !std::is_move_constructible_v<item::Value>);
    for (unsigned missing = 0; missing < 3; ++missing) {
        auto broken = binding;
        if (!missing) broken.construct = nullptr;
        if (missing == 1) broken.assign = nullptr;
        if (missing == 2) broken.destroy = nullptr;
        item::Value value(broken);
        require(value.construct() == item::Code::invalid_binding && !value.data(), "Incomplete lifecycle rejected before construction");
    }
    require(!creates && !destroys, "Rejected lifecycle never touches native functions");
    alignas(16) std::array<std::uint8_t, action::item_size> source{};
    source[0x40] = 27;
    {
        item::Value a(binding);
        require(!a.data() && a.release() == item::Code::empty && a.assign(source.data()) == item::Code::empty, "Unconstructed item is inaccessible");
        require(a.construct() == item::Code::constructed && a.data() && creates == 1, "Construct once");
        require(a.construct() == item::Code::already_constructed && creates == 1, "No duplicate constructor");
        require(a.assign(nullptr) == item::Code::invalid_source && !copies, "Null source refused");
        require(a.assign(static_cast<const std::uint8_t*>(a.data()) + 8) == item::Code::invalid_source && !copies, "Overlapping source refused");
        require(a.assign(source.data() + 1) == item::Code::invalid_source && !copies, "Unaligned source refused");
        for (const auto at : {std::size_t(0x60), std::size_t(0x78), std::size_t(0xa8)}) {
            const std::uint32_t one = 1, zero = 0;
            std::memcpy(source.data() + at + 8, &one, 4);
            require(a.assign(source.data()) == item::Code::invalid_source && !copies, "Count beyond allocation refused before native assignment");
            std::memcpy(source.data() + at + 8, &zero, 4);
            std::memcpy(source.data() + at + 12, &one, 4);
            require(a.assign(source.data()) == item::Code::invalid_source && !copies, "Allocated capacity without buffer refused");
            std::memcpy(source.data() + at + 12, &zero, 4);
        }
        source[0x70] = 3;
        require(a.assign(source.data()) == item::Code::copied && copies == 1 &&
            static_cast<const std::uint8_t*>(a.data())[0x40] == 27 && static_cast<const std::uint8_t*>(a.data())[0x70] == 3,
            "Unstored logical slots permit a null socket buffer and use the bound copy method");
        require(a.assign(a.data()) == item::Code::copied && copies == 2, "Native self-assignment remains available");
        bool rejected = false;
        std::thread other([&] { rejected = !a.data() && a.construct() == item::Code::wrong_thread &&
            a.assign(source.data()) == item::Code::wrong_thread && a.release() == item::Code::wrong_thread; });
        other.join();
        require(rejected && destroys == 0 && copies == 2, "Cross-thread operations have no native side effects");
        {
            item::Value b({construct, assign, destroy_alternate});
            require(b.construct() == item::Code::constructed, "Second value has independent lifecycle binding");
        }
        require(alternate_destroys == 1 && destroys == 0, "Correct destructor retained per value");
        require(a.release() == item::Code::released && !a.data() && destroys == 1, "Release calls destructor and hides value");
        require(a.release() == item::Code::empty && destroys == 1, "No double destruction");
        require(a.construct() == item::Code::constructed && a.data(), "Storage can be reconstructed after destruction");
        bad_assign = true;
        require(a.assign(source.data()) == item::Code::native_failure && !a.data(), "Bad copy result cannot expose usable event data");
        const auto attempts = copies;
        require(a.assign(source.data()) == item::Code::native_failure && copies == attempts, "Failed native assignment cannot be retried before cleanup");
        bad_assign = false;
    }
    require(destroys == 2, "Scope exit destroys remaining value after failed assignment");
    {
        item::Value a(binding); bad_construct = true;
        require(a.construct() == item::Code::native_failure && !a.data(), "Malformed constructor return reported without usable data");
        bad_construct = false;
    }
    require(destroys == 3 && alternate_destroys == 1 && creates == 4, "Every constructed object receives exactly its bound cleanup");
}
}
int main() {
    try { tests(); std::cout << "{\"ok\":true,\"checks\":" << checks
        << ",\"native_item_owner\":true,\"binding_source\":\"fixture_callbacks\",\"game_process_access\":false}\n"; return 0; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
