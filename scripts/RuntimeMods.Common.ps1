Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$script:ExpectedGameHash = '57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7'
$script:ExpectedLoaderHash = '031a3e5576d91dce1e438d36b9a3d462c7334ab4791990a8ff1e3ddc0e132daf'
$script:ExpectedEmptyConfigHash = '05182fc0e1c71ec38f90aca073d8b67f6f351ccc78286f7055e6602f929b2849'
$script:ReceiptName = 'CrimsonWorkbench.runtime-install.json'
$script:OwnedNames = @('CrimsonLiveItems.asi', 'CrimsonExtraSockets.asi', 'winmm.dll')
function Get-RuntimeHash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
function Assert-PlainPath([string]$Path) {
    $part = [IO.Path]::GetFullPath($Path)
    while ($part) {
        if (Test-Path -LiteralPath $part) {
            $item = Get-Item -LiteralPath $part -Force
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Links/junctions are not allowed: $part" }
        }
        $parent = Split-Path -Parent $part
        if ($parent -eq $part) { break }
        $part = $parent
    }
}
function Get-CheckedGameDirectory([string]$GameDirectory) {
    $resolved = (Resolve-Path -LiteralPath $GameDirectory).ProviderPath
    Assert-PlainPath $resolved
    $bin = Join-Path $resolved 'bin64'
    Assert-PlainPath $bin
    if (-not (Test-Path -LiteralPath (Join-Path $bin 'CrimsonDesert.exe') -PathType Leaf)) {
        throw 'Choose the game root containing bin64/CrimsonDesert.exe.'
    }
    if (@(Get-Process -Name CrimsonDesert, crimson-workbench, crimson-workbench-live -ErrorAction SilentlyContinue).Count) {
        throw 'Close Crimson Desert and Crimson Workbench before changing runtime files.'
    }
    return $bin
}
function Open-GameGuard([string]$Bin, [switch]$RequireSupported) {
    $exe = Join-Path $Bin 'CrimsonDesert.exe'
    Assert-PlainPath $exe
    # No sharing: an active game prevents this lock; the lock prevents a start
    # while any runtime file is being installed or removed.
    $guard = [IO.File]::Open($exe, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::None)
    try {
        if ($RequireSupported) {
            $sha = [Security.Cryptography.SHA256]::Create()
            try { $hash = [BitConverter]::ToString($sha.ComputeHash($guard)).Replace('-', '').ToLowerInvariant() } finally { $sha.Dispose() }
            if ($hash -ne $script:ExpectedGameHash) { throw 'Unsupported game EXE: runtime modules require the verified 1.0.0.2976 build.' }
        }
        return $guard
    } catch { $guard.Dispose(); throw }
}
function Read-RuntimeManifest([string]$PackageDirectory) {
    $path = Join-Path $PackageDirectory 'runtime-manifest.json'
    Assert-PlainPath $path
    $manifest = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    $expected = @('CrimsonLiveItems.asi', 'CrimsonExtraSockets.asi', 'CrimsonExtraSockets.dat', 'loader/winmm.dll')
    if ($manifest.schema -ne 1 -or $manifest.gameExeSha256 -ne $script:ExpectedGameHash -or @($manifest.files).Count -ne 4) { throw 'Invalid runtime manifest.' }
    if (@(Compare-Object -ReferenceObject $expected -DifferenceObject @($manifest.files.name)).Count) { throw 'Unexpected runtime manifest paths.' }
    foreach ($entry in $manifest.files) {
        $file = Join-Path $PackageDirectory $entry.name
        Assert-PlainPath $file
        if ($entry.sha256 -notmatch '^[0-9a-f]{64}$' -or (Get-RuntimeHash $file) -ne $entry.sha256 -or (Get-Item -LiteralPath $file).Length -ne $entry.bytes) { throw "Runtime package hash mismatch: $($entry.name)" }
        if ($entry.name -eq 'loader/winmm.dll' -and $entry.sha256 -ne $script:ExpectedLoaderHash) { throw 'Unexpected loader version.' }
        if ($entry.name -eq 'CrimsonExtraSockets.dat' -and $entry.sha256 -ne $script:ExpectedEmptyConfigHash) { throw 'Package must contain an empty V2 configuration, never personal records.' }
    }
    return $manifest
}
function Publish-NewChecked([IO.Stream]$InputStream, [string]$Destination, [string]$Sha256) {
    Assert-PlainPath $Destination
    if (Test-Path -LiteralPath $Destination) { throw "Existing file will not be overwritten: $Destination" }
    $temporary = $Destination + '.' + [guid]::NewGuid().ToString('N') + '.installing'
    $createdTemporary = $false
    try {
        $outputFile = [IO.File]::Open($temporary, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
        $createdTemporary = $true
        try { $InputStream.CopyTo($outputFile); $outputFile.Flush($true) } finally { $outputFile.Dispose() }
        if ((Get-RuntimeHash $temporary) -ne $Sha256) { throw "Staged file hash mismatch: $Destination" }
        # Same-directory, no-replace rename publishes only complete verified
        # bytes. Nothing after successful publication can fail before return.
        [IO.File]::Move($temporary, $Destination)
    } finally {
        if ($createdTemporary -and (Test-Path -LiteralPath $temporary)) { Remove-Item -LiteralPath $temporary }
    }
}
function Copy-NewChecked([string]$Source, [string]$Destination, [string]$Sha256) {
    Assert-PlainPath $Source
    $inputFile = [IO.File]::Open($Source, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
    try { Publish-NewChecked $inputFile $Destination $Sha256 } finally { $inputFile.Dispose() }
}
function Write-NewChecked([byte[]]$Bytes, [string]$Destination) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { $digest = [BitConverter]::ToString($sha.ComputeHash($Bytes)).Replace('-', '').ToLowerInvariant() } finally { $sha.Dispose() }
    $inputStream = [IO.MemoryStream]::new($Bytes, $false)
    try { Publish-NewChecked $inputStream $Destination $digest } finally { $inputStream.Dispose() }
}
