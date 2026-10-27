#pragma once
#include <atomic>
#include <cstdint>
#include <mach/mach.h>
#include <mach/mach_vm.h>

namespace crimson::mac_items {
inline bool install_data_hook(std::uintptr_t address,std::uintptr_t expected,std::uintptr_t replacement) {
    if(!address||address%alignof(std::uintptr_t)||!expected||!replacement)return false;
    std::uintptr_t current{};mach_vm_size_t size{};
    if(mach_vm_read_overwrite(mach_task_self(),address,sizeof(current),reinterpret_cast<mach_vm_address_t>(&current),&size)!=KERN_SUCCESS
        ||size!=sizeof(current)||current!=expected)return false;
    const auto page=address&~(static_cast<std::uintptr_t>(vm_page_size)-1);
    mach_vm_address_t region=page;mach_vm_size_t region_size{};
    vm_region_basic_info_data_64_t info{};mach_msg_type_number_t count=VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t object=MACH_PORT_NULL;
    const auto query=mach_vm_region(mach_task_self(),&region,&region_size,VM_REGION_BASIC_INFO_64,
        reinterpret_cast<vm_region_info_t>(&info),&count,&object);
    if(object!=MACH_PORT_NULL)mach_port_deallocate(mach_task_self(),object);
    if(query!=KERN_SUCCESS||region>page||page-region>=region_size
        ||!(info.protection&VM_PROT_READ)||(info.protection&VM_PROT_EXECUTE))return false;
    if(mach_vm_protect(mach_task_self(),page,vm_page_size,false,VM_PROT_READ|VM_PROT_WRITE|VM_PROT_COPY)!=KERN_SUCCESS)return false;
    auto slot=std::atomic_ref<std::uintptr_t>(*reinterpret_cast<std::uintptr_t*>(address));
    current=expected;const bool installed=slot.compare_exchange_strong(current,replacement,std::memory_order_acq_rel);
    if(mach_vm_protect(mach_task_self(),page,vm_page_size,false,info.protection)==KERN_SUCCESS)return installed;
    if(installed){current=replacement;slot.compare_exchange_strong(current,expected,std::memory_order_acq_rel);}
    mach_vm_protect(mach_task_self(),page,vm_page_size,false,info.protection);
    return false;
}
}
