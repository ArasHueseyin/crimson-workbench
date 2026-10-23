use super::*;
use std::{
    ffi::{OsStr, c_int, c_void},
    fs,
    io::{Read, Write},
    os::unix::net::UnixStream,
    os::{
        fd::AsRawFd,
        unix::{
            ffi::OsStrExt,
            fs::{FileTypeExt, MetadataExt, PermissionsExt},
        },
    },
    path::PathBuf,
    time::Duration,
};

#[link(name = "proc")]
unsafe extern "C" {
    fn proc_listallpids(buffer: *mut c_void, size: c_int) -> c_int;
    fn proc_name(pid: c_int, buffer: *mut c_void, size: u32) -> c_int;
    fn proc_pidpath(pid: c_int, buffer: *mut c_void, size: u32) -> c_int;
}
unsafe extern "C" {
    fn geteuid() -> u32;
    fn kill(pid: c_int, signal: c_int) -> c_int;
    fn getpeereid(fd: c_int, uid: *mut u32, gid: *mut u32) -> c_int;
    fn getsockopt(
        fd: c_int,
        level: c_int,
        option: c_int,
        value: *mut c_void,
        size: *mut u32,
    ) -> c_int;
}
fn executable(game: &Path) -> Result<PathBuf> {
    cd_core::discovery::Installation {
        path: game.into(),
        platform: cd_core::discovery::Platform::Explicit,
        build_id: None,
    }
    .executable()
    .canonicalize()
    .map_err(|e| error(e.to_string()))
}
pub(super) fn game_pids(game: &Path) -> Result<Vec<u32>> {
    let expected = executable(game)?;
    let count = unsafe { proc_listallpids(std::ptr::null_mut(), 0) };
    if count <= 0 || count > 100_000 {
        return Err(error("Process list unavailable."));
    }
    let mut pids = vec![0i32; count as usize + 128];
    let count = unsafe { proc_listallpids(pids.as_mut_ptr().cast(), (pids.len() * 4) as c_int) };
    if count <= 0 || count as usize >= pids.len() {
        return Err(error("Process list changed; try again."));
    }
    let mut found = Vec::new();
    for pid in &pids[..count as usize] {
        let mut name = [0u8; 64];
        let n = unsafe { proc_name(*pid, name.as_mut_ptr().cast(), name.len() as u32) };
        if n <= 0 || name.split(|b| *b == 0).next() != Some(b"CrimsonDesert_Steam".as_slice()) {
            continue;
        }
        let mut path = [0u8; 4096];
        if unsafe { proc_pidpath(*pid, path.as_mut_ptr().cast(), path.len() as u32) } <= 0 {
            return Err(error(
                "Could not verify the running game's executable path.",
            ));
        }
        let end = path.iter().position(|b| *b == 0).unwrap_or(path.len());
        let path = Path::new(OsStr::from_bytes(&path[..end]));
        if path.canonicalize().ok().as_ref() == Some(&expected) {
            found.push(*pid as u32);
        }
    }
    Ok(found)
}
pub(super) fn process_exited(pid: u32) -> Result<bool> {
    let pid = i32::try_from(pid)
        .ok()
        .filter(|p| *p > 0)
        .ok_or_else(|| error("Invalid game process ID."))?;
    // Signal zero tests existence without sending a signal; ESRCH proves termination.
    Ok(unsafe { kill(pid, 0) } != 0 && std::io::Error::last_os_error().raw_os_error() == Some(3))
}
fn socket_path(pid: u32) -> PathBuf {
    PathBuf::from(format!(
        "/tmp/crimson-workbench-{}/items-{pid}.sock",
        unsafe { geteuid() }
    ))
}
pub fn transact(pid: u32, request: &[u8; 40]) -> Result<[u8; 48]> {
    transact_at(&socket_path(pid), pid, request)
}
fn transact_at(path: &Path, pid: u32, request: &[u8; 40]) -> Result<[u8; 48]> {
    let directory = fs::symlink_metadata(
        path.parent()
            .ok_or_else(|| error("Invalid connection path."))?,
    )
    .map_err(|_| {
        error(
            "Item module is not connected. Close the game and use Launch game with item spawning.",
        )
    })?;
    let uid = unsafe { geteuid() };
    if !directory.is_dir()
        || directory.uid() != uid
        || directory.permissions().mode() & 0o777 != 0o700
    {
        return Err(error(
            "The local item connection directory could not be verified.",
        ));
    }
    let socket = fs::symlink_metadata(path).map_err(|_| {
        error(
            "Item module is not connected. Close the game and use Launch game with item spawning.",
        )
    })?;
    if !socket.file_type().is_socket() || socket.uid() != uid {
        return Err(error("The local item connection could not be verified."));
    }
    let mut stream = UnixStream::connect(path).map_err(|e| error(e.to_string()))?;
    stream
        .set_read_timeout(Some(Duration::from_millis(1500)))
        .map_err(|e| error(e.to_string()))?;
    stream
        .set_write_timeout(Some(Duration::from_millis(1500)))
        .map_err(|e| error(e.to_string()))?;
    let (mut peer_uid, mut peer_gid, mut peer_pid, mut size) = (0, 0, 0i32, 4u32);
    let fd = stream.as_raw_fd();
    if unsafe { getpeereid(fd, &mut peer_uid, &mut peer_gid) } != 0
        || peer_uid != uid
        || unsafe { getsockopt(fd, 0, 0x002, (&mut peer_pid as *mut i32).cast(), &mut size) } != 0
        || size != 4
        || peer_pid as u32 != pid
    {
        return Err(error(
            "Could not verify the connection to the game process.",
        ));
    }
    // A timeout never resends a grant; the caller queries the original request ID.
    stream
        .write_all(request)
        .map_err(|e| error(e.to_string()))?;
    let mut reply = [0; 48];
    stream
        .read_exact(&mut reply)
        .map_err(|e| error(e.to_string()))?;
    Ok(reply)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::os::unix::net::UnixListener;
    #[test]
    fn live_process_is_never_reported_as_ended() {
        assert!(!process_exited(std::process::id()).unwrap());
        assert!(process_exited(0).is_err());
        assert!(process_exited(u32::MAX).is_err());
    }
    #[test]
    fn backup_copies_nested_saves_and_records_verified_hashes() {
        let root = tempfile::tempdir().unwrap();
        let source = root.path().join("saves");
        fs::create_dir_all(source.join("user")).unwrap();
        fs::write(source.join("user/slot"), b"synthetic save").unwrap();
        let destination = root.path().join("backup");
        backup_saves(&[source.clone()], &destination).unwrap();
        assert_eq!(
            fs::read(destination.join("0/user/slot")).unwrap(),
            b"synthetic save"
        );
        let manifest: serde_json::Value =
            serde_json::from_slice(&fs::read(destination.join("workbench-backup.json")).unwrap())
                .unwrap();
        assert_eq!(manifest["files"].as_array().unwrap().len(), 1);
        assert_eq!(
            manifest["files"][0]["sha256"],
            cd_core::fingerprint::hash_file(&source.join("user/slot"))
                .unwrap()
                .1
        );
        assert!(backup_saves(&[source], &destination).is_err());
    }
    #[test]
    fn backup_rejects_links_empty_sources_and_recursive_destination() {
        let root = tempfile::tempdir().unwrap();
        let source = root.path().join("saves");
        fs::create_dir(&source).unwrap();
        assert!(backup_saves(&[source.clone()], &source.join("backup")).is_err());
        fs::remove_dir(source.join("backup")).unwrap();
        assert!(backup_saves(&[source.clone()], &root.path().join("empty")).is_err());
        fs::write(root.path().join("outside"), b"private").unwrap();
        std::os::unix::fs::symlink(root.path().join("outside"), source.join("link")).unwrap();
        assert!(backup_saves(&[source], &root.path().join("linked")).is_err());
        assert!(!root.path().join("linked/workbench-backup.json").exists());
    }
    #[test]
    fn socket_exchange_checks_peer_and_transmits_one_request() {
        let root = tempfile::tempdir().unwrap();
        fs::set_permissions(root.path(), fs::Permissions::from_mode(0o700)).unwrap();
        let path = root.path().join("items.sock");
        let listener = UnixListener::bind(&path).unwrap();
        let server = std::thread::spawn(move || {
            let (mut peer, _) = listener.accept().unwrap();
            let mut request = [0; 40];
            peer.read_exact(&mut request).unwrap();
            assert_eq!(request, [7; 40]);
            peer.write_all(&[9; 48]).unwrap();
        });
        assert_eq!(
            transact_at(&path, std::process::id(), &[7; 40]).unwrap(),
            [9; 48]
        );
        server.join().unwrap();
    }
    #[test]
    fn socket_exchange_rejects_wrong_process_before_transmission() {
        let root = tempfile::tempdir().unwrap();
        fs::set_permissions(root.path(), fs::Permissions::from_mode(0o700)).unwrap();
        let path = root.path().join("items.sock");
        let listener = UnixListener::bind(&path).unwrap();
        let server = std::thread::spawn(move || {
            let (mut peer, _) = listener.accept().unwrap();
            let mut byte = [0];
            assert_eq!(peer.read(&mut byte).unwrap(), 0);
        });
        assert!(transact_at(&path, std::process::id() + 1, &[7; 40]).is_err());
        server.join().unwrap();
    }
}

