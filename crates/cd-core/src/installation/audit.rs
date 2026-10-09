//! Cancellable, read-only content comparison against local Steam cache hashes.
//! SHA-1 is legacy interoperability only; also record SHA-256. No trust promotion,
//! snapshot import, game writer or launch-denying handle exists in this module.
use super::{Inventory, bad, inspect_with, linked, relative};
use crate::{Error, Result, discovery::DiscoveryOptions};
use serde::{Deserialize, Serialize};
use sha1::{Digest, Sha1};
use sha2::Sha256;
use std::{
    collections::BTreeMap,
    fs::{self, File, OpenOptions},
    io::Read,
    path::{Path, PathBuf},
    sync::atomic::{AtomicBool, Ordering},
    time::{Duration, Instant, SystemTime},
};

#[derive(Clone, Debug, Serialize)]
pub struct Progress {
    pub phase: &'static str,
    pub files_done: usize,
    pub total_files: usize,
    pub bytes_done: String,
    pub total_bytes: String,
    pub current_path: Option<String>,
}
impl Default for Progress {
    fn default() -> Self {
        Self {
            phase: "preparing",
            files_done: 0,
            total_files: 0,
            bytes_done: "0".into(),
            total_bytes: "0".into(),
            current_path: None,
        }
    }
}
#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ContentFile {
    pub path: String,
    pub bytes: String,
    pub expected_sha1: String,
    pub sha1: String,
    pub sha256: String,
    pub matches_cache: bool,
}
#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct Report {
    pub version: u32,
    pub kind: String,
    pub game_path: PathBuf,
    pub build_id: String,
    pub started_at: u64,
    pub finished_at: u64,
    pub all_files_match_cache: bool,
    pub metadata_stable: bool,
    pub certified_vanilla: bool,
    pub can_apply: bool,
    pub manifest_sha256: Vec<String>,
    pub files: Vec<ContentFile>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub foreign_approval: Option<String>,
    pub limitations: Vec<String>,
}

