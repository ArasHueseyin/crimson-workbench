param([string]$OutputDirectory, [switch]$SkipChecks)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not [Environment]::Is64BitOperatingSystem -or $env:OS -ne 'Windows_NT') {
    throw 'The release package requires Windows x64.'
}
if (-not ((& rustc -vV) -match '^host: x86_64-pc-windows-msvc$')) {
    throw 'Use the x86_64-pc-windows-msvc Rust toolchain for these x64 packages.'
}
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$package = Get-Content -LiteralPath (Join-Path $projectRoot 'app/package.json') -Raw | ConvertFrom-Json
$version = $package.version
$config = Get-Content -LiteralPath (Join-Path $projectRoot 'app/src-tauri/tauri.conf.json') -Raw | ConvertFrom-Json
$cargo = Get-Content -LiteralPath (Join-Path $projectRoot 'app/src-tauri/Cargo.toml') -Raw
if ($version -notmatch '^\d+\.\d+\.\d+$' -or $config.version -ne $version -or $cargo -notmatch ('(?m)^version = "' + [regex]::Escape($version) + '"\r?$')) {
    throw 'The npm, Cargo and Tauri release versions must agree.'
}
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $projectRoot ".local/release/$version" }
$output = [IO.Path]::GetFullPath($OutputDirectory)
$assets = Join-Path $output 'assets'
if (Test-Path -LiteralPath $output) {
    if (@(Get-ChildItem -LiteralPath $output -Force).Count) {
        throw "Release output must be empty. Choose a fresh -OutputDirectory: $output"
    }
}
New-Item -ItemType Directory -Path $assets -Force | Out-Null
function Invoke-Checked([scriptblock]$Action) {
    & $Action
    if ($LASTEXITCODE -ne 0) { throw "Command failed with exit code $LASTEXITCODE" }
}
Push-Location $projectRoot
try {
    Push-Location (Join-Path $projectRoot 'app')
    try { Invoke-Checked { npm ci --no-audit --no-fund } } finally { Pop-Location }
    # This sets CRIMSON_EXTRA_SOCKETS_SHA256 for the Rust build in this process.
    & (Join-Path $PSScriptRoot 'build-native.ps1') -OutputDirectory (Join-Path $output 'runtime') -Version $version
    & (Join-Path $PSScriptRoot 'test-runtime-package.ps1') -PackageDirectory (Join-Path $output 'runtime')
    if (-not $env:CRIMSON_EXTRA_SOCKETS_SHA256 -or $env:CRIMSON_EXTRA_SOCKETS_SHA256 -notmatch '^[a-f0-9]{64}$') {
        throw 'The native build did not provide a verified ExtraSockets hash.'
    }
    if (-not $SkipChecks) {
        Invoke-Checked { npm test --prefix app }
        Invoke-Checked { npm run test:e2e --prefix app }
        Invoke-Checked { cargo fmt --all -- --check }
        Invoke-Checked { cargo test --workspace --locked --lib }
        Invoke-Checked { cargo clippy --workspace --all-targets --locked -- -D warnings }
    }
    $buildStarted = [DateTime]::UtcNow
    Push-Location (Join-Path $projectRoot 'app')
    try { Invoke-Checked { npm run desktop:installer } } finally { Pop-Location }
    $desktopExe = Join-Path $projectRoot 'target/release/crimson-workbench.exe'
    if (-not [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($desktopExe)).Contains($env:CRIMSON_EXTRA_SOCKETS_SHA256)) {
        throw 'The desktop binary does not embed this runtime package hash.'
    }
    $installers = @(Get-ChildItem -LiteralPath (Join-Path $projectRoot 'target/release/bundle/nsis') -Filter '*-setup.exe' |
        Where-Object { $_.LastWriteTimeUtc -ge $buildStarted -and $_.Name.Contains($version) })
    if ($installers.Count -ne 1) { throw 'Expected exactly one freshly built NSIS installer.' }
    Copy-Item -LiteralPath $installers[0].FullName -Destination (Join-Path $assets "Crimson-Workbench_${version}_x64-setup.exe")
    $portable = Join-Path $output 'portable'
    New-Item -ItemType Directory -Path $portable -Force | Out-Null
    Copy-Item -LiteralPath $desktopExe -Destination $portable
    foreach ($file in @('LICENSE', 'THIRD_PARTY_NOTICES.md', 'docs/INSTALLATION.md')) {
        Copy-Item -LiteralPath (Join-Path $projectRoot $file) -Destination $portable
    }
    Copy-Item -LiteralPath (Join-Path $projectRoot 'licenses') -Destination $portable -Recurse
    '@echo off', 'start "" "%~dp0crimson-workbench.exe"' | Set-Content -LiteralPath (Join-Path $portable 'Start-Workbench.cmd') -Encoding ascii
    Compress-Archive -Path (Join-Path $portable '*') -DestinationPath (Join-Path $assets "Crimson-Workbench_${version}_windows-x64-portable.zip")
    Compress-Archive -Path (Join-Path $output 'runtime/*') -DestinationPath (Join-Path $assets "Crimson-Workbench_${version}_optional-runtime-mods.zip")
    $checksums = @(Get-ChildItem -LiteralPath $assets -File | Sort-Object Name | ForEach-Object {
        '{0}  {1}' -f (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant(), $_.Name
    })
    $checksums | Set-Content -LiteralPath (Join-Path $assets 'SHA256SUMS.txt') -Encoding ascii
    [ordered]@{ version = $version; assets = $assets; nativeManifest = (Join-Path $output 'runtime/runtime-manifest.json'); checked = -not $SkipChecks } |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'build-result.json') -Encoding utf8
    Write-Output "Release assets: $assets"
} finally { Pop-Location }
