#pragma once
#include "blackstar_policy.h"

// The client frame event selects a mercenary before evaluating the action.
// Its verified CharacterInfo call identifies the requested mount even while
// the player is on foot; a mounted-actor test cannot do that.
struct FrameSummonScope;
inline thread_local FrameSummonScope* current_frame_summon=nullptr;
struct FrameSummonScope{
    FrameSummonScope* previous;
    bool previous_summon;
    std::uintptr_t selector_return;
    std::uintptr_t validator_return;
    std::uint32_t selected_key=0;
    FrameSummonScope(std::uintptr_t caller,std::uintptr_t validator):previous(current_frame_summon),
        previous_summon(blackstar_summon),selector_return(caller),validator_return(validator){
        current_frame_summon=this;blackstar_summon=false;
    }
    ~FrameSummonScope(){current_frame_summon=previous;blackstar_summon=previous_summon;}
    FrameSummonScope(const FrameSummonScope&)=delete;
    FrameSummonScope& operator=(const FrameSummonScope&)=delete;
};
inline void select_frame_summon(std::uintptr_t caller,std::uint32_t character_key){
    auto frame=current_frame_summon;
    if(!frame||(caller!=frame->selector_return&&caller!=frame->validator_return))return;
    frame->selected_key=character_key;
    blackstar_summon=character_key==kBlackstarKey;
}