/// Guard runs BEFORE inventory, before each content open, between read blocks
/// once 250ms have elapsed, and before publishing the result. No file is locked against
/// a game launch/update. A detected launch aborts, it never terminates the game.
pub fn verify(game: &Path, cancel: &AtomicBool, progress: impl FnMut(Progress)) -> Result<Report> {
    verify_with(
        game,
        &DiscoveryOptions::from_environment().steam_roots,
        cancel,
        progress,
        crate::apply::require_game_stopped,
    )
}
fn verify_with(
    game: &Path,
    caches: &[PathBuf],
    cancel: &AtomicBool,
    progress: impl FnMut(Progress),
    guard: impl FnMut() -> Result<()>,
) -> Result<Report> {
    verify_scoped(game, caches, cancel, progress, guard, None)
}
pub fn verify_managed(
    policy: &crate::paths::PathPolicy,
    game: &Path,
    cancel: &AtomicBool,
    progress: impl FnMut(Progress),
) -> Result<Report> {
    verify_scoped(
        game,
        &DiscoveryOptions::from_environment().steam_roots,
        cancel,
        progress,
        crate::apply::require_game_stopped,
        Some(policy),
    )
}
fn verify_scoped(
    game: &Path,
    caches: &[PathBuf],
    cancel: &AtomicBool,
    mut progress: impl FnMut(Progress),
    mut guard: impl FnMut() -> Result<()>,
    policy: Option<&crate::paths::PathPolicy>,
) -> Result<Report> {
    check_cancel(cancel)?;
    guard()?;
    let started_at = now();
    progress(Progress::default());
    let inspect = |root: &Path| -> Result<Inventory> {
        let report = inspect_with(root, caches)?;
        match policy {
            Some(p) => super::classify(p, report),
            None => Ok(report),
        }
    };
    let before = inspect(game)?;
    validate_inventory(&before)?;
    let root = &before.game_path;
    let initial_stamps = stamps(root, &before)?;
    let mut state = Progress {
        total_files: before.expected_files,
        total_bytes: before.expected_bytes.clone(),
        phase: "hashing",
        ..Progress::default()
    };
    let mut files = Vec::with_capacity(state.total_files);
    let mut done = 0u64;
    let mut buffer = vec![0u8; 1024 * 1024];
    let mut last_progress = Instant::now();
    for wanted in &before.files {
        check_cancel(cancel)?;
        guard()?;
        let mut last_guard = Instant::now();
        let expected_size = wanted
            .expected_bytes
            .as_deref()
            .ok_or_else(|| bad("missing expected size"))?
            .parse::<u64>()
            .map_err(|_| bad("invalid expected size"))?;
        let expected_sha1 = wanted
            .expected_sha1
            .clone()
            .ok_or_else(|| bad("missing expected content SHA-1"))?;
        state.current_path = Some(wanted.path.clone());
        progress(state.clone());
        let mut source = open_source(root, &wanted.path)?;
        let first = stamp(&source)?;
        if initial_stamps.get(&wanted.path) != Some(&first) || first.bytes != expected_size {
            return Err(bad(format!("Quelle vor dem Lesen geändert: {}", wanted.path)).into());
        }
        let mut sha1 = Sha1::new();
        let mut sha256 = Sha256::new();
        let mut bytes = 0u64;
        loop {
            check_cancel(cancel)?;
            if last_guard.elapsed() >= Duration::from_millis(250) {
                guard()?;
                last_guard = Instant::now();
            }
            // Limit to the expected size + one byte so a concurrently growing
            // source can never turn this into an unbounded stream.
            let remaining = expected_size.saturating_sub(bytes);
            let len = buffer
                .len()
                .min(remaining.saturating_add(1).min(usize::MAX as u64) as usize);
            let n = source.read(&mut buffer[..len])?;
            if n == 0 {
                break;
            }
            bytes = bytes
                .checked_add(n as u64)
                .ok_or_else(|| bad("content size overflow"))?;
            if bytes > expected_size {
                return Err(bad(format!("Quelldatei gewachsen: {}", wanted.path)).into());
            }
            sha1.update(&buffer[..n]);
            sha256.update(&buffer[..n]);
            done = done
                .checked_add(n as u64)
                .ok_or_else(|| bad("audit size overflow"))?;
            state.bytes_done = done.to_string();
            if last_progress.elapsed() >= Duration::from_millis(100) {
                progress(state.clone());
                last_progress = Instant::now();
            }
        }
        check_cancel(cancel)?;
        guard()?;
        if bytes != expected_size || first != stamp(&source)? {
            return Err(bad(format!(
                "Quelle während des Lesens geändert: {}",
                wanted.path
            ))
            .into());
        }
        let sha1 = format!("{:x}", sha1.finalize());
        let sha256 = format!("{:x}", sha256.finalize());
        files.push(ContentFile {
            path: wanted.path.clone(),
            bytes: bytes.to_string(),
            matches_cache: sha1 == expected_sha1,
            expected_sha1,
            sha1,
            sha256,
        });
        state.files_done += 1;
        progress(state.clone());
    }
    state.phase = "validating";
    state.current_path = None;
    progress(state.clone());
    check_cancel(cancel)?;
    guard()?;
    let after = inspect(root)?;
    validate_inventory(&after)?;
    if identity(&before)? != identity(&after)?
        || serde_json::to_vec(&before.managed_files)? != serde_json::to_vec(&after.managed_files)?
        || before.foreign_approval != after.foreign_approval
        || serde_json::to_vec(&before.foreign_files)? != serde_json::to_vec(&after.foreign_files)?
        || initial_stamps != stamps(root, &after)?
    {
        return Err(bad("Installation oder Steam-Metadaten während der Inhaltsprüfung geändert; kein vollständiger Bericht erstellt").into());
    }
    check_cancel(cancel)?;
    guard()?;
    let report=Report {version:1,kind:"cache-content-audit".into(),game_path:root.clone(),build_id:before.build_id.unwrap(),started_at,finished_at:now(),
        all_files_match_cache:files.iter().all(|f|f.matches_cache),metadata_stable:true,certified_vanilla:false,can_apply:false,
        foreign_approval:if before.foreign_files.is_empty(){None}else{before.foreign_approval},manifest_sha256:before.depots.into_iter().map(|d|d.manifest_sha256).collect(),files,
        limitations:vec![
            "Geprüft werden Depot-Originaldateien. foreign_approval kennzeichnet separat bestätigte Zusatzdateien; deren Inhalt wird von diesem Bericht nicht zertifiziert.".into(),
            "SHA-1-Vergleich mit lokalen, nicht unabhängig authentifizierten Steam-Depotlisten. SHA-256 zusätzlich aufgezeichnet; kein Vanilla-Zertifikat und keine Schreibfreigabe.".into(),
            "Lesende Beobachtung ohne Startsperren. Größen, Zeitstempel und Dateimetadaten vor/nach dem Lauf verglichen; kein atomarer oder gegen gezielte Manipulation abgesicherter Snapshot.".into(),
            "Erneute Starts und Updates können den Befund sofort veralten lassen. Live-Apply prüft Herkunftsbestätigung, Basis, gehaltene Quellsperren und Backup separat.".into(),
        ]};
    state.phase = "complete";
    progress(state);
    Ok(report)
}
fn check_cancel(cancel: &AtomicBool) -> Result<()> {
    if cancel.load(Ordering::Acquire) {
        Err(Error::Invalid(
            "Inhaltsprüfung abgebrochen; kein vollständiger Bericht erstellt.".into(),
        ))
    } else {
        Ok(())
    }
}
pub(super) fn validate_inventory(report: &Inventory) -> Result<()> {
    if !report.directory_scan_complete
        || !report.depot_comparison_available
        || !report.issues.is_empty()
        || report.files.is_empty()
        || report.files.len() != report.expected_files
        || report.files.iter().any(|f| f.state != "size_matches")
    {
        return Err(bad("Dateiliste ist unvollständig oder abweichend. Zuerst Dateiliste prüfen; keine Inhaltsprüfung gestartet.").into());
    }
    Ok(())
}
pub(super) fn identity(report: &Inventory) -> Result<Vec<u8>> {
    Ok(serde_json::to_vec(&(
        &report.build_id,
        &report.expected_bytes,
        &report.depots,
        &report.files,
        &report.groups,
        &report.registry_sha256,
    ))?)
}
#[derive(PartialEq, Eq)]
pub(super) struct Stamp {
    bytes: u64,
    modified: SystemTime,
    created: Option<SystemTime>,
    identity: (u64, u64),
}
pub(super) fn stamp(file: &File) -> Result<Stamp> {
    let meta = file.metadata()?;
    if !meta.is_file() || linked(&meta) {
        return Err(bad("Quelle ist keine reguläre Datei").into());
    }
    #[cfg(windows)]
    let identity = {
        use std::os::windows::io::AsRawHandle;
        use windows_sys::Win32::Storage::FileSystem::{
            BY_HANDLE_FILE_INFORMATION, GetFileInformationByHandle,
        };
        let mut info: BY_HANDLE_FILE_INFORMATION = unsafe { std::mem::zeroed() };
        if unsafe { GetFileInformationByHandle(file.as_raw_handle(), &mut info) } == 0 {
            return Err(std::io::Error::last_os_error().into());
        }
        (
            info.dwVolumeSerialNumber as u64,
            (info.nFileIndexHigh as u64) << 32 | info.nFileIndexLow as u64,
        )
    };
    #[cfg(unix)]
    let identity = {
        use std::os::unix::fs::MetadataExt;
        if meta.nlink() != 1 {
            return Err(bad("Hardlink als Inhaltsquelle abgelehnt").into());
        }
        (meta.dev(), meta.ino())
    };
    #[cfg(not(any(windows, unix)))]
    let identity = (0, 0);
    Ok(Stamp {
        bytes: meta.len(),
        modified: meta.modified()?,
        created: meta.created().ok(),
        identity,
    })
}
fn stamps(root: &Path, inventory: &Inventory) -> Result<BTreeMap<String, Stamp>> {
    inventory
        .files
        .iter()
        .map(|f| {
            let file = open_source(root, &f.path).map_err(|e| bad(format!("{}: {e}", f.path)))?;
            Ok((f.path.clone(), stamp(&file)?))
        })
        .collect()
}
pub(super) fn open_source(root: &Path, name: &str) -> Result<File> {
    let name = relative(name)?;
    let path = root.join(name);
    for part in path.ancestors() {
        if linked(&fs::symlink_metadata(part)?) {
            return Err(bad("Verknüpfte Inhaltsquelle wird nicht geöffnet").into());
        }
    }
    let mut options = OpenOptions::new();
    options.read(true);
    #[cfg(windows)]
    {
        use std::os::windows::fs::OpenOptionsExt;
        options.share_mode(7).custom_flags(0x0020_0000);
    }
    let file = options.open(&path)?;
    // Validate the OPEN handle before reading a single content byte. Windows
    // final-path lookup prevents a raced parent junction from redirecting reads.
    let actual = handle_path(&file, &path)?;
    if !actual.starts_with(root) || actual == root {
        return Err(bad("Inhaltsquelle liegt außerhalb der Installation").into());
    }
    stamp(&file)?;
    Ok(file)
}
#[cfg(windows)]
fn handle_path(file: &File, _path: &Path) -> Result<PathBuf> {
    use std::os::windows::{ffi::OsStringExt, io::AsRawHandle};
    use windows_sys::Win32::Storage::FileSystem::{
        BY_HANDLE_FILE_INFORMATION, GetFileInformationByHandle, GetFinalPathNameByHandleW,
    };
    unsafe {
        let h = file.as_raw_handle();
        let mut info: BY_HANDLE_FILE_INFORMATION = std::mem::zeroed();
        if GetFileInformationByHandle(h, &mut info) == 0 {
            return Err(std::io::Error::last_os_error().into());
        }
        if info.dwFileAttributes & 0x410 != 0 || info.nNumberOfLinks != 1 {
            return Err(bad(
                "Reparse-Punkt, Verzeichnis oder Hardlink als Inhaltsquelle abgelehnt",
            )
            .into());
        }
        let mut buf = vec![0u16; 32768];
        let n = GetFinalPathNameByHandleW(h, buf.as_mut_ptr(), buf.len() as u32, 0);
        if n == 0 {
            return Err(std::io::Error::last_os_error().into());
        }
        if n as usize >= buf.len() {
            return Err(bad("Inhaltsquellenpfad zu lang").into());
        }
        Ok(PathBuf::from(std::ffi::OsString::from_wide(
            &buf[..n as usize],
        )))
    }
}
#[cfg(target_os = "linux")]
fn handle_path(file: &File, _path: &Path) -> Result<PathBuf> {
    use std::os::fd::AsRawFd;
    Ok(fs::read_link(format!(
        "/proc/self/fd/{}",
        file.as_raw_fd()
    ))?)
}
#[cfg(not(any(windows, target_os = "linux")))]
fn handle_path(_file: &File, _path: &Path) -> Result<PathBuf> {
    Err(bad("Content handle verification unsupported on this platform").into())
}
fn now() -> u64 {
    SystemTime::now()
        .duration_since(SystemTime::UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs()
}

#[cfg(test)]
mod tests;

#[cfg(test)]
pub(crate) fn synthetic_managed_audit(
    policy: &crate::paths::PathPolicy,
    root: &Path,
) -> Result<Report> {
    verify_scoped(
        root,
        &[],
        &AtomicBool::new(false),
        |_| {},
        || Ok(()),
        Some(policy),
    )
}
