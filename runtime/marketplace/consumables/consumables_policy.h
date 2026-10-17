#pragma once
#include <Windows.h>
#include <cstring>
#include "consumables_scope.h"

namespace crimson::marketplace::consumables {
using Use = void*(__fastcall*)(void*,void*,void*,void*,std::uint16_t,void*,
                              std::uint16_t,void*,std::uint16_t,std::uint64_t);
using Lookup = void*(__fastcall*)(const std::uint32_t*);
struct RequestVector { const void* data{}; std::uint32_t count{}, capacity{}; };
static_assert(sizeof(RequestVector)==16);
// The relocated native wrapper writes the ninth stack home as a temporary
// result (entry RSP+0x48). Reserve it explicitly when calling the trampoline.
using Pop = void*(__fastcall*)(void*,void*,void*,const RequestVector*,std::uint64_t,
                              std::uint8_t,std::uint16_t,std::uint8_t,std::uint64_t);
struct Memory {
    template<class T> T at(Address address) {
        T value; std::memcpy(&value,reinterpret_cast<const void*>(address),sizeof(value)); return value;
    }
};
struct Shadow {
    Address original{};
    std::uint32_t handle{};
    std::uint32_t reserved{};
    alignas(16) unsigned char info[0x20]{};
    alignas(16) unsigned char data[0x40]{};
    unsigned char original_info[0x20]{}, original_data[0x40]{};
};
struct Context {
    Proof proof{};
    bool valid{};
    std::array<Shadow,kMaxUses> shadows{};
    std::uint32_t shadow_count{};
};
inline thread_local Context* current = nullptr;

inline bool guarded_begin(Context& context, Address base, const Settings& settings,
    void* inventory, void* actor, std::uint16_t handle, void* amount, std::uint16_t definition_handle,
    void* source, std::uint16_t slot_index, std::uint64_t extra) {
    __try {
        Memory read;
        return begin(read,base,settings,reinterpret_cast<Address>(inventory),reinterpret_cast<Address>(actor),
            handle,reinterpret_cast<Address>(amount),definition_handle,reinterpret_cast<Address>(source),slot_index,extra,context.proof);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}

// Native UseSkill retains this pointer only in its own stack frame. Each outer
// ItemUse invocation owns stable shadows, so recursion cannot overwrite one.
inline void* guarded_lookup(Context& context, Address caller, const std::uint32_t* handle, void* original) {
    __try {
        const auto& proof = context.proof;
        if (!context.valid || !handle || !original || !proof.item || proof.item->kind==Kind::Arrow ||
            caller!=proof.base+kSkillLookupReturn) return original;
        Memory read;
        const auto use = requested(proof,*handle);
        if (!use || !use->cost || use->type || use->info!=reinterpret_cast<Address>(original) || !fresh(read,proof)) return original;
        for (std::uint32_t i=0; i<context.shadow_count; ++i) {
            auto& shadow = context.shadows[i];
            if (shadow.handle!=*handle) continue;
            if (shadow.original!=use->info ||
                std::memcmp(shadow.original_info,original,sizeof(shadow.original_info)) ||
                std::memcmp(shadow.original_data,reinterpret_cast<const void*>(use->data),sizeof(shadow.original_data))) return original;
            return shadow.info;
        }
        if (context.shadow_count>=context.shadows.size()) return original;
        auto& shadow = context.shadows[context.shadow_count];
        std::memcpy(shadow.original_info,original,sizeof(shadow.original_info));
        std::memcpy(shadow.original_data,reinterpret_cast<const void*>(use->data),sizeof(shadow.original_data));
        std::memcpy(shadow.info,shadow.original_info,sizeof(shadow.info));
        std::memcpy(shadow.data,shadow.original_data,sizeof(shadow.data));
        // This flag solely appends the current self-item PopItemParameter.
        // Skill refs, skill levels, conditions and other native use flags stay stock.
        shadow.data[0x24]=0;
        const auto copied_data = reinterpret_cast<Address>(shadow.data);
        std::memcpy(shadow.info+0x18,&copied_data,sizeof(copied_data));
        shadow.original=use->info; shadow.handle=*handle;
        ++context.shadow_count;
        return shadow.info;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return original; }
}

inline bool guarded_arrow(Context& context, Address caller, void* actor,
    const RequestVector* input, std::uint8_t type, std::uint16_t reason, std::uint8_t flag) {
    __try {
        const auto& proof = context.proof;
        if (!context.valid || !proof.item || proof.item->kind!=Kind::Arrow ||
            caller!=proof.base+kArrowPopReturn || reinterpret_cast<Address>(actor)!=proof.actor ||
            !input || input->count!=1 || !input->data || type!=2 || reason!=8 || flag!=1) return false;
        // Only the separately scheduled DestroyOnly phase may have an empty
        // self-cost vector. Reserve-slot setup and other inventory callers run unchanged.
        const auto& destroy = proof.uses[2];
        if (destroy.type!=9 || !requested(proof,destroy.index)) return false;
        Memory read;
        const auto entry = reinterpret_cast<Address>(input->data);
        if (read.at<std::uint16_t>(entry)!=0xffff || read.at<std::uint64_t>(entry+8)!=proof.requested ||
            read.at<std::uint16_t>(entry+0x10)!=proof.inventory_handle ||
            read.at<std::uint16_t>(entry+0x12)!=proof.slot_index ||
            read.at<std::uint64_t>(entry+0x18)!=proof.extra ||
            read.at<std::uint32_t>(entry+0x22)!=0xffff0000) return false;
        return fresh(read,proof);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}

inline void* dispatch_lookup(Lookup original, Address caller, const std::uint32_t* handle) {
    // Lazy loading and the accessor's error behavior remain native.
    auto* result = original(handle);
    return current ? guarded_lookup(*current,caller,handle,result) : result;
}
inline void* dispatch_pop(Pop original, Address caller, void* transaction, void* result,
    void* actor, const RequestVector* input, std::uint64_t time, std::uint8_t type,
    std::uint16_t reason, std::uint8_t flag, std::uint64_t scratch) {
    if (current && guarded_arrow(*current,caller,actor,input,type,reason,flag)) {
        const RequestVector empty{};
        // Native prepare/validate/commit/finalize still run, with their real
        // result/error. No inventory writes or invented pop receipts occur.
        return original(transaction,result,actor,&empty,time,type,reason,flag,scratch);
    }
    return original(transaction,result,actor,input,time,type,reason,flag,scratch);
}
inline void* invoke_use(Use original, Context* previous, void* inventory, void* result,
    void* actor, void* target, std::uint16_t handle, void* amount,
    std::uint16_t definition_handle, void* source, std::uint16_t slot_index, std::uint64_t extra) {
    // __finally also restores the TLS chain when a native SEH exception leaves
    // the callback. This function has no C++ objects requiring unwinding.
    __try { return original(inventory,result,actor,target,handle,amount,definition_handle,source,slot_index,extra); }
    __finally { current=previous; }
}
inline void* dispatch_use(Use original, Address base, const Settings& settings,
    void* inventory, void* result, void* actor, void* target, std::uint16_t handle,
    void* amount, std::uint16_t definition_handle, void* source,
    std::uint16_t slot_index, std::uint64_t extra) {
    Context context{};
    context.valid=guarded_begin(context,base,settings,inventory,actor,handle,amount,definition_handle,source,slot_index,extra);
    auto* previous=current;
    // Push even invalid nested calls: they must never inherit an outer item.
    current=&context;
    return invoke_use(original,previous,inventory,result,actor,target,handle,amount,definition_handle,source,slot_index,extra);
}
} // namespace crimson::marketplace::consumables
