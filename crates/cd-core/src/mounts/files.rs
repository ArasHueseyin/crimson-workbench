use super::{Mount, Request, Result, bad, edit};
use crate::fingerprint::hash_bytes;
use serde::{Deserialize, Serialize};
use std::{
    fs::{self, File, OpenOptions},
    io::{Read, Write},
    path::{Path, PathBuf},
    time::UNIX_EPOCH,
};

#[derive(Serialize)]
pub struct SaveChoice {
    pub id: String,
    pub label: String,
    pub modified: u64,
}
#[derive(Clone, Serialize, Deserialize)]
pub struct Receipt {
    pub request: Request,
    pub name: String,
    pub mercenary_no: String,
    pub donor_key: u32,
    pub backup: PathBuf,
    pub message: String,
}
#[derive(Serialize, Deserialize)]
struct Intent {
    request: Request,
    receipt: Receipt,
    save_after: String,
    lobby_after: String,
}
fn plain(path: &Path, directory: bool) -> Result<()> {
    let m = fs::symlink_metadata(path)?;
    #[cfg(windows)]
    {
        use std::os::windows::fs::MetadataExt;
        if m.file_attributes() & 0x400 != 0 {
            return Err(bad("Verknüpfter Pfad: keine Änderung."));
        }
    }
    if m.file_type().is_symlink() || m.is_dir() != directory {
        return Err(bad("Kein regulärer Pfad."));
    }
    if !directory && crate::paths::link_count(path, &m)? != 1 {
        return Err(bad("Hardlink: keine Änderung."));
    }
    Ok(())
}
fn ancestors(path: &Path) -> Result<()> {
    for p in path.ancestors() {
        plain(p, true)?;
    }
    Ok(())
}
fn slot(root: &Path, id: &str) -> Result<PathBuf> {
    let parts: Vec<_> = id.split('/').collect();
    if parts.len() != 2
        || parts[0].is_empty()
        || parts[0].len() > 20
        || !parts[0].bytes().all(|b| b.is_ascii_digit())
        || !parts[1]
            .strip_prefix("slot")
            .is_some_and(|s| !s.is_empty() && s.len() <= 3 && s.bytes().all(|b| b.is_ascii_digit()))
    {
        return Err(bad("Ungültiger Spielstandpfad."));
    }
    let p = root.join(parts[0]).join(parts[1]);
    ancestors(&p)?;
    Ok(p)
}
pub(crate) fn choices(root: &Path) -> Result<Vec<SaveChoice>> {
    if !root.try_exists()? {
        return Ok(Vec::new());
    }
    ancestors(root)?;
    let mut out = Vec::new();
    for account in fs::read_dir(root)? {
        let account = account?;
        let name = account.file_name().to_string_lossy().into_owned();
        if name.is_empty() || !name.bytes().all(|b| b.is_ascii_digit()) {
            continue;
        }
        plain(&account.path(), true)?;
        for entry in fs::read_dir(account.path())? {
            let entry = entry?;
            let name = entry.file_name().to_string_lossy().into_owned();
            if !name.strip_prefix("slot").is_some_and(|s| {
                !s.is_empty() && s.len() <= 3 && s.bytes().all(|b| b.is_ascii_digit())
            }) {
                continue;
            }
            let id = format!("{}/{name}", account.file_name().to_string_lossy());
            let p = slot(root, &id)?;
            if !p.join("save.save").try_exists()? || !p.join("lobby.save").try_exists()? {
                continue;
            }
            plain(&p.join("save.save"), false)?;
            plain(&p.join("lobby.save"), false)?;
            let modified = fs::metadata(p.join("save.save"))?
                .modified()?
                .duration_since(UNIX_EPOCH)
                .map_err(|e| bad(e.to_string()))?
                .as_secs();
            out.push(SaveChoice {
                id,
                label: format!(
                    "Slot {} · Konto {}",
                    name.trim_start_matches("slot"),
                    account.file_name().to_string_lossy()
                ),
                modified,
            });
        }
    }
    out.sort_by(|a, b| b.modified.cmp(&a.modified).then(a.id.cmp(&b.id)));
    Ok(out)
}
fn read(path: &Path) -> Result<Vec<u8>> {
    plain(path, false)?;
    let mut f = File::open(path)?;
    let n = f.metadata()?.len();
    if n == 0 || n > crimson_format::MAX_FILE_BYTES as u64 {
        return Err(bad("Spielstandgröße außerhalb des Limits."));
    }
    let mut bytes = Vec::new();
    (&mut f)
        .take(crimson_format::MAX_FILE_BYTES as u64 + 1)
        .read_to_end(&mut bytes)?;
    if bytes.len() as u64 != n {
        return Err(bad("Spielstand wird gerade geändert. Erneut einlesen."));
    }
    Ok(bytes)
}
pub(crate) fn read_pair(root: &Path, id: &str) -> Result<(Vec<u8>, Vec<u8>)> {
    let p = slot(root, id)?;
    Ok((read(&p.join("save.save"))?, read(&p.join("lobby.save"))?))
}
fn stopped() -> Result<()> {
    if crate::apply::guard::game_running()? {
        return Err(bad(
            "Crimson Desert läuft. Erst speichern und vollständig schließen.",
        ));
    }
    Ok(())
}
fn exclusive(path: &Path, dir: bool) -> Result<File> {
    plain(path, dir)?;
    let mut opt = OpenOptions::new();
    opt.read(true);
    #[cfg(windows)]
    {
        use std::os::windows::fs::OpenOptionsExt;
        opt.share_mode(if dir { 3 } else { 0 })
            .custom_flags(if dir { 0x02200000 } else { 0x00200000 });
    }
    Ok(opt.open(path)?)
}
fn put_new(path: &Path, bytes: &[u8]) -> Result<()> {
    let mut f = OpenOptions::new().write(true).create_new(true).open(path)?;
    f.write_all(bytes)?;
    f.sync_all()?;
    drop(f);
    if read(path)? != bytes {
        return Err(bad("Readback fehlgeschlagen."));
    }
    Ok(())
}
fn replace(from: &Path, to: &Path) -> Result<()> {
    #[cfg(windows)]
    {
        use std::os::windows::ffi::OsStrExt;
        use windows_sys::Win32::Storage::FileSystem::*;
        let a: Vec<u16> = from.as_os_str().encode_wide().chain(Some(0)).collect();
        let b: Vec<u16> = to.as_os_str().encode_wide().chain(Some(0)).collect();
        if unsafe {
            MoveFileExW(
                a.as_ptr(),
                b.as_ptr(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH,
            )
        } == 0
        {
            return Err(std::io::Error::last_os_error().into());
        }
    }
    #[cfg(not(windows))]
    {
        fs::rename(from, to)?;
        File::open(to.parent().unwrap())?.sync_all()?;
    }
    Ok(())
}
fn commit_pair(
    directory: &Path,
    backup: &Path,
    save: &[u8],
    lobby: &[u8],
    before_save: &[u8],
    before_lobby: &[u8],
    fault: Option<usize>,
) -> Result<()> {
    let save_temp = directory.join(".crimson-mount-save.tmp");
    let lobby_temp = directory.join(".crimson-mount-lobby.tmp");
    // Existing stages are evidence of an interrupted operation; never overwrite.
    put_new(&save_temp, save)?;
    put_new(&lobby_temp, lobby)?;
    let result = (|| {
        if read(&directory.join("save.save"))? != before_save
            || read(&directory.join("lobby.save"))? != before_lobby
        {
            return Err(bad("Spielstand seit Vorschau geändert."));
        }
        // Counter first: a crash between these replacements leaves an advanced
        // counter and the unchanged save. It cannot reuse a newly allocated ID.
        replace(&lobby_temp, &directory.join("lobby.save"))?;
        if fault == Some(1) {
            return Err(bad("Injected failure between replacements"));
        }
        replace(&save_temp, &directory.join("save.save"))?;
        if read(&directory.join("save.save"))? != save
            || read(&directory.join("lobby.save"))? != lobby
        {
            return Err(bad("Installierter Spielstand stimmt nicht."));
        }
        Ok(())
    })();
    if result.is_err() {
        // Restore only exact before/after generations; never overwrite foreign
        // saves, even when cloud sync or another editor changed them mid-commit.
        recover_pair(directory, backup, save, lobby, before_save, before_lobby)?;
    }
    result
}
fn recover_pair(
    directory: &Path,
    backup: &Path,
    after_save: &[u8],
    after_lobby: &[u8],
    before_save: &[u8],
    before_lobby: &[u8],
) -> Result<()> {
    let current_save = read(&directory.join("save.save"))?;
    let current_lobby = read(&directory.join("lobby.save"))?;
    if ![before_save, after_save].contains(&current_save.as_slice())
        || ![before_lobby, after_lobby].contains(&current_lobby.as_slice())
    {
        return Err(bad(format!(
            "Fremde Spielstandänderung: nicht überschrieben. Sicherung: {}",
            backup.display()
        )));
    }
    for (name, bytes, current) in [
        ("save.save", before_save, current_save),
        ("lobby.save", before_lobby, current_lobby),
    ] {
        if current != bytes {
            let tmp = directory.join(format!(".mount-restore-{name}.tmp"));
            put_new(&tmp, bytes)?;
            replace(&tmp, &directory.join(name))?;
        }
    }
    for name in [".crimson-mount-save.tmp", ".crimson-mount-lobby.tmp"] {
        let p = directory.join(name);
        if p.try_exists()? {
            plain(&p, false)?;
            fs::remove_file(p)?;
        }
    }
    if read(&directory.join("save.save"))? != before_save
        || read(&directory.join("lobby.save"))? != before_lobby
    {
        return Err(bad("Wiederherstellung unvollständig."));
    }
    Ok(())
}
pub(super) fn register(
    game: &Path,
    project: &Path,
    root: &Path,
    request: &Request,
    mount: &Mount,
    mounts: &[Mount],
) -> Result<Receipt> {
    stopped()?;
    let directory = slot(root, &request.save)?;
    let mut guards = Vec::new();
    for p in directory.ancestors() {
        guards.push(exclusive(p, true)?);
    }
    for p in game.join("bin64").ancestors() {
        guards.push(exclusive(p, true)?);
    }
    let mut exe = exclusive(&game.join("bin64/CrimsonDesert.exe"), false)?;
    let mut exe_bytes = Vec::new();
    exe.read_to_end(&mut exe_bytes)?;
    if hash_bytes(&exe_bytes) != "57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7"
    {
        return Err(bad(
            "Reittierregistrierung ist nur für den geprüften Build 2976 freigegeben.",
        ));
    }
    drop(exe_bytes);
    guards.push(exe);
    stopped()?;
    ancestors(project)?;
    let local = project.join(".local");
    if !local.try_exists()? {
        fs::create_dir(&local)?;
    }
    ancestors(&local)?;
    let history = local.join("mount-grants");
    if !history.try_exists()? {
        fs::create_dir(&history)?;
    }
    ancestors(&history)?;
    let lock_path = history.join("lock");
    if !lock_path.try_exists()? {
        put_new(&lock_path, b"mount-grants-v1")?;
    }
    let _lock = exclusive(&lock_path, false)?;
    let backup = history.join(&request.id);
    if backup.try_exists()? {
        plain(&backup, true)?;
        let intent: Intent = serde_json::from_slice(&read(&backup.join("intent.json"))?)?;
        if &intent.request != request {
            return Err(bad("Anfrage-ID wurde für eine andere Auswahl verwendet."));
        }
        if backup.join("receipt.json").try_exists()? {
            return Ok(serde_json::from_slice(&read(
                &backup.join("receipt.json"),
            )?)?);
        }
        let (save, lobby) = read_pair(root, &request.save)?;
        if hash_bytes(&save) == intent.save_after && hash_bytes(&lobby) == intent.lobby_after {
            put_new(
                &backup.join("receipt.json"),
                &serde_json::to_vec_pretty(&intent.receipt)?,
            )?;
            return Ok(intent.receipt);
        }
        recover_pair(
            &directory,
            &backup,
            &read(&backup.join("candidate.save"))?,
            &read(&backup.join("candidate.lobby"))?,
            &read(&backup.join("original.save"))?,
            &read(&backup.join("original.lobby"))?,
        )?;
        return Err(bad(
            "Unterbrochene Registrierung zurückgesetzt. Neu einlesen und erneut auswählen.",
        ));
    }
    // Serialize against another Workbench operation and hold both original
    // files while preparing. No target handle remains open across MoveFileEx.
    let mut sf = exclusive(&directory.join("save.save"), false)?;
    let mut lf = exclusive(&directory.join("lobby.save"), false)?;
    let mut before_save = Vec::new();
    sf.read_to_end(&mut before_save)?;
    let mut before_lobby = Vec::new();
    lf.read_to_end(&mut before_lobby)?;
    if hash_bytes(&before_save) != request.save_sha256
        || hash_bytes(&before_lobby) != request.lobby_sha256
    {
        return Err(bad("Spielstand seit Auswahl geändert. Erst neu einlesen."));
    }
    let candidate = edit::build(&before_save, &before_lobby, mount, mounts)?;
    stopped()?;
    fs::create_dir(&backup)?;
    plain(&backup, true)?;
    for (name, bytes) in [
        ("original.save", &before_save),
        ("original.lobby", &before_lobby),
        ("candidate.save", &candidate.save),
        ("candidate.lobby", &candidate.lobby),
    ] {
        put_new(&backup.join(name), bytes)?;
    }
    let receipt = Receipt {
        request: request.clone(),
        name: mount.name.clone(),
        mercenary_no: candidate.mercenary_no.to_string(),
        donor_key: candidate.donor_key,
        backup: backup.clone(),
        message: format!(
            "{} im Stall von {} registriert ({} → {} Tiere). Diesen Spielstand im Spiel laden.{}",
            mount.name,
            request.save,
            candidate.count_before,
            candidate.count_after,
            if candidate.donor_key == 0 {
                " Basiseintrag: Herbeirufen und Reiten im Spiel prüfen."
            } else {
                ""
            }
        ),
    };
    let intent = Intent {
        request: request.clone(),
        receipt: receipt.clone(),
        save_after: hash_bytes(&candidate.save),
        lobby_after: hash_bytes(&candidate.lobby),
    };
    put_new(
        &backup.join("intent.json"),
        &serde_json::to_vec_pretty(&intent)?,
    )?;
    drop(sf);
    drop(lf);
    stopped()?;
    commit_pair(
        &directory,
        &backup,
        &candidate.save,
        &candidate.lobby,
        &before_save,
        &before_lobby,
        None,
    )?;
    put_new(
        &backup.join("receipt.json"),
        &serde_json::to_vec_pretty(&receipt)?,
    )?;
    // The exclusive game EXE handle stays alive through receipt/readback.
    drop(guards);
    Ok(receipt)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn pair_failure_restores_both_files_and_removes_stages() {
        let dir = tempfile::tempdir().unwrap();
        let p = dir.path();
        put_new(&p.join("save.save"), b"old-save").unwrap();
        put_new(&p.join("lobby.save"), b"old-lobby").unwrap();
        assert!(
            commit_pair(
                p,
                p,
                b"new-save",
                b"new-lobby",
                b"old-save",
                b"old-lobby",
                Some(1)
            )
            .is_err()
        );
        assert_eq!(read(&p.join("save.save")).unwrap(), b"old-save");
        assert_eq!(read(&p.join("lobby.save")).unwrap(), b"old-lobby");
        assert!(!p.join(".crimson-mount-save.tmp").exists());
        assert!(!p.join(".crimson-mount-lobby.tmp").exists());
    }
    #[test]
    fn pair_success_changes_only_the_pair() {
        let dir = tempfile::tempdir().unwrap();
        let p = dir.path();
        put_new(&p.join("save.save"), b"old-save").unwrap();
        put_new(&p.join("lobby.save"), b"old-lobby").unwrap();
        put_new(&p.join("keep.bin"), b"preserve").unwrap();
        commit_pair(
            p,
            p,
            b"new-save",
            b"new-lobby",
            b"old-save",
            b"old-lobby",
            None,
        )
        .unwrap();
        assert_eq!(read(&p.join("keep.bin")).unwrap(), b"preserve");
    }
    #[test]
    fn foreign_changes_and_path_traversal_are_rejected() {
        let dir = tempfile::tempdir().unwrap();
        let p = dir.path();
        put_new(&p.join("save.save"), b"foreign").unwrap();
        put_new(&p.join("lobby.save"), b"old-lobby").unwrap();
        assert!(recover_pair(p, p, b"new-save", b"new-lobby", b"old-save", b"old-lobby").is_err());
        assert_eq!(read(&p.join("save.save")).unwrap(), b"foreign");
        for id in [
            "../../slot2",
            "account/slot2",
            "123/slot2/extra",
            "123/slot..",
            "123\\slot2",
        ] {
            assert!(slot(p, id).is_err());
        }
    }
}
