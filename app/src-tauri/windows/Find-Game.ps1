$ErrorActionPreference = 'Stop'
# Read-only discovery. Multiple installations require an explicit selection.
$steamRoots = @(
    (Get-ItemProperty 'HKCU:\Software\Valve\Steam' -ErrorAction SilentlyContinue).SteamPath
    (Join-Path ${env:ProgramFiles(x86)} 'Steam')
) | Where-Object { $_ } | Select-Object -Unique
$libraries = @($steamRoots)
foreach ($root in $steamRoots) {
    $vdf = Join-Path $root 'steamapps/libraryfolders.vdf'
    if (Test-Path -LiteralPath $vdf) {
        foreach ($line in Get-Content -LiteralPath $vdf) {
            if ($line -match '^\s*"path"\s+"([^"]+)"') {
                $libraries += $Matches[1].Replace('\\', '\')
            }
        }
    }
}
$games = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($library in $libraries) {
    $game = Join-Path $library 'steamapps/common/Crimson Desert'
    if (Test-Path -LiteralPath (Join-Path $game 'bin64/CrimsonDesert.exe') -PathType Leaf) {
        $null = $games.Add((Get-Item -LiteralPath $game).FullName)
    }
}
if ($games.Count -eq 1) { foreach ($game in $games) { Write-Output $game } }
