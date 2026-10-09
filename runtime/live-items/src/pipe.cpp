#include <Windows.h>
#include <sddl.h>
#include <string>
#include <vector>
#include "pipe.h"
#include "native.h"
namespace crimson::live_items {
namespace {
bool io(HANDLE pipe,bool writing,void* data,DWORD size) noexcept {
    OVERLAPPED op{};op.hEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!op.hEvent)return false;
    DWORD count{};bool ok=writing?WriteFile(pipe,data,size,&count,&op):ReadFile(pipe,data,size,&count,&op);
    if(!ok&&GetLastError()==ERROR_IO_PENDING) {
        if(WaitForSingleObject(op.hEvent,2000)!=WAIT_OBJECT_0)CancelIoEx(pipe,&op);
        ok=GetOverlappedResult(pipe,&op,&count,TRUE)!=0;
    }
    CloseHandle(op.hEvent);return ok&&count==size;
}
PSECURITY_DESCRIPTOR security() {
    HANDLE token{};if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))return nullptr;
    DWORD bytes{};GetTokenInformation(token,TokenUser,nullptr,0,&bytes);
    std::vector<unsigned char> buffer(bytes);bool ok=GetTokenInformation(token,TokenUser,buffer.data(),bytes,&bytes)!=0;CloseHandle(token);
    if(!ok)return nullptr;
    LPWSTR sid{};if(!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid,&sid))return nullptr;
    std::wstring dacl=L"D:P(A;;GA;;;";dacl+=sid;dacl+=L")";LocalFree(sid);
    PSECURITY_DESCRIPTOR sd{};if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(dacl.c_str(),SDDL_REVISION_1,&sd,nullptr))return nullptr;return sd;
}
}
void serve(Queue& queue,std::uintptr_t base) noexcept {
    try {
        auto sd=security();if(!sd)return;
        SECURITY_ATTRIBUTES sa{sizeof(sa),sd,FALSE};
        auto name=L"\\\\.\\pipe\\CrimsonWorkbench.Items.v2."+std::to_wstring(GetCurrentProcessId());
        // One local user, one bounded message, no remote clients or executable commands.
        HANDLE pipe=CreateNamedPipeW(name.c_str(),PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED|FILE_FLAG_FIRST_PIPE_INSTANCE,
            PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT|PIPE_REJECT_REMOTE_CLIENTS,1,48,40,0,&sa);
        LocalFree(sd);if(pipe==INVALID_HANDLE_VALUE)return;
        for(;;) {
            OVERLAPPED op{};op.hEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!op.hEvent)break;
            bool connected=ConnectNamedPipe(pipe,&op)!=0;
            if(!connected) {
                auto error=GetLastError();
                if(error==ERROR_PIPE_CONNECTED)connected=true;
                else if(error==ERROR_IO_PENDING){DWORD n{};WaitForSingleObject(op.hEvent,INFINITE);connected=GetOverlappedResult(pipe,&op,&n,TRUE)!=0;}
            }
            CloseHandle(op.hEvent);
            if(connected) {
                Request request{};
                if(io(pipe,false,&request,sizeof(request))){
                    auto now=GetTickCount64();
                    if(base)queue.confirm_selection(now,selected_identity(base));
                    auto reply=queue.handle(request,now);
                    // DisconnectNamedPipe discards unread output. Keep the
                    // reply available until the client ACKs, with a 2s bound.
                    if(io(pipe,true,&reply,sizeof(reply))){unsigned char ack{};io(pipe,false,&ack,1);}
                }
            }
            DisconnectNamedPipe(pipe);
        }
        CloseHandle(pipe);
    } catch(...) {}
}
}
