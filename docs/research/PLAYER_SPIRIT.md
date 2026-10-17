# Selected-player spirit and stamina protection

Local implementation checkpoint: 17 October 2026. Native tests run against
owned fixtures. No game module was installed or loaded, no game process was
controlled, and no user settings, saves or game files were changed for this
implementation. In-game acceptance remains pending.

## One native hook for both resources

The existing **CrimsonPlayerStamina.asi** now implements the English Marketplace
card **Unlimited player stamina & spirit**, with independent startup options for
stamina and spirit. There is no second spirit ASI. Both resources use the same
final native stat commit, so separate hooks at that entry would conflict.

Only executable build **1.0.0.2976**, SHA256
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`,
PE timestamp `1790086912` and image size `389722112` are admitted. The module
retains the single hook at RVA `c7efde0`, reached through `17b41b0`. It validates
the complete on-disk executable hash and three instruction spans: the commit
prologue, its native bound handling and the final flag/current stores. Module
lifetime and a published trampoline are retained after any activation attempt.

Original StatusInfo definitions were independently checked against immutable
table body/header hashes. The StatusInfo serializer loads the blocked byte
at `+10`, resource kind at `+11`, and resolved ordinal at `+14`.

| Resource | Semantic key | Original ordinal | Kind |
| --- | --- | --- | --- |
| Health (`Hp`) | 1000000 | 0 | 1 |
| Stamina | 1000026 | 22 | 2 |
| Spirit (`Mp`) | 1000027 | 23 | 2 |
| Breath | 1000078 | 24 | 2 |

The policy resolves the incoming entry's loaded definition and matches its
semantic key, kind, blocked state and resolved ordinal. It does not assume that
every kind-2 entry is spirit or copy ordinal 23 without semantic checks.

## Scope and native bounds

The callback borrows only the selected character's current client and matching
server representations. It validates full actor identity and generation,
liveness, allowed character kind, manager/actor/status classes, component and
possessor ownership, container sizes and aligned membership. It then rechecks
selection, registry, ownership and definition witnesses. Volatile integral
native reads prevent the optimizer from reusing the earlier witnesses. Actor
addresses are not retained for later calls.

For an enabled resource, a decrease of its current positive value is replaced
with that value. The original native commit is still called exactly once with
the original time and flag; its return value is preserved. The hook performs
no resource-memory writes. Positive grants, regeneration, equal values and
unrelated entries retain the original target.

Native commit computes its upper bound from entry `+18` plus `+20`, and reads
the lower bound from `+28`. The policy checks signed-addition overflow, valid
bounds, positive current value within those bounds, and rechecks those fields
before suppressing a decrease. It leaves invalid or reduced bounds to native
handling. Each client/server resource keeps its own native capacity; no maximum
is enlarged or borrowed from the other representation.

The selected character's health, breath, hunger/relief, mount resources, NPCs,
inactive characters and unrecognized resource keys remain unchanged. Invalid,
ambiguous or changing ownership and guarded read exceptions forward unchanged.

This prevents spirit decreases reaching the verified final commit, including
ordinary costs and spirit drain. It does **not** invent spirit when the current
value is zero, bypass a skill's affordability/learned-state requirements, or
refill a nearly empty gauge to its maximum. A character must already have enough
spirit for the intended action. Ordinary regeneration can still restore it.
Direct writes that bypass the native commit are outside this implementation's
coverage.

## Startup setting contract

The file is `%LOCALAPPDATA%\local.crimson.workbench\player-resources.dat`.
It contains exactly eight bytes:

| Offset | Format | Meaning |
| --- | --- | --- |
| 0..3 | ASCII `CWR1` | Format marker |
| 4..7 | Little-endian uint32 | Flags: bit 0 stamina, bit 1 spirit |

Values 0, 1, 2 and 3 mean normal resources, stamina only, spirit only and both.
An absent file or absent settings directory means **1**, preserving the
previous stamina-only behavior. An existing malformed, unsafe or unreadable
file means **0**, preserving normal resources until explicitly repaired.
Wrong magic, length or flags, hard-linked files, final reparse points and a
reparse-point settings directory are rejected. A zero setting creates no hook.

Settings are read once at module startup. Saving changed options, updating or
removing the ASI requires the game to be closed, then the next launch loads the
selected behavior. There is no live injection or live setting reload.

## Verification and pending game acceptance

The private MSVC build uses `/W4 /WX`; both the existing stamina test group and
the new spirit test group pass with assertions enabled. Fixtures cover all
four resource combinations, client/server entries, reordered definitions,
unrelated semantic keys, grants/regeneration, zero and negative current values,
valid capacity modifiers, overflow, reduced or changing bounds, stale/dead or
foreign actors, malformed and changing ownership and exception forwarding.
Owned temporary setting files additionally exercise missing, valid, oversized,
locked, hard-linked and directory-path cases. No user setting is involved.

The ignored independent audit rechecks the complete original EXE, three runtime
code spans, original StatusInfo hashes and resource semantics. It verifies one
hook owner and the exact setting contract. The existing mount-protection delta
hook remains at its distinct entry `c7e0860`; stamina/spirit share one commit
hook instead of competing for it. The other bundled hook entry RVAs are
unchanged.

Remaining acceptance: after a convenient restart, test spirit-only, stamina-only
and both enabled; repeat spirit-consuming skills, Force Palm, character swaps,
mount transitions and reloads; check normal health/breath/mount resources,
regeneration and low/zero-spirit behavior; disable both and confirm normal
resource costs return. These offline checks do not claim observed gameplay.
