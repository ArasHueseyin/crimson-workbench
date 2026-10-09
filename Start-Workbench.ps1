# Starts only Crimson Workbench. Never starts, stops, or modifies Crimson Desert.
$executable = Join-Path $PSScriptRoot 'target\release\crimson-workbench-0.5.9.exe'
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    $executable = Join-Path $PSScriptRoot 'target\release\crimson-workbench.exe'
}
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    Write-Error 'Desktop-Build fehlt. Im Ordner app zuerst npm ci und npm run desktop:build ausführen.'
    exit 1
}
Start-Process -FilePath $executable -ArgumentList @('--project', ('"' + $PSScriptRoot + '"')) -WorkingDirectory $PSScriptRoot
