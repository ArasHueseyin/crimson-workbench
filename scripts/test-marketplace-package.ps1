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
$expectedIds = @('blackstar', 'sleep_cooldown', 'sleep_duration', 'no_fall', 'player_stamina', 'mount_jump', 'player_jump', 'player_speed', 'preserve_consumables', 'mount_protection', 'free_crafting_materials', 'force_palm', 'force_palm_height', 'axiom_reach', 'glide_speed', 'mount_dismount', 'auto_loot', 'world_controls', 'photo_mode', 'teleport')
$published = Get-Content -LiteralPath (Join-Path $package 'marketplace-manifest.json') -Raw | ConvertFrom-Json
$baseManifest = Get-Content -LiteralPath (Join-Path $package 'runtime-manifest.json') -Raw | ConvertFrom-Json
Assert ($published.schema -eq 1 -and $published.version -eq $baseManifest.version) 'Base and optional package versions must agree.'
Assert (@($published.mods).Count -eq 20 -and @($published.mods.id | Select-Object -Unique).Count -eq 20) 'Exactly twenty distinct optional modules must be packaged.'
Assert (@(Compare-Object $expectedIds @($published.mods.id)).Count -eq 0) 'Optional module IDs do not match the shipped cards.'
Assert (@($published.mods.file | Select-Object -Unique).Count -eq 20) 'Optional payload filenames must be unique.'
$archiveLedger = Get-Content -LiteralPath (Join-Path $package 'archive-mods-manifest.json') -Raw | ConvertFrom-Json
$archiveHelper = Join-Path $package 'CrimsonArchiveMods.exe'
Assert ($archiveLedger.schema -eq 1 -and $archiveLedger.file -eq 'CrimsonArchiveMods.exe' -and (Hash $archiveHelper) -eq $archiveLedger.sha256 -and (Get-Item -LiteralPath $archiveHelper).Length -eq $archiveLedger.bytes) 'Independent archive installer is missing or damaged.'
foreach ($entry in $published.mods) {
    Assert ($entry.file -cmatch '^Crimson[A-Za-z]+\.asi$') 'Optional payload must be a fixed ASI basename.'
    $payload = Join-Path $package ('addons/' + $entry.file)
    Assert ($entry.sha256 -cmatch '^[a-f0-9]{64}$' -and (Hash $payload) -eq $entry.sha256 -and (Get-Item -LiteralPath $payload).Length -eq $entry.bytes) "Invalid optional payload metadata: $($entry.id)"
    foreach ($prior in @($entry.priorHashes)) { Assert ($prior -cmatch '^[a-f0-9]{64}$') "Invalid previous-module hash: $($entry.id)" }
}
# Recognize distributed local-test modules and the published 0.9.1 glide ASI
# as known older modules, including builds produced on a fresh runner.
$knownPrevious = @{
    player_stamina = 'bbab63542f81c570116163d6381cb88af07ee353448c977554faa0f8f18b8d82'
    player_jump = '6a021b1d1e9faa8441605df304bddb953985b2c556211191f0e4e65f4dbe21ec'
    force_palm_height = '13b287af5246d83a6f73872d3a1da357d692caad1d318e02d60b1788ad083da3'
    axiom_reach = '0610e54c48812f8042349a7974f95f76062886831fe5d7d70fa1fb76e7068450'
    glide_speed = @('fbc4f4c084400859cf3ea0f07b55fd57594ac431173eb8c8e9eeaa8d58cbb059', '2c6bdd2adb3e014ef9a5049b3c9cef7eea4c2a0afbd946a09e70725914a46af8', 'afc8d4320a4cae92490e264a17e27d77d8096d71372ba6f1d6212aea930e1987', '9916efa51896537294058fed49347700f4ce413e6652fc9cfe3a273cd04b48e3')
}
foreach ($id in $knownPrevious.Keys) {
    $entry = @($published.mods | Where-Object id -eq $id)[0]
    foreach ($previous in @($knownPrevious[$id])) {
        Assert ($entry.sha256 -eq $previous -or @($entry.priorHashes) -contains $previous) "Known previous module is missing from the upgrade ledger: $id"
    }
}
function Game([string]$Name) {
    $path = Join-Path $testRoot $Name
    New-Item -ItemType Directory -Path (Join-Path $path 'bin64') -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $path 'bin64/CrimsonDesert.exe'), 'synthetic, not game bytes')
    Copy-Item -LiteralPath (Join-Path $fixture 'loader/winmm.dll') -Destination (Join-Path $path 'bin64/winmm.dll')
    return $path
}
function Run([string]$Game, [string]$Mode = 'Status', [string]$Id = 'blackstar', [bool]$Success = $true, [switch]$Elevate) {
    $arguments = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $scriptFile, '-GameDirectory', $Game, '-Mode', $Mode, '-ModId', $Id)
    if ($Elevate) { $arguments += '-Elevate' }
    $output = & $powershell @arguments
    $code = $LASTEXITCODE
    $r = ($output -join "`n") | ConvertFrom-Json
    Assert (($code -eq 0) -eq $Success -and $r.ok -eq $Success) "Unexpected $Mode result: $output"
    return $r
}
$unsupported = Game 'unsupported'
Run $unsupported Install blackstar $false | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $unsupported 'bin64/CrimsonBlackstar.asi'))) 'Unsupported EXE was changed.'
Run $unsupported Install region_return_30min $false | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $unsupported '9000'))) 'Region installer changed an unsupported game.'

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
$runtimeManifestFile = Join-Path $fixture 'runtime-manifest.json'
[IO.File]::WriteAllText($runtimeManifestFile, [IO.File]::ReadAllText($runtimeManifestFile).Replace($originalHash, $fakeHash))

