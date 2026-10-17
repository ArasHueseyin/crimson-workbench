# Borrowed native task observers

17 October 2026. Local infrastructure for the unfinished live world controls;
it does not itself enable time, weather or teleport actions.

LiveItems owns the existing build-2976 entry hook at RVA `2774b20`. Other ASIs
must not install another hook at that entry. Its two pinned native callers keep
the actor reference alive until the wrapper returns. The original wrapper and
existing item pump run first, then a bounded observer registry may synchronously
notify registered consumers before that reference is released.

The public process-local contract is
`runtime/live-items/src/task_observer.h`. Export
`RegisterWorkbenchTaskObserverV1` takes ABI version, observation byte size and a
C calling-convention callback. `TaskObservationV1` is a standard-layout 56-byte
structure containing the ABI header, executable base, borrowed task context and
native error pointer, original return address, monotonic time and freshly checked
selected-player identity. Reserved bytes must be zero.

Registration validates the version, size and callback, pins its containing module
before publication, and retains it until process exit. Four slots are available;
registration of the same callback is idempotent even under concurrent requests.
There is no unregister or module-unload operation. Consumers disable their own
policies while leaving the callback available for native passthrough. LiveItems
also pins itself before installing its hook.

Immediately before each callback, `task_player` checks the original caller,
successful task, server realm/context pool, server actor and inventory ownership,
possessor round trip and selected client identity. A preceding callback cannot
grant a later observer a stale validation. A TLS guard prevents nested observer
or item-pump dispatch during a callback. The registry stores callback addresses
only: no task, actor or field pointer survives the current synchronous call.

This is an execution opportunity, not a general permission to manipulate engine
objects. Each consumer must additionally acquire and validate its actual native
subsystem, world/stage and request generation. IPC handlers and readiness polling
never execute callbacks. Native exceptions are not swallowed or retried by
notification. RAII restores the reentrancy guard when an exception unwinds.

Owned fixtures cover ABI layout, concurrent idempotent registration, pin failure,
version/size/null/capacity refusal, fresh validation between callbacks, recursive
dispatch exclusion, exception unwinding and unchanged original-task arguments,
return value and error side effects. All three LiveItems CTest targets pass.
No running-game module, process, save or archive was changed for these checks.
