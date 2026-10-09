[CmdletBinding()]
param(
    [string]$OutputDirectory,
    [string]$Version = '0.6.0'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = Split-Path -Parent $PSScriptRoot
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $repo 'target/distribution/runtime' }
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
$build = Join-Path $repo 'target/native-release'
$cache = Join-Path $repo '.local/native-deps'
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
CMakeRun @('--build', $live, '--config', 'Release', '--target', 'CrimsonLiveItems', 'live-items-probe-tests', 'live-items-grant-tests', 'runtime-package-loader-tests')
& $ctest --test-dir $live -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Native policy tests failed.' }
$liveAsi = Join-Path $live 'Release/CrimsonLiveItems.asi'
$liveHash = Hash $liveAsi
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
[IO.File]::WriteAllBytes((Join-Path $OutputDirectory 'CrimsonExtraSockets.dat'), $empty)
New-Item -ItemType Directory -Path (Join-Path $OutputDirectory 'loader') | Out-Null
Copy-Item -LiteralPath $loader -Destination (Join-Path $OutputDirectory 'loader/winmm.dll')
foreach ($script in @('Install-RuntimeMods.ps1', 'Uninstall-RuntimeMods.ps1', 'RuntimeMods.Common.ps1')) {
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