#[derive(Serialize)]
pub struct Launch {
    pub pid: u32,
    pub backup_path: PathBuf,
}

fn backup_saves(sources: &[PathBuf], destination: &Path) -> Result<()> {
    use cd_core::fingerprint::hash_file;
    if sources.is_empty() {
        return Err(error(
            "No save folder found. Load and save the game normally before launching item spawning.",
        ));
    }
    fs::create_dir(destination).map_err(|e| error(e.to_string()))?;
    fs::set_permissions(destination, fs::Permissions::from_mode(0o700))
        .map_err(|e| error(e.to_string()))?;
    let destination = destination
        .canonicalize()
        .map_err(|e| error(e.to_string()))?;
    let mut files = Vec::new();
    let mut bytes = 0u64;
    for (index, source) in sources.iter().enumerate() {
        let source = source.canonicalize().map_err(|e| error(e.to_string()))?;
        if destination.starts_with(&source) {
            return Err(error("Backup folder must be outside the save folder."));
        }
        let target = destination.join(index.to_string());
        let mut todo = vec![(source.clone(), target, 0usize)];
        while let Some((from, to, depth)) = todo.pop() {
            let metadata = fs::symlink_metadata(&from).map_err(|e| error(e.to_string()))?;
            if metadata.file_type().is_symlink() {
                return Err(error("Save backup refuses symbolic links."));
            }
            if metadata.is_dir() {
                if depth > 32 {
                    return Err(error("Save folder nesting exceeds backup limits."));
                }
                fs::create_dir(&to).map_err(|e| error(e.to_string()))?;
                for entry in fs::read_dir(&from).map_err(|e| error(e.to_string()))? {
                    let entry = entry.map_err(|e| error(e.to_string()))?;
                    todo.push((entry.path(), to.join(entry.file_name()), depth + 1));
                }
            } else if metadata.is_file() {
                bytes = bytes
                    .checked_add(metadata.len())
                    .ok_or_else(|| error("Save backup is too large."))?;
                if bytes > 2 * 1024 * 1024 * 1024 || files.len() >= 10000 {
                    return Err(error("Save backup exceeds file or size limits."));
                }
                let before = hash_file(&from).map_err(|e| error(e.to_string()))?;
                fs::copy(&from, &to).map_err(|e| error(e.to_string()))?;
                let copied = hash_file(&to).map_err(|e| error(e.to_string()))?;
                let after = hash_file(&from).map_err(|e| error(e.to_string()))?;
                if before != copied || before != after {
                    return Err(error(
                        "A save changed during backup. Game launch cancelled.",
                    ));
                }
                fs::OpenOptions::new()
                    .write(true)
                    .open(&to)
                    .and_then(|f| f.sync_all())
                    .map_err(|e| error(e.to_string()))?;
                files.push(serde_json::json!({"source":from,"copy":to.strip_prefix(&destination).unwrap(),"bytes":copied.0,"sha256":copied.1}));
            } else {
                return Err(error("Save backup encountered a non-regular file."));
            }
        }
    }
    if files.is_empty() {
        return Err(error(
            "Save folder contains no files; game launch cancelled.",
        ));
    }
    let manifest = serde_json::to_vec_pretty(&serde_json::json!({"files":files}))
        .map_err(|e| error(e.to_string()))?;
    let mut file = fs::File::create(destination.join("workbench-backup.json"))
        .map_err(|e| error(e.to_string()))?;
    file.write_all(&manifest)
        .and_then(|_| file.sync_all())
        .map_err(|e| error(e.to_string()))?;
    Ok(())
}

