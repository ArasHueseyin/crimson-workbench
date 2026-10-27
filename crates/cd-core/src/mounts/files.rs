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
            return Err(bad("Linked path: nothing changed."));
        }
    }
    if m.file_type().is_symlink() || m.is_dir() != directory {
        return Err(bad("Not a regular path."));
    }
    if !directory && crate::paths::link_count(path, &m)? != 1 {
        return Err(bad("Hardlink: nothing changed."));
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
        return Err(bad("Invalid save path."));
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
                    "Slot {} · Account {}",
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
        return Err(bad("Save size outside the limit."));
    }
    let mut bytes = Vec::new();
    (&mut f)
        .take(crimson_format::MAX_FILE_BYTES as u64 + 1)
        .read_to_end(&mut bytes)?;
    if bytes.len() as u64 != n {
        return Err(bad("Save is being modified. Reload it."));
    }
    Ok(bytes)
}
pub(crate) fn read_pair(root: &Path, id: &str) -> Result<(Vec<u8>, Vec<u8>)> {
    let p = slot(root, id)?;
    Ok((read(&p.join("save.save"))?, read(&p.join("lobby.save"))?))
}
pub(super) fn game_running() -> Result<bool> {
    #[cfg(target_os = "macos")]
    {
        let output = std::process::Command::new("/bin/ps")
            .args(["-axo", "comm="])
            .output()?;
        if !output.status.success() {
            return Err(bad("Could not check game processes."));
        }
        let names = String::from_utf8(output.stdout).map_err(|_| bad("Invalid process list"))?;
        Ok(names
            .lines()
            .any(|name| name.to_ascii_lowercase().contains("crimsondesert")))
    }
    #[cfg(not(target_os = "macos"))]
    {
        Ok(crate::apply::guard::game_running()?)
    }
}
fn stopped() -> Result<()> {
    if game_running()? {
        return Err(bad(
            "Crimson Desert is running. Save and fully close the game first.",
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
    let file = opt.open(path)?;
    #[cfg(target_os = "macos")]
    if !dir {
        use std::os::fd::AsRawFd;
        // Advisory locks serialize Workbench writers; foreign writes are hash checked.
        if unsafe { libc::flock(file.as_raw_fd(), libc::LOCK_EX | libc::LOCK_NB) } != 0 {
            return Err(std::io::Error::last_os_error().into());
        }
    }
    Ok(file)
}
fn put_new(path: &Path, bytes: &[u8]) -> Result<()> {
    let mut f = OpenOptions::new().write(true).create_new(true).open(path)?;
    f.write_all(bytes)?;
    f.sync_all()?;
    drop(f);
    if read(path)? != bytes {
        return Err(bad("Readback failed."));
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
            return Err(bad("Save changed since preview."));
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
            return Err(bad("Installed save does not match."));
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
            "External save change: not overwritten. Backup: {}",
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
        return Err(bad("Recovery incomplete."));
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
    #[cfg(target_os = "macos")]
    let _slot_lock = {
        use std::os::fd::AsRawFd;
        let file = File::open(&directory)?;
        if unsafe { libc::flock(file.as_raw_fd(), libc::LOCK_EX | libc::LOCK_NB) } != 0 {
            return Err(bad("Another save operation is using this slot."));
        }
        file
    };
    let mut guards = Vec::new();
    for p in directory.ancestors() {
        guards.push(exclusive(p, true)?);
    }
    #[cfg(not(target_os = "macos"))]
    {
        for p in game.join("bin64").ancestors() {
            guards.push(exclusive(p, true)?);
        }
        let mut exe = exclusive(&game.join("bin64/CrimsonDesert.exe"), false)?;
        let mut exe_bytes = Vec::new();
        exe.read_to_end(&mut exe_bytes)?;
        if hash_bytes(&exe_bytes)
            != "57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7"
        {
            return Err(bad(
                "Mount registration supports only the verified build 2976.",
            ));
        }
        drop(exe_bytes);
        guards.push(exe);
    }
    #[cfg(target_os = "macos")]
    {
        // Revalidate the installed table schema before preparing a save mutation.
        crate::GameData::open(game, "eng")?;
    }
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
            return Err(bad("Request ID was used for another selection."));
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
            "Interrupted registration rolled back. Reload and select again.",
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
        return Err(bad("Save changed since selection. Reload first."));
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
            "{} registered in the stable of {} ({} → {} animals). Load this save in game.{}",
            mount.name,
            request.save,
            candidate.count_before,
            candidate.count_after,
            if candidate.donor_key == 0 {
                " Base entry: test summoning and riding in game."
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

/// Reuses the tested two-file commit/recovery kernel. The lobby bytes are
/// preserved verbatim; its lock prevents concurrent stable edits in this slot.
pub(crate) fn unlock_skills(
    game: &Path,
    project: &Path,
    root: &Path,
    request: &crate::skill_unlock::Request,
) -> Result<crate::skill_unlock::Receipt> {
    use crate::skill_unlock::{Receipt as SkillReceipt, build_selected, plan_id};
    if root.canonicalize()? != request.save_root {
        return Err(bad(
            "Save folder changed since the unlock preview. Select and confirm the save again.",
        ));
    }
    #[derive(Serialize, Deserialize)]
    struct SkillIntent {
        receipt: SkillReceipt,
        save_after: String,
        lobby_after: String,
    }
    if request.id.len() != 32
        || !request.id.bytes().all(|b| b.is_ascii_hexdigit())
        || [
            &request.save_sha256,
            &request.lobby_sha256,
            &request.plan_id,
        ]
        .iter()
        .any(|s| s.len() != 64 || !s.bytes().all(|b| b.is_ascii_hexdigit()))
    {
        return Err(bad("Invalid skill unlock request"));
    }
    stopped()?;
    let directory = slot(root, &request.save)?;
    let mut guards = Vec::new();
    for p in directory.ancestors().chain(game.join("bin64").ancestors()) {
        guards.push(exclusive(p, true)?);
    }
    let mut exe = exclusive(&game.join("bin64/CrimsonDesert.exe"), false)?;
    let mut bytes = Vec::new();
    exe.read_to_end(&mut bytes)?;
    if hash_bytes(&bytes) != "57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7" {
        return Err(bad("All skills supports only verified game build 2976"));
    }
    guards.push(exe);
    stopped()?;
    ancestors(project)?;
    let local = project.join(".local");
    if !local.try_exists()? {
        fs::create_dir(&local)?;
    }
    ancestors(&local)?;
    let history = local.join("skill-unlocks");
    if !history.try_exists()? {
        fs::create_dir(&history)?;
    }
    ancestors(&history)?;
    let lock = history.join("lock");
    if !lock.try_exists()? {
        put_new(&lock, b"skill-unlocks-v1")?;
    }
    let _lock = exclusive(&lock, false)?;
    let backup = history.join(&request.id);
    if backup.try_exists()? {
        plain(&backup, true)?;
        let intent: SkillIntent = serde_json::from_slice(&read(&backup.join("intent.json"))?)?;
        if &intent.receipt.request != request {
            return Err(bad("Unlock request ID already used for another selection"));
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
            &read(&backup.join("original.lobby"))?,
            &read(&backup.join("original.save"))?,
            &read(&backup.join("original.lobby"))?,
        )?;
        return Err(bad(
            "Interrupted skill unlock rolled back. Reload the preview before trying again.",
        ));
    }
    let mut sf = exclusive(&directory.join("save.save"), false)?;
    let mut lf = exclusive(&directory.join("lobby.save"), false)?;
    let mut save = Vec::new();
    let mut lobby = Vec::new();
    sf.read_to_end(&mut save)?;
    lf.read_to_end(&mut lobby)?;
    if hash_bytes(&save) != request.save_sha256
        || hash_bytes(&lobby) != request.lobby_sha256
        || plan_id(&save, &lobby, request.abilities.as_ref()) != request.plan_id
    {
        return Err(bad("Save changed since preview. Reload first."));
    }
    let (candidate, summary) = build_selected(&save, request.abilities.as_ref())?;
    if summary.already_maxed == summary.total {
        return Err(bad("All skills are already unlocked. Nothing changed."));
    }
    stopped()?;
    fs::create_dir(&backup)?;
    plain(&backup, true)?;
    for (name, bytes) in [
        ("original.save", &save),
        ("original.lobby", &lobby),
        ("candidate.save", &candidate),
    ] {
        put_new(&backup.join(name), bytes)?;
    }
    let message = format!(
        "{} selected abilities are at their verified maximum. Available points, inventory and unrelated save data are preserved. Load this slot in game.",
        summary.total
    );
    let receipt = SkillReceipt {
        request: request.clone(),
        summary,
        backup: backup.clone(),
        message,
    };
    let intent = SkillIntent {
        receipt: receipt.clone(),
        save_after: hash_bytes(&candidate),
        lobby_after: hash_bytes(&lobby),
    };
    put_new(
        &backup.join("intent.json"),
        &serde_json::to_vec_pretty(&intent)?,
    )?;
    drop(sf);
    drop(lf);
    stopped()?;
    commit_pair(&directory, &backup, &candidate, &lobby, &save, &lobby, None)?;
    put_new(
        &backup.join("receipt.json"),
        &serde_json::to_vec_pretty(&receipt)?,
    )?;
    drop(guards);
    Ok(receipt)
}

/// Locked save-pair transaction shared by contribution edits and map reveal.
/// Candidate builders transform bytes only; every commit keeps the lobby exact.
pub(crate) struct SaveEditBinding<'a> {
    pub id: &'a str,
    pub save: &'a str,
    pub root: &'a Path,
    pub save_sha256: &'a str,
    pub lobby_sha256: &'a str,
    pub history: &'static str,
    pub message: &'static str,
}
pub(crate) fn edit_save_values<R, P>(
    game: &Path,
    project: &Path,
    root: &Path,
    request: &R,
    binding: SaveEditBinding<'_>,
    build: impl FnOnce(&[u8], &[u8]) -> Result<(Vec<u8>, P)>,
) -> Result<crate::save_action::Receipt<R, P>>
where
    R: Serialize + serde::de::DeserializeOwned + Clone + PartialEq,
    P: Serialize + serde::de::DeserializeOwned + Clone,
{
    if root.canonicalize()?.as_path() != binding.root {
        return Err(bad("Save folder changed since preview. Reload first."));
    }
    #[derive(Serialize, Deserialize)]
    struct SaveEditIntent<R, P> {
        receipt: crate::save_action::Receipt<R, P>,
        save_after: String,
        lobby_after: String,
    }
    stopped()?;
    let directory = slot(root, binding.save)?;
    let mut guards = Vec::new();
    for p in directory.ancestors().chain(game.join("bin64").ancestors()) {
        guards.push(exclusive(p, true)?);
    }
    let mut exe = exclusive(&game.join("bin64/CrimsonDesert.exe"), false)?;
    let mut bytes = Vec::new();
    exe.read_to_end(&mut bytes)?;
    if hash_bytes(&bytes) != "57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7" {
        return Err(bad("Save edits support only verified game build 2976"));
    }
    guards.push(exe);
    stopped()?;
    ancestors(project)?;
    let local = project.join(".local");
    if !local.try_exists()? {
        fs::create_dir(&local)?;
    }
    ancestors(&local)?;
    let history = local.join(binding.history);
    if !history.try_exists()? {
        fs::create_dir(&history)?;
    }
    ancestors(&history)?;
    let lock = history.join("lock");
    if !lock.try_exists()? {
        put_new(&lock, b"reviewed-save-edit-v1")?;
    }
    let _lock = exclusive(&lock, false)?;
    let backup = history.join(binding.id);
    if backup.try_exists()? {
        plain(&backup, true)?;
        let intent: SaveEditIntent<R, P> =
            serde_json::from_slice(&read(&backup.join("intent.json"))?)?;
        if &intent.receipt.request != request {
            return Err(bad("Save request ID already used for another selection"));
        }
        if backup.join("receipt.json").try_exists()? {
            let receipt: crate::save_action::Receipt<R, P> =
                serde_json::from_slice(&read(&backup.join("receipt.json"))?)?;
            if &receipt.request != request {
                return Err(bad("Save receipt does not match its request"));
            }
            return Ok(receipt);
        }
        let (save, lobby) = read_pair(root, binding.save)?;
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
            &read(&backup.join("original.lobby"))?,
            &read(&backup.join("original.save"))?,
            &read(&backup.join("original.lobby"))?,
        )?;
        return Err(bad(
            "Interrupted save edit rolled back. Reload the preview before trying again.",
        ));
    }
    let mut sf = exclusive(&directory.join("save.save"), false)?;
    let mut lf = exclusive(&directory.join("lobby.save"), false)?;
    let mut save = Vec::new();
    let mut lobby = Vec::new();
    sf.read_to_end(&mut save)?;
    lf.read_to_end(&mut lobby)?;
    if hash_bytes(&save) != binding.save_sha256 || hash_bytes(&lobby) != binding.lobby_sha256 {
        return Err(bad("Save changed since preview. Reload first."));
    }
    let (candidate, plan) = build(&save, &lobby)?;
    stopped()?;
    fs::create_dir(&backup)?;
    plain(&backup, true)?;
    for (name, bytes) in [
        ("original.save", &save),
        ("original.lobby", &lobby),
        ("candidate.save", &candidate),
    ] {
        put_new(&backup.join(name), bytes)?;
    }
    let receipt = crate::save_action::Receipt {
        request: request.clone(),
        plan,
        backup: backup.clone(),
        message: binding.message.into(),
    };
    let intent = SaveEditIntent {
        receipt: receipt.clone(),
        save_after: hash_bytes(&candidate),
        lobby_after: hash_bytes(&lobby),
    };
    put_new(
        &backup.join("intent.json"),
        &serde_json::to_vec_pretty(&intent)?,
    )?;
    drop(sf);
    drop(lf);
    stopped()?;
    commit_pair(&directory, &backup, &candidate, &lobby, &save, &lobby, None)?;
    put_new(
        &backup.join("receipt.json"),
        &serde_json::to_vec_pretty(&receipt)?,
    )?;
    drop(guards);
    Ok(receipt)
}

pub(crate) fn edit_contributions(
    game: &Path,
    project: &Path,
    root: &Path,
    request: &crate::contribution_action::Request,
) -> Result<crate::contribution_action::Receipt> {
    crate::contribution_action::validate_request(request)?;
    edit_save_values(
        game,
        project,
        root,
        request,
        SaveEditBinding {
            id: &request.id,
            save: &request.save,
            root: &request.save_root,
            save_sha256: &request.save_sha256,
            lobby_sha256: &request.lobby_sha256,
            history: "contribution-edits",
            message: "Selected contribution balances updated. Faction experience, levels and other save progress are preserved. Load this slot in game.",
        },
        |save, lobby| {
            let (candidate, plan) =
                crate::contribution_values::build(save, lobby, &request.edits, &request.plan_id)?;
            if plan.changes.iter().all(|c| c.before == c.after) {
                return Err(bad(
                    "Selected contribution balances already match their targets. Nothing changed.",
                ));
            }
            Ok((candidate, plan))
        },
    )
}

#[cfg(test)]
mod tests {
    use super::*;
    #[cfg(target_os = "macos")]
    #[test]
    fn native_file_lock_excludes_another_writer() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("lock");
        put_new(&path, b"lock").unwrap();
        let lock = exclusive(&path, false).unwrap();
        assert!(exclusive(&path, false).is_err());
        drop(lock);
        assert!(exclusive(&path, false).is_ok());
    }

    #[cfg(target_os = "macos")]
    #[test]
    #[ignore = "requires private save and installed game; writes only a temporary copy"]
    fn native_registration_on_copy_is_idempotent() {
        let source = PathBuf::from(std::env::var_os("CD_MOUNT_FIXTURE").unwrap());
        let game = PathBuf::from(std::env::var_os("CD_MOUNT_GAME").unwrap());
        let temp = tempfile::tempdir().unwrap();
        let project = temp.path().canonicalize().unwrap();
        let root = project.join("saves");
        let slot = root.join("123/slot0");
        fs::create_dir_all(&slot).unwrap();
        let save = fs::read(&source).unwrap();
        let lobby = fs::read(source.with_file_name("lobby.save")).unwrap();
        put_new(&slot.join("save.save"), &save).unwrap();
        put_new(&slot.join("lobby.save"), &lobby).unwrap();
        let data = crate::GameData::open(&game, "eng").unwrap();
        let mounts = super::super::catalog(&data).unwrap();
        let snapshot = super::super::snapshot(mounts.clone(), &root, None).unwrap();
        let mount = snapshot.mounts.iter().find(|m| m.supported).unwrap();
        let request = Request {
            id: "a".repeat(32),
            save: "123/slot0".into(),
            save_sha256: hash_bytes(&save),
            lobby_sha256: hash_bytes(&lobby),
            key: mount.key,
        };
        let receipt = super::super::register(&game, &project, &mounts, &root, &request).unwrap();
        let after = read_pair(&root, "123/slot0").unwrap();
        let retry = super::super::register(&game, &project, &mounts, &root, &request).unwrap();
        assert_eq!(receipt.mercenary_no, retry.mercenary_no);
        assert_eq!(after, read_pair(&root, "123/slot0").unwrap());
        assert_eq!(read(&receipt.backup.join("original.save")).unwrap(), save);
        assert_eq!(read(&receipt.backup.join("original.lobby")).unwrap(), lobby);
        let mut annotated = mounts;
        edit::annotate(&after.0, &mut annotated).unwrap();
        assert_eq!(
            annotated
                .iter()
                .find(|m| m.key == request.key)
                .unwrap()
                .owned,
            1
        );
    }
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
