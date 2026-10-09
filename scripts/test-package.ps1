#Requires -Version 7.4
[CmdletBinding()]
param(
    [string]$AssetsDirectory,
    [string]$InstallerPath,
    [string]$PortablePath,
    [switch]$AllowInstall,
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not $IsWindows) { throw 'Windows x64 is required for package smoke tests.' }
if ($AssetsDirectory) {
    $assets = (Resolve-Path -LiteralPath $AssetsDirectory).Path
    $setups = @(Get-ChildItem -LiteralPath $assets -File -Filter '*_x64-setup.exe')
    $zips = @(Get-ChildItem -LiteralPath $assets -File -Filter '*_windows-x64-portable.zip')
    if ($setups.Count -ne 1 -or $zips.Count -ne 1) { throw 'Expected one installer and one portable archive in the assets directory.' }
    $InstallerPath = $setups[0].FullName
    $PortablePath = $zips[0].FullName
    if (-not $OutputDirectory) { $OutputDirectory = Join-Path (Split-Path $assets -Parent) 'package-test' }
}
if (-not $InstallerPath -and -not $PortablePath) { throw 'Provide -InstallerPath and/or -PortablePath.' }
if ($InstallerPath -and (-not $AllowInstall -or $env:GITHUB_ACTIONS -ne 'true' -or $env:RUNNER_ENVIRONMENT -ne 'github-hosted' -or $env:RUNNER_OS -ne 'Windows')) {
    throw 'Installer tests require -AllowInstall on a disposable GitHub-hosted Windows runner. Use -PortablePath locally.'
}
$repo = Split-Path $PSScriptRoot -Parent
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('crimson-package-test-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRoot | Out-Null
$output = if ($OutputDirectory) { [IO.Path]::GetFullPath($OutputDirectory) } else { Join-Path $testRoot 'results' }
New-Item -ItemType Directory -Path $output -Force | Out-Null

function Wait-Exit([Diagnostics.Process]$Process, [int]$Seconds, [string]$Label) {
    if (-not $Process.WaitForExit($Seconds * 1000)) {
        Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue
        throw "$Label timed out after $Seconds seconds."
    }
    $Process.Refresh()
    if ($Process.ExitCode -ne 0) { throw "$Label exited with code $($Process.ExitCode)." }
}

function Test-App([string]$AppDirectory, [string]$Kind) {
    $executables = @(Get-ChildItem -LiteralPath $AppDirectory -Recurse -File -Filter 'crimson-workbench.exe')
    if ($executables.Count -ne 1) { throw "Expected exactly one packaged Workbench executable ($Kind)." }
    $exe = $executables[0]
    foreach ($required in @('LICENSE', 'THIRD_PARTY_NOTICES.md', 'licenses', 'INSTALLATION.md')) {
        if (-not (Test-Path -LiteralPath (Join-Path $exe.DirectoryName $required))) { throw "Missing packaged resource: $required ($Kind)." }
    }
    $private = Join-Path $testRoot $Kind
    $project = Join-Path $private 'localappdata/CrimsonWorkbench'
    New-Item -ItemType Directory -Path $project -Force | Out-Null
    $settingsPath = Join-Path $project 'settings.json'
    '{"version":1,"game_dir":null,"save_dir":null,"language":"eng"}' | Set-Content -LiteralPath $settingsPath -Encoding utf8NoBOM
    $sentinel = Join-Path $project 'keep-after-uninstall.txt'
    [guid]::NewGuid().ToString('N') | Set-Content -LiteralPath $sentinel -Encoding utf8NoBOM
    $sentinelHash = (Get-FileHash -LiteralPath $sentinel -Algorithm SHA256).Hash
    $listener = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback, 0)
    $listener.Start()
    $port = $listener.LocalEndpoint.Port
    $listener.Stop()
    $environment = @{
        LOCALAPPDATA = (Join-Path $private 'localappdata')
        APPDATA = (Join-Path $private 'appdata')
        CD_WORKBENCH_PROJECT = $null
        CD_PACKAGE_PROJECT = $project
        CD_GAME_DIR = (Join-Path $private 'NO-GAME-INSTALLATION')
        CD_SAVE_DIR = (Join-Path $private 'NO-GAME-SAVES')
        WEBVIEW2_USER_DATA_FOLDER = (Join-Path $private 'webview2')
        WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS = "--remote-debugging-port=$port"
        CD_PACKAGE_CDP = "http://127.0.0.1:$port"
        CD_SMOKE_OUTPUT = (Join-Path $output $Kind)
    }
    $previous = @{}
    foreach ($key in $environment.Keys) {
        $previous[$key] = [Environment]::GetEnvironmentVariable($key, 'Process')
        [Environment]::SetEnvironmentVariable($key, $environment[$key], 'Process')
    }
    $appProcess = $null
    try {
        $appProcess = Start-Process -FilePath $exe.FullName -ArgumentList '--background-test' -WorkingDirectory $private -WindowStyle Hidden -PassThru
        & node (Join-Path $repo 'app/scripts/package-smoke.mjs') | ForEach-Object { Write-Host $_ }
        if ($LASTEXITCODE -ne 0) { throw "Packaged UI smoke failed ($Kind)." }
    } finally {
        if ($appProcess -and -not $appProcess.HasExited) {
            Stop-Process -Id $appProcess.Id -Force
            $appProcess.WaitForExit(15000) | Out-Null
        }
        foreach ($key in $environment.Keys) { [Environment]::SetEnvironmentVariable($key, $previous[$key], 'Process') }
    }
    if (Test-Path -LiteralPath $environment.CD_GAME_DIR) { throw 'Test unexpectedly created a game directory.' }
    if (Test-Path -LiteralPath $environment.CD_SAVE_DIR) { throw 'Test unexpectedly created a save directory.' }
    return @{ project = $project; localAppData = $environment.LOCALAPPDATA; settings = $settingsPath; settingsHash = (Get-FileHash -LiteralPath $settingsPath -Algorithm SHA256).Hash; sentinel = $sentinel; sentinelHash = $sentinelHash; exe = $exe.FullName }
}

