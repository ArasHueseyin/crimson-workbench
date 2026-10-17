#include <Windows.h>
#include <sddl.h>
#include <memory>
#include <string>
#include <vector>
#include "control_pipe.h"

namespace crimson::marketplace::world_controls {
namespace {
struct Handle {
    HANDLE value{};
    ~Handle(){if(value && value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
    Handle(const Handle&)=delete;
    Handle& operator=(const Handle&)=delete;
    explicit Handle(HANDLE initial=nullptr):value(initial){}
};
struct LocalDeleter {void operator()(void* value) const noexcept {if(value)LocalFree(value);}};
using LocalMemory=std::unique_ptr<void,LocalDeleter>;
bool io(HANDLE pipe,bool writing,void* buffer,DWORD bytes) noexcept {
    OVERLAPPED operation{};
    operation.hEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    if(!operation.hEvent)return false;
    DWORD transferred{};
    bool complete=(writing?WriteFile(pipe,buffer,bytes,&transferred,&operation):
        ReadFile(pipe,buffer,bytes,&transferred,&operation))!=FALSE;
    if(!complete && GetLastError()==ERROR_IO_PENDING) {
        if(WaitForSingleObject(operation.hEvent,2000)!=WAIT_OBJECT_0)CancelIoEx(pipe,&operation);
        complete=GetOverlappedResult(pipe,&operation,&transferred,TRUE)!=FALSE;
    }
    CloseHandle(operation.hEvent);
    return complete && transferred==bytes;
}
PSECURITY_DESCRIPTOR user_security() {
    Handle token;
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token.value))return nullptr;
    DWORD bytes{};
    GetTokenInformation(token.value,TokenUser,nullptr,0,&bytes);
    if(!bytes || bytes>1024*1024)return nullptr;
    std::vector<unsigned char> storage(bytes);
    const bool read=GetTokenInformation(token.value,TokenUser,storage.data(),bytes,&bytes)!=FALSE;
    if(!read)return nullptr;
    LPWSTR sid{};
    if(!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(storage.data())->User.Sid,&sid))return nullptr;
    LocalMemory sid_owner(sid);
    std::wstring access=L"D:P(A;;GA;;;";
    access+=sid;access+=L")";
    PSECURITY_DESCRIPTOR security{};
    if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(access.c_str(),SDDL_REVISION_1,&security,nullptr))return nullptr;
    return security;
}
}

void serve_control_pipe(ControlHandler handler) noexcept {
    if(!handler)return;
    try {
        LocalMemory descriptor(user_security());
        if(!descriptor)return;
        SECURITY_ATTRIBUTES security{sizeof(security),descriptor.get(),FALSE};
        const auto name=std::wstring(kControlPipePrefix)+std::to_wstring(GetCurrentProcessId());
        Handle pipe_owner(CreateNamedPipeW(name.c_str(),PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED|FILE_FLAG_FIRST_PIPE_INSTANCE,
            PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT|PIPE_REJECT_REMOTE_CLIENTS,
            1,sizeof(Reply),sizeof(Request),0,&security));
        const auto pipe=pipe_owner.value;
        if(pipe==INVALID_HANDLE_VALUE)return;
        for(;;) {
            OVERLAPPED operation{};
            operation.hEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
            if(!operation.hEvent)break;
            bool connected=ConnectNamedPipe(pipe,&operation)!=FALSE;
            if(!connected) {
                const auto error=GetLastError();
                if(error==ERROR_PIPE_CONNECTED)connected=true;
                else if(error==ERROR_IO_PENDING) {
                    DWORD transferred{};
                    WaitForSingleObject(operation.hEvent,INFINITE);
                    connected=GetOverlappedResult(pipe,&operation,&transferred,TRUE)!=FALSE;
                }
            }
            CloseHandle(operation.hEvent);
            if(connected) {
                Request request{};
                // Message mode rejects oversized messages (ERROR_MORE_DATA) as
                // well as truncated frames. Exactly one request per connection.
                if(io(pipe,false,&request,sizeof(request))) {
                    try {
                        auto reply=handler(request,GetTickCount64());
                        if(reply.magic==kControlMagic && reply.version==kControlVersion && reply.bytes==sizeof(Reply) &&
                            io(pipe,true,&reply,sizeof(reply))) {
                            // DisconnectNamedPipe discards unread output. The
                            // bounded client ACK proves the reply was consumed.
                            unsigned char acknowledgement{};
                            io(pipe,false,&acknowledgement,1);
                        }
                    } catch(...) {
                        // No automatic resubmission. The client can query the
                        // same ID after a lost/uncertain transport response.
                    }
                }
                DisconnectNamedPipe(pipe);
            }
        }
    } catch(...) {}
}
} // namespace crimson::marketplace::world_controls
