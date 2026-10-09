[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$GameDirectory,
    [switch]$InstallLoader
)
. (Join-Path $PSScriptRoot 'RuntimeMods.Common.ps1')
$manifest = Read-RuntimeManifest $PSScriptRoot
$bin = Get-CheckedGameDirectory $GameDirectory
$guard = Open-GameGuard $bin -RequireSupported
$created = [Collections.Generic.List[object]]::new()
try {
    $receiptPath = Join-Path $bin $script:ReceiptName
    Assert-PlainPath $receiptPath
    if (Test-Path -LiteralPath $receiptPath) { throw 'Workbench runtime already has an installation receipt. Uninstall that package first; personal socket data is preserved.' }
    $files = @($manifest.files | Where-Object { $_.name -like '*.asi' })
    $loader = @($manifest.files | Where-Object { $_.name -eq 'loader/winmm.dll' })[0]
    $loaderPath = Join-Path $bin 'winmm.dll'
    Assert-PlainPath $loaderPath
    if (Test-Path -LiteralPath $loaderPath) {
        if ((Get-RuntimeHash $loaderPath) -ne $loader.sha256) { throw 'An existing different winmm.dll was found. It will not be changed. Resolve loader compatibility manually first.' }
    } elseif ($InstallLoader) {
        $proxies = @('dinput8.dll', 'version.dll', 'dxgi.dll', 'd3d9.dll', 'd3d10.dll', 'd3d11.dll', 'd3d12.dll', 'dsound.dll', 'winhttp.dll', 'wininet.dll', 'binkw64.dll', 'bink2w64.dll', 'xinput1_1.dll', 'xinput1_2.dll', 'xinput1_3.dll', 'xinput1_4.dll', 'xinput9_1_0.dll', 'xinputuap.dll')
        foreach ($proxy in $proxies) {
            if (Test-Path -LiteralPath (Join-Path $bin $proxy)) { throw "Existing proxy DLL $proxy needs a manual compatibility check before adding another ASI loader." }
        }
        $files += $loader
    } else { throw 'No supported winmm.dll loader is installed. Use -InstallLoader to install the bundled Ultimate ASI Loader.' }
    # Preflight every destination before writing the first byte.
    foreach ($file in $files) {
        $destination = Join-Path $bin ([IO.Path]::GetFileName($file.name))
        Assert-PlainPath $destination
        if (Test-Path -LiteralPath $destination) { throw "Existing file will not be overwritten: $destination" }
    }
    $configPath = Join-Path $bin 'CrimsonExtraSockets.dat'
    Assert-PlainPath $configPath
    $empty = @($manifest.files | Where-Object { $_.name -eq 'CrimsonExtraSockets.dat' })[0]
    # An existing personal configuration is always preserved.
    if (-not (Test-Path -LiteralPath $configPath)) { $files += $empty }
    foreach ($file in $files) {
        $name = [IO.Path]::GetFileName($file.name)
        $destination = Join-Path $bin $name
        Copy-NewChecked (Join-Path $PSScriptRoot $file.name) $destination $file.sha256
        $created.Add([ordered]@{ name = $name; sha256 = $file.sha256 })
    }
    $receipt = [ordered]@{ schema = 1; version = $manifest.version; files = @($created | Where-Object { $_.name -ne 'CrimsonExtraSockets.dat' }) }
    $bytes = [Text.UTF8Encoding]::new($false).GetBytes(($receipt | ConvertTo-Json -Depth 5))
    Write-NewChecked $bytes $receiptPath
    Write-Host 'Live Items and Extra Sockets installed. Existing personal socket data was preserved.'
    Write-Host 'This experimental runtime supports game build 1.0.0.2976 only; in-game acceptance on another PC is still required.'
} catch {
    # Roll back only newly created, byte-identical files from this attempt.
    foreach ($file in $created) {
        $path = Join-Path $bin $file.name
        if ((Test-Path -LiteralPath $path) -and (Get-RuntimeHash $path) -eq $file.sha256) { Remove-Item -LiteralPath $path }
    }
    throw
} finally { $guard.Dispose() }