$report = [ordered]@{ testRoot = $testRoot; output = $output; installer = $null; portable = $null }
if ($InstallerPath) {
    $installer = (Resolve-Path -LiteralPath $InstallerPath).Path
    & (Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe') -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'test-installer-page.ps1') -InstallerPath $installer
    if ($LASTEXITCODE -ne 0) { throw 'Native installer page test failed.' }
    $destination = Join-Path $testRoot 'installed-app'
    # NSIS requires /D to be last and unquoted. Our generated directory has no shell expansion.
    $install = Start-Process -FilePath $installer -ArgumentList "/S /D=$destination" -WindowStyle Hidden -PassThru
    Wait-Exit $install 240 'Silent installation'
    $installed = Test-App $destination 'installed'
    $bundledRuntime = Join-Path (Split-Path $installed.exe -Parent) 'runtime-mods'
    foreach ($name in @('Setup-RuntimeMods.ps1', 'Install-RuntimeMods.ps1', 'Uninstall-RuntimeMods.ps1', 'RuntimeMods.Common.ps1', 'runtime-manifest.json', 'CrimsonLiveItems.asi', 'CrimsonExtraSockets.asi', 'loader/winmm.dll')) {
        if (-not (Test-Path -LiteralPath (Join-Path $bundledRuntime $name) -PathType Leaf)) { throw "Setup is missing bundled runtime file: $name" }
    }
    # Exercise the actual installed resources through private synthetic fixtures.
    & (Join-Path $PSScriptRoot 'test-runtime-package.ps1') -PackageDirectory $bundledRuntime
    # Drive the real install hook with explicit opt-in, but an unsupported fake
    # game. Its nonzero exit proves the bundled helper ran and refused writes.
    $fakeGame = Join-Path $testRoot "unsupported friend's game"
    New-Item -ItemType Directory -Path (Join-Path $fakeGame 'bin64') | Out-Null
    [IO.File]::WriteAllText((Join-Path $fakeGame 'bin64/CrimsonDesert.exe'), 'synthetic unsupported game')
    $refused = Start-Process -FilePath $installer -ArgumentList "/S /INSTALLMODS /GAMEDIR=`"$fakeGame`" /D=$destination" -WindowStyle Hidden -PassThru
    $null = $refused.Handle
    if (-not $refused.WaitForExit(120000)) { Stop-Process -Id $refused.Id -Force; throw 'Runtime setup refusal timed out.' }
    $refused.Refresh()
    if ($refused.ExitCode -ne 1) { throw "Expected runtime setup refusal (1), got $($refused.ExitCode)." }
    if (@(Get-ChildItem -LiteralPath (Join-Path $fakeGame 'bin64') -Force).Count -ne 1) { throw 'Refused setup modified the fake game.' }
    $uninstallers = @(Get-ChildItem -LiteralPath $destination -Recurse -File -Filter '*uninstall*.exe')
    if ($uninstallers.Count -ne 1) { throw 'Expected exactly one NSIS uninstaller.' }
    # _?= avoids a detached temp uninstaller, so waiting observes actual completion.
    $uninstall = Start-Process -FilePath $uninstallers[0].FullName -ArgumentList "/S _?=$destination" -Environment @{ LOCALAPPDATA = $installed.localAppData } -WindowStyle Hidden -PassThru
    Wait-Exit $uninstall 120 'Silent uninstallation'
    if (Test-Path -LiteralPath $installed.exe) { throw 'Uninstall left the application executable installed.' }
    if ((Get-FileHash -LiteralPath $installed.settings -Algorithm SHA256).Hash -ne $installed.settingsHash -or
        (Get-FileHash -LiteralPath $installed.sentinel -Algorithm SHA256).Hash -ne $installed.sentinelHash) {
        throw 'Uninstall changed or deleted the isolated user data.'
    }
    $report.installer = @{ passed = $true; uninstalled = $true; userDataPreserved = $true }
}
if ($PortablePath) {
    # When both artifacts are supplied, the installer first provisions WebView2.
    # Its uninstaller preserves that shared runtime for the portable app.
    $zip = (Resolve-Path -LiteralPath $PortablePath).Path
    $destination = Join-Path $testRoot 'portable-app'
    Expand-Archive -LiteralPath $zip -DestinationPath $destination
    $portable = Test-App $destination 'portable'
    $report.portable = @{ passed = $true; executable = $portable.exe }
}
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $output 'package-result.json') -Encoding utf8NoBOM
Write-Host "PASS: packaged Workbench tests. Evidence: $output"
# Keep the unique temporary directory as evidence; no recursive deletion is performed.
