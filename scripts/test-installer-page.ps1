[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$InstallerPath)
$ErrorActionPreference = 'Stop'
# Inspect only our own hidden installer window, including hidden native controls.
Add-Type -TypeDefinition @"
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
public static class CrimsonSetupUi {
    public class Control { public IntPtr Handle; public string Text; public string Class; }
    delegate bool EnumProc(IntPtr h, IntPtr p);
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc cb, IntPtr p);
    [DllImport("user32.dll")] static extern bool EnumChildWindows(IntPtr h, EnumProc cb, IntPtr p);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetClassName(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool IsWindowEnabled(IntPtr h);
    [DllImport("user32.dll", EntryPoint="SendMessageW", CharSet=CharSet.Unicode)] static extern IntPtr ReadText(IntPtr h, uint m, IntPtr n, StringBuilder s);
    public static Control[] Read(uint pid) {
        var found = new List<Control>();
        EnumProc visit = (h,p) => { var s=new StringBuilder(2048); var c=new StringBuilder(256);
            GetWindowText(h,s,s.Capacity); GetClassName(h,c,c.Capacity);
            if(c.ToString()=="Edit") ReadText(h,0xD,new IntPtr(s.Capacity),s);
            found.Add(new Control {Handle=h,Text=s.ToString(),Class=c.ToString()}); return true; };
        EnumWindows((h,p)=>{uint owner; GetWindowThreadProcessId(h,out owner);
            if(owner==pid){visit(h,p);EnumChildWindows(h,visit,IntPtr.Zero);} return true;},IntPtr.Zero);
        return found.ToArray();
    }
}
"@
$installer = (Resolve-Path -LiteralPath $InstallerPath).Path
$process = Start-Process -FilePath $installer -WindowStyle Hidden -PassThru
try {
    $deadline = [DateTime]::UtcNow.AddSeconds(30)
    $checkbox = $null
    while ([DateTime]::UtcNow -lt $deadline) {
        $controls = @([CrimsonSetupUi]::Read($process.Id))
        $checkbox = $controls | Where-Object { $_.Class -eq 'Button' -and $_.Text -match '^(Live-Items und Zusatzsockel installieren|Install Live Items and Extra Sockets)' } | Select-Object -First 1
        if ($checkbox) { break }
        # Only dismiss the language selector. Never click Next or Install.
        $ok = $controls | Where-Object { $_.Class -eq 'Button' -and $_.Text -eq 'OK' } | Select-Object -First 1
        if ($ok) { $null = [CrimsonSetupUi]::PostMessage($ok.Handle, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero) }
        Start-Sleep -Milliseconds 200
    }
    if (-not $checkbox) { throw ('Runtime page did not appear. Controls: ' + ($controls.Text -join ' | ')) }
    if ([CrimsonSetupUi]::SendMessage($checkbox.Handle, 0xF0, [IntPtr]::Zero, [IntPtr]::Zero).ToInt64() -ne 1) { throw 'Runtime modules are not selected by default.' }
    $inputs = @($controls | Where-Object { $_.Class -eq 'Edit' })
    if ($inputs.Count -ne 1 -or -not [CrimsonSetupUi]::IsWindowEnabled($inputs[0].Handle)) { throw 'Expected an enabled game-directory field.' }
    $expectedGame = & (Join-Path $PSScriptRoot '../app/src-tauri/windows/Find-Game.ps1')
    if ($expectedGame -and $inputs[0].Text -ne $expectedGame) { throw "Detected game folder was not populated: expected $expectedGame, got $($inputs[0].Text)" }
    $null = [CrimsonSetupUi]::PostMessage($checkbox.Handle, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 200
    if ([CrimsonSetupUi]::IsWindowEnabled($inputs[0].Handle)) { throw 'Opt-out did not disable the game field.' }
    $null = [CrimsonSetupUi]::PostMessage($checkbox.Handle, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 200
    if (-not [CrimsonSetupUi]::IsWindowEnabled($inputs[0].Handle)) { throw 'Selecting modules did not enable the game field.' }
    Write-Output "PASS: native setup page, default module selection, opt-out and folder field. Detected: $($inputs[0].Text)"
} finally {
    # Stop only this test's own installer, while still on its first page.
    # No installation or game-file operation has been started.
    if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
    $process.Dispose()
}
