#pragma once
#include "control_protocol.h"

namespace crimson::marketplace::world_controls {
inline constexpr wchar_t kControlPipePrefix[]=L"\\\\.\\pipe\\CrimsonWorkbench.World.v1.";
// Run on an owned background worker. The handler may inspect/enqueue pointer-
// free control data only. It must never call native game functions here.
void serve_control_pipe(ControlHandler handler) noexcept;
} // namespace crimson::marketplace::world_controls
