[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$GameDirectory,
    [switch]$Elevate,
    [switch]$ShowErrors
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
try {
    . (Join-Path $PSScriptRoot 'RuntimeMods.Common.ps1')
    $manifest = Read-RuntimeManifest $PSScriptRoot
    $bin = Get-CheckedGameDirectory $GameDirectory
    $guard = Open-GameGuard $bin -RequireSupported
    try {
        # Re-running the same setup is harmless. A different/modified package is
        # never overwritten; removal still requires its original receipt/package.
        $receiptPath = Join-Path $bin $script:ReceiptName
        Assert-PlainPath $receiptPath
        if (Test-Path -LiteralPath $receiptPath) {
            $receipt = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json
            if ($receipt.schema -ne 1 -or @($receipt.files).Count -lt 2 -or @($receipt.files).Count -gt 3) { throw 'Invalid runtime installation receipt.' }
            $seen = @{}
            foreach ($entry in $receipt.files) {
                if ($entry.name -notin $script:OwnedNames -or $seen.ContainsKey($entry.name)) { throw 'Invalid runtime receipt entry.' }
                $seen[$entry.name] = $true
                $known = @($manifest.files | Where-Object { [IO.Path]::GetFileName($_.name) -eq $entry.name })
                if ($known.Count -ne 1 -or $entry.sha256 -ne $known[0].sha256) {
                    throw 'Another runtime version is installed. Remove it with its original package before installing this version.'
                }
            }
            if (-not $seen.ContainsKey('CrimsonLiveItems.asi') -or -not $seen.ContainsKey('CrimsonExtraSockets.asi')) { throw 'Incomplete runtime receipt.' }
            foreach ($entry in $manifest.files | Where-Object { $_.name -ne 'CrimsonExtraSockets.dat' }) {
                $installed = Join-Path $bin ([IO.Path]::GetFileName($entry.name))
                Assert-PlainPath $installed
                if (-not (Test-Path -LiteralPath $installed -PathType Leaf) -or (Get-RuntimeHash $installed) -ne $entry.sha256) {
                    throw "Installed runtime file is missing or changed: $installed"
                }
            }
            $config = Join-Path $bin 'CrimsonExtraSockets.dat'
            Assert-PlainPath $config
            if (-not (Test-Path -LiteralPath $config -PathType Leaf)) { throw 'Runtime configuration is missing.' }
            Write-Output 'Live Items and Extra Sockets are already installed and match this package.'
            exit 0
        }
    } finally { $guard.Dispose() }

    if ($Elevate -and -not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        # Windows paths cannot contain quotes. Use -File arguments, never embed
        # a game path in executable PowerShell source or a shell command string.
        $gamePath = [IO.Path]::GetFullPath($GameDirectory).TrimEnd('\', '/')
        if ($gamePath.Contains('"') -or $gamePath.Contains("`r") -or $gamePath.Contains("`n")) { throw 'Invalid game directory.' }
        $arguments = '-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File "' + $PSCommandPath + '" -GameDirectory "' + $gamePath + '" -ShowErrors'
        $powershell = Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe'
        $worker = Start-Process -FilePath $powershell -ArgumentList $arguments -Verb RunAs -WindowStyle Hidden -PassThru
        $null = $worker.Handle
        $worker.WaitForExit()
        $worker.Refresh()
        exit $worker.ExitCode
    }
    & (Join-Path $PSScriptRoot 'Install-RuntimeMods.ps1') -GameDirectory $GameDirectory -InstallLoader
    exit 0
} catch {
    $message = $_.Exception.Message
    if ($ShowErrors) {
        Add-Type -AssemblyName System.Windows.Forms
        [Windows.Forms.MessageBox]::Show("Die Spielmodule wurden nicht installiert. / Runtime installation failed.`r`n`r`n$message", 'Crimson Workbench', 'OK', 'Error') | Out-Null
    }
    [Console]::Error.WriteLine($message)
    exit 1
}
