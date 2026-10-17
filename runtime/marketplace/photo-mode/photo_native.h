#pragma once
#include <Windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <cmath>
#include "photo_protocol.h"
namespace crimson::marketplace::photo {
using Address=std::uintptr_t;
using FrameUpdate=void(__fastcall*)(void*);
using Lookup=void*(__fastcall*)(void*,const char*);
using GetView=void*(__fastcall*)(void*);
using TimeScale=float(__fastcall*)(void*);
using Enter=void(__fastcall*)(void*,bool);
using UiAction=void(__fastcall*)(void*);
using Input=void(__fastcall*)(void*,void*,void*,bool,const void*);
struct Bindings {Lookup lookup{};GetView get_view{};TimeScale time_scale{};Enter enter{};UiAction hide{},show{};Input input{};};
template<class T> inline T read(const void* object,std::size_t offset) {
    T value{};std::memcpy(&value,static_cast<const unsigned char*>(object)+offset,sizeof(T));return value;
}
// All pointers in Frame are borrowed only for one leased UI callback. The
// policy retains token alone, which cannot be used to dereference game memory.
struct Frame {
    void* view{};void* control{};void* close_widget{};
    std::uint64_t token{};bool photo{},hidden{},frozen{},camera_busy{},controls{};
};
class Native {
    Address base_{};Bindings b_{};
    bool view_readable(void* view) const noexcept {
        __try {
            return view&&read<std::uint8_t>(view,0x312c4)==0&&read<void*>(view,0x31298)&&
                read<std::uint8_t>(view,0x31339)&&!read<std::uint8_t>(view,0x3133d)&&
                (!read<std::uint32_t>(view,0x30ec0)||read<void*>(view,0x30eb8));
        } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    bool locate(void* entity,void*& control,void*& body) const noexcept {
        __try {
            if(!entity)return false;
            auto* meta=read<void*>(entity,0xa0);if(!meta)return false;
            auto* panel=read<void*>(meta,0x10);if(!panel)return false;
            auto* instances=read<void*>(panel,0x120);if(!instances)return false;
            auto* override=read<void*>(instances,0xa0);
            // The native request has a separate override/multiple-instance
            // route. Restrict this adapter to its ordinary single instance.
            if(override&&(read<void*>(override,8)||read<void*>(override,0x58)))return false;
            if(read<std::uint32_t>(instances,0x48)!=1)return false;
            auto* entries=read<void*>(instances,0x40);if(!entries)return false;
            auto* instance=read<void*>(entries,0);if(!instance)return false;
            auto* info=read<void*>(instance,0xa0);
            if(!info||read<void*>(info,8))return false;
            control=read<void*>(instance,0xd8);body=read<void*>(info,0x10);
            return control&&body&&read<Address>(control,0)==base_+0x56f1968&&
                read<void*>(control,8)==instance&&read<std::uint8_t>(control,0x20)==0&&
                read<void*>(body,0x118)==control&&read<float>(body,0x23c)==0.0f;
        } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    bool details(Frame& f) const noexcept {
        __try {
            auto* root=read<void*>(reinterpret_cast<void*>(base_),0x6d691e8);
            auto* manager=root?read<void*>(root,0x50):nullptr;
            auto* phase=root?read<void*>(root,0x98):nullptr;
            // ad0fb0 builds this aggregate, not a Frame+18 alias:
            // +58=Frame+60 delta state, +68=[Frame+18]+88 UI registry.
            auto* delta=root?read<void*>(root,0x58):nullptr;
            auto* registry=root?read<void*>(root,0x68):nullptr;
            if(!manager||!phase||!delta||!registry||read<Address>(manager,0)!=base_+0x55ca3b8)return false;
            auto* views=read<void*>(registry,0x78);
            if(!views||!read<std::uint32_t>(registry,0x80)||read<void*>(views,0)!=f.view)return false;
            auto* camera=read<void*>(manager,0x10);
            if(!camera||read<Address>(camera,0)!=base_+0x55ca780)return false;
            const auto count=read<std::uint32_t>(manager,0x28);
            if(count>3||count>read<std::uint32_t>(manager,0x2c))return false;
            auto* entries=read<void*>(manager,0x20);if(count&&!entries)return false;
            bool photo=false;
            for(std::uint32_t i=0;i<count;++i) {
                auto* entry=read<void*>(entries,static_cast<std::size_t>(i)*8);if(!entry)return false;
                const auto mode=read<std::uint8_t>(entry,0);
                if(mode>2)return false;
                if(mode==1) {
                    if(photo||read<void*>(entry,8)!=camera)return false;photo=true;
                    f.photo=i+1==count;
                }
                if(mode==2)f.camera_busy=true;
            }
            // A covered PhotoCamera is foreign to this narrow normal route.
            if(photo&&!f.photo)f.camera_busy=true;
            const auto scaled_delta=read<float>(delta,0x64),raw_delta=read<float>(delta,0x6c);
            if(!std::isfinite(scaled_delta)||!std::isfinite(raw_delta))return false;
            f.frozen=f.photo&&scaled_delta==0.0f&&raw_delta>0.0f;
            // Mix addresses to detect context replacement without retaining a
            // dereferenceable borrowed pointer in the policy or IPC reply.
            auto mix=[](std::uint64_t x){x^=x>>30;x*=0xbf58476d1ce4e5b9ULL;x^=x>>27;x*=0x94d049bb133111ebULL;return x^(x>>31);};
            f.token=0x6a09e667f3bcc909ULL;
            const std::array<Address,7> members{reinterpret_cast<Address>(f.view),reinterpret_cast<Address>(manager),
                reinterpret_cast<Address>(phase),reinterpret_cast<Address>(camera),reinterpret_cast<Address>(root),
                reinterpret_cast<Address>(delta),reinterpret_cast<Address>(registry)};
            for(auto member:members)f.token=mix(f.token^mix(member));
            if(!f.token)f.token=1;
            return true;
        } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    bool controls(Frame& f,void* body) const noexcept {
        __try {
            f.hidden=read<std::uint8_t>(f.control,0x218)!=0;
            f.close_widget=read<void*>(f.control,0x170);
            return f.close_widget&&read<void*>(f.control,0x168)&&read<void*>(f.control,0x200)&&
                (!f.photo||read<std::uint8_t>(body,0x26f)<2);
        } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
public:
    Native(Address base,Bindings bindings):base_(base),b_(bindings){}
    bool snapshot(void* view,Frame& f) const {
        f={};if(!view_readable(view))return false;
        // Native calls are deliberately outside SEH. Exceptions from the
        // engine must propagate; only our own guarded reads fail closed.
        f.view=view;if(!details(f))return false;
        auto* entity=b_.lookup(view,reinterpret_cast<const char*>(base_+0x56f13d0));
        void* body{};
        if(locate(entity,f.control,body)&&b_.get_view(f.control)==view&&controls(f,body))f.controls=true;
        else {f.control=nullptr;f.close_widget=nullptr;f.hidden=false;}
        const auto scale=b_.time_scale(view);
        f.frozen=f.frozen&&scale==0.0f;
        return std::isfinite(scale);
    }
    void enter(const Frame& f) const {b_.enter(f.view,true);}
    void hide(const Frame& f) const {b_.hide(f.control);}
    void show(const Frame& f) const {b_.show(f.control);}
    void leave(const Frame& f) const {b_.input(f.control,nullptr,f.close_widget,false,nullptr);}
};
inline bool leased_caller(Address base,Address caller) noexcept {return base&&caller==base+0xad2502;}
}
