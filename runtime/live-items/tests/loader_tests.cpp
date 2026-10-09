#include <Windows.h>
#include <mmsystem.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char**) {
    const auto tick=timeGetTime();
    for(unsigned i=0;i<250;++i){
        auto probe=GetModuleHandleW(L"CrimsonLiveItemsProbe.asi");
        auto live=GetModuleHandleW(L"CrimsonLiveItems.asi");
        bool loaded=probe&&GetProcAddress(probe,"InitializeASI")&&live&&GetProcAddress(live,"InitializeASI");
        for(auto name:{L"CrimsonBlackstar.asi",L"CrimsonSleepCooldown.asi",L"CrimsonSleepDuration.asi",L"CrimsonNoFall.asi"})loaded=loaded&&GetModuleHandleW(name);
        if(loaded){
            if(argc>1){
                wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);
                std::ifstream f(std::filesystem::path(path).parent_path()/L"CrimsonLiveItemsProbe.log");
                std::string log((std::istreambuf_iterator<char>(f)),{});
                if(log.find("DISABLED: unknown executable or inventory routine; no hook.")==std::string::npos){Sleep(20);continue;}
                if(log.find("READY:")!=std::string::npos)return 2;
                std::ifstream live_file(std::filesystem::path(path).parent_path()/L"CrimsonLiveItems.log");
                std::string live_log((std::istreambuf_iterator<char>(live_file)),{});
                if(live_log.find("DISABLED: unsupported executable.")==std::string::npos){Sleep(20);continue;}
                if(live_log.find("ARMED:")!=std::string::npos)return 3;
            }
            std::cout<<"PASS: winmm forwarding, all six ASIs loaded, InitializeASI exported"<<(argc>1?", unsupported executable refused before hook":"")<<"; tick="<<tick<<"\n";return 0;
        }
        Sleep(20);
    }
    return 1;
}
