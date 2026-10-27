#include "../src/world/weather_native.h"
#include <cassert>
#include <cstring>
#include <limits>
#include <vector>
using namespace crimson::mac_items;
namespace {
struct Buffer {
    std::vector<std::uintptr_t> words;
    explicit Buffer(std::size_t size):words((size+7)/8){}
    std::uintptr_t address()const{return reinterpret_cast<std::uintptr_t>(words.data());}
    template<class T>void put(std::size_t offset,T value){std::memcpy(reinterpret_cast<std::byte*>(words.data())+offset,&value,sizeof(value));}
};
template<std::size_t Offset>void scalar(void* object,const float* value){std::memcpy(static_cast<std::byte*>(object)+Offset,value,4);}
unsigned manual_calls{},scalar_calls{};
void manual(void* object,const std::uint8_t* value){++manual_calls;std::memcpy(static_cast<std::byte*>(object)+0x31,value,1);}
void precipitation(void* object,const float* value){++scalar_calls;scalar<0x154>(object,value);}
}
int main(){
    Buffer service(0x50),parent(0x10),world(0xf70),manager(0x120),group(0x58),constant(0x1d8),atmosphere(0x158),job(0xa8);
    service.put(0x40,parent.address());parent.put(8,world.address());world.put(0xf68,manager.address());
    manager.put(0,std::uintptr_t{0x1081c3b98});manager.put(0xd0,world.address());manager.put(0x60,group.address()+0x28);
    group.put(0,std::uintptr_t{0x1081ca3c0});group.put(0x40,constant.address()+0x28);group.put(0x48,atmosphere.address()+0x28);
    constant.put(0,std::uintptr_t{0x1081ca690});constant.put(0x10,group.address());
    atmosphere.put(0,std::uintptr_t{0x1081c9e40});atmosphere.put(0x10,group.address());
    job.put(0,std::uintptr_t{0x108288ab0});job.put(0x90,world.address());
    const auto read=[&](std::uintptr_t at,std::uintptr_t& value){
        if(at==0x108ce55f0ULL){value=service.address();return true;}
        for(auto buffer:{&service,&parent,&world,&manager,&group,&constant,&atmosphere,&job}) {
            if(at>=buffer->address()&&at-buffer->address()<=buffer->words.size()*8-8){std::memcpy(&value,reinterpret_cast<void*>(at),8);return true;}
        }
        return false;
    };
    WeatherBindings bindings{manual,precipitation,scalar<0x158>,scalar<0x15c>,scalar<0x160>,scalar<0xc4>,scalar<0xd0>,0};
    auto frame=open_weather_frame(job.address(),0,123,read);
    assert(frame.ready&&!frame.view.manual&&frame.fingerprint);
    const auto desired=selected_weather({.3f,.7f,.5f,4,.01f,world::PrecipitationPhase::snow});
    assert(apply_weather(frame,bindings,desired,true,read)==WeatherPublication::published);
    assert(manual_calls==1&&scalar_calls==1);
    frame=open_weather_frame(job.address(),0,123,read);
    assert(frame.ready&&frame.view.manual&&frame.settings.same_control(desired)&&frame.settings.phase==-1);
    assert(hold_weather_phase(frame,bindings,5,read)==WeatherPublication::published);
    assert(manual_weather(frame,bindings,false,read)==WeatherPublication::published&&manual_calls==2);
    assert(apply_weather(frame,bindings,desired,false,read)==WeatherPublication::rejected&&scalar_calls==1);
    frame=open_weather_frame(job.address(),0,123,read);
    auto invalid=desired;invalid.wind=std::numeric_limits<float>::infinity();
    assert(apply_weather(frame,bindings,invalid,true,read)==WeatherPublication::rejected&&manual_calls==2);
    constant.put(0x10,manager.address());assert(!open_weather_frame(job.address(),0,123,read).ready);
    constant.put(0x10,group.address());job.put(0x90,group.address());
    assert(!open_weather_frame(job.address(),0,123,read).ready);
}
