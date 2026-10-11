[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$PackageDirectory)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$package = (Resolve-Path -LiteralPath $PackageDirectory).ProviderPath
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('crimson-marketplace-test-' + [guid]::NewGuid().ToString('N'))
$fixture = Join-Path $testRoot 'package'
New-Item -ItemType Directory -Path $fixture -Force | Out-Null
Get-ChildItem -LiteralPath $package -Force | Copy-Item -Destination $fixture -Recurse
$scriptFile = Join-Path $fixture 'Marketplace-Mods.ps1'
$powershell = Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe'
function Assert([bool]$Condition, [string]$Message) { if (-not $Condition) { throw $Message } }
function Hash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
function Game([string]$Name) {
    $path = Join-Path $testRoot $Name
    New-Item -ItemType Directory -Path (Join-Path $path 'bin64') -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $path 'bin64/CrimsonDesert.exe'), 'synthetic, not game bytes')
    Copy-Item -LiteralPath (Join-Path $fixture 'loader/winmm.dll') -Destination (Join-Path $path 'bin64/winmm.dll')
    return $path
}
function Run([string]$Game, [string]$Mode = 'Status', [string]$Id = 'blackstar', [bool]$Success = $true) {
    $output = & $powershell -NoProfile -ExecutionPolicy Bypass -File $scriptFile -GameDirectory $Game -Mode $Mode -ModId $Id
    $code = $LASTEXITCODE
    $r = ($output -join "`n") | ConvertFrom-Json
    Assert (($code -eq 0) -eq $Success -and $r.ok -eq $Success) "Unexpected $Mode result: $output"
    return $r
}
$unsupported = Game 'unsupported'
Run $unsupported Install blackstar $false | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $unsupported 'bin64/CrimsonBlackstar.asi'))) 'Unsupported EXE was changed.'

# Only private temporary copies admit fake bytes and suppress the process list.
# Neither bypass is available in the shipped scripts.
$commonFile = Join-Path $fixture 'RuntimeMods.Common.ps1'
$common = [IO.File]::ReadAllText($commonFile)
$originalHash = '57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7'
$fakeHash = Hash (Join-Path $unsupported 'bin64/CrimsonDesert.exe')
$common = $common.Replace($originalHash, $fakeHash)
[IO.File]::WriteAllText($commonFile, $common)
$entry = [IO.File]::ReadAllText($scriptFile)
$processCall = 'Get-Process -Name CrimsonDesert -ErrorAction SilentlyContinue'
Assert $entry.Contains($processCall) 'Process fixture hook changed.'
$entry = $entry.Replace($processCall, '@()')
[IO.File]::WriteAllText($scriptFile, $entry)
$manifestFile = Join-Path $fixture 'marketplace-manifest.json'
$manifest = ([IO.File]::ReadAllText($manifestFile).Replace($originalHash, $fakeHash)) | ConvertFrom-Json
$legacy = [Text.Encoding]::ASCII.GetBytes('our synthetic older Blackstar module')
$sha = [Security.Cryptography.SHA256]::Create()
try { $legacyHash = ([BitConverter]::ToString($sha.ComputeHash($legacy))).Replace('-', '').ToLowerInvariant() } finally { $sha.Dispose() }
$mod = @($manifest.mods | Where-Object id -eq blackstar)[0]
$mod.priorHashes = @($legacyHash)
[IO.File]::WriteAllText($manifestFile, ($manifest | ConvertTo-Json -Depth 6))

