# Contributor experience audit

Reviewed on 2026-10-23 against `master` at `0c86f9b` (v0.10.0). Scope: contributor onboarding, documentation, build/test ergonomics and repository workflow.

## Findings and changes

| Finding | Result of this pass |
| --- | --- |
| No contribution entry point or PR template | Added `CONTRIBUTING.md`, a focused PR template and an English developer guide with setup, checks and a code map |
| README mixes installation, feature inventories and implementation history | Reduced it from 1,245 to about 420 words; added clear download/contribution links and an optimized Item Picker GIF |
| Conflicting feature counts and old test claims in the main entry path | Removed volatile counts from the README, added a documentation index, and marked early architecture/test logs as historical |
| Build guide hard-codes an old output version and native-module count | Replaced these with the script's version-derived path and package terminology |
| Browser tests require Edge on every platform | Retained Edge as the Windows default; added Chromium elsewhere and an explicit channel override, with browser installation instructions |
| Core CI has no explicit token permissions or run limit | Added read-only repository permissions, a job timeout and cancellation of superseded runs; preserved required check names |
| No shared basic editor settings | Added `.editorconfig`; retained Rustfmt and avoided unrelated source reformatting |
| Bug report example version becomes stale | Replaced it with a version-neutral prompt and added guidance to redact personal data |

## Existing strengths retained

- Public MIT license, third-party notices and attribution.
- Four English issue forms for bugs, features, mods and feedback.
- Protected `master`: PRs, up-to-date Windows/Linux checks, no force push or deletion, no mandatory second maintainer approval.
- Committed npm/Cargo lockfiles, pinned native dependency hashes, Windows/Linux CI and a separate Windows release pipeline.
- Synthetic tests and ignored personal/generated data; a game installation is optional for contributor checks.

## Follow-up improvements

1. **Formatting and linting:** agree on TypeScript/C++ tooling and introduce it in a dedicated change. The existing compact source style should not require every contributor to reformat unrelated files.
2. **Private security reports:** GitHub private vulnerability reporting is currently disabled. Establish that channel and add a matching `SECURITY.md` with the reporting process before advertising private reports.
3. **Linux browser CI:** Chromium is now selectable without Edge. Consider running the frontend browser suite on Linux too, after validating the whole suite there; the existing Linux job covers Rust and frontend build/unit checks.

## Validation

The onboarding changes were checked with a fresh `npm ci`, TypeScript/production frontend build, all frontend unit tests, selected Item Picker/setup browser flows, relative-link and Markdown-anchor checks, workflow YAML inspection, and a rendered README preview. The GIF was checked for dimensions, animation, looping and file size. No game or save operations are needed for this change.
