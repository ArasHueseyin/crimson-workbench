# Black Star – Implementation of 01.10.2026

**Current diagnostic/invocation update installed, passed technical examinations; Unrestricted external summoning still unconfirmed.** After the city flight update, the user still reports approximately "mounts cannot be called here". The new version also detects Black Star during the previous summon action and captures the error code of the game message. The desired summoning time is 1,000 minutes. Installation and previous save changes took place when the game was closed.

## Behaviour

- **Practically unlimited endurance:** Only Schwarzstern's endurance regeneration was greatly increased. This does not necessarily prevent any visible short removal of the beam; sustained flight, acceleration and fire breath must be tested in the game.
- **Additional summon locations outside:** Black Star is recognized by its instance and character dataset. Region, navigation, voxel and platform locks are bypassed in the associated call. The recognition now also includes the normal and the reserved early client summon action, via precisely occupied character/instance calls. As a result, the navigation/voxel release also applies to later action checks in the same call. The final interior and placement test remains active. Others mounts keep their rules. Whether this extension fixes the reported lock must be checked in-game.
- **City Flight and Restricted Areas:** The separate rule `IsInTown() && !IsAboveRoad(Bird,20)` and the test `IsVehicleAllowedInEnteredRegion` are released for a black star riding character or black star itself. This covers the red areas described by the user as test cases for summoning and border crossing; Success in each of these areas is still at stake.
- **Height limit:** The separate test `CheckVehicleAllowableHeight` is released for black stars.
- **Cooldown one second:** Schwarzstern's own basic value was set from 3600 to 1. In addition, the remaining time already stored in the latest save has been reduced from 3,596,726 to 1,000 milliseconds.
- ** summoning time 1,000 minutes (16 hours 40 minutes):** The own basic value has been increased from 600 to 60,000 seconds. The already active summon in slot 1 also received 1,000 minutes of stored remaining time.

Automatic flight away after descent/landing remains its own mechanism. Invalid spawn positions, enclosed spaces, not yet loaded world areas and separate expiration/quest states can still set limits. The extension of the summoning period does not prevent other reasons for a premature end. “Any outside point” has not yet been proven in the game.

## Installation and dismantling

Additional file:

`C:\Program Files (x86)\Steam\steamapps\common\Crimson Desert\bin64\CrimsonBlackstar.asi`

SHA-256: `8e1b254874c27022f6746b01f0520b31ed3ee44447d2919dce15f090ed8a886f`, size 212,480 bytes.

The existing `winmm.dll` loader loads them at the next normal game start. The extension only accepts the tested EXE `1.0.0.2976` with SHA-256 `57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`. In the case of another version or different target bytes, it remains deactivated. The EXE on the hard drive and all existing mod archives have not been changed.

The runtime protocol is created next to the extension as `CrimsonBlackstar.log`:

- `READY: Blackstar early summon action support ...`: fourteen hooks and a condition slot installed, character data still await. Older READY lines remain in the existing log.
- `APPLIED: Blackstar stamina regeneration ...`: Black Star's fully loaded data detected and modified.
- `APPLIED: Blackstar summon region restriction bypass ...`: summon release was used for the first time.
- `APPLIED: Blackstar town/regional flight/flight height restriction bypass`: the new flight clearance was used for black stars for the first time.
- `APPLIED: Blackstar summon navigation/voxel/platform ...`: an additional summon release was used.
- `REFUSED: Blackstar initial/spawn validator result ...`: a remaining Zustands/placement check has declined. Examine location and visible game message along with this result.
- `REFUSED: Blackstar common/server validator ...`: the additional joint/server-side check has declined; Your response will only be logged.
- `ATTEMPT: Blackstar ... frame event returned ...`: Result of the previous summon action, maximum 24 lines per session.
- `GAME_ERROR: code=... caller=... BlackstarScope=...; stack: ...`: Error code and call path of the game message, maximum 64 different combinations per session. Other game messages may also be included. All match notifications will be forwarded unchanged; A message is never converted to success.
- `REFUSED` / `DISABLED`: unexpected data or game version; the corresponding amendment shall not apply.

