#pragma once
#include <limits>
#include "crafting_recipes.h"
#include "crafting_targets.h"

namespace crimson::marketplace::crafting {
inline bool address(Address value, std::size_t size = 1) noexcept {
    return value >= 0x10000 && value <= 0x00007fffffffffffULL - size;
}
struct ResolvedKnowledge {
    Address manager{}, buckets{}, nodes{}, bucket{}, node{}, matched{};
    std::uint32_t rows{}, bucket_count{}, node_count{}, used{}, node_index{};
    std::uint16_t handle = 0xffff;
};
// Mirrors the pinned KnowledgeInfo deserializer's semantic-key lookup. It does
// not call a game function, load a table, cache a borrowed row, or edit knowledge.
template<class Read>
bool knowledge(Read& read, Address base, std::uint32_t key, ResolvedKnowledge& out) {
    if (!base || !key) return false;
    ResolvedKnowledge value;
    value.manager = read.template at<Address>(base + kKnowledgeManager);
    if (!address(value.manager,0x88)) return false;
    value.rows = read.template at<std::uint32_t>(value.manager+8);
    value.bucket_count = read.template at<std::uint32_t>(value.manager+0x68);
    value.node_count = read.template at<std::uint32_t>(value.manager+0x6c);
    value.buckets = read.template at<Address>(value.manager+0x78);
    value.nodes = read.template at<Address>(value.manager+0x80);
    if (!value.rows || value.rows > 65535 || !value.bucket_count || value.bucket_count > 1000000 ||
        !value.node_count || value.node_count > 1000000 || !address(value.buckets) || !address(value.nodes)) return false;
    value.bucket = value.buckets + (key % value.bucket_count) * Address{0x100};
    if (!address(value.bucket,0x100)) return false;
    value.used = read.template at<std::uint32_t>(value.bucket);
    if (value.used > 31) return false;
    bool found = false;
    for (std::uint32_t i = 0; i < value.used; ++i) {
        const auto entry = value.bucket+8+i*Address{8};
        if (read.template at<std::uint32_t>(entry) != key) continue;
        if (found) return false;
        value.matched = entry;
        value.node_index = read.template at<std::uint32_t>(entry+4);
        if (value.node_index >= value.node_count) return false;
        value.node = read.template at<Address>(value.nodes+value.node_index*Address{8});
        if (!address(value.node,10) || read.template at<std::uint32_t>(value.node+4) != key) return false;
        value.handle = read.template at<std::uint16_t>(value.node+8);
        if (value.handle >= value.rows) return false;
        found = true;
    }
    if (!found || read.template at<Address>(base+kKnowledgeManager) != value.manager ||
        read.template at<std::uint32_t>(value.manager+8) != value.rows ||
        read.template at<std::uint32_t>(value.manager+0x68) != value.bucket_count ||
        read.template at<std::uint32_t>(value.manager+0x6c) != value.node_count ||
        read.template at<Address>(value.manager+0x78) != value.buckets ||
        read.template at<Address>(value.manager+0x80) != value.nodes ||
        read.template at<std::uint32_t>(value.bucket) != value.used ||
        read.template at<std::uint32_t>(value.matched) != key ||
        read.template at<std::uint32_t>(value.matched+4) != value.node_index ||
        read.template at<Address>(value.nodes+value.node_index*Address{8}) != value.node ||
        read.template at<std::uint32_t>(value.node+4) != key ||
        read.template at<std::uint16_t>(value.node+8) != value.handle) return false;
    out = value;
    return true;
}
struct Recipe {
    Address manager{}, rows{}, row{};
    std::uint16_t handle = 0xffff;
    std::uint16_t need_knowledge = 0xffff;
    const KnownRecipe* known{};
};
template<class Read>
bool recipe(Read& read, Address base, std::uint16_t handle, Recipe& out) {
    if (!base || handle >= kRecipeCount) return false;
    Recipe value;
    value.handle = handle;
    value.manager = read.template at<Address>(base+kRecipeManager);
    if (!address(value.manager,0x60) || read.template at<std::uint32_t>(value.manager+8) != kRecipeCount) return false;
    value.rows = read.template at<Address>(value.manager+0x58);
    if (!address(value.rows, kRecipeCount*8)) return false;
    value.row = read.template at<Address>(value.rows+handle*Address{8});
    if (!address(value.row,0xfc) || read.template at<Address>(value.row) != base+kRecipeVtable) return false;
    value.known = known_recipe(read.template at<std::uint32_t>(value.row+8));
    if (!value.known || read.template at<std::uint8_t>(value.row+0x18) != 0 ||
        read.template at<std::uint8_t>(value.row+0x1c) != 1 ||
        read.template at<std::uint32_t>(value.row+0x48) != 0 ||
        read.template at<std::uint32_t>(value.row+0x58) != 0 ||
        read.template at<std::uint32_t>(value.row+0x28) != value.known->conditions ||
        read.template at<std::uint32_t>(value.row+0xe8) != 1) return false;
    for (std::size_t i=0; i<5; ++i)
        if (read.template at<std::uint8_t>(value.row+0x38+i) != value.known->flags[i]) return false;
    value.need_knowledge = read.template at<std::uint16_t>(value.row+0x30);
    if (!value.known->knowledge) {
        if (value.need_knowledge != 0xffff) return false;
    } else {
        ResolvedKnowledge required;
        if (!knowledge(read,base,value.known->knowledge,required) || required.handle != value.need_knowledge) return false;
    }
    out = value;
    return true;
}
template<class Read>
bool stable(Read& read, Address base, const Recipe& value) {
    Recipe current;
    return recipe(read,base,value.handle,current) && current.row == value.row && current.rows == value.rows &&
        current.manager == value.manager && current.known == value.known && current.need_knowledge == value.need_knowledge;
}
} // namespace crimson::marketplace::crafting
