#pragma once
#include <cstdint>
namespace crimson::marketplace::teleport {
// A failed revision remains dirty and is not retried by status polling. A new
// explicit capture/remove creates another revision and authorizes one attempt.
class PersistenceRevision {
    std::uint64_t attempted_{};std::uint32_t status_{1};
public:
    template<class Save>std::uint32_t persist(std::uint64_t revision,Save&& save) {
        if(!revision||revision==attempted_)return status_;
        attempted_=revision;status_=2;if(save())status_=1;return status_;
    }
};
}