pub fn launch(game: &Path, resources: &Path, data: &Path) -> Result<Launch> {
    static LAUNCH: std::sync::Mutex<()> = std::sync::Mutex::new(());
    let _guard = LAUNCH
        .lock()
        .map_err(|_| error("Game launcher unavailable."))?;
    if !game_pids(game)?.is_empty() {
        return Err(error(
            "Exit Crimson Desert normally before launching it with item spawning.",
        ));
    }
    let executable = executable(game)?;
    let (_, digest) =
        cd_core::fingerprint::hash_file(&executable).map_err(|e| error(e.to_string()))?;
    if digest != "fc0727d176ad7434e9fcea5b358cdb6b7c78ebb1f73975b76efb100c1c129a74" {
        return Err(error(
            "This Mac game build has not been verified for item spawning.",
        ));
    }
    let library = resources
        .join("libCrimsonItems.dylib")
        .canonicalize()
        .map_err(|_| {
            error("The bundled Mac item module is missing. Reinstall the Mac application.")
        })?;
    fs::create_dir_all(data.join("item-backups")).map_err(|e| error(e.to_string()))?;
    let stamp = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map_err(|e| error(e.to_string()))?
        .as_nanos();
    let backup_path = data.join("item-backups").join(stamp.to_string());
    backup_saves(&cd_core::discovery::locate_saves(), &backup_path)?;
    if !game_pids(game)?.is_empty() {
        return Err(error("The game started during backup. Launch cancelled."));
    }
    let log = fs::OpenOptions::new()
        .create_new(true)
        .write(true)
        .open(backup_path.join("runtime.log"))
        .map_err(|e| error(e.to_string()))?;
    let mut child = std::process::Command::new(executable)
        .env("DYLD_INSERT_LIBRARIES", library)
        .env("SteamAppId", "3321460")
        .env("SteamGameId", "3321460")
        .env_remove("CW_MAC_ITEMS_TEST_ONCE")
        .stdin(std::process::Stdio::null())
        .stdout(log.try_clone().map_err(|e| error(e.to_string()))?)
        .stderr(log)
        .spawn()
        .map_err(|e| {
            error(format!(
                "Game launch failed: {e}. Verified backup: {}",
                backup_path.display()
            ))
        })?;
    let pid = child.id();
    std::thread::spawn(move || {
        let _ = child.wait();
    });
    Ok(Launch { pid, backup_path })
}
