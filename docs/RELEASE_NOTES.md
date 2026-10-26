## Crimson Workbench 0.10.1

**[Download the Windows installer](https://github.com/ArasHueseyin/crimson-workbench/releases/download/v0.10.1/Crimson-Workbench_0.10.1_x64-setup.exe)** · One setup includes Workbench and its optional game modules.

- **Fixed language-dependent build detection:** original Steam installations without the optional German archive group `0037` can open the catalog. Workbench no longer reports these installations as an unknown or modified build solely because `0037/0.pamt` is absent. You do not need to download German files or rename archive folders.
- **Consistent backup checks:** installation baseline validation accepts the same optional-group absence. Required files and the exact supported EXE, registry and table hashes remain checked. An installed `0037` group with missing or changed metadata is still rejected.
- **Regression coverage:** synthetic tests cover other language layouts, present and absent optional metadata, incomplete groups, changed files and managed registry backups. No game files or saves are changed by this fix.

Windows x64 · supported game EXE **1.0.0.2976** · unsigned Windows binaries. Built and tested through GitHub Actions; **SHA256SUMS.txt** accompanies the downloads. Private saves, local tools and game assets are excluded.
