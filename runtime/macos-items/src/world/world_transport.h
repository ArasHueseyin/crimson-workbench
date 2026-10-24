#pragma once
#include "world_control.h"
#include "../transport.h"

namespace crimson::mac_items {
template<class Selection>
void serve_world_peer(int peer,WorldControl& control,Selection selection) {
    uid_t uid{};gid_t gid{};int no_sigpipe=1;
    if(getpeereid(peer,&uid,&gid)!=0||uid!=geteuid()
        ||setsockopt(peer,SOL_SOCKET,SO_NOSIGPIPE,&no_sigpipe,sizeof(no_sigpipe))!=0)return;
    world::Request request;
    if(!receive_all(peer,&request,sizeof(request)))return;
    auto reply=control.handle(request,now_ms(),selection());
    transfer_all(peer,&reply,sizeof(reply),true);
}
template<class Selection>
bool start_world_transport(WorldControl* control,Selection selection) {
    return start_socket_transport("world",[control,selection](int peer){serve_world_peer(peer,*control,selection);});
}
}
