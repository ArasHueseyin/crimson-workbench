#pragma once
#include "control.h"
#include "../transport.h"
namespace crimson::mac_items {
template<class Selection>
bool start_teleport_transport(TeleportControl* control,Selection selection) {
    return start_socket_transport("teleport",[control,selection](int peer){
        uid_t uid{};gid_t gid{};int no_sigpipe=1;
        if(getpeereid(peer,&uid,&gid)!=0||uid!=geteuid()
            ||setsockopt(peer,SOL_SOCKET,SO_NOSIGPIPE,&no_sigpipe,sizeof(no_sigpipe))!=0)return;
        teleport::Request request;if(!receive_all(peer,&request,sizeof(request)))return;
        auto reply=control->handle(request,now_ms(),selection());transfer_all(peer,&reply,sizeof(reply),true);
    });
}
}