For disassembly with **closed game** only remove `CrimsonBlackstar.asi` from `bin64` or place it outside the game folder. Keep existing sleep mods and `winmm.dll`. Already stored timers are retained; new incantations use the original basic values again. A complete old savebackup should not be copied over later earned progress. In addition, for the redemption of the renewal only, the previous ASI is in the new backup referred to below; an even active long timer would be separate and specifically adapted.

## Save and backup

The two previous Save Changes concerned the then most recent:

`%LOCALAPPDATA%\Pearl Abyss\CD\save\<Konto-ID>\slot1\save.save`

First change: `MercenaryClanSaveData._callMercenaryCoolTimeSaveList`, entry for black star (`_mercenaryNo = 1003255`), `_coolTimeLeft`.

Nine save files or accompanying files were backed up in advance under:

`<Projektordner>\.local\blackstar-mod-20261001\backups\20260930T231256Z`

HMAC and independent decoder after writing successfully. The complete semantic save comparison shows exactly the change of this scalar. Only the bodybytes `1927537`, `1927538`, `1927539` differ. All other bodybytes, field masks, reservation dates and the nonce vote. Abilities, inventory, quest progression, active summoning time, other slots and lobby files were not changed on this first change. The reverse exchange of the scalar gives the original body exactly.

**Subsequent extension to 1,000 minutes:** The field `MercenaryClanSaveData._callMercenarySpawnDurationSaveList._spawnEndDurationLeft` of the same black star entry (`_vehicleKey = 16984`) was set from 596,726 to 60,000,000 milliseconds. The full semantic comparison shows only this change; In the body, only the four bytes `1927589` to `1927592` differ. HMAC, independent reopening, inverse restoration of the original body and preservation of all remaining fields were tested. The one-second cooldown is maintained.

Freshness backup before renewal, including previous ASI and all nine save/accompany files:

`<Projektordner>\.local\blackstar-mod-20261001\duration-1000m\backups\20260930T232129Z`

Extension test and installation reports: `.local/blackstar-mod-20261001/duration-1000m/`. The slot 1 installed at that time had SHA-256 `068f7fe02c759261ddd9d2108ea375333c0443909c83952862d996c46a95bd93`. 17 protected files were verified unchanged after this installation.

**City/territorial flight update:** No save files have been changed here. All nine current save/accompany files, the previous ASI and the previous runtime log were freshly backed up at:

`<Projektordner>\.local\blackstar-mod-20261001\city-flight\backups\20260930T234228Z`

19 protected files were verified unchanged after exchange exclusively of the ASI. The latest save in this installation was **Slot 2**. Test and installation reports: `.local/blackstar-mod-20261001/city-flight/`.

**Early summoning action and diagnosis, 01.10.2026:** The previous runtime log occupies the loading of the city flight update and the executed city/region/altitude clearances, but does not contain a REFUSED line for the continued reported rejection. From this, the blocking test cannot yet be clearly determined. The previous client actions and the additional joint audit are now recorded. Special Territorial/Quest Conditions were not released in a flat-rate manner.

Freshness backup of the previous ASI, all nine save/accompanying files and the previous log:

`<Projektordner>\.local\blackstar-mod-20261001\summon-precheck\backups\20261001T184027Z`

Only the ASI has been replaced. All 19 protected files, including all saves, are verified unchanged. Evidence and installation report: `.local/blackstar-mod-20261001/summon-precheck/`. The exact residual cause and the success at the reported field offices remain open.

## Technical demonstration

The native `CharacterLevelData` reader and the `DataDefinedDefaultStatData` reader were executed as private code copies with the actual black star data. The reader consumes exactly the occupied level data set. Above `statusgroupinfo` (`1000001`) and `statusinfo` (`1000026`, `Stamina`) regeneration entry 12 is clearly assigned.

In the character record, the extension only changes after successful loading:

- Character dataset `_callMercenaryCoolTime`, runtime offset `0x70`: 3600 → 1.
- Character dataset `_callMercenarySpawnDuration`, runtime offset `0x78`: 600 → 60,000 seconds. The native reader reads eight bytes at this offset; 60,000,000 Milliseconds also fit into a signed 32 bit time value.
- Level data → Regeneration entry 12 → `_regenStat`: 20 → 30.000.000. Maximum 300,000, initial value 100,000 and all other properties remain identical.

