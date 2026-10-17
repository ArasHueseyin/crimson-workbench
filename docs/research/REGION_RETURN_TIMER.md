# Restricted-region return timer, build 2976

The requested Marketplace setting changes the restricted-region return warning
from 30 seconds to 30 minutes. It uses an archive overlay and requires the game
to be closed when applied. Development did not modify the running installation.

## Identified source and execution path

Four original scripts in archive group `0014`, under
`sequencer/binary__/stageseq/system`, define the restricted desert, snow, water
and general-region transitions. Their names are
`cd_seq_quest_getout_region_0100` through `0400`.

The corresponding source graph identifies a `branch_warning` transition to
`SCENE`, with message key `UI_Forbidden_Region` and a 30-second duration.
The compiled `.pastage` files contain the duration as a 64-bit millisecond value,
30,000. The recipe changes that value to 1,800,000. It preserves all conditions,
exit events, destination references and the return scene.

Static inspection of the current executable's transition evaluator (RVA
`0x5dd3b0`) shows a shared duration used in the elapsed-time check and the warning
event. The UI receiver at `0xe83250` stores the start time and duration; its
update at `0xe83490` computes remaining seconds from those values. Editing the
compiled transition therefore targets the logic duration as well as its warning,
rather than only replacing displayed text. These addresses are research
evidence, not runtime hooks installed by this recipe.

The `TrocTrMoveFieldOnceTimer` type was investigated separately. A nearby literal
`0x65` is a string-buffer size passed to `strncpy_s`, not a verified timer type ID.
No recipe depends on that inference.

## Original file pins

| Suffix | Region key | Bytes | Duration offset | Original SHA256 |
| --- | --- | --- | --- | --- |
| 0100 | 60301 | 9633 | 1659 | `3a3bb041fb3b7b835e59a64275f97c2ba9ad06631bb49cb41a922eb066fb8bc5` |
| 0200 | 60101 | 9627 | 1653 | `343fde48a424df71cc2566d08c763921d23a7911de1e17a275be801525f117f5` |
| 0300 | 60201 | 9630 | 1656 | `ffdd48d3a2e84877817eabd3f2c96a791a2f1ae1495ec312ec090a6ece3de77b` |
| 0400 | 60001 | 9612 | 1638 | `5abc2bb00e15400e8e91fda74ad47e60c16b2e255ef7d1be4cb8b94d10670e55` |

Game assets are read from the user's installation. No script bodies or executable
fixtures are committed or bundled. Unknown, truncated or already modified
originals are refused. All four files must be present, and each inverse edit must
reproduce the complete original file exactly.

## Verification and limits

Automated checks cover original-file pins, exact byte preservation outside the
duration, rejection of changed sources, archive round trips, combined overlay
transactions and restoration. The Marketplace uses the existing review and
closed-game write guards.

An actual game test is still required after a normal installation/start: enter
each supported restricted region, verify the 30-minute warning, remain past the
old 30-second boundary, then verify cancellation on leaving the region and the
eventual return. Other world limits, unloaded geometry, quest-specific returns,
death and drowning can still interrupt exploration. This setting does not make
every out-of-bounds location playable or disable other timers.
