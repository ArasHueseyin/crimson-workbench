#include "../src/prepare.h"
#include <cassert>
#include <initializer_list>

using namespace crimson::mac_items;
unsigned extra_offset{}, destroyed{}, serialized{};
void* construct(void* value, const std::uint16_t* type, std::int64_t quantity) {
    const std::int64_t uid = -1;
    std::memcpy(value, &uid, 8);
    std::memcpy(static_cast<char*>(value)+8, type, 2);
    std::memcpy(static_cast<char*>(value)+0x10, &quantity, 8);
    const std::uint32_t backing_count = 5;
    std::memcpy(static_cast<char*>(value)+0x68, &backing_count, 4);
    if (extra_offset) static_cast<unsigned char*>(value)[extra_offset] = extra_offset == 0x68 ? 6 : 1;
    return value;
}
void* initialize(void* init) { return init; }
void serialize(const void* value, std::uint16_t slot, void* init) {
    assert(slot == 0xffff);
    assert(value_field<std::uint16_t>(value, 8) == 1243);
    ++serialized;
    static_cast<unsigned char*>(init)[0x5e] = 0;
}
void* destroy(void* value) { ++destroyed; return value; }
int main() {
    ItemBindings bindings{construct, initialize, serialize, destroy};
    alignas(16) unsigned char init[0x1c0]{};
    assert(prepare_timber(bindings, 1243, init) == init);
    assert(destroyed == 1 && serialized == 1);
    for (auto offset : {0x68U, 0x70U, 0x80U, 0xb0U}) {
        extra_offset = offset;
        assert(prepare_timber(bindings, 1243, init) == nullptr);
    }
    assert(destroyed == 5 && serialized == 1);
}
