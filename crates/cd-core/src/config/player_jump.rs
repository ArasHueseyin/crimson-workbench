//! Per-user runtime setting shared with CrimsonPlayerJump. Never writes game files.
use serde::Serialize;
use std::{
    fs,
    io::{self, Read, Write},
    path::Path,
    sync::atomic::{AtomicU64, Ordering},
};

pub const FILE_NAME: &str = "player-jump.dat";
const BUILD: &str = "1.0.0.2976";
static NEXT: AtomicU64 = AtomicU64::new(0);

#[derive(Debug, Serialize)]
pub struct Snapshot {
    pub height_percent: u32,
    pub game_running: bool,
    pub can_change: bool,
    pub message: String,
}

fn decode(bytes: &[u8]) -> io::Result<u32> {
    if bytes.len() != 8 || &bytes[..4] != b"CWJ1" {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "Invalid player jump settings.",
        ));
    }
    let percent = u32::from_le_bytes(bytes[4..8].try_into().unwrap());
    if !(100..=300).contains(&percent) {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "Jump height must be between 100% and 300%.",
        ));
    }
    Ok(percent)
}

fn load(directory: &Path) -> io::Result<u32> {
    let path = directory.join(FILE_NAME);
    let meta = match fs::symlink_metadata(&path) {
        Ok(meta) => meta,
        Err(e) if e.kind() == io::ErrorKind::NotFound => return Ok(100),
        Err(e) => return Err(e),
    };
    super::ordinary_settings_file(&path, &meta)?;
    let mut bytes = Vec::new();
    fs::File::open(path)?.take(9).read_to_end(&mut bytes)?;
    decode(&bytes)
}

pub fn snapshot(directory: &Path, version: Option<&str>) -> io::Result<Snapshot> {
    let running = crate::apply::guard::game_running()?;
    let (height_percent, invalid) = match load(directory) {
        Ok(percent) => (percent, false),
        Err(e) if e.kind() == io::ErrorKind::InvalidData => (100, true),
        Err(e) => return Err(e),
    };
    Ok(Snapshot {
        height_percent,
        game_running: running,
        can_change: !running && version == Some(BUILD),
        message: if invalid {
            "Saved jump settings are invalid. Normal height is used until you save a new setting."
                .into()
        } else {
            "Settings load at game startup when the player jump mod is installed.".into()
        },
    })
}

pub fn save(directory: &Path, version: Option<&str>, percent: u32) -> io::Result<Snapshot> {
    if version != Some(BUILD) {
        return Err(io::Error::other(
            "Player jump settings require game version 1.0.0.2976.",
        ));
    }
    store(directory, percent, crate::apply::guard::game_running)?;
    snapshot(directory, version)
}

fn store(
    directory: &Path,
    percent: u32,
    mut running: impl FnMut() -> io::Result<bool>,
) -> io::Result<()> {
    let mut bytes = *b"CWJ1\0\0\0\0";
    bytes[4..8].copy_from_slice(&percent.to_le_bytes());
    decode(&bytes)?;
    let stopped = |running: &mut dyn FnMut() -> io::Result<bool>| -> io::Result<()> {
        if running()? {
            Err(io::Error::other(
                "Close Crimson Desert before saving jump settings.",
            ))
        } else {
            Ok(())
        }
    };
    stopped(&mut running)?;
    fs::create_dir_all(directory)?;
    let destination = directory.join(FILE_NAME);
    match fs::symlink_metadata(&destination) {
        Ok(meta) => super::ordinary_settings_file(&destination, &meta)?,
        Err(e) if e.kind() == io::ErrorKind::NotFound => {}
        Err(e) => return Err(e),
    }
    let temporary = directory.join(format!(
        ".player-jump-{}-{}.tmp",
        std::process::id(),
        NEXT.fetch_add(1, Ordering::Relaxed)
    ));
    let mut file = fs::OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(&temporary)?;
    let result = (|| {
        file.write_all(&bytes)?;
        file.sync_all()?;
        drop(file);
        stopped(&mut running)?;
        #[cfg(windows)]
        {
            use std::os::windows::ffi::OsStrExt;
            use windows_sys::Win32::Storage::FileSystem::{
                MOVEFILE_REPLACE_EXISTING, MOVEFILE_WRITE_THROUGH, MoveFileExW,
            };
            let from: Vec<u16> = temporary.as_os_str().encode_wide().chain(Some(0)).collect();
            let to: Vec<u16> = destination
                .as_os_str()
                .encode_wide()
                .chain(Some(0))
                .collect();
            if unsafe {
                MoveFileExW(
                    from.as_ptr(),
                    to.as_ptr(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH,
                )
            } == 0
            {
                return Err(io::Error::last_os_error());
            }
        }
        #[cfg(not(windows))]
        {
            fs::rename(&temporary, &destination)?;
            fs::File::open(directory)?.sync_all()?;
        }
        if load(directory)? != percent {
            return Err(io::Error::other("Saved jump setting did not verify."));
        }
        Ok(())
    })();
    if result.is_err() {
        let _ = fs::remove_file(&temporary);
    }
    result
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn settings_roundtrip_and_atomic_replacement_preserve_other_files() {
        let root = tempfile::tempdir().unwrap();
        let directory = root.path().join("friend's gämé & settings");
        assert_eq!(load(&directory).unwrap(), 100);
        store(&directory, 140, || Ok(false)).unwrap();
        assert_eq!(
            fs::read(directory.join(FILE_NAME)).unwrap(),
            b"CWJ1\x8c\0\0\0"
        );
        fs::write(directory.join("other.dat"), b"unrelated").unwrap();
        for percent in [100, 101, 150, 300] {
            store(&directory, percent, || Ok(false)).unwrap();
            assert_eq!(load(&directory).unwrap(), percent);
        }
        assert_eq!(fs::read(directory.join("other.dat")).unwrap(), b"unrelated");
        assert_eq!(fs::read_dir(directory).unwrap().count(), 2);
    }
    #[test]
    fn malformed_settings_and_running_or_unknown_process_state_fail_closed() {
        for bytes in [
            b"".as_slice(),
            b"CWJ2\x96\0\0\0",
            b"CWJ1\x63\0\0\0",
            b"CWJ1\x2d\x01\0\0",
            b"CWJ1\x96\0\0\0x",
        ] {
            assert!(decode(bytes).is_err());
        }
        let root = tempfile::tempdir().unwrap();
        let directory = root.path().join("settings");
        assert!(store(&directory, 150, || Ok(true)).is_err());
        assert!(
            store(&directory, 150, || Err(io::Error::other(
                "unknown process state"
            )))
            .is_err()
        );
        assert!(!directory.exists());
        store(&directory, 140, || Ok(false)).unwrap();
        let mut checks = 0;
        assert!(
            store(&directory, 200, || {
                checks += 1;
                Ok(checks == 2)
            })
            .is_err()
        );
        assert_eq!(load(&directory).unwrap(), 140);
        assert_eq!(fs::read_dir(&directory).unwrap().count(), 1);
        for value in [0, 99, 301, u32::MAX] {
            assert!(store(&directory, value, || Ok(false)).is_err());
        }
        assert_eq!(load(&directory).unwrap(), 140);
        fs::write(directory.join(FILE_NAME), b"damaged").unwrap();
        store(&directory, 100, || Ok(false)).unwrap();
        assert_eq!(load(&directory).unwrap(), 100);
    }
}