An additional resolver hook covers already loaded data sets. All layout, number and initial value checks must be correct. The additional summon releases are only valid within a thread-local black star summon call. Nested calls to other mounts and other threads are not shared.

The new flight clearance reads the actor from the native condition context (`+0x828`), resolves the character index via the occupied component route and checks character key `1000799`. For a tab, the native mount lookup is used and its temporary reference is also released in the event of a reading error. The first summon checker uses the original instance lookup with the instance number as 64 bit value. The platform test shall only receive a private data copy with a shared flag at its occupied call point; Common gimmick data is not rewritten.

The elevation check uses an atomically replaced Vtable slot (`0x5949e48`) with the original function as a relapse. Your AVX function start could not be shifted in the test by MinHook. The slot solution is tested separately; Deviating pointers are not overwritten. All fourteen codehooks, helpers, selector calls and the original slot are checked before activation. If the installation fails, the codehooks are taken back.

Passed:

- Original level/statreaders on real data; Exact limits and endurance assignment.
- Byte comparison: change only the intended character/endurance fields; Health, maxima and other mounts remain unchanged.
- Ten cases with different data are rejected without write access, including incorrect duration and incompletely changed records.
- Repeated application, nested calls, incorrect call points, and thread separation.
- MinHook can correctly relocate the fourteen original functional beginnings used; Activate and remove the hooks in private copies successfully.
- Both early selection paths, nested actions, missing/foreign selection, other threads, and recovery tested by exception. The five original action arguments, return pointers and error output are retained. For other mounts, the original "no"/"unknown" answers remain.
- The error diagnosis is limited to 64 different combinations; Nevertheless, in the test, all 100 game reports are passed on with their original arguments and returns.
- The original city, region and elevation checks were carried out in private code copies: allowed, forbidden and unknown answers correctly assigned.
- The original instance lookup was recreated and executed with real memory structures: black star, other mount and missing instance correctly detected.
- Demarcation against Wyvern, unmounted players, other types of actors, other road parameters and foreign call points. Mount references are also compensated for a triggered reading error.
- Atomic condition slot: virtual execution, rejection of foreign pointer, restoration of the original and checked read-only memory.
- Seven shortened inputs are rejected by the original Statreader.
- Existing loader loads the new extension along with both sleep plugins; An unsupported EXE is rejected before hook installation.
- After installation, the checksums of the existing inventory archives, the EXE, the loader and both sleep mods are still correct.

These tests do not replace a run in the actual gaming process. In particular, “any external point” is thus not proven.

## Test now in the game

1. Start normally via Steam and load the latest save (**slot 2** at the time of this update). In the log, check the new `READY` line with `town/region/height`.
2. Black stars regularly leave or dismiss and summon again. The cooldown should have expired after about one second; Existing instances and ongoing animations remain protected.
3. Fly for several minutes, accelerate and repeatedly use fire breath. Endurance should practically not go out; Life points behave normally.
4. Invoke in a previously locked, free outdoor area. Afterwards check on the house, cave and indoor/outdoor threshold that closed rooms are still rejected.
5. Visit a red area from the screenshot: fly from the outside over the border, fly deep over a city area off a road and then re-create on open ground in the area. In the event of a refusal, record the location and the exact report; the new extension additionally logs the rejecting validator.
6. If the flight continues, check that the summoning continues after ten minutes. Storage and reloading should receive the extended remaining time; A new summoning should also be possible for longer than ten minutes. An actual run over full 1,000 minutes was not performed.
7. When outdoors are open, exceed the previous height limit and check that it no longer triggers discarding. Wyverns and ground mounts retain their previous rules.

Sources of the previous analysis: [Unlimited Dragon Flying](https://www.nexusmods.com/crimsondesert/mods/356?tab=description), [Flight Freedom](https://github.com/shin2344234/flight-freedom/tree/e8e53f8081a113fa1708632b883783cebe76a8a5). Implementation from local game data; Existing third party mod archives were not installed.

Source code, private fixtures and test logs: `.local/blackstar-mod-20261001/`. The results of the previous analysis are retained in `.local/blackstar-analysis-20261001/`.
