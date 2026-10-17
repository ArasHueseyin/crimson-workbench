#include "../jump/player_jump_policy.h"
#include "../jump/player_jump_settings.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <limits>

namespace {
using namespace crimson::marketplace::player_jump;
template<class T> void put(void* value, std::size_t offset, T data) { std::memcpy(static_cast<unsigned char*>(value) + offset, &data, sizeof(data)); }
template<class T> T get(const void* value, std::size_t offset) { T data; std::memcpy(&data, static_cast<const unsigned char*>(value) + offset, sizeof(data)); return data; }
struct Fixture {
    Address base{};
    std::array<unsigned char, 0x38> root{};
    std::array<unsigned char, 0x58> manager{};
    std::array<unsigned char, 0xb0> actor{};
    std::array<unsigned char, 2> metadata{};
    std::array<unsigned char, 0x48> components{};
    std::array<unsigned char, 0xd8> possessor{};
    std::array<unsigned char, 0xd0> control{};
    std::array<unsigned char, 0x5c0> state{};
    Fixture() {
        base = reinterpret_cast<Address>(VirtualAlloc(nullptr, 0x6d6a000, MEM_RESERVE, PAGE_NOACCESS));
        assert(base && VirtualAlloc(reinterpret_cast<void*>(base + 0x6d69000), 4096, MEM_COMMIT, PAGE_READWRITE));
        put(reinterpret_cast<void*>(base + crimson::marketplace::stamina::kClientRoot), 0, root.data());
        put(root.data(), 0x30, manager.data());
        put(manager.data(), 0, base + crimson::marketplace::stamina::kClientManager);
        put(manager.data(), 0x50, actor.data());
        put(actor.data(), 0, base + crimson::marketplace::stamina::kClientActor);
        put(actor.data(), 0x60, std::uint32_t{0xa0101234});
        put(actor.data(), 0x68, components.data());
        put(actor.data(), 0x70, std::uint32_t{24});
        put(actor.data(), 0x88, metadata.data()); metadata[1] = 1;
        put(actor.data(), 0x96, std::uint8_t{1});
        put(actor.data(), 0xa0, possessor.data());
        put(possessor.data(), 0xd0, actor.data());
        put(components.data(), 0x40, control.data());
        put(control.data(), 8, actor.data());
        put(control.data(), 0xc8, state.data());
        put(state.data(), 0x68, actor.data());
        put(state.data(), 0x320, std::uint8_t{1});
        put(state.data(), 0x32c, 2.5f);
    }
    ~Fixture() { assert(VirtualFree(reinterpret_cast<void*>(base), 0, MEM_RELEASE)); }
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
};
Fixture* fixture_ptr{};
float expected{};
bool success=true, recursive=false, changed_by_original=false, selection_changed=false;
unsigned calls=0;
bool __fastcall consume(void* state, std::uintptr_t arg2, std::uint8_t arg3, std::uint8_t arg4) {
    assert(state == fixture_ptr->state.data() && arg2 == 0xfedcba9876543210 && arg3 == 0xa5 && arg4 == 0x5a);
    const float height = get<float>(state, 0x32c);
    assert((std::isnan(expected) && std::isnan(height)) || height == expected);
    ++calls;
    if (selection_changed) put(fixture_ptr->manager.data(),0x50,Address{0});
    if (recursive) {
        recursive = false;
        assert(dispatch(consume, fixture_ptr->base, 140, state, arg2, arg3, arg4) == success);
    }
    if (changed_by_original) put(state, 0x32c, 17.0f);
    else if (success) { put(state, 0x320, std::uint8_t{0}); put(state, 0x32c, 0.0f); }
    return success;
}
void invoke(Fixture& value, float height, bool result=true) {
    fixture_ptr = &value; expected = height; success = result;
    assert(dispatch(consume, value.base, 140, value.state.data(), 0xfedcba9876543210, 0xa5, 0x5a) == result);
}
bool __fastcall invalid(void* state, std::uintptr_t arg2, std::uint8_t a, std::uint8_t b) {
    assert(state == reinterpret_cast<void*>(1) && arg2 == 123 && a == 4 && b == 5); ++calls; return false;
}
struct ChangingMemory : Memory {
    Fixture& value;
    explicit ChangingMemory(Fixture& f) : value(f) {}
    unsigned reads=0;
    template<class T> T at(Address address) {
        const auto result = Memory::at<T>(address);
        if(address == reinterpret_cast<Address>(value.actor.data()) + 0x60 && ++reads == 2)
            ::put(value.manager.data(), 0x50, Address{0});
        return result;
    }
};
}
int main() {
    const unsigned char config[]{'C','W','J','1',140,0,0,0};
    assert(setting(config, 8) == 140);
    assert(setting(nullptr, 8) == 100 && setting(config, 7) == 100 && setting(config, 9) == 100);
    for (auto percent : {0u, 99u, 100u, 101u, 150u, 300u, 301u, 0xffffffffu}) {
        auto bytes = std::array<unsigned char,8>{'C','W','J','1',0,0,0,0};
        put(bytes.data(), 4, percent);
        assert(setting(bytes.data(), bytes.size()) == (percent>=100 && percent<=300 ? percent : 100));
    }
    for (unsigned kind : {1u, 4u}) {
        Fixture f; f.metadata[1] = static_cast<unsigned char>(kind);
        for (unsigned repeat=0; repeat<8; ++repeat) {
            put(f.state.data(), 0x320, std::uint8_t{1}); put(f.state.data(), 0x32c, 2.5f);
            invoke(f, 3.5f);
            assert(get<float>(f.state.data(), 0x32c) == 0);
        }
    }
    { Fixture f; invoke(f, 3.5f, false); assert(get<float>(f.state.data(),0x32c)==2.5f); invoke(f,3.5f,false); }
    { Fixture f; recursive=true; invoke(f,3.5f,false); assert(get<float>(f.state.data(),0x32c)==2.5f); }
    { Fixture f; changed_by_original=true; invoke(f,3.5f,false); changed_by_original=false; assert(get<float>(f.state.data(),0x32c)==17.0f); }
    { Fixture f; selection_changed=true; invoke(f,3.5f,false); selection_changed=false; assert(get<float>(f.state.data(),0x32c)==2.5f); }
    { Fixture f; fixture_ptr=&f; expected=2.5f; success=true; assert(dispatch(consume,f.base,100,f.state.data(),0xfedcba9876543210,0xa5,0x5a)); }
    for (unsigned invalid_case=0; invalid_case<15; ++invalid_case) {
        Fixture f;
        switch(invalid_case) {
        case 0: put(f.manager.data(),0x50,Address{0}); break;
        case 1: f.metadata[1]=6; break; // Mounts must remain untouched.
        case 2: put(f.actor.data(),0,Address{0}); break;
        case 3: put(f.actor.data(),0x60,std::uint32_t{0}); break;
        case 4: put(f.actor.data(),0x96,std::uint8_t{0}); break;
        case 5: put(f.actor.data(),0x70,std::uint32_t{8}); break;
        case 6: put(f.actor.data(),0x70,std::uint32_t{58}); break;
        case 7: put(f.possessor.data(),0xd0,Address{0}); break;
        case 8: put(f.control.data(),8,Address{0}); break;
        case 9: put(f.control.data(),0xc8,Address{0}); break;
        case 10: put(f.state.data(),0x68,Address{0}); break;
        case 11: put(f.state.data(),0x320,std::uint8_t{0}); break;
        case 12: put(f.state.data(),0x34d,std::uint8_t{1}); break;
        case 13: put(f.actor.data(),0x88,Address{0}); break;
        case 14: put(f.root.data(),0x30,Address{0}); break;
        }
        invoke(f,2.5f,false);
        assert(get<float>(f.state.data(),0x32c)==2.5f);
    }
    for (float value : {0.0f,-1.0f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::max()}) {
        Fixture f; put(f.state.data(),0x32c,value); invoke(f,value,false);
    }
    { Fixture f; ChangingMemory read(f); Adjustment change; assert(!adjust(read,f.base,reinterpret_cast<Address>(f.state.data()),140,change)); assert(get<float>(f.state.data(),0x32c)==2.5f); }
    assert(!dispatch(invalid,1,140,reinterpret_cast<void*>(1),123,4,5));
    std::printf("PASS: player jump settings, selected-owner dispatch, restoration, reentrancy and invalid-memory fallback (%u native calls).\n",calls);
}
