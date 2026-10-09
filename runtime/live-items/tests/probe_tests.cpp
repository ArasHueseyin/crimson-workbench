#undef NDEBUG
#include "../src/observe.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
namespace {
unsigned calls{},records{};void* expected_holder{};int* expected_error{};
void* __fastcall original(void* holder,int* error) {++calls;assert(holder==expected_holder&&error==expected_error);*error=71;return error;}
void record(crimson::live_items::Observation) {++records;}
}
int main(){
    std::array<unsigned char,0x200> holder{},owner{},metadata{},component{};
    *reinterpret_cast<void**>(holder.data()+8)=owner.data();
    *reinterpret_cast<void**>(owner.data()+0x88)=metadata.data();
    *reinterpret_cast<void**>(owner.data()+0x68)=component.data();
    *reinterpret_cast<void**>(component.data()+0xb8)=holder.data();
    metadata[1]=1;owner[0x96]=1;
    assert(crimson::live_items::observe(holder.data()).player_holder);
    auto before=holder;auto before_owner=owner;auto before_metadata=metadata;auto before_component=component;
    int error=0;expected_holder=holder.data();expected_error=&error;
    assert(crimson::live_items::forward(original,record,holder.data(),&error)==&error);
    assert(calls==1&&records==1&&error==71&&holder==before&&owner==before_owner&&metadata==before_metadata&&component==before_component);
    metadata[1]=2;assert(!crimson::live_items::observe(holder.data()).player_holder);
    metadata[1]=1;owner[0x96]=0;assert(!crimson::live_items::observe(holder.data()).player_holder);
    owner[0x96]=1;*reinterpret_cast<void**>(component.data()+0xb8)=nullptr;assert(!crimson::live_items::observe(holder.data()).player_holder);
    assert(!crimson::live_items::observe(nullptr).readable);
    assert(!crimson::live_items::observe(reinterpret_cast<void*>(1)).readable);
    expected_holder=nullptr;assert(crimson::live_items::forward(original,record,nullptr,&error)==&error);
    assert(calls==2&&records==2);
    std::puts("PASS: player/nonplayer/dead/mismatched/null/unreadable capture; original forwards exactly once; no observed bytes changed.");
}
