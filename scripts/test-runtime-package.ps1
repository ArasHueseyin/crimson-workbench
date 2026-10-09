[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$PackageDirectory)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$package = (Resolve-Path -LiteralPath $PackageDirectory).ProviderPath
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('crimson-runtime-fixture-' + [guid]::NewGuid().ToString('N'))
$fixturePackage = Join-Path $testRoot 'package'
New-Item -ItemType Directory -Path $fixturePackage -Force | Out-Null
Get-ChildItem -LiteralPath $package -Force | Copy-Item -Destination $fixturePackage -Recurse
function Assert-True([bool]$Condition, [string]$Message) { if (-not $Condition) { throw $Message } }
function Assert-Refused([scriptblock]$Action, [string]$Message) {
    $failed = $false
    try { & $Action | Out-Null } catch { $failed = $true }
    if (-not $failed) { throw "Expected refusal: $Message" }
}
function Hash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
function New-Fixture([string]$Name) {
    $root = Join-Path $testRoot $Name
    New-Item -ItemType Directory -Path (Join-Path $root 'bin64') -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $root 'bin64/CrimsonDesert.exe'), 'synthetic test bytes; this is not a game executable')
    return $root
}
function Assert-OnlyFakeExe([string]$Game) {
    $files = @(Get-ChildItem -LiteralPath (Join-Path $Game 'bin64') -Force)
    Assert-True ($files.Count -eq 1 -and $files[0].Name -eq 'CrimsonDesert.exe') 'Unexpected output remained after refusal/rollback.'
}
$unsupported = New-Fixture 'unsupported'
# Original production guard refuses our fake EXE even when real game processes
# exist elsewhere. No real game path or process is opened by this test.
. (Join-Path $fixturePackage 'RuntimeMods.Common.ps1')
Read-RuntimeManifest $fixturePackage | Out-Null
Assert-Refused { $lock = Open-GameGuard (Join-Path $unsupported 'bin64') -RequireSupported; $lock.Dispose() } 'production EXE admission'
Assert-OnlyFakeExe $unsupported

# For transaction tests ONLY, private temporary copies use our fake EXE hash
# and an empty process inventory. Production scripts expose neither bypass.
$originalGameHash = $script:ExpectedGameHash
$fakeHash = Hash (Join-Path $unsupported 'bin64/CrimsonDesert.exe')
$commonPath = Join-Path $fixturePackage 'RuntimeMods.Common.ps1'
$common = [IO.File]::ReadAllText($commonPath).Replace($originalGameHash, $fakeHash)
$processCall = 'Get-Process -Name CrimsonDesert, crimson-workbench, crimson-workbench-live -ErrorAction SilentlyContinue'
Assert-True $common.Contains($processCall) 'Fixture process-inventory hook changed.'
$common = $common.Replace($processCall, '@()')
[IO.File]::WriteAllText($commonPath, $common)
$manifestPath = Join-Path $fixturePackage 'runtime-manifest.json'
[IO.File]::WriteAllText($manifestPath, [IO.File]::ReadAllText($manifestPath).Replace($originalGameHash, $fakeHash))
$install = Join-Path $fixturePackage 'Install-RuntimeMods.ps1'
$uninstall = Join-Path $fixturePackage 'Uninstall-RuntimeMods.ps1'

$fresh = New-Fixture 'fresh'
& $install -GameDirectory $fresh -InstallLoader
$bin = Join-Path $fresh 'bin64'
foreach ($name in @('CrimsonLiveItems.asi', 'CrimsonExtraSockets.asi', 'CrimsonExtraSockets.dat', 'winmm.dll', 'CrimsonWorkbench.runtime-install.json')) {
    Assert-True (Test-Path -LiteralPath (Join-Path $bin $name)) "Missing installed $name"
}
$data = Join-Path $bin 'CrimsonExtraSockets.dat'
Assert-True ((Hash $data) -eq $script:ExpectedEmptyConfigHash) 'New config is not the empty V2 format.'
[IO.File]::AppendAllText($data, 'personal preservation sentinel')
$personalHash = Hash $data
& $uninstall -GameDirectory $fresh
Assert-True ((Hash $data) -eq $personalHash) 'Uninstall changed personal config.'
Assert-True (@(Get-ChildItem -LiteralPath $bin -File).Count -eq 2) 'Uninstall left owned runtime files.'

$preserve = New-Fixture 'preserve'
$bin = Join-Path $preserve 'bin64'
Copy-Item -LiteralPath (Join-Path $package 'loader/winmm.dll') -Destination (Join-Path $bin 'winmm.dll')
[IO.File]::WriteAllText((Join-Path $bin 'CrimsonExtraSockets.dat'), 'personal existing data')
$personalHash = Hash (Join-Path $bin 'CrimsonExtraSockets.dat')
& $install -GameDirectory $preserve
Assert-True ((Hash (Join-Path $bin 'CrimsonExtraSockets.dat')) -eq $personalHash) 'Install changed existing config.'
& $uninstall -GameDirectory $preserve
Assert-True (Test-Path -LiteralPath (Join-Path $bin 'winmm.dll')) 'Uninstall removed a pre-existing loader.'
Assert-True ((Hash (Join-Path $bin 'CrimsonExtraSockets.dat')) -eq $personalHash) 'Uninstall changed existing config.'

