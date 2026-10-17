#pragma once
#include <algorithm>
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>
#include "crafting_scope.h"
#include "crafting_source_costs.h"

namespace crimson::marketplace::crafting {
// Native allocation sites 4a6e95..4a6ee9 and f5420c2..f54210f allocate
// 0x100 bytes aligned to 0x10 before calling MultiChangeInfo constructor 2428070.
using RecipeBytes = std::array<std::uint8_t,0x100>;
using FixedBytes = std::array<std::uint8_t,0x20>;
template<class T, std::size_t N> T field(const std::array<std::uint8_t,N>& bytes, std::size_t at) noexcept {
    T value{}; std::memcpy(&value,bytes.data()+at,sizeof(value)); return value;
}
template<class T, std::size_t N> void set_field(std::array<std::uint8_t,N>& bytes, std::size_t at, T value) noexcept {
    std::memcpy(bytes.data()+at,&value,sizeof(value));
}
struct ShadowSource {
    Address manager{}, rows{}, original{};
    std::uint16_t handle=0xffff;
    RecipeBytes row{};
    FixedBytes fee{};
    bool retain_fee{};
    bool operator==(const ShadowSource&) const = default;
};

template<class Read> bool fixed_item_key(Read& read, Address base, std::uint16_t handle, std::uint32_t& key) {
    constexpr Address item_manager_rva=0x6d69aa8;
    const auto manager=read.template at<Address>(base+item_manager_rva);
    if (!address(manager,0x60)) return false;
    const auto count=read.template at<std::uint32_t>(manager+8);
    const auto rows=read.template at<Address>(manager+0x58);
    if (!count || count>65535 || handle>=count || !address(rows,count*Address{8})) return false;
    const auto row=read.template at<Address>(rows+handle*Address{8});
    if (!address(row,0x11) || read.template at<std::uint8_t>(row+0x10)) return false;
    const auto found=read.template at<std::uint32_t>(row);
    if (!found || read.template at<Address>(base+item_manager_rva)!=manager ||
        read.template at<std::uint32_t>(manager+8)!=count ||
        read.template at<Address>(manager+0x58)!=rows ||
        read.template at<Address>(rows+handle*Address{8})!=row ||
        read.template at<std::uint32_t>(row)!=found || read.template at<std::uint8_t>(row+0x10)) return false;
    key=found; return true;
}

// Capture the original row, without changing it or replacing a table entry.
// The optional pre-existing archive currency component removes exactly one
// known fee. Both supported original layouts preserve native prerequisites.
template<class Read> bool capture_shadow_source(Read& read, Address base, std::uint16_t handle,
                                              Address returned, ShadowSource& output) {
    if (!base || handle>=kRecipeCount || !address(returned,0x100)) return false;
    ShadowSource value; value.handle=handle; value.original=returned;
    value.manager=read.template at<Address>(base+kRecipeManager);
    if (!address(value.manager,0x60) || read.template at<std::uint32_t>(value.manager+8)!=kRecipeCount) return false;
    value.rows=read.template at<Address>(value.manager+0x58);
    if (!address(value.rows,kRecipeCount*Address{8}) ||
        read.template at<Address>(value.rows+handle*Address{8})!=returned) return false;
    value.row=read.template at<RecipeBytes>(returned);
    const auto known=known_recipe(field<std::uint32_t>(value.row,8));
    if (!known || field<Address>(value.row,0)!=base+kRecipeVtable || value.row[0x18] ||
        value.row[0x1c]!=1 || field<std::uint32_t>(value.row,0x28)!=known->conditions ||
        field<std::uint32_t>(value.row,0xe8)!=1) return false;
    for (std::size_t i=0;i<5;++i) if (value.row[0x38+i]!=known->flags[i]) return false;
    const auto needed=field<std::uint16_t>(value.row,0x30);
    if (!known->knowledge) { if (needed!=0xffff) return false; }
    else {
        ResolvedKnowledge definition;
        if (!knowledge(read,base,known->knowledge,definition) || definition.handle!=needed) return false;
    }
    const auto counts=source_cost_counts(known->key);
    const auto fixed=field<std::uint32_t>(value.row,0x48);
    const auto groups=field<std::uint32_t>(value.row,0x58);
    const bool fee_recipe=known->key==1010292;
    const bool removed_fee=fee_recipe && fixed==4;
    if (counts==0xff || groups!=static_cast<std::uint32_t>(counts&15) ||
        (fixed!=static_cast<std::uint32_t>(counts>>4) && !removed_fee) ||
        fixed>field<std::uint32_t>(value.row,0x4c) || groups>field<std::uint32_t>(value.row,0x5c) ||
        (!fixed && !groups)) return false;
    if (fixed && !address(field<Address>(value.row,0x40),fixed*Address{0x20})) return false;
    if (groups && !address(field<Address>(value.row,0x50),groups*Address{0x18})) return false;
    if (fee_recipe) {
        constexpr std::array<std::uint32_t,5> keys{1002130,1003633,1003827,1000485,1002198};
        constexpr std::array<std::uint64_t,5> amounts{1,3,1,300,200};
        const auto items=field<Address>(value.row,0x40);
        for (std::uint32_t i=0;i<fixed;++i) {
            const auto entry=read.template at<FixedBytes>(items+i*Address{0x20});
            const auto expected=removed_fee && i>=3 ? i+1 : i;
            std::uint32_t key{};
            if (!fixed_item_key(read,base,field<std::uint16_t>(entry,0),key) || key!=keys[expected] ||
                field<std::uint16_t>(entry,2)!=0xffff || field<std::uint16_t>(entry,4)!=0xffff ||
                field<std::uint64_t>(entry,8)!=amounts[expected] || field<std::uint64_t>(entry,0x10) ||
                field<std::uint16_t>(entry,0x18)) return false;
            if (key==1000485) { value.fee=entry; value.retain_fee=true; }
            if(read.template at<FixedBytes>(items+i*Address{0x20})!=entry)return false;
        }
        if (value.retain_fee==removed_fee) return false;
    }
    if (read.template at<Address>(base+kRecipeManager)!=value.manager ||
        read.template at<std::uint32_t>(value.manager+8)!=kRecipeCount ||
        read.template at<Address>(value.manager+0x58)!=value.rows ||
        read.template at<Address>(value.rows+handle*Address{8})!=returned ||
        read.template at<RecipeBytes>(returned)!=value.row) return false;
    output=value; return true;
}

// Returned views may be retained by native UI/model readers. They are immutable
// and are never recycled when a later lookup observes a new table generation.
// The pinned production module must retain the pool until process exit, without
// invoking native row destructors on these borrowed-definition views.
class ShadowPool {
    struct alignas(16) View {
        alignas(16) RecipeBytes row;
        alignas(16) FixedBytes fee;
        ShadowSource source;
        explicit View(const ShadowSource& input) : row(input.row), fee(input.fee), source(input) {
            std::fill(row.begin()+0x40,row.begin()+0x60,std::uint8_t{0});
            if (source.retain_fee) {
                set_field(row,0x40,reinterpret_cast<Address>(fee.data()));
                set_field(row,0x48,std::uint32_t{1}); set_field(row,0x4c,std::uint32_t{1});
            }
        }
    };
    std::mutex mutex_;
    std::vector<std::unique_ptr<View>> views_;
    std::size_t maximum_;
public:
    explicit ShadowPool(std::size_t maximum=8192) : maximum_(maximum) {}
    ShadowPool(const ShadowPool&)=delete;
    ShadowPool& operator=(const ShadowPool&)=delete;
    Address view(const ShadowSource& source) noexcept {
        try {
            std::lock_guard lock(mutex_);
            for (const auto& old:views_) if (old->source==source) return reinterpret_cast<Address>(old->row.data());
            if (views_.size()>=maximum_) return 0;
            auto next=std::make_unique<View>(source);
            const auto result=reinterpret_cast<Address>(next->row.data());
            views_.push_back(std::move(next)); return result;
        } catch (...) { return 0; }
    }
};

// Only the recipe-row pointer lookup is virtualized for a policy probe. All
// actor ownership, knowledge, original-manager and other reads stay native.
// Construction and use belong to one current native callback, never an IPC
// worker or a persistent borrowed-memory cache.
template<class Read> class ShadowRead {
    Read& original_;
    ShadowPool& pool_;
    Address base_,rows_{};
public:
    ShadowRead(Read& original,ShadowPool& pool,Address image) : original_(original),pool_(pool),base_(image) {
        const auto manager=original_.template at<Address>(image+kRecipeManager);
        if (address(manager,0x60) && original_.template at<std::uint32_t>(manager+8)==kRecipeCount)
            rows_=original_.template at<Address>(manager+0x58);
        if (!address(rows_,kRecipeCount*Address{8})) rows_=0;
    }
    template<class T> T at(Address location) {
        const auto native=original_.template at<T>(location);
        if constexpr (std::is_same_v<T,Address>) {
            if (rows_ && location>=rows_ && location-rows_<kRecipeCount*Address{8} && !((location-rows_)&7)) {
                const auto handle=static_cast<std::uint16_t>((location-rows_)/8);
                ShadowSource source;
                if (capture_shadow_source(original_,base_,handle,native,source))
                    if (const auto view=pool_.view(source)) return view;
            }
        }
        return native;
    }
};
} // namespace crimson::marketplace::crafting