$game = Game "friend's game & mods"
foreach ($id in @('blackstar', 'sleep_cooldown', 'sleep_duration')) { Run ('\\?\' + $game) Install $id | Out-Null }
$snapshot = (Run $game).snapshot
Assert (@($snapshot.mods | Where-Object state -eq installed).Count -eq 3) 'All three cards must report installed.'
Assert (@((Run ('\\?\' + $game)).snapshot.mods | Where-Object state -eq installed).Count -eq 3) 'Canonical Windows path failed.'
$receipt = Join-Path $game 'bin64/CrimsonWorkbench-mods/blackstar/receipt.json'
$receiptHash = Hash $receipt
Run $game Install blackstar | Out-Null
Assert ((Hash $receipt) -eq $receiptHash) 'Repeated installation changed ownership.'
[IO.File]::WriteAllText((Join-Path $game 'bin64/CrimsonExtraSockets.dat'), 'personal socket data')
Run $game Remove blackstar | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonBlackstar.asi'))) 'Remove did not disable Blackstar.'
Assert (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonSleepDuration.asi')) 'Remove touched another mod.'
Assert ([IO.File]::ReadAllText((Join-Path $game 'bin64/CrimsonExtraSockets.dat')) -eq 'personal socket data') 'Remove touched personal data.'
Run $game Install blackstar | Out-Null

$foreign = Game 'foreign'
$dest = Join-Path $foreign 'bin64/CrimsonBlackstar.asi'
[IO.File]::WriteAllText($dest, 'someone else mod')
$hash = Hash $dest
Run $foreign Install blackstar $false | Out-Null
Run $foreign Remove blackstar $false | Out-Null
Assert ((Hash $dest) -eq $hash) 'Foreign mod changed.'
Assert (@((Run $foreign).snapshot.mods | Where-Object { $_.id -eq 'blackstar' -and -not $_.can_install -and -not $_.can_remove }).Count -eq 1) 'Conflict did not block both actions.'

$import = Game 'import'
$dest = Join-Path $import 'bin64/CrimsonBlackstar.asi'
[IO.File]::WriteAllBytes($dest, $legacy)
Run $import Remove blackstar $false | Out-Null
Run $import Install blackstar | Out-Null
$store = Join-Path $import 'bin64/CrimsonWorkbench-mods/blackstar'
Assert ((Hash $dest) -eq $mod.sha256) 'Known legacy mod was not updated.'
Assert ((Hash (Join-Path $store ('original-' + $legacyHash + '.bak'))) -eq $legacyHash) 'Legacy module backup missing.'
[IO.File]::AppendAllText($dest, 'changed')
Run $import Remove blackstar $false | Out-Null
Assert (Test-Path -LiteralPath (Join-Path $store 'receipt.json')) 'Refused removal deleted ownership.'

# Force one failed payload copy after the old module has been moved.
$rollback = Game 'rollback'
$dest = Join-Path $rollback 'bin64/CrimsonBlackstar.asi'
[IO.File]::WriteAllBytes($dest, $legacy)
$fault = $common.Replace('Assert-PlainPath $Source', 'if (-not $script:CopyFailed -and $Destination.EndsWith("CrimsonBlackstar.asi")) { $script:CopyFailed = $true; throw "synthetic copy failure" }; Assert-PlainPath $Source')
$fault = '$script:CopyFailed = $false' + "`n" + $fault
[IO.File]::WriteAllText($commonFile, $fault)
Run $rollback Install blackstar $false | Out-Null
Assert ((Hash $dest) -eq $legacyHash) 'Failed update did not restore the original module.'
Assert (-not (Test-Path -LiteralPath (Join-Path $rollback 'bin64/CrimsonWorkbench-mods/blackstar/pending.json'))) 'Rollback left a pending transaction.'
[IO.File]::WriteAllText($commonFile, $common)

# Receipt publication fails after installing our new payload: no orphan ASI.
$receiptFailure = Game 'receipt-failure'
$fault = $common.Replace('function Write-NewChecked([byte[]]$Bytes, [string]$Destination) {', 'function Write-NewChecked([byte[]]$Bytes, [string]$Destination) { if ($Destination.EndsWith("receipt.json")) { throw "synthetic receipt failure" };')
[IO.File]::WriteAllText($commonFile, $fault)
Run $receiptFailure Install blackstar $false | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $receiptFailure 'bin64/CrimsonBlackstar.asi'))) 'Failed ownership publication left a new ASI.'
[IO.File]::WriteAllText($commonFile, $common)

# Crash after payload publication, before receipt: retry restores prior bytes.
$interrupted = Game 'interrupted'
$store = Join-Path $interrupted 'bin64/CrimsonWorkbench-mods/blackstar'
New-Item -ItemType Directory -Path $store -Force | Out-Null
[IO.File]::WriteAllBytes((Join-Path $store ('original-' + $legacyHash + '.bak')), $legacy)
[IO.File]::WriteAllBytes((Join-Path $store 'rollback.bin'), $legacy)
Copy-Item -LiteralPath (Join-Path $fixture 'addons/CrimsonBlackstar.asi') -Destination (Join-Path $interrupted 'bin64/CrimsonBlackstar.asi')
$journal = @{ schema = 1; id = 'blackstar'; operation = 'Install'; before = $legacyHash; after = $mod.sha256; receipt_before = $null }
[IO.File]::WriteAllText((Join-Path $store 'pending.json'), ($journal | ConvertTo-Json))
Run $interrupted Install blackstar | Out-Null
Assert ((Hash (Join-Path $interrupted 'bin64/CrimsonBlackstar.asi')) -eq $legacyHash) 'Interrupted install recovery did not restore prior bytes.'
Assert (-not (Test-Path -LiteralPath (Join-Path $store 'receipt.json'))) 'Recovery started a new installation without another click.'

foreach ($committed in @($false, $true)) {
    $crashRemove = Game ('remove-crash-' + $committed)
    Run $crashRemove Install blackstar | Out-Null
    $store = Join-Path $crashRemove 'bin64/CrimsonWorkbench-mods/blackstar'
    $dest = Join-Path $crashRemove 'bin64/CrimsonBlackstar.asi'
    $receipt = Join-Path $store 'receipt.json'
    $receiptHash = Hash $receipt
    Copy-Item -LiteralPath $receipt -Destination (Join-Path $store 'receipt-before.json')
    Copy-Item -LiteralPath $dest -Destination (Join-Path $store ('original-' + $mod.sha256 + '.bak'))
    [IO.File]::Move($dest, (Join-Path $store 'rollback.bin'))
    $journal = @{ schema = 1; id = 'blackstar'; operation = 'Remove'; before = $mod.sha256; after = $mod.sha256; receipt_before = $receiptHash }
    [IO.File]::WriteAllText((Join-Path $store 'pending.json'), ($journal | ConvertTo-Json))
    if ($committed) { Remove-Item -LiteralPath $receipt }
    Run $crashRemove Install blackstar | Out-Null
    Assert ((Test-Path -LiteralPath $dest) -ne $committed) 'Remove recovery did not respect its commit boundary.'
    Assert ((Test-Path -LiteralPath $receipt) -ne $committed) 'Remove recovery lost receipt consistency.'
}

$badReceipt = Game 'bad-receipt'
Run $badReceipt Install blackstar | Out-Null
$receipt = Join-Path $badReceipt 'bin64/CrimsonWorkbench-mods/blackstar/receipt.json'
$bad = Get-Content -LiteralPath $receipt -Raw | ConvertFrom-Json
$bad.file = '../outside.asi'
[IO.File]::WriteAllText($receipt, ($bad | ConvertTo-Json))
Run $badReceipt Remove blackstar $false | Out-Null
Assert (Test-Path -LiteralPath (Join-Path $badReceipt 'bin64/CrimsonBlackstar.asi')) 'Invalid receipt removed the module.'

$gameLock = [IO.File]::Open((Join-Path $game 'bin64/CrimsonDesert.exe'), [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
try { Run $game Remove sleep_duration $false | Out-Null } finally { $gameLock.Dispose() }
Assert (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonSleepDuration.asi')) 'Exclusive game guard failed.'

# Running game disables actions and production mutation entry points refuse it.
$running = $entry.Replace('[bool]@(@()).Count', '$true').Replace('if (@(@()).Count)', 'if ($true)')
[IO.File]::WriteAllText($scriptFile, $running)
Run $game Remove sleep_duration $false | Out-Null
Assert ((Run $game).snapshot.game_running) 'Running-game status not reported.'
[IO.File]::WriteAllText($scriptFile, $entry)

# Both source integrity and fixed receipt paths are checked before mutation.
[IO.File]::AppendAllText((Join-Path $fixture 'addons/CrimsonSleepCooldown.asi'), 'tampered')
Run $game Install sleep_cooldown $false | Out-Null
Copy-Item -LiteralPath (Join-Path $package 'addons/CrimsonSleepCooldown.asi') -Destination (Join-Path $fixture 'addons/CrimsonSleepCooldown.asi') -Force
Assert ((Run $game).ok) 'Status did not recover after the synthetic package was restored.'
Write-Host "PASS: three module lifecycles, Windows PowerShell paths, legacy backups, foreign/modified refusal, copy and receipt rollback, interruption recovery, running-game and tampered-package refusal. Evidence: $testRoot"
# Expected child refusals are asserted above, not this test script's outcome.
$global:LASTEXITCODE = 0
