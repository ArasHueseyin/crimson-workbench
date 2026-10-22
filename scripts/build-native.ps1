[CmdletBinding()]
param(
    [string]$OutputDirectory,
    [string]$Version
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = Split-Path -Parent $PSScriptRoot
if (-not $Version) { $Version = (Get-Content -LiteralPath (Join-Path $repo 'app/package.json') -Raw | ConvertFrom-Json).version }
if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw 'A complete numeric release version is required.' }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $repo 'target/distribution/runtime' }
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
$build = Join-Path $repo 'target/native-release'
$cache = Join-Path $build 'dependency-cache'
$pins = Get-Content -LiteralPath (Join-Path $repo 'runtime/dependencies.lock.json') -Raw | ConvertFrom-Json
New-Item -ItemType Directory -Path $build, $cache, $OutputDirectory -Force | Out-Null
if (@(Get-ChildItem -LiteralPath $OutputDirectory -Force).Count) { throw 'Native output directory must be empty; use a new output directory.' }
function Hash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
function Fetch([string]$Url, [string]$Path, [string]$Sha256) {
    if (-not (Test-Path -LiteralPath $Path)) { Invoke-WebRequest -Uri $Url -OutFile $Path -UseBasicParsing }
    if ((Hash $Path) -ne $Sha256) { throw "Pinned download hash mismatch: $Path" }
}
$minhookZip = Join-Path $cache 'minhook-v1.3.4.zip'
$loaderZip = Join-Path $cache 'ual-v9.7.4-x64.zip'
Fetch $pins.minhook.url $minhookZip $pins.minhook.sha256
Fetch $pins.asiLoader.url $loaderZip $pins.asiLoader.sha256
# Extract into a unique tree, so a previously edited dependency cannot enter a release.
$deps = Join-Path $build ('dependencies-' + [guid]::NewGuid().ToString('N'))
Expand-Archive -LiteralPath $minhookZip -DestinationPath (Join-Path $deps 'minhook')
Expand-Archive -LiteralPath $loaderZip -DestinationPath (Join-Path $deps 'loader')
$minhook = Join-Path $deps 'minhook/minhook-1.3.4'
$loader = Join-Path $deps 'loader/dinput8.dll'
if ((Hash $loader) -ne $pins.asiLoader.dllSha256) { throw 'Unexpected ASI loader DLL hash.' }
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmakeCommand) { $cmake = $cmakeCommand.Source } else {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio 2022 C++ tools and CMake are required.' }
    $vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $cmake = Join-Path $vs 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
}
$ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
function CMakeRun([string[]]$Arguments) {
    & $cmake @Arguments
    if ($LASTEXITCODE -ne 0) { throw "CMake failed: $Arguments" }
}
$live = Join-Path $build 'live-items'
$sockets = Join-Path $build 'extra-sockets'
CMakeRun @('-S', (Join-Path $repo 'runtime/live-items'), '-B', $live, '-G', 'Visual Studio 17 2022', '-A', 'x64', "-DMINHOOK_ROOT=$minhook")
# Build the public target graph, including owned pipe hosts and every registered
# test. A new CTest must not silently depend on an unbuilt, omitted target.
CMakeRun @('--build', $live, '--config', 'Release', '--parallel', '2')
& $ctest --test-dir $live -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Native policy tests failed.' }
$liveAsi = Join-Path $live 'Release/CrimsonLiveItems.asi'
$liveHash = Hash $liveAsi
$market = Join-Path $build 'marketplace'
CMakeRun @('-S', (Join-Path $repo 'runtime/marketplace'), '-B', $market, '-G', 'Visual Studio 17 2022', '-A', 'x64', "-DMINHOOK_ROOT=$minhook", '-DCRIMSON_PRIVATE_CONSUMABLE_EVIDENCE_HEADER=', '-DCRIMSON_PRIVATE_GLIDE_FIXTURE_HEADER=', '-DCRIMSON_PRIVATE_FORCE_PALM_HEIGHT_FIXTURE_HEADER=')
CMakeRun @('--build', $market, '--config', 'Release', '--parallel', '2')
& $ctest --test-dir $market -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Marketplace native policy tests failed.' }
$marketModules = @(
    @{ id = 'sleep_cooldown'; file = 'CrimsonSleepCooldown.asi'; priorHashes = @('a66c149adb7f304e7c6ae3fc80e45169bd6bd98a0cbedd4531f75613cc7d8201', 'a4d33c648cf39e0670018e72517c13a8c3220f0ae6635bf1fe8a60fe4b0b8155') },
    @{ id = 'sleep_duration'; file = 'CrimsonSleepDuration.asi'; priorHashes = @('d250918a898702c2cd79a03c574c8be7a9184285cd2dabd4bde4d3d643f2e56e', 'b7ffbb1fc70ab9755f7c6b6e6491f192d0a811b3002368f395fe15b4f49ebb88') },
    @{ id = 'blackstar'; file = 'CrimsonBlackstar.asi'; priorHashes = @('8e1b254874c27022f6746b01f0520b31ed3ee44447d2919dce15f090ed8a886f', 'cd6556137b25c600f172e473b3f179eab571785b2565d968ffb83ff47eeb30ba') },
    @{ id = 'no_fall'; file = 'CrimsonNoFall.asi'; priorHashes = @('4a7078a959e344a73ae95d01349774c9fb1078ce8fa43f193ca04be0338398f6', '3f0224d502bd1dcab9dd26757078d6820d2104d31886ca6792c56bc116047835') },
    @{ id = 'player_stamina'; file = 'CrimsonPlayerStamina.asi'; priorHashes = @('fd71d19ed2af26d1ba4c53e4f00aa372a1128a3ae63282d85b0035b5f5f0ad5c', 'bbab63542f81c570116163d6381cb88af07ee353448c977554faa0f8f18b8d82') },
    @{ id = 'mount_jump'; file = 'CrimsonMountJump.asi'; priorHashes = @('04689b0f2dbb0c0645e039d1a655cb92193915a53153e8ee71c818641cb45d41') },
    @{ id = 'player_jump'; file = 'CrimsonPlayerJump.asi'; priorHashes = @('2fa1a87df2fa4bd6f2a9967c7a4f0585923217c9244fee5c57a2c2e9e94cc21c', '6a021b1d1e9faa8441605df304bddb953985b2c556211191f0e4e65f4dbe21ec') },
    @{ id = 'player_speed'; file = 'CrimsonPlayerSpeed.asi'; priorHashes = @('9803e3b257a5fe529ddca1ea9534419d9e342c99c9fa94e39d7f93c6fd748de8') },
    @{ id = 'preserve_consumables'; file = 'CrimsonPreserveConsumables.asi'; priorHashes = @('174e43f11508775fc318f7f9f1a567ee9569dcbe461aeb8994b3561ab279ca6f') },
    @{ id = 'mount_protection'; file = 'CrimsonMountProtection.asi'; priorHashes = @('f45fd110ad53de2fe2c83bf4643a0f08dafad1ab8e2396c20ffdfd346a4471fb') },
    @{ id = 'free_crafting_materials'; file = 'CrimsonFreeCrafting.asi'; priorHashes = @('5f3e272e201d84e213d72c2659f3c1905786b4a1d5e8916252e94b285f6b90b8') },
    @{ id = 'force_palm'; file = 'CrimsonForcePalm.asi'; priorHashes = @('7ab93ae127f11959904b13eef537f158141c125836ff3378c82900eb3d38ff70') },
    @{ id = 'force_palm_height'; file = 'CrimsonForcePalmHeight.asi'; priorHashes = @('13b287af5246d83a6f73872d3a1da357d692caad1d318e02d60b1788ad083da3') },
    @{ id = 'axiom_reach'; file = 'CrimsonAxiomReach.asi'; priorHashes = @('0610e54c48812f8042349a7974f95f76062886831fe5d7d70fa1fb76e7068450') },
    @{ id = 'glide_speed'; file = 'CrimsonGlideSpeed.asi'; priorHashes = @('fbc4f4c084400859cf3ea0f07b55fd57594ac431173eb8c8e9eeaa8d58cbb059', '2c6bdd2adb3e014ef9a5049b3c9cef7eea4c2a0afbd946a09e70725914a46af8', 'afc8d4320a4cae92490e264a17e27d77d8096d71372ba6f1d6212aea930e1987', '9916efa51896537294058fed49347700f4ce413e6652fc9cfe3a273cd04b48e3') },
    @{ id = 'mount_dismount'; file = 'CrimsonMountDismount.asi'; priorHashes = @('4f1999f03bf2166b768e875b8173fc2c150b41f9893b25904156801105e49e13') },
    @{ id = 'auto_loot'; file = 'CrimsonAutoLoot.asi'; priorHashes = @('744b9f0d8eeac78c10f86b653094422bfe43820f3e76bafbbb384768f43ae907') },
    @{ id = 'world_controls'; file = 'CrimsonWorldControls.asi'; priorHashes = @('81862f017cc8d1c4c21579ba0547681feff9ead087e302c71e075b97e479a4cf','909daf3f9ae7d3cc35e48700c2b27b931b927b2077c57aea0268b384fafd8d10') },
    @{ id = 'photo_mode'; file = 'CrimsonPhotoMode.asi'; priorHashes = @('2fcda5425df0e0e8071396c842621bfe3affb1f49b2d745930d03f20a48f84a9') },
    @{ id = 'teleport'; file = 'CrimsonTeleport.asi'; priorHashes = @('00d2e87a2f134d9962ee69f91192d95bb90a526a8247d31f55950d624fc9424f') }
)
CMakeRun @('-S', (Join-Path $repo 'runtime/extra-sockets'), '-B', $sockets, '-G', 'Visual Studio 17 2022', '-A', 'x64', "-DMINHOOK_ROOT=$minhook", "-DCRIMSON_LIVE_ITEMS_SHA256=$liveHash")
CMakeRun @('--build', $sockets, '--config', 'Release', '--target', 'CrimsonExtraSockets')
$socketsAsi = Join-Path $sockets 'Release/CrimsonExtraSockets.asi'
$socketsHash = Hash $socketsAsi
if (-not [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($socketsAsi)).Contains($liveHash)) {
    throw 'ExtraSockets binary does not contain the paired LiveItems hash.'
}
# Loader integration runs only in a new synthetic directory, never a game folder.
$smoke = Join-Path $build ('loader-smoke-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $smoke | Out-Null
Copy-Item -LiteralPath $liveAsi, $socketsAsi -Destination $smoke
foreach ($mod in $marketModules) { Copy-Item -LiteralPath (Join-Path $market ('Release/' + $mod.file)) -Destination $smoke }
Copy-Item -LiteralPath $loader -Destination (Join-Path $smoke 'winmm.dll')
# The runtime also pins the process basename. This is still our tiny test host,
# not a copied game executable; its PE/header/hash must be rejected by both ASIs.
Copy-Item -LiteralPath (Join-Path $live 'Release/runtime-package-loader-tests.exe') -Destination (Join-Path $smoke 'CrimsonDesert.exe')
$empty = New-Object byte[] 48
[Text.Encoding]::ASCII.GetBytes('CWEXSOCK').CopyTo($empty, 0)
[BitConverter]::GetBytes([uint32]2).CopyTo($empty, 8)
$sha = [Security.Cryptography.SHA256]::Create()
try { $sha.ComputeHash([byte[]]@()).CopyTo($empty, 16) } finally { $sha.Dispose() }
[IO.File]::WriteAllBytes((Join-Path $smoke 'CrimsonExtraSockets.dat'), $empty)
& (Join-Path $smoke 'CrimsonDesert.exe')
if ($LASTEXITCODE -ne 0) { throw 'Pinned loader + native ASI smoke failed.' }
Copy-Item -LiteralPath $liveAsi, $socketsAsi -Destination $OutputDirectory
$addons = Join-Path $OutputDirectory 'addons'
New-Item -ItemType Directory -Path $addons -Force | Out-Null
$addonManifest = @($marketModules | ForEach-Object {
    $source = Join-Path $market ('Release/' + $_.file)
    Copy-Item -LiteralPath $source -Destination $addons
    [ordered]@{ id = $_.id; file = $_.file; sha256 = (Hash $source); bytes = (Get-Item -LiteralPath $source).Length; priorHashes = $_.priorHashes }
})
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'marketplace-manifest.json'), (([ordered]@{ schema = 1; version = $Version; gameExeSha256 = '57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7'; mods = $addonManifest }) | ConvertTo-Json -Depth 6), [Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Marketplace-Mods.ps1') -Destination $OutputDirectory
# The small companion builds script overlays from each user's verified game
# assets. No game scripts or personal archive registry are shipped.
& cargo build -p cd-cli --bin crimson-archive-mods --release --locked
if ($LASTEXITCODE -ne 0) { throw 'Archive mod installer build failed.' }
$archiveHelper = Join-Path $repo 'target/release/crimson-archive-mods.exe'
Copy-Item -LiteralPath $archiveHelper -Destination (Join-Path $OutputDirectory 'CrimsonArchiveMods.exe')
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'archive-mods-manifest.json'), (([ordered]@{
    schema = 1; file = 'CrimsonArchiveMods.exe'; sha256 = (Hash $archiveHelper); bytes = (Get-Item -LiteralPath $archiveHelper).Length
}) | ConvertTo-Json), [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllBytes((Join-Path $OutputDirectory 'CrimsonExtraSockets.dat'), $empty)
New-Item -ItemType Directory -Path (Join-Path $OutputDirectory 'loader') | Out-Null
Copy-Item -LiteralPath $loader -Destination (Join-Path $OutputDirectory 'loader/winmm.dll')
foreach ($script in @('Install-RuntimeMods.ps1', 'Uninstall-RuntimeMods.ps1', 'RuntimeMods.Common.ps1', 'Setup-RuntimeMods.ps1')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $script) -Destination $OutputDirectory
}
Copy-Item -LiteralPath (Join-Path $repo 'runtime/README-install.md') -Destination (Join-Path $OutputDirectory 'README.md')
foreach ($file in @('LICENSE', 'THIRD_PARTY_NOTICES.md', 'runtime/dependencies.lock.json')) {
    Copy-Item -LiteralPath (Join-Path $repo $file) -Destination $OutputDirectory
}
Copy-Item -LiteralPath (Join-Path $repo 'licenses') -Destination (Join-Path $OutputDirectory 'licenses') -Recurse
$manifest = [ordered]@{
    schema = 1; version = $Version; platform = 'windows-x86_64'; gameBuild = '1.0.0.2976'
    gameExeSha256 = '57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7'
    liveItemsSha256 = $liveHash; extraSocketsSha256 = $socketsHash
    asiLoader = $pins.asiLoader
    files = @('CrimsonLiveItems.asi', 'CrimsonExtraSockets.asi', 'CrimsonExtraSockets.dat', 'loader/winmm.dll') | ForEach-Object {
        $file = Join-Path $OutputDirectory $_
        [ordered]@{ name = $_; sha256 = (Hash $file); bytes = (Get-Item -LiteralPath $file).Length }
    }
}
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'runtime-manifest.json'), ($manifest | ConvertTo-Json -Depth 6), [Text.UTF8Encoding]::new($false))
$env:CRIMSON_EXTRA_SOCKETS_SHA256 = $socketsHash
Write-Host "Native package: $OutputDirectory"
Write-Host "Compile desktop with CRIMSON_EXTRA_SOCKETS_SHA256=$socketsHash"
