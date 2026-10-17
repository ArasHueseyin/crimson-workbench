use crate::service::{AppError, Result};
use serde::{Deserialize, Serialize};
use std::path::Path;

#[derive(Serialize, Deserialize)]
pub struct ModStatus {
    id: String,
    state: String,
    message: String,
    can_install: bool,
    can_remove: bool,
}
#[derive(Serialize, Deserialize)]
pub struct Snapshot {
    mods: Vec<ModStatus>,
    game_running: bool,
}
#[derive(Deserialize)]
#[cfg(any(windows, test))]
struct Response {
    ok: bool,
    snapshot: Option<Snapshot>,
    message: Option<String>,
}
fn error(message: impl Into<String>) -> AppError {
    AppError {
        code: "marketplace_mod",
        message: message.into(),
    }
}
#[cfg(any(windows, test))]
fn parse_response(bytes: &[u8]) -> Result<Snapshot> {
    let response: Response = serde_json::from_slice(bytes)
        .map_err(|_| error("The mod installer returned an invalid response."))?;
    if !response.ok {
        return Err(error(
            response
                .message
                .unwrap_or_else(|| "Mod installation failed.".into()),
        ));
    }
    response
        .snapshot
        .ok_or_else(|| error("The mod installer returned no status."))
}

// std::fs::canonicalize returns verbatim Windows paths; Windows PowerShell's
// filesystem provider does not accept that prefix. Preserve Unicode and spaces.
#[cfg(any(windows, test))]
fn powershell_path(path: &Path) -> std::path::PathBuf {
    let text = path.to_string_lossy();
    if let Some(rest) = text.strip_prefix(r"\\?\UNC\") {
        return std::path::PathBuf::from(format!(r"\\{rest}"));
    }
    if let Some(rest) = text.strip_prefix(r"\\?\") {
        return rest.into();
    }
    path.to_owned()
}

pub fn invoke(resources: &Path, game: &Path, action: Option<(&str, bool)>) -> Result<Snapshot> {
    if let Some((id, _)) = action
        && ![
            "blackstar",
            "sleep_cooldown",
            "sleep_duration",
            "no_fall",
            "player_stamina",
            "mount_jump",
            "player_jump",
            "player_speed",
            "preserve_consumables",
            "mount_protection",
            "free_crafting_materials",
            "force_palm",
            "force_palm_height",
            "axiom_reach",
            "glide_speed",
            "mount_dismount",
            "auto_loot",
            "world_controls",
            "photo_mode",
            "teleport",
        ]
        .contains(&id)
    {
        return Err(error("Unknown marketplace mod."));
    }
    #[cfg(windows)]
    if action.is_some()
        && cd_core::apply::game_running().map_err(|e| {
            error(format!(
                "Could not check whether Crimson Desert is running: {e}"
            ))
        })?
    {
        return Err(error(
            "Close Crimson Desert before installing or removing a mod. Nothing was changed.",
        ));
    }
    run(resources, game, action)
}
#[cfg(windows)]
fn run(resources: &Path, game: &Path, action: Option<(&str, bool)>) -> Result<Snapshot> {
    use std::{os::windows::process::CommandExt, process::Command};
    let script = powershell_path(resources).join("runtime-mods/Marketplace-Mods.ps1");
    if !script.is_file() {
        return Err(error(
            "This build has no bundled marketplace modules. Install the latest complete Workbench setup.",
        ));
    }
    let powershell = std::env::var_os("SystemRoot")
        .map(std::path::PathBuf::from)
        .ok_or_else(|| error("Windows PowerShell is unavailable."))?
        .join("System32/WindowsPowerShell/v1.0/powershell.exe");
    let mut command = Command::new(powershell);
    // A desktop started from PowerShell 7 inherits its module search path.
    // Let Windows PowerShell construct its own compatible built-in paths.
    command.env_remove("PSModulePath");
    command
        .creation_flags(0x08000000)
        .args([
            "-NoProfile",
            "-NonInteractive",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
        ])
        .arg(script)
        .arg("-GameDirectory")
        .arg(powershell_path(game));
    if let Some((id, enabled)) = action {
        command
            .arg("-Mode")
            .arg(if enabled { "Install" } else { "Remove" })
            .arg("-ModId")
            .arg(id)
            .arg("-Elevate");
    }
    let output = command
        .output()
        .map_err(|e| error(format!("Could not start the mod installer: {e}")))?;
    if output.stdout.is_empty() {
        return Err(error(
            "The mod installer could not start. Windows permission may have been canceled.",
        ));
    }
    parse_response(&output.stdout)
}
#[cfg(not(windows))]
fn run(_: &Path, _: &Path, _: Option<(&str, bool)>) -> Result<Snapshot> {
    Err(error("Marketplace runtime modules require Windows x64."))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn installer_errors_and_missing_status_fail_closed() {
        assert!(
            parse_response(br#"{"ok":false,"message":"Unknown existing mod"}"#)
                .err()
                .unwrap()
                .message
                .contains("Unknown existing mod")
        );
        assert!(parse_response(br#"{"ok":true}"#).is_err());
        assert!(parse_response(b"not JSON").is_err());
        assert!(invoke(Path::new("."), Path::new("."), Some(("../other", true))).is_err());
    }
    #[test]
    fn verbatim_local_and_network_paths_become_powershell_paths() {
        assert_eq!(
            powershell_path(Path::new(r"\\?\C:\friend's game & data\Crimson Desert")),
            Path::new(r"C:\friend's game & data\Crimson Desert")
        );
        assert_eq!(
            powershell_path(Path::new(r"\\?\UNC\server\games\Crimson Desert")),
            Path::new(r"\\server\games\Crimson Desert")
        );
        assert_eq!(
            powershell_path(Path::new(r"C:\Game")),
            Path::new(r"C:\Game")
        );
    }
    #[cfg(windows)]
    #[test]
    fn powershell_bridge_passes_paths_literally_and_parses_utf8() {
        let directory = tempfile::tempdir().unwrap();
        let runtime = directory.path().join("runtime-mods");
        std::fs::create_dir(&runtime).unwrap();
        std::fs::write(runtime.join("Marketplace-Mods.ps1"), r#"
param([string]$GameDirectory)
[Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
$ErrorActionPreference = 'Stop'
$null = Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256
@{ok=$true;snapshot=@{game_running=$false;mods=@(@{id='blackstar';state='not_installed';message=$GameDirectory;can_install=$false;can_remove=$false})}} | ConvertTo-Json -Depth 6 -Compress
"#).unwrap();
        let game = Path::new("\\\\?\\C:\\friend's gämé & data\\Crimson Desert");
        let status = invoke(&directory.path().canonicalize().unwrap(), game, None).unwrap();
        assert_eq!(
            status.mods[0].message,
            powershell_path(game).to_string_lossy()
        );
    }
}
