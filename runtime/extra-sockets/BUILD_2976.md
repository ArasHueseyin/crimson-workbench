# extra sockets for three items – Build 1.0.0.2976

## Correction and general selection

Version 2 supports up to 256 individual equipment instances; Existing version 1 with the three UIDs mentioned below remains readable. Workbench reads its own instances from inventory/equipment/registered characters of a selected save, checks individually stored items, five padding entries, 0–5 open base base and empty locked padding. An object is extended once by ten additional seats. saved games and ItemInfo remain unchanged. The new configuration is not bound to the three old UIDs.

The first implementation missed Runtime→InitData RVA `0x240cac0`. This routine has five inline six-byte entries at `+0x40`; the sixth overwrites the counter at `+0x5e`. Therefore, in the client prefix 255 and a corrupted display was created. This converter also now receives a borrowed normalized item copy. Subsequent client construction loads the same ten extras. The mod checks the LiveItems peer and waits for its activated taskhook before redirecting the converter pre-certified by LiveItems. All three hooks are activated together. The existing production path retains its tests.

Isolated checked: the complete original routine, all base sockets 0–5, empty/stocked extras and deviating prefix values 0/255 with memory canaries, unchanged sets/flags/refining/durability and subsequent client extension step. The new real in-game test remains necessary.

Ten additional native bases: Frost Curse Plate Tank UID 1001416 (total 10), Shadow Helmet UID 1003059 (11), Electric Mecha Long Sword UID 1003664 (15). No global ItemInfo patching; No change to existing stones or refinement.

The ASI extends the engine's own six-byte base vector after the original construction. The following save loading path uses the already dynamic native copy routine. Original five base padding entries and open base base remain in the save. When saved, the original native write routine gets a borrowed full item copy with unchanged remaining content and five base entries. The running object remains unchanged. Ten extra sockets per item are stored separately in `bin64/CrimsonExtraSockets.dat` (SHA-256, exact UIDs/keys, ten entries, validated Abyss stones). This leaves the normal save readable without this mod.

Workbench → **extra sockets** allows selection and direct assembly. The normal witch/tooltip interface shall continue to have a maximum of five lines; The tank is normally not pedestrian. Changes in Workbench require a closed game, current file revision and create a backup. storage messages are diagnostic; Actual effect and loading/storing shall be checked in-game.

Build: CMake, Visual Studio 2022 x64, static MSVC runtime, unchanged MinHook 1.3.4 over `-DMINHOOK_ROOT=…`. Pins and allowed stone keys are generated for the SHA-tested EXE 2976. Foreign EXEs, deviant functions and damaged configuration disable the mod before the hooks. ASI hash in `cd-core/src/extra_sockets.rs` must correspond to the actual output when released.

Private native routine/policy/codec/loader checks and installation credentials: `.local/extra-sockets-plus10-20261004/`. Previous pure ten entry save samples shall not be installed.