$game = Game "friend's game & mods"
Run $game Install photo_mode | Out-Null
Assert (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonPhotoMode.asi')) 'Photo mode unexpectedly depends on the task broker.'
Run $game Remove photo_mode | Out-Null
$worldBefore = @((Run $game).snapshot.mods | Where-Object id -eq world_controls)[0]
Assert (-not $worldBefore.can_install) 'Live controls admitted a missing task broker.'
Run $game Install world_controls $false | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonWorldControls.asi'))) 'Missing broker installed world controls.'
Run $game Install teleport $false | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonTeleport.asi'))) 'Missing broker installed teleport.'
[IO.File]::WriteAllText((Join-Path $game 'bin64/CrimsonLiveItems.asi'), 'synthetic old broker without observer ABI')
Run $game Install world_controls $false | Out-Null
Remove-Item -LiteralPath (Join-Path $game 'bin64/CrimsonLiveItems.asi')
Copy-Item -LiteralPath (Join-Path $fixture 'CrimsonLiveItems.asi') -Destination (Join-Path $game 'bin64/CrimsonLiveItems.asi')
foreach ($id in $expectedIds) { Run ('\\?\' + $game) Install $id | Out-Null }
$snapshot = (Run $game).snapshot
Assert (@($snapshot.mods | Where-Object state -eq installed).Count -eq 20) 'All twenty runtime cards must report installed.'
Assert (@((Run ('\\?\' + $game)).snapshot.mods | Where-Object state -eq installed).Count -eq 20) 'Canonical Windows path failed.'
Run $game Remove photo_mode | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonPhotoMode.asi'))) 'Photo-mode removal left its module.'
Assert (@((Run $game).snapshot.mods | Where-Object state -eq installed).Count -eq 19) 'Photo-mode removal touched another runtime mod.'
Run $game Install photo_mode | Out-Null
Run $game Remove teleport | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonTeleport.asi'))) 'Teleport removal left its module.'
Assert (@((Run $game).snapshot.mods | Where-Object state -eq installed).Count -eq 19) 'Teleport removal touched another runtime mod.'
Assert ((Hash (Join-Path $game 'bin64/CrimsonLiveItems.asi')) -eq (Hash (Join-Path $fixture 'CrimsonLiveItems.asi'))) 'Teleport removal touched the task broker.'
Run $game Install teleport | Out-Null
Run $game Remove world_controls | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonWorldControls.asi'))) 'World-control removal left its module.'
Assert (@((Run $game).snapshot.mods | Where-Object state -eq installed).Count -eq 19) 'World-control removal touched another runtime mod.'
Assert ((Hash (Join-Path $game 'bin64/CrimsonLiveItems.asi')) -eq (Hash (Join-Path $fixture 'CrimsonLiveItems.asi'))) 'World-control removal touched the task broker.'
Run $game Install world_controls | Out-Null
Run $game Remove auto_loot | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonAutoLoot.asi'))) 'Auto-loot removal left its module.'
Assert (@((Run $game).snapshot.mods | Where-Object state -eq installed).Count -eq 19) 'Auto-loot removal touched another runtime mod.'
Assert (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonNoFall.asi')) 'Auto-loot removal touched fall protection.'
Run $game Install auto_loot | Out-Null
Run $game Remove mount_dismount | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonMountDismount.asi'))) 'Mount dismount removal left its module.'
Assert (@((Run $game).snapshot.mods | Where-Object state -eq installed).Count -eq 19) 'Mount dismount removal touched another runtime mod.'
Assert (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonMountJump.asi')) 'Mount dismount removal touched jump height.'
Assert (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonMountProtection.asi')) 'Mount dismount removal touched mount protection.'
Run $game Install mount_dismount | Out-Null
Run $game Remove force_palm | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonForcePalm.asi'))) 'Force Palm removal left its module.'
Assert (@((Run $game).snapshot.mods | Where-Object state -eq installed).Count -eq 19) 'Force Palm removal touched another runtime mod.'
Run $game Install force_palm | Out-Null
Run $game Remove force_palm_height | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonForcePalmHeight.asi'))) 'Force Palm height removal left its module.'
Assert (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonForcePalm.asi')) 'Height removal touched Force Palm chain limits.'
Assert (@((Run $game).snapshot.mods | Where-Object state -eq installed).Count -eq 19) 'Height removal touched another runtime mod.'
Run $game Install force_palm_height | Out-Null
Run $game Remove axiom_reach | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonAxiomReach.asi'))) 'Axiom removal left its module.'
Assert (@((Run $game).snapshot.mods | Where-Object state -eq installed).Count -eq 19) 'Axiom removal touched another runtime mod.'
Run $game Install axiom_reach | Out-Null
Run $game Remove glide_speed | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonGlideSpeed.asi'))) 'Glide removal left its module.'
Assert (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonPlayerSpeed.asi')) 'Glide removal touched ground movement.'
Assert (@((Run $game).snapshot.mods | Where-Object state -eq installed).Count -eq 19) 'Glide removal touched another runtime mod.'
Run $game Install glide_speed | Out-Null
Run $game Remove free_crafting_materials | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonFreeCrafting.asi'))) 'Crafting removal left its module.'
Assert (@((Run $game).snapshot.mods | Where-Object state -eq installed).Count -eq 19) 'Crafting removal touched another runtime mod.'
Run $game Install free_crafting_materials | Out-Null
Run $game Remove mount_protection | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonMountProtection.asi'))) 'Mount protection removal left its module.'
Assert (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonNoFall.asi')) 'Mount protection removal touched fall protection.'
Assert (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonPlayerStamina.asi')) 'Mount protection removal touched player stamina.'
Run $game Install mount_protection | Out-Null
Run $game Remove preserve_consumables | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonPreserveConsumables.asi'))) 'Consumable removal left its module.'
Assert (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonPlayerStamina.asi')) 'Consumable removal touched player stamina.'
Assert (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonPlayerSpeed.asi')) 'Consumable removal touched movement speed.'
Run $game Install preserve_consumables | Out-Null
Run $game Remove player_speed | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonPlayerSpeed.asi'))) 'Player speed removal left its module.'
Assert (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonPlayerJump.asi')) 'Player speed removal touched player jump.'
Assert (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonMountJump.asi')) 'Player speed removal touched mount jumps.'
Run $game Install player_speed | Out-Null
Run $game Remove player_jump | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonPlayerJump.asi'))) 'Player jump removal left its module.'
Assert (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonMountJump.asi')) 'Player jump removal touched mount jumps.'
Run $game Install player_jump | Out-Null
Run $game Remove mount_jump | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonMountJump.asi'))) 'Mount jump removal left its module.'
Run $game Install mount_jump | Out-Null
Run $game Remove player_stamina | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonPlayerStamina.asi'))) 'Player stamina removal left its module.'
Run $game Install player_stamina | Out-Null
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
$running = $entry.Replace('@(@()).Count', '1')
[IO.File]::WriteAllText($scriptFile, $running)
Run $game Remove sleep_duration $false | Out-Null
Assert ((Run $game).snapshot.game_running) 'Running-game status not reported.'
[IO.File]::WriteAllText($scriptFile, $entry)

# Exercise the real parent/worker argument and result handoff in an owned game.
# The private function replaces only UAC with a hidden ordinary child process;
# it never elevates, loads an ASI, or targets the real game installation.
$workerMock = @'
function Start-Process {
    param([string]$FilePath, [string]$ArgumentList, [string]$Verb, [string]$WindowStyle, [switch]$Wait, [switch]$PassThru)
    if ($Verb -ne 'RunAs' -or $WindowStyle -ne 'Hidden' -or -not $ArgumentList.Contains('-WindowStyle Hidden') -or -not $ArgumentList.Contains('-NonInteractive')) { throw 'Worker must explicitly start hidden and noninteractive.' }
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $FilePath
    $start.Arguments = $ArgumentList
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.EnvironmentVariables.Remove('PSModulePath')
    $worker = [Diagnostics.Process]::Start($start)
    $null = $worker.Handle
    return $worker
}
'@
$commonImport = ". (Join-Path `$PSScriptRoot 'RuntimeMods.Common.ps1')"
Assert ($entry.Contains($commonImport)) 'Worker fixture import changed.'
$workerEntry = $entry.Replace($commonImport, ($commonImport + "`n" + $workerMock))
[IO.File]::WriteAllText($scriptFile, $workerEntry)
$workerGame = Game "hidden worker's game & mods"
Run $workerGame Install mount_dismount $true -Elevate | Out-Null
Assert (@((Run $workerGame).snapshot.mods | Where-Object { $_.id -eq 'mount_dismount' -and $_.state -eq 'installed' }).Count -eq 1) 'Worker installation did not reach installed status.'
Run $workerGame Remove mount_dismount $true -Elevate | Out-Null
Assert (-not (Test-Path -LiteralPath (Join-Path $workerGame 'bin64/CrimsonMountDismount.asi'))) 'Worker removal left its module.'

$unicodeFailure = $workerEntry.Replace('function Read-MarketplaceManifest {', "function Read-MarketplaceManifest { throw ('Unicode worker error: ' + [char]0xdc + 'berpr' + [char]0xfc + 'fung failed.');")
[IO.File]::WriteAllText($scriptFile, $unicodeFailure)
$failedWorker = Run $workerGame Install mount_dismount $false -Elevate
Assert ($failedWorker.message -eq "Unicode worker error: $([char]0xdc)berpr$([char]0xfc)fung failed.") 'Worker error text was not decoded as UTF-8.'

# A stale enabled button must not even request UAC when the game is running.
$noWorker = $running.Replace($commonImport, ($commonImport + "`nfunction Start-Process { throw 'Unexpected elevation request.' }"))
[IO.File]::WriteAllText($scriptFile, $noWorker)
$blockedWorker = Run $workerGame Install mount_dismount $false -Elevate
Assert ($blockedWorker.message.StartsWith('Close Crimson Desert')) 'Running-game refusal happened after requesting elevation.'
[IO.File]::WriteAllText($scriptFile, $entry)

# Both source integrity and fixed receipt paths are checked before mutation.
[IO.File]::AppendAllText((Join-Path $fixture 'addons/CrimsonSleepCooldown.asi'), 'tampered')
Run $game Install sleep_cooldown $false | Out-Null
Copy-Item -LiteralPath (Join-Path $package 'addons/CrimsonSleepCooldown.asi') -Destination (Join-Path $fixture 'addons/CrimsonSleepCooldown.asi') -Force
Assert ((Run $game).ok) 'Status did not recover after the synthetic package was restored.'
Write-Host "PASS: all packaged module lifecycles, Windows PowerShell paths, legacy backups, foreign/modified refusal, copy and receipt rollback, interruption recovery, running-game and tampered-package refusal. Evidence: $testRoot"
# Expected child refusals are asserted above, not this test script's outcome.
$global:LASTEXITCODE = 0
