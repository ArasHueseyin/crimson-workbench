#pragma once
#include <cstdint>
#include <cstring>
#include <cstddef>

constexpr std::uint32_t kBlackstarKey=1000799;
constexpr std::int64_t kStaminaRegen=30000000;
constexpr std::int64_t kSummonDurationSeconds=1000*60;
template<class T> T read_at(const void* p,std::size_t off){T v;std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
template<class T> void write_at(void* p,std::size_t off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
struct GameArray{void* data;std::uint32_t count,capacity;};
static_assert(sizeof(GameArray)==16);
enum class RowResult{Other,Applied,AlreadyApplied,Rejected};

// Layout traced from this build's CharacterInfo/CharacterLevelData readers.
// Called on the completed reader result, before the record is published.
inline RowResult apply_row(void* row){
    if(!row || read_at<std::uint32_t>(row,0)!=kBlackstarKey)return RowResult::Other;
    auto levels=read_at<GameArray>(row,0x5c0);
    if(!levels.data||levels.count!=1||levels.capacity<1||levels.capacity>1024)return RowResult::Rejected;
    if(read_at<std::uint32_t>(levels.data,0)!=1)return RowResult::Rejected;
    auto stats=read_at<GameArray>(levels.data,0x60);
    if(!stats.data||stats.count!=20||stats.capacity<20||stats.capacity>1024)return RowResult::Rejected;
    auto stamina=static_cast<unsigned char*>(stats.data)+12*40;
    if(read_at<std::int64_t>(stamina,0)!=300000||read_at<std::int64_t>(stamina,8)!=0||
       read_at<std::int64_t>(stamina,24)!=100000||read_at<std::int64_t>(stamina,32)!=65000)return RowResult::Rejected;
    const auto cooldown=read_at<std::int64_t>(row,0x70),regen=read_at<std::int64_t>(stamina,16);
    const auto duration=read_at<std::int64_t>(row,0x78);
    if(cooldown==1&&regen==kStaminaRegen&&duration==kSummonDurationSeconds)return RowResult::AlreadyApplied;
    if(cooldown!=3600||regen!=20||duration!=600)return RowResult::Rejected;
    write_at<std::int64_t>(stamina,16,kStaminaRegen);
    write_at<std::int64_t>(row,0x70,1);
    write_at<std::int64_t>(row,0x78,kSummonDurationSeconds);
    return RowResult::Applied;
}

inline thread_local bool blackstar_summon=false;
struct SummonScope{
    bool previous;
    explicit SummonScope(bool eligible):previous(blackstar_summon){blackstar_summon=eligible;}
    ~SummonScope(){blackstar_summon=previous;}
};
inline bool bypass_region(std::uintptr_t caller,std::uintptr_t expected){return blackstar_summon&&caller==expected;}
