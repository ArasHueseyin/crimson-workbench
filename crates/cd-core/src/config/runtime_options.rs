//! Small per-user startup settings shared with the corresponding native modules.
use serde::{Deserialize, Serialize};
use std::{
    fs,
    io::{self, Read, Write},
    path::Path,
    sync::atomic::{AtomicU64, Ordering},
};

static NEXT: AtomicU64 = AtomicU64::new(0);
const BUILD: &str = "1.0.0.2976";
#[derive(Clone, Copy, Debug, Deserialize, Serialize, PartialEq, Eq)]
#[serde(rename_all = "snake_case")]
pub enum RuntimeOption {
    ForcePalmHeight,
    AxiomReach,
    GlideSpeed,
    PlayerResources,
}
impl RuntimeOption {
    fn file(self) -> &'static str {
        match self {
            Self::ForcePalmHeight => "force-palm-height.dat",
            Self::AxiomReach => "axiom-reach.dat",
            Self::GlideSpeed => "glide-speed.dat",
            Self::PlayerResources => "player-resources.dat",
        }
    }
    fn magic(self) -> &'static [u8; 4] {
        match self {
            Self::ForcePalmHeight => b"CWF1",
            Self::AxiomReach => b"CWA1",
            Self::GlideSpeed => b"CWG1",
            Self::PlayerResources => b"CWR1",
        }
    }
    fn default_value(self) -> u32 {
        if self == Self::PlayerResources {
            1
        } else {
            100
        }
    }
    fn invalid_value(self) -> u32 {
        if self == Self::PlayerResources {
            0
        } else {
            100
        }
    }
    fn valid(self, value: u32) -> bool {
        match self {
            Self::ForcePalmHeight => (100..=10_000).contains(&value),
            Self::AxiomReach => value == 100 || (200..=500).contains(&value),
            Self::GlideSpeed => (100..=500).contains(&value),
            Self::PlayerResources => value <= 3,
        }
    }
    fn label(self) -> &'static str {
        match self {
            Self::ForcePalmHeight => "Force Palm height",
            Self::AxiomReach => "Axiom reach",
            Self::GlideSpeed => "Gliding speed",
            Self::PlayerResources => "Player resource protection",
        }
    }
}
#[derive(Debug, Serialize)]
pub struct Snapshot {
    pub id: RuntimeOption,
    pub value: u32,
    pub invalid: bool,
    pub game_running: bool,
    pub can_change: bool,
    pub message: String,
}
fn invalid() -> io::Error {
    io::Error::new(
        io::ErrorKind::InvalidData,
        "Invalid runtime startup setting.",
    )
}
fn decode(id: RuntimeOption, bytes: &[u8]) -> io::Result<u32> {
    if bytes.len() != 8 || &bytes[..4] != id.magic() {
        return Err(invalid());
    }
    let value = u32::from_le_bytes(bytes[4..8].try_into().unwrap());
    if !id.valid(value) {
        return Err(invalid());
    }
    Ok(value)
}
fn load(directory: &Path, id: RuntimeOption) -> io::Result<u32> {
    let path = directory.join(id.file());
    let meta = match fs::symlink_metadata(&path) {
        Ok(meta) => meta,
        Err(e) if e.kind() == io::ErrorKind::NotFound => return Ok(id.default_value()),
        Err(e) => return Err(e),
    };
    super::ordinary_settings_file(&path, &meta)?;
    let mut bytes = Vec::new();
    fs::File::open(path)?.take(9).read_to_end(&mut bytes)?;
    decode(id, &bytes)
}
fn read_value(directory: &Path, id: RuntimeOption) -> io::Result<(u32, bool)> {
    match load(directory, id) {
        Ok(value) => Ok((value, false)),
        Err(e) if e.kind() == io::ErrorKind::InvalidData => Ok((id.invalid_value(), true)),
        Err(e) => Err(e),
    }
}
pub fn snapshot(
    directory: &Path,
    version: Option<&str>,
    id: RuntimeOption,
) -> io::Result<Snapshot> {
    let running = crate::apply::guard::game_running()?;
    let (value, invalid) = read_value(directory, id)?;
    Ok(Snapshot {
        id,
        value,
        invalid,
        game_running: running,
        can_change: !running && version == Some(BUILD),
        message: if invalid {
            format!(
                "Saved {} settings are invalid. Normal behavior is used until you save a new setting.",
                id.label()
            )
        } else {
            format!(
                "{} settings load at game startup when the corresponding mod is installed.",
                id.label()
            )
        },
    })
}
pub fn save(
    directory: &Path,
    version: Option<&str>,
    id: RuntimeOption,
    value: u32,
) -> io::Result<Snapshot> {
    if version != Some(BUILD) {
        return Err(io::Error::other(
            "Runtime settings require game version 1.0.0.2976.",
        ));
    }
    store(directory, id, value, crate::apply::guard::game_running)?;
    snapshot(directory, version, id)
}
fn store(
    directory: &Path,
    id: RuntimeOption,
    value: u32,
    mut running: impl FnMut() -> io::Result<bool>,
) -> io::Result<()> {
    let mut bytes = [0; 8];
    bytes[..4].copy_from_slice(id.magic());
    bytes[4..].copy_from_slice(&value.to_le_bytes());
    decode(id, &bytes)?;
    let stopped = |running: &mut dyn FnMut() -> io::Result<bool>| -> io::Result<()> {
        if running()? {
            Err(io::Error::other(
                "Close Crimson Desert before saving runtime settings.",
            ))
        } else {
            Ok(())
        }
    };
    stopped(&mut running)?;
    fs::create_dir_all(directory)?;
    let destination = directory.join(id.file());
    match fs::symlink_metadata(&destination) {
        Ok(meta) => super::ordinary_settings_file(&destination, &meta)?,
        Err(e) if e.kind() == io::ErrorKind::NotFound => {}
        Err(e) => return Err(e),
    }
    let temporary = directory.join(format!(
        ".{}-{}-{}.tmp",
        id.file(),
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
        if load(directory, id)? != value {
            return Err(io::Error::other("Saved runtime setting did not verify."));
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
    fn independent_settings_preserve_legacy_defaults_and_exact_bytes() {
        let root = tempfile::tempdir().unwrap();
        let dir = root.path().join("friend's gämé settings");
        assert_eq!(load(&dir, RuntimeOption::PlayerResources).unwrap(), 1);
        assert_eq!(load(&dir, RuntimeOption::ForcePalmHeight).unwrap(), 100);
        assert_eq!(load(&dir, RuntimeOption::GlideSpeed).unwrap(), 100);
        for (id, value) in [
            (RuntimeOption::ForcePalmHeight, 10_000),
            (RuntimeOption::AxiomReach, 250),
            (RuntimeOption::GlideSpeed, 350),
            (RuntimeOption::PlayerResources, 2),
        ] {
            store(&dir, id, value, || Ok(false)).unwrap();
            assert_eq!(load(&dir, id).unwrap(), value);
            let bytes = fs::read(dir.join(id.file())).unwrap();
            assert_eq!(&bytes[..4], id.magic());
            assert_eq!(&bytes[4..], &value.to_le_bytes());
        }
        store(&dir, RuntimeOption::ForcePalmHeight, 100, || Ok(false)).unwrap();
        assert_eq!(load(&dir, RuntimeOption::AxiomReach).unwrap(), 250);
        assert_eq!(load(&dir, RuntimeOption::GlideSpeed).unwrap(), 350);
        assert_eq!(load(&dir, RuntimeOption::PlayerResources).unwrap(), 2);
        assert_eq!(fs::read_dir(&dir).unwrap().count(), 4);
    }
    #[test]
    fn bounds_corruption_and_game_start_during_save_preserve_previous_settings() {
        let root = tempfile::tempdir().unwrap();
        let dir = root.path();
        for (id, values) in [
            (
                RuntimeOption::ForcePalmHeight,
                vec![0, 99, 10_001, u32::MAX],
            ),
            (RuntimeOption::AxiomReach, vec![0, 99, 101, 199, 501]),
            (RuntimeOption::GlideSpeed, vec![0, 99, 501, u32::MAX]),
            (RuntimeOption::PlayerResources, vec![4, u32::MAX]),
        ] {
            for value in values {
                assert!(store(dir, id, value, || Ok(false)).is_err());
            }
            assert!(!dir.join(id.file()).exists());
            store(dir, id, id.default_value(), || Ok(false)).unwrap();
            fs::write(dir.join(id.file()), b"damaged").unwrap();
            assert_eq!(read_value(dir, id).unwrap(), (id.invalid_value(), true));
            store(dir, id, id.default_value(), || Ok(false)).unwrap();
            assert_eq!(read_value(dir, id).unwrap(), (id.default_value(), false));
        }
        let id = RuntimeOption::ForcePalmHeight;
        assert!(store(dir, id, 200, || Ok(true)).is_err());
        assert!(store(dir, id, 200, || Err(io::Error::other("Unknown game state"))).is_err());
        let mut checks = 0;
        assert!(
            store(dir, id, 200, || {
                checks += 1;
                Ok(checks == 2)
            })
            .is_err()
        );
        assert_eq!(load(dir, id).unwrap(), 100);
        assert_eq!(fs::read_dir(dir).unwrap().count(), 4);
    }
    #[test]
    fn wrong_magic_extra_bytes_and_linked_settings_are_refused() {
        let root = tempfile::tempdir().unwrap();
        let id = RuntimeOption::PlayerResources;
        assert!(decode(id, b"CWF1\x01\0\0\0").is_err());
        assert!(decode(id, b"CWR1\x01\0\0\0x").is_err());
        let original = root.path().join("original.dat");
        fs::write(&original, b"CWR1\x02\0\0\0").unwrap();
        let linked = root.path().join(id.file());
        fs::hard_link(&original, &linked).unwrap();
        assert!(load(root.path(), id).is_err());
        assert!(store(root.path(), id, 3, || Ok(false)).is_err());
        assert_eq!(fs::read(original).unwrap(), b"CWR1\x02\0\0\0");
    }
}
