# Third-party notices — Crimson Workbench 0.6.0

Crimson Workbench's original code is licensed under the MIT License in `LICENSE`.
Third-party components retain their own copyright notices and licenses. This
document and the accompanying `licenses/` directory must accompany distributed
desktop and optional runtime packages.

## Desktop application

The full copyright and license texts for the Windows x64 Cargo dependency graph
are in [`licenses/RUST_DEPENDENCIES.txt`](licenses/RUST_DEPENDENCIES.txt). That
file lists the exact package versions and downloadable source archives. The
inventory includes normal and build dependencies of the desktop app; some listed
packages are used only during compilation. Versions are pinned by `Cargo.lock`.

The complete notices for production frontend packages are in
[`licenses/FRONTEND_DEPENDENCIES.txt`](licenses/FRONTEND_DEPENDENCIES.txt). They
cover React, React DOM, Scheduler, Tauri's JavaScript API, TanStack Table and
Virtual, Lucide React, and Zustand at the versions in `app/package-lock.json`.
Development test runners and development servers are not shipped in the desktop
frontend. For dual-licensed components, the original alternatives remain available;
this package does not replace their terms with its own MIT license.

The unmodified MPL-2.0 dependencies in the Cargo inventory include cssparser,
cssparser-macros, dtoa-short, option-ext and selectors. Their Source Code Form is
available without charge from the exact `https://crates.io/api/v1/crates/NAME/VERSION/download`
links beside each entry. Those sources remain governed by MPL-2.0, whose full text
is included in the inventory. No rights in those sources are restricted by
Workbench's license. The [MPL-2.0 text](https://www.mozilla.org/en-US/MPL/2.0/)
also explains the terms for source and executable distribution.

### crimson-rs format and save port

Copyright (c) 2026 Tommy Tran. MIT License.

Selected code was adapted from
[`bbfox0703/crimson-rs` commit `b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0`](https://github.com/bbfox0703/crimson-rs/tree/b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0).
The complete notices are in [`licenses/crimson-format-LICENSE.txt`](licenses/crimson-format-LICENSE.txt)
and [`licenses/crimson-format-save-LICENSE.txt`](licenses/crimson-format-save-LICENSE.txt).
The source checkout also retains the original notices in `vendor/crimson-format/`;
the port and changes are documented in `docs/FORMAT_PORT.md`.

## Optional native runtime package

The runtime archive contains Workbench's own `CrimsonLiveItems.asi` and
`CrimsonExtraSockets.asi`, plus the following third-party components. No game
executable, game data, save files, or third-party example plugins are included.

| Component | Exact source | Included notice |
|---|---|---|
| MinHook 1.3.4, linked into Workbench's two ASIs | [TsudaKageyu/minhook v1.3.4](https://github.com/TsudaKageyu/minhook/tree/v1.3.4) | [`minhook-LICENSE.txt`](licenses/minhook-LICENSE.txt), including HDE32 and HDE64 notices |
| Ultimate ASI Loader 9.7.4 x64 | [ThirteenAG, `6b440669144c4a0bef5718ab155df160d231cd42`](https://github.com/ThirteenAG/Ultimate-ASI-Loader/tree/6b440669144c4a0bef5718ab155df160d231cd42) | [`ultimate-asi-loader-LICENSE.txt`](licenses/ultimate-asi-loader-LICENSE.txt), MIT, Copyright (c) 2023 ThirteenAG |
| Injector, included by the loader | [ThirteenAG/injector, `3a384e8d1b575c09383b0fab8bd92e34cb654949`](https://github.com/ThirteenAG/injector/tree/3a384e8d1b575c09383b0fab8bd92e34cb654949) | [`ultimate-asi-loader-injector-LICENSE.txt`](licenses/ultimate-asi-loader-injector-LICENSE.txt), zlib, Copyright (C) 2012–2014 LINK/2012 |
| Injector's FunctionHookMinHook utility | [Same pinned injector revision, `utility/`](https://github.com/ThirteenAG/injector/tree/3a384e8d1b575c09383b0fab8bd92e34cb654949/utility) | [`ultimate-asi-loader-utility-LICENSE.txt`](licenses/ultimate-asi-loader-utility-LICENSE.txt), MIT, Copyright (c) 2019 praydog |
| MinHook, included by the loader's injector submodule | [TsudaKageyu/minhook, `d94c64d32ea37bc4f5ee47d580709f70c6fb6080`](https://github.com/TsudaKageyu/minhook/tree/d94c64d32ea37bc4f5ee47d580709f70c6fb6080) | [`ultimate-asi-loader-minhook-LICENSE.txt`](licenses/ultimate-asi-loader-minhook-LICENSE.txt), including HDE32 and HDE64 notices |
| miniz, included by the loader | [Pinned loader revision, `external/miniz/`](https://github.com/ThirteenAG/Ultimate-ASI-Loader/tree/6b440669144c4a0bef5718ab155df160d231cd42/external/miniz) | [`ultimate-asi-loader-miniz-LICENSE.txt`](licenses/ultimate-asi-loader-miniz-LICENSE.txt), MIT-style terms, RAD Game Tools, Valve Software, Rich Geldreich and Tenacious Software LLC |

The loader DLL is the unmodified official x64 release binary, renamed from
`dinput8.dll` to the supported proxy filename `winmm.dll`. The downloaded
`Ultimate-ASI-Loader-NoPDB_x64.zip` has SHA-256
`e5860e7d9a1805267535b65749575b5e406cc6ea3325c7392189c578815045d1`.
Renaming does not alter its code. The x86-only MemoryModule, d3d8to9, minidx9,
and the upstream example plugins are not part of this package. The x64 loader
dependency list was checked against its pinned `premake5.lua` and submodule tree.

MinHook and its Hacker Disassembler Engine portions use two-clause BSD terms:
Copyright (C) 2009–2017 Tsuda Kageyu and Copyright (c) 2008–2009 Vyacheslav Patkov.
Their complete redistribution conditions and warranty disclaimers are preserved
in the included license files. `licenses/sources.json` records the URLs and
SHA-256 hashes of the retrieved native notices.

## Platform and game content

Windows system libraries and Microsoft Edge WebView2 remain Microsoft components
with their own terms. The installer can invoke Microsoft's WebView2 bootstrapper
if the runtime is missing; the portable archive requires an installed WebView2
runtime. Workbench does not relicense Microsoft software.

Crimson Desert and its content belong to Pearl Abyss and their respective owners.
Workbench is an independent project. Item icons and other game content are read
from the user's own installation, not redistributed in these packages. Additional
research acknowledgements and format references are documented in `CREDITS.md`;
references there do not imply that their code is bundled.
