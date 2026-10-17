[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$GameDirectory,
    [ValidateSet('Status', 'Install', 'Remove')][string]$Mode = 'Status',
    [ValidateSet('blackstar', 'sleep_cooldown', 'sleep_duration', 'no_fall', 'player_stamina', 'mount_jump', 'player_jump', 'player_speed', 'preserve_consumables', 'mount_protection', 'free_crafting_materials', 'force_palm', 'force_palm_height', 'axiom_reach', 'glide_speed', 'mount_dismount', 'auto_loot', 'world_controls', 'photo_mode', 'teleport')][string]$ModId,
    [switch]$Elevate,
    [string]$ResultPath
)
. (Join-Path $PSScriptRoot 'RuntimeMods.Common.ps1')
[Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
if ($GameDirectory.StartsWith('\\?\UNC\', [StringComparison]::OrdinalIgnoreCase)) { $GameDirectory = '\\' + $GameDirectory.Substring(8) }
elseif ($GameDirectory.StartsWith('\\?\')) { $GameDirectory = $GameDirectory.Substring(4) }

function Read-MarketplaceManifest {
    $path = Join-Path $PSScriptRoot 'marketplace-manifest.json'
    Assert-PlainPath $path
    $manifest = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    $names = @{ blackstar = 'CrimsonBlackstar.asi'; sleep_cooldown = 'CrimsonSleepCooldown.asi'; sleep_duration = 'CrimsonSleepDuration.asi'; no_fall = 'CrimsonNoFall.asi'; player_stamina = 'CrimsonPlayerStamina.asi'; mount_jump = 'CrimsonMountJump.asi'; player_jump = 'CrimsonPlayerJump.asi'; player_speed = 'CrimsonPlayerSpeed.asi'; preserve_consumables = 'CrimsonPreserveConsumables.asi'; mount_protection = 'CrimsonMountProtection.asi'; free_crafting_materials = 'CrimsonFreeCrafting.asi'; force_palm = 'CrimsonForcePalm.asi'; force_palm_height = 'CrimsonForcePalmHeight.asi'; axiom_reach = 'CrimsonAxiomReach.asi'; glide_speed = 'CrimsonGlideSpeed.asi'; mount_dismount = 'CrimsonMountDismount.asi'; auto_loot = 'CrimsonAutoLoot.asi'; world_controls = 'CrimsonWorldControls.asi'; photo_mode = 'CrimsonPhotoMode.asi'; teleport = 'CrimsonTeleport.asi' }
    if ($manifest.schema -ne 1 -or $manifest.gameExeSha256 -ne $script:ExpectedGameHash -or @($manifest.mods).Count -ne $names.Count) { throw 'Invalid marketplace package.' }
    $seen = @{}
    foreach ($mod in $manifest.mods) {
        if (-not $names.ContainsKey($mod.id) -or $seen.ContainsKey($mod.id) -or $mod.file -ne $names[$mod.id] -or $mod.sha256 -cnotmatch '^[a-f0-9]{64}$') { throw 'Invalid marketplace module entry.' }
        $seen[$mod.id] = $true
        foreach ($hash in $mod.priorHashes) { if ($hash -cnotmatch '^[a-f0-9]{64}$') { throw 'Invalid previous module fingerprint.' } }
        $source = Join-Path $PSScriptRoot ('addons/' + $mod.file)
        Assert-PlainPath $source
        if ((Get-RuntimeHash $source) -ne $mod.sha256 -or (Get-Item -LiteralPath $source).Length -ne $mod.bytes) { throw "Marketplace package is damaged: $($mod.file)" }
    }
    return $manifest
}
function Allowed-Hash($Mod, [string]$Hash) { return $Hash -eq $Mod.sha256 -or $Hash -in @($Mod.priorHashes) }
function Test-WorldBroker([string]$Bin) {
    $runtime = Read-RuntimeManifest $PSScriptRoot
    $entry = @($runtime.files | Where-Object name -eq 'CrimsonLiveItems.asi')[0]
    $installed = Join-Path $Bin 'CrimsonLiveItems.asi'
    Assert-PlainPath $installed
    return (Test-Path -LiteralPath $installed -PathType Leaf) -and (Get-RuntimeHash $installed) -eq $entry.sha256
}
function Read-ModReceipt($Mod, [string]$Path) {
    Assert-PlainPath $Path
    if (-not (Test-Path -LiteralPath $Path)) { return $null }
    $r = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    if ($r.schema -ne 1 -or $r.id -ne $Mod.id -or $r.file -ne $Mod.file -or -not (Allowed-Hash $Mod $r.sha256)) { throw 'Invalid or unrecognized mod receipt. No files will be changed.' }
    return $r
}
function Remove-Checked([string]$Path, [string]$Hash) {
    Assert-PlainPath $Path
    if (Test-Path -LiteralPath $Path) {
        if ((Get-RuntimeHash $Path) -ne $Hash) { throw "Changed file will not be removed: $Path" }
        Remove-Item -LiteralPath $Path
    }
}
function Finish-ModTransaction($Mod, [string]$Bin, [string]$Store) {
    $pending = Join-Path $Store 'pending.json'
    Assert-PlainPath $pending
    if (-not (Test-Path -LiteralPath $pending)) { return }
    $pendingHash = Get-RuntimeHash $pending
    $p = Get-Content -LiteralPath $pending -Raw | ConvertFrom-Json
    if ($p.schema -ne 1 -or $p.id -ne $Mod.id -or $p.operation -notin @('Install', 'Remove') -or $p.after -ne $Mod.sha256 -or ($p.before -and -not (Allowed-Hash $Mod $p.before)) -or ($p.receipt_before -and $p.receipt_before -cnotmatch '^[a-f0-9]{64}$')) { throw 'Invalid pending mod transaction.' }
    $dest = Join-Path $Bin $Mod.file
    $rollback = Join-Path $Store 'rollback.bin'
    $receipt = Join-Path $Store 'receipt.json'
    $oldReceipt = Join-Path $Store 'receipt-before.json'
    foreach ($path in @($dest, $rollback, $receipt, $oldReceipt)) { Assert-PlainPath $path }
    $r = Read-ModReceipt $Mod $receipt
    $current = if (Test-Path -LiteralPath $dest) { Get-RuntimeHash $dest } else { $null }
    $committed = if ($p.operation -eq 'Install') { $r -and $r.sha256 -eq $p.after -and $current -eq $p.after } else { -not $r -and -not $current }
    if (-not $committed) {
        # Roll back only bytes named in this fixed-path, hash-checked journal.
        if ($current -and $current -ne $p.before) { Remove-Checked $dest $p.after; $current = $null }
        if ($p.before -and -not $current) {
            $backup = Join-Path $Store ('original-' + $p.before + '.bak')
            Assert-PlainPath $backup
            Copy-NewChecked $backup $dest $p.before
        }
        if ($p.receipt_before) {
            Read-ModReceipt $Mod $oldReceipt | Out-Null
            if ((Get-RuntimeHash $oldReceipt) -ne $p.receipt_before) { throw 'Previous receipt backup changed.' }
            if (Test-Path -LiteralPath $receipt) { Remove-Checked $receipt (Get-RuntimeHash $receipt) }
            Copy-NewChecked $oldReceipt $receipt $p.receipt_before
        } elseif ($r) { Remove-Checked $receipt (Get-RuntimeHash $receipt) }
    }
    if (Test-Path -LiteralPath $rollback) { Remove-Checked $rollback $p.before }
    if (Test-Path -LiteralPath $oldReceipt) { Remove-Checked $oldReceipt $p.receipt_before }
    Remove-Checked $pending $pendingHash
}
function Get-ModSnapshot($Manifest, [string]$Bin) {
    $running = [bool]@(Get-Process -Name CrimsonDesert -ErrorAction SilentlyContinue).Count
    $loader = Join-Path $Bin 'winmm.dll'
    Assert-PlainPath $loader
    $loaderReady = (Test-Path -LiteralPath $loader) -and (Get-RuntimeHash $loader) -eq $script:ExpectedLoaderHash
    $items = @()
    foreach ($mod in $Manifest.mods) {
        $state = 'not_installed'; $message = 'Not installed'; $canInstall = $loaderReady; $canRemove = $false
        try {
            $dest = Join-Path $Bin $mod.file
            $store = Join-Path $Bin ('CrimsonWorkbench-mods/' + $mod.id)
            foreach ($path in @($dest, $store, (Join-Path $store 'pending.json'))) { Assert-PlainPath $path }
            $r = Read-ModReceipt $mod (Join-Path $store 'receipt.json')
            if (Test-Path -LiteralPath (Join-Path $store 'pending.json')) {
                $state = 'pending'; $message = 'Interrupted operation. Close the game and retry to recover.'; $canInstall = $true
            } elseif (Test-Path -LiteralPath $dest) {
                $hash = Get-RuntimeHash $dest
                if (-not (Allowed-Hash $mod $hash) -or ($r -and $r.sha256 -ne $hash)) { throw 'Existing module is modified or unknown. It will not be overwritten or removed.' }
                if ($r) {
                    $state = if ($hash -eq $mod.sha256) { 'installed' } else { 'update_available' }
                    $message = if ($state -eq 'installed') { 'Installed - loads at the next game start' } else { 'Known older version - update available' }
                    $canRemove = $true; $canInstall = $state -ne 'installed' -and $loaderReady
                } else { $state = 'unmanaged'; $message = 'Your known existing mod can be imported and updated. Its original file will be backed up.' }
            } elseif ($r) { throw 'Managed module is missing. Restore it before changing ownership.' }
            if (-not $loaderReady -and $state -eq 'not_installed') { $message = 'Install the bundled game modules with Workbench setup first.' }
            if ($mod.id -in @('world_controls', 'teleport') -and $state -notin @('pending', 'conflict') -and -not (Test-WorldBroker $Bin)) {
                $canInstall = $false
                $message = 'Update the bundled game modules with this Workbench setup first. Live controls require its matching LiveItems runtime.'
            }
        } catch { $state = 'conflict'; $message = $_.Exception.Message; $canInstall = $false; $canRemove = $false }
        $items += [ordered]@{ id = $mod.id; state = $state; message = $message; can_install = $canInstall -and -not $running; can_remove = $canRemove -and -not $running }
    }
    return [ordered]@{ mods = $items; game_running = $running }
}
function Change-MarketplaceMod($Manifest, [string]$Bin) {
    if (@(Get-Process -Name CrimsonDesert -ErrorAction SilentlyContinue).Count) { throw 'Close Crimson Desert before installing or removing a mod.' }
    $mod = @($Manifest.mods | Where-Object { $_.id -eq $ModId })[0]
    $guard = Open-GameGuard $Bin -RequireSupported:($Mode -eq 'Install')
    $mutex = $null
    try {
        $store = Join-Path $Bin ('CrimsonWorkbench-mods/' + $mod.id)
        Assert-PlainPath $store
        New-Item -ItemType Directory -Path $store -Force | Out-Null
        $lock = Join-Path $store 'lock'
        Assert-PlainPath $lock
        $mutex = [IO.File]::Open($lock, [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
        $wasPending = Test-Path -LiteralPath (Join-Path $store 'pending.json')
        Finish-ModTransaction $mod $Bin $store
        if ($wasPending) { return }
        $dest = Join-Path $Bin $mod.file
        $receipt = Join-Path $store 'receipt.json'
        $pending = Join-Path $store 'pending.json'
        $oldReceipt = Join-Path $store 'receipt-before.json'
        $rollback = Join-Path $store 'rollback.bin'
        foreach ($path in @($dest, $receipt, $pending, $oldReceipt, $rollback)) { Assert-PlainPath $path }
        $r = Read-ModReceipt $mod $receipt
        $before = if (Test-Path -LiteralPath $dest) { Get-RuntimeHash $dest } else { $null }
        if ($before -and (-not (Allowed-Hash $mod $before) -or ($r -and $r.sha256 -ne $before))) { throw 'Unknown or modified existing mod will not be changed.' }
        if ($r -and -not $before) { throw 'Managed module is missing.' }
        if ($Mode -eq 'Remove' -and -not $r) { throw 'Only a mod imported or installed through this marketplace can be removed.' }
        if ($Mode -eq 'Install') {
            $loader = Join-Path $Bin 'winmm.dll'
            Assert-PlainPath $loader
            if (-not (Test-Path -LiteralPath $loader) -or (Get-RuntimeHash $loader) -ne $script:ExpectedLoaderHash) { throw 'Install the bundled game modules with Workbench setup first. Existing loaders are preserved.' }
            if ($mod.id -in @('world_controls', 'teleport') -and -not (Test-WorldBroker $Bin)) { throw 'Live controls require the matching LiveItems runtime. Update the bundled game modules with this Workbench setup first.' }
            if ($r -and $before -eq $mod.sha256) { return }
        }
        if ($before) {
            $backup = Join-Path $store ('original-' + $before + '.bak')
            Assert-PlainPath $backup
            if (Test-Path -LiteralPath $backup) { if ((Get-RuntimeHash $backup) -ne $before) { throw 'Original module backup changed.' } }
            else { Copy-NewChecked $dest $backup $before }
        }
        $receiptBeforeHash = $null
        if ($r) { $receiptBeforeHash = Get-RuntimeHash $receipt; Copy-NewChecked $receipt $oldReceipt $receiptBeforeHash }
        $journal = [ordered]@{ schema = 1; id = $mod.id; operation = $Mode; before = $before; after = $mod.sha256; receipt_before = $receiptBeforeHash }
        try { Write-NewChecked ([Text.UTF8Encoding]::new($false).GetBytes(($journal | ConvertTo-Json))) $pending }
        catch { if ($receiptBeforeHash) { Remove-Checked $oldReceipt $receiptBeforeHash }; throw }
        try {
            if ($Mode -eq 'Remove' -or ($before -and $before -ne $mod.sha256)) { [IO.File]::Move($dest, $rollback) }
            if ($r) { Remove-Checked $receipt $receiptBeforeHash }
            if ($Mode -eq 'Install') {
                if (-not (Test-Path -LiteralPath $dest)) { Copy-NewChecked (Join-Path $PSScriptRoot ('addons/' + $mod.file)) $dest $mod.sha256 }
                $new = [ordered]@{ schema = 1; id = $mod.id; file = $mod.file; sha256 = $mod.sha256; version = $Manifest.version }
                Write-NewChecked ([Text.UTF8Encoding]::new($false).GetBytes(($new | ConvertTo-Json))) $receipt
            }
        } finally { Finish-ModTransaction $mod $Bin $store }
    } finally { if ($mutex) { $mutex.Dispose() }; $guard.Dispose() }
}

try {
    if ($Mode -ne 'Status' -and -not $ModId) { throw 'Select a marketplace mod.' }
    # Refuse before requesting UAC or opening a worker. The exclusive game
    # guard in Change-MarketplaceMod still covers a start after this check.
    if ($Mode -ne 'Status' -and @(Get-Process -Name CrimsonDesert -ErrorAction SilentlyContinue).Count) { throw 'Close Crimson Desert before installing or removing a mod. Nothing was changed.' }
    if ($Elevate -and $Mode -ne 'Status') {
        if ($GameDirectory.Contains('"') -or $GameDirectory.Contains("`r") -or $GameDirectory.Contains("`n")) { throw 'Invalid game path.' }
        $result = Join-Path ([IO.Path]::GetTempPath()) ('crimson-marketplace-' + [guid]::NewGuid().ToString('N') + '.json')
        Assert-PlainPath $result
        $powershell = Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe'
        $arguments = '-NoProfile -NonInteractive -ExecutionPolicy Bypass -WindowStyle Hidden -File "{0}" -GameDirectory "{1}" -Mode {2} -ModId {3} -ResultPath "{4}"' -f $PSCommandPath, $GameDirectory.TrimEnd('\'), $Mode, $ModId, $result
        try {
            $worker = Start-Process -FilePath $powershell -ArgumentList $arguments -Verb RunAs -WindowStyle Hidden -PassThru
            $null = $worker.Handle
            $worker.WaitForExit()
            $worker.Refresh()
            if (-not (Test-Path -LiteralPath $result)) { throw 'Mod installation was canceled or the worker could not start.' }
            Assert-PlainPath $result
            $output = Get-Content -LiteralPath $result -Raw -Encoding UTF8
            Write-Output $output
            exit $worker.ExitCode
        } finally { if (Test-Path -LiteralPath $result) { Remove-Item -LiteralPath $result } }
    }
    $game = (Resolve-Path -LiteralPath $GameDirectory).ProviderPath
    $bin = Join-Path $game 'bin64'
    Assert-PlainPath $bin
    if (-not (Test-Path -LiteralPath (Join-Path $bin 'CrimsonDesert.exe') -PathType Leaf)) { throw 'Select the game installation folder containing bin64.' }
    $manifest = Read-MarketplaceManifest
    if ($Mode -ne 'Status') { Change-MarketplaceMod $manifest $bin }
    $response = [ordered]@{ ok = $true; snapshot = Get-ModSnapshot $manifest $bin }
    $exitCode = 0
} catch { $response = [ordered]@{ ok = $false; message = $_.Exception.Message }; $exitCode = 1 }
$json = $response | ConvertTo-Json -Depth 8 -Compress
if ($ResultPath) { Write-NewChecked ([Text.UTF8Encoding]::new($false).GetBytes($json)) $ResultPath }
else { Write-Output $json }
exit $exitCode
