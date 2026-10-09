param([string]$GameExe, [string]$RegistryGameExe)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$buildDir = Join-Path $projectRoot '.local/repair-runtime-build'
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vsPath = & $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) { throw 'Visual Studio C++ Build Tools fehlen.' }
$cmake = Join-Path $vsPath 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
$ctest = Join-Path $vsPath 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe'
& $cmake -S $PSScriptRoot -B $buildDir -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE) { throw 'CMake configuration failed.' }
& $cmake --build $buildDir --config Release --parallel 2
if ($LASTEXITCODE) { throw 'C++ build failed.' }
& $ctest --test-dir $buildDir -C Release --output-on-failure
if ($LASTEXITCODE) { throw 'Contract tests failed.' }
if ($GameExe) {
    # Source is only opened for reading; execution uses a private fixture arena.
    & (Join-Path $buildDir 'Release/repair-harness.exe') --exe $GameExe
    if ($LASTEXITCODE) { throw 'Hash-pinned native fixture tests failed.' }
}
if ($RegistryGameExe) {
    # Separate build pin: 2949 registry/reference integration on private objects.
    & (Join-Path $buildDir 'Release/repair-registry-native.exe') --exe $RegistryGameExe
    if ($LASTEXITCODE) { throw 'Hash-pinned registry/reference integration failed.' }
}
