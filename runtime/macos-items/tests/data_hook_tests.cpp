#include "../src/data_hook.h"
#include <cassert>
using namespace crimson::mac_items;
__attribute__((noinline)) void executable_code(){asm volatile("");}
int main(){
    const auto code=reinterpret_cast<std::uintptr_t>(&executable_code)&~std::uintptr_t{7};
    std::uintptr_t instruction{};mach_vm_size_t copied{};
    assert(mach_vm_read_overwrite(mach_task_self(),code,8,reinterpret_cast<mach_vm_address_t>(&instruction),&copied)==KERN_SUCCESS&&copied==8);
    assert(!install_data_hook(code,instruction,123));
    mach_vm_address_t page{};
    assert(mach_vm_allocate(mach_task_self(),&page,vm_page_size,VM_FLAGS_ANYWHERE)==KERN_SUCCESS);
    auto slot=reinterpret_cast<std::uintptr_t*>(page);*slot=123;
    assert(mach_vm_protect(mach_task_self(),page,vm_page_size,false,VM_PROT_READ)==KERN_SUCCESS);
    assert(!install_data_hook(page,789,456)&&*slot==123);
    assert(!install_data_hook(page+1,123,456)&&*slot==123);
    assert(install_data_hook(page,123,456)&&*slot==456);
    mach_vm_address_t region=page;mach_vm_size_t size{};
    vm_region_basic_info_data_64_t info{};mach_msg_type_number_t count=VM_REGION_BASIC_INFO_COUNT_64;mach_port_t object{};
    assert(mach_vm_region(mach_task_self(),&region,&size,VM_REGION_BASIC_INFO_64,reinterpret_cast<vm_region_info_t>(&info),&count,&object)==KERN_SUCCESS);
    if(object)mach_port_deallocate(mach_task_self(),object);
    assert(info.protection==VM_PROT_READ);
    assert(install_data_hook(page,456,123)&&*slot==123);
    assert(mach_vm_deallocate(mach_task_self(),page,vm_page_size)==KERN_SUCCESS);
    assert(!install_data_hook(page,123,456));
}
