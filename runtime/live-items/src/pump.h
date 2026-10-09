#pragma once
#include "native.h"
namespace crimson::live_items {
inline thread_local bool pumping=false;
inline bool pump(Queue& queue,Bindings bindings,std::uintptr_t base,void* context,int* error,std::uintptr_t caller,std::uint64_t now) {
    if(pumping)return false;
    std::uint32_t identity{};
    auto holder=task_player(base,context,error,caller,&identity);if(!holder)return false;
    queue.observe(now,identity);
    auto request=queue.take(now,identity);if(!request)return true;
    pumping=true;auto outcome=execute(bindings,holder,*request);pumping=false;
    queue.finish(*request,outcome.state,outcome.error);return true;
}
}
