[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$GameDirectory)
. (Join-Path $PSScriptRoot 'RuntimeMods.Common.ps1')
$manifest = Read-RuntimeManifest $PSScriptRoot
$bin = Get-CheckedGameDirectory $GameDirectory
$guard = Open-GameGuard $bin
try {
    $receiptPath = Join-Path $bin $script:ReceiptName
    Assert-PlainPath $receiptPath
    $receipt = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json
    if ($receipt.schema -ne 1 -or @($receipt.files).Count -lt 2 -or @($receipt.files).Count -gt 3) { throw 'Invalid installation receipt.' }
    $seen = @{}
    foreach ($file in $receipt.files) {
        if ($file.name -notin $script:OwnedNames -or $seen.ContainsKey($file.name)) { throw 'Unexpected or duplicate receipt entry.' }
        $seen[$file.name] = $true
        $known = @($manifest.files | Where-Object { [IO.Path]::GetFileName($_.name) -eq $file.name })
        if ($known.Count -ne 1 -or $file.sha256 -ne $known[0].sha256) { throw 'Use the original runtime package to uninstall this version.' }
        $path = Join-Path $bin $file.name
        Assert-PlainPath $path
        if ((Test-Path -LiteralPath $path) -and (Get-RuntimeHash $path) -ne $file.sha256) { throw "Modified runtime file will not be removed: $path" }
    }
    foreach ($file in $receipt.files) {
        $path = Join-Path $bin $file.name
        if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path }
    }
    Remove-Item -LiteralPath $receiptPath
    Write-Host 'Owned, unchanged runtime files removed. CrimsonExtraSockets.dat and runtime logs were preserved.'
} finally { $guard.Dispose() }
