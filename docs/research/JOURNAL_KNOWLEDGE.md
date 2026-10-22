# Journal knowledge unlock, build 2976

The production recipe contains 3,939 numeric knowledge key / maximum-level pairs.
It was derived from the current build's KnowledgeInfo and KnowledgeGroupInfo
readers and tables. It includes only unblocked, UI-visible entries whose group
and all parent groups are visible, excluding skill references. Both directions
of group membership were cross-checked, and each full knowledge-level array
was decoded and verified to contain consecutive levels from one to its maximum.
The 140 selectable skill keys do not intersect the recipe. No game text, assets,
account identifiers or save contents are included in the recipe or tests.

The transform decodes and authenticates each user's selected save, rejects
ambiguous blocks and duplicate keys, adds absent entries and raises incomplete
entries. Higher existing levels and all non-target entries remain intact. New
entries are constructed from the save's validated reflection schema, without
copying another user's record or requiring a particular donor entry. Empty and
absent lists are supported. Existing fields, learned times and new-entry marks
are preserved; new entries use the latest existing learned time or zero.

Candidate verification requires an exact original-body inverse, encrypted
readback, a complete target check and byte-exact no-op behavior when repeated.
The apply layer locks the supported game executable and selected save pair,
validates the preview hashes, backs up both files and uses the existing
commit/recovery kernel. Lobby bytes remain unchanged.

Synthetic tests cover empty, absent, partial and complete lists, higher existing
levels, unrelated entries/counters, stale save/lobby bindings, corrupt saves,
duplicates and repeated application. A private, ignored regression compares
production output byte-for-byte with the independently verified candidate
accepted in game by the user. Private fixtures and research tools stay local.
