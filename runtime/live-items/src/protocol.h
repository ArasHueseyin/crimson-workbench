#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <optional>
#include <vector>

namespace crimson::live_items {
constexpr std::uint32_t wire_magic=0x49574443;
constexpr std::uint16_t wire_version=2;
constexpr std::uint64_t context_fresh_ms=5000;
enum class Operation : std::uint16_t { status=1, grant=2, receipt=3, cancel=4 };
enum class State : std::uint16_t { offline, ready, queued, executing, applied, rejected, uncertain, expired, cancelled, starting };
enum class Error : std::uint32_t { none, protocol, invalid_request, busy, unknown_request, unavailable, unsupported_item, quantity_limit, context, native_fault, native_error, inventory_mismatch, history_full, definition_mismatch, socket_limit, conversion_mismatch };
#pragma pack(push,1)
struct Request {
    std::uint32_t magic{wire_magic}; std::uint16_t version{wire_version}; Operation operation{Operation::status};
    std::array<std::uint8_t,16> id{}; std::uint32_t key{},quantity{};
    std::uint64_t epoch{};
};
struct Reply {
    std::uint32_t magic{wire_magic}; std::uint16_t version{wire_version}; State state{State::offline};
    std::uint32_t pid{}; Error error{}; std::array<std::uint8_t,16> id{};
    std::uint32_t key{},quantity{}; std::uint64_t epoch{};
};
#pragma pack(pop)
static_assert(sizeof(Request)==40&&sizeof(Reply)==48);
inline bool valid_id(const Request& r) {for(auto b:r.id)if(b)return true;return false;}
// No engine pointer crosses the queue. Never evict an ID and risk replaying it.
class Queue {
    struct Entry { Request request; Reply reply; std::uint64_t deadline; std::uint32_t context; };
    std::mutex mutex_; std::vector<Entry> entries_;
    bool observed_{},faulted_{}; std::uint32_t pid_; std::uint64_t epoch_;
    std::uint64_t last_seen_{};std::uint32_t context_{};
    Reply answer(const Request& r, State s,Error e=Error::none)const {
        return {wire_magic,wire_version,s,pid_,e,r.id,r.key,r.quantity,epoch_};
    }
    void expire(std::uint64_t now) {for(auto& e:entries_)if(e.reply.state==State::queued&&now>=e.deadline)e.reply.state=State::expired;}
    bool fresh(std::uint64_t now)const{return observed_&&now>=last_seen_&&now-last_seen_<=context_fresh_ms;}
public:
    Queue(std::uint32_t pid,std::uint64_t epoch):pid_(pid),epoch_(epoch){}
    void confirm_selection(std::uint64_t now,std::uint32_t selection) {
        std::lock_guard lock(mutex_);expire(now);
        if(observed_&&selection&&selection==context_){last_seen_=now;return;}
        // Readiness can survive pause/Alt-Tab only while the already confirmed
        // character is still selected. This method never observes a new owner.
        for(auto& e:entries_)if(e.reply.state==State::queued){e.reply.state=State::rejected;e.reply.error=Error::context;}
        observed_=false;context_=0;last_seen_=now;
    }
    void observe(std::uint64_t now,std::uint32_t context=1) {
        std::lock_guard lock(mutex_);expire(now);
        if(!context)return;
        // A changed character generation or a long context gap must not send
        // an old queued request into the newly loaded inventory.
        if(observed_&&(context_!=context||!fresh(now)))for(auto& e:entries_)if(e.reply.state==State::queued){e.reply.state=State::rejected;e.reply.error=Error::context;}
        observed_=true;last_seen_=now;context_=context;
    }
    Reply handle(const Request& r,std::uint64_t now) {
        std::lock_guard lock(mutex_);expire(now);
        if(r.magic!=wire_magic||r.version!=wire_version)return answer(r,State::rejected,Error::protocol);
        if(r.operation==Operation::status)return answer(r,faulted_?State::uncertain:fresh(now)?State::ready:State::starting);
        if(!valid_id(r))return answer(r,State::rejected,Error::invalid_request);
        if(r.epoch!=epoch_)return answer(r,State::rejected,Error::unavailable);
        for(auto& e:entries_)if(e.request.id==r.id) {
            if(r.operation==Operation::grant&&(r.key!=e.request.key||r.quantity!=e.request.quantity))return answer(r,State::rejected,Error::invalid_request);
            if(r.operation==Operation::cancel&&e.reply.state==State::queued)e.reply.state=State::cancelled;
            return e.reply;
        }
        if(r.operation!=Operation::grant)return answer(r,State::rejected,Error::unknown_request);
        if(!r.key||!r.quantity||r.quantity>10000)return answer(r,State::rejected,Error::invalid_request);
        if(!fresh(now)||faulted_)return answer(r,State::rejected,Error::unavailable);
        for(auto& e:entries_)if(e.reply.state==State::queued||e.reply.state==State::executing)return answer(r,State::rejected,Error::busy);
        if(entries_.size()>=512)return answer(r,State::rejected,Error::history_full);
        auto reply=answer(r,State::queued);entries_.push_back({r,reply,now+30000,context_});return reply;
    }
    std::optional<Request> take(std::uint64_t now,std::uint32_t context=1) {
        std::lock_guard lock(mutex_);expire(now);if(faulted_||!fresh(now)||context_!=context)return {};
        for(auto& e:entries_)if(e.reply.state==State::queued&&e.context==context){e.reply.state=State::executing;return e.request;}return {};
    }
    void finish(const Request& r,State s,Error error) {
        std::lock_guard lock(mutex_);
        for(auto& e:entries_)if(e.request.id==r.id&&e.reply.state==State::executing){e.reply.state=s;e.reply.error=error;if(s==State::uncertain)faulted_=true;return;}
    }
};
}
