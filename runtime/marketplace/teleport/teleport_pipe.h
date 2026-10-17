#pragma once
#include "teleport_protocol.h"
namespace crimson::marketplace::teleport {
inline constexpr wchar_t kPipePrefix[]=L"\\\\.\\pipe\\CrimsonWorkbench.Teleport.v1.";
using Handler=Reply(*)(const Request&,std::uint64_t);
void serve_teleport_pipe(Handler handler) noexcept;
}
