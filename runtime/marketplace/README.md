# Marketplace runtime modules

These are Workbench's original Blackstar and sleep implementations, promoted from the private development workspace under the repository's MIT license. They use pinned MinHook 1.3.4 without modification. Private assembly fixtures, assets, tables and saves are excluded.

Supported process: `CrimsonDesert.exe`, build 1.0.0.2976, SHA-256 `57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`. Each ASI checks the executable hash, PE identity and short target fingerprints before installing hooks. Hook setup rolls back on failure.

- `CrimsonSleepCooldown.asi`: clears the native sleep/wait cooldown and repeats the original validity check immediately. The previous personal version imposed one real second; this release imposes none. Other validity failures remain failures.
- `CrimsonSleepDuration.asi`: the existing 12-hour sleep event advances time by 30 hours. Other choices, calendar fields and unrelated calls remain unchanged. The menu label is not edited.
- `CrimsonBlackstar.asi`: Blackstar-specific stamina regeneration, summon cooldown/duration, region/navigation/voxel/platform and city/region/height flight changes. Placement, indoor and quest/state checks can still refuse summoning. Existing saved timers are not edited.

`scripts/build-native.ps1` builds the modules, runs synthetic policy and loader tests, and writes `marketplace-manifest.json`. `scripts/Marketplace-Mods.ps1` provides the fixed-path, hash-checked install/remove worker used by the desktop app. Its tests use temporary synthetic game folders.
