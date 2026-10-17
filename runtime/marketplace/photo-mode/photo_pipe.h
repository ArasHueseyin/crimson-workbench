#pragma once
#include "photo_protocol.h"
namespace crimson::marketplace::photo {
inline constexpr wchar_t kPipePrefix[]=L"\\\\.\\pipe\\CrimsonWorkbench.Photo.v1.";
using Handler=Reply(*)(const Request&,std::uint64_t);
// Pointer-free enqueue/status only. No native game functions on this worker.
void serve_photo_pipe(Handler handler) noexcept;
}