$conflict = New-Fixture 'conflict'
$bin = Join-Path $conflict 'bin64'
[IO.File]::WriteAllText((Join-Path $bin 'CrimsonExtraSockets.asi'), 'other mod')
$otherHash = Hash (Join-Path $bin 'CrimsonExtraSockets.asi')
Assert-Refused { & $install -GameDirectory $conflict -InstallLoader } 'existing ASI'
Assert-True ((Hash (Join-Path $bin 'CrimsonExtraSockets.asi')) -eq $otherHash) 'An existing ASI was modified.'
Assert-True (@(Get-ChildItem -LiteralPath $bin -File).Count -eq 2) 'Conflict preflight wrote additional files.'

$loaderConflict = New-Fixture 'loader-conflict'
$bin = Join-Path $loaderConflict 'bin64'
[IO.File]::WriteAllText((Join-Path $bin 'winmm.dll'), 'other loader')
Assert-Refused { & $install -GameDirectory $loaderConflict -InstallLoader } 'existing different loader'
Assert-True ([IO.File]::ReadAllText((Join-Path $bin 'winmm.dll')) -eq 'other loader') 'An existing loader was modified.'
Assert-True (@(Get-ChildItem -LiteralPath $bin -File).Count -eq 2) 'Loader preflight wrote additional files.'

$modified = New-Fixture 'modified-uninstall'
& $install -GameDirectory $modified -InstallLoader
$bin = Join-Path $modified 'bin64'
[IO.File]::AppendAllText((Join-Path $bin 'CrimsonExtraSockets.asi'), 'changed')
Assert-Refused { & $uninstall -GameDirectory $modified } 'modified file uninstall'
Assert-True (@(Get-ChildItem -LiteralPath $bin -File).Count -eq 6) 'Refused uninstall removed other files.'

$proxyConflict = New-Fixture 'proxy-conflict'
$bin = Join-Path $proxyConflict 'bin64'
[IO.File]::WriteAllText((Join-Path $bin 'dinput8.dll'), 'existing proxy loader')
Assert-Refused { & $install -GameDirectory $proxyConflict -InstallLoader } 'existing alternate loader proxy'
Assert-True (@(Get-ChildItem -LiteralPath $bin -File).Count -eq 2) 'Alternate loader preflight wrote additional files.'

$copyFailure = New-Fixture 'copy-rollback'
$faulty = $common.Replace('Assert-PlainPath $Source', 'if ($Destination.EndsWith("CrimsonExtraSockets.asi")) { throw "synthetic second-copy failure" }; Assert-PlainPath $Source')
[IO.File]::WriteAllText($commonPath, $faulty)
Assert-Refused { & $install -GameDirectory $copyFailure -InstallLoader } 'second-copy failure'
Assert-OnlyFakeExe $copyFailure

$receiptFailure = New-Fixture 'receipt-rollback'
$faulty = $common.Replace('function Write-NewChecked([byte[]]$Bytes, [string]$Destination) {', 'function Write-NewChecked([byte[]]$Bytes, [string]$Destination) { throw "synthetic receipt failure";')
[IO.File]::WriteAllText($commonPath, $faulty)
Assert-Refused { & $install -GameDirectory $receiptFailure -InstallLoader } 'receipt failure'
Assert-OnlyFakeExe $receiptFailure
[IO.File]::WriteAllText($commonPath, $common)

# A stream that fails after writing bytes tests cleanup of partial staging.
if (-not ('CrimsonPartialReadFixture' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.IO;
public sealed class CrimsonPartialReadFixture : Stream {
    bool read;
    public override bool CanRead { get { return true; } }
    public override bool CanSeek { get { return false; } }
    public override bool CanWrite { get { return false; } }
    public override long Length { get { throw new NotSupportedException(); } }
    public override long Position { get { throw new NotSupportedException(); } set { throw new NotSupportedException(); } }
    public override int Read(byte[] buffer, int offset, int count) {
        if (read) throw new IOException("Synthetic interrupted source stream");
        read = true; buffer[offset] = 42; return 1;
    }
    public override void Flush() { }
    public override long Seek(long a, SeekOrigin b) { throw new NotSupportedException(); }
    public override void SetLength(long n) { throw new NotSupportedException(); }
    public override void Write(byte[] b, int o, int n) { throw new NotSupportedException(); }
}
'@
}
. $commonPath
$stageRoot = Join-Path $testRoot 'partial-stage'
New-Item -ItemType Directory -Path $stageRoot | Out-Null
$stream = New-Object CrimsonPartialReadFixture
try { Assert-Refused { Publish-NewChecked $stream (Join-Path $stageRoot 'partial.dat') ('0' * 64) } 'partial source read' } finally { $stream.Dispose() }
Assert-True (@(Get-ChildItem -LiteralPath $stageRoot -Force).Count -eq 0) 'Partial staging leaked a file.'
$stream = [IO.MemoryStream]::new([byte[]]@(1, 2, 3), $false)
try { Assert-Refused { Publish-NewChecked $stream (Join-Path $stageRoot 'bad-hash.dat') ('0' * 64) } 'staging hash mismatch' } finally { $stream.Dispose() }
Assert-True (@(Get-ChildItem -LiteralPath $stageRoot -Force).Count -eq 0) 'Hash mismatch leaked a file.'

$tampered = New-Fixture 'tampered-package'
[IO.File]::AppendAllText((Join-Path $fixturePackage 'CrimsonLiveItems.asi'), 'tampered')
Assert-Refused { & $install -GameDirectory $tampered -InstallLoader } 'tampered package bytes'
Assert-OnlyFakeExe $tampered
Write-Host "PASS: runtime install/uninstall, DAT preservation, foreign loader/ASI refusal, tamper refusal, partial-copy cleanup and transaction rollback. Synthetic evidence: $testRoot"
