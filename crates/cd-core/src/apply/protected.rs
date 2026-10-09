//! Held launch/source protection around the entire private transaction session.
//! Project copies and explicitly admitted live sources keep distinct provenance.
use super::transaction::{self, Plan, checked, plain};
use crate::{
    Error, Result,
    fingerprint::{FileHash, hash_bytes, hash_file},
};
use serde::{Deserialize, Serialize};
use std::{
    collections::{BTreeMap, BTreeSet},
    fs::File,
    path::{Path, PathBuf},
};

fn bad(message: impl Into<String>) -> Error {
    Error::Invalid(message.into())
}

#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub(super) struct Baseline {
    version: u32,
    kind: String,
    pub registry_sha256: String,
    sources: BTreeMap<String, Source>,
    executables: BTreeSet<String>,
}
#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Source {
    bytes: u64,
    sha256: String,
}
impl Baseline {
    pub(super) fn admitted(
        registry_sha256: String,
        files: &[crate::fingerprint::FileHash],
    ) -> Result<Self> {
        let sources: BTreeMap<_, _> = files
            .iter()
            .filter(|f| f.path != "meta/0.papgt")
            .map(|f| {
                (
                    f.path.clone(),
                    Source {
                        bytes: f.bytes,
                        sha256: f.sha256.clone(),
                    },
                )
            })
            .collect();
        let executables = sources
            .keys()
            .filter(|p| p.to_ascii_lowercase().ends_with(".exe"))
            .cloned()
            .collect();
        let value = Self {
            version: 1,
            kind: "steam-verified-by-user".into(),
            registry_sha256,
            sources,
            executables,
        };
        if files.len() != value.sources.len() + 1 {
            return Err(bad("duplicate or missing admission files"));
        }
        value.validate()?;
        Ok(value)
    }
    pub fn capture(root: &Path, sources: &[&str], executables: &[&str]) -> Result<Self> {
        plain(root, true)?;
        let mut value = Self {
            version: 1,
            kind: "project-copy-only".into(),
            registry_sha256: hash_file(&checked(root, "meta/0.papgt")?)?.1,
            sources: BTreeMap::new(),
            executables: executables.iter().map(|s| (*s).into()).collect(),
        };
        for &path in sources {
            let (bytes, sha256) = hash_file(&checked(root, path)?)?;
            if value
                .sources
                .insert(path.into(), Source { bytes, sha256 })
                .is_some()
            {
                return Err(bad("duplicate protected source"));
            }
        }
        value.validate()?;
        Ok(value)
    }
    fn validate(&self) -> Result<()> {
        let digest = |s: &str| {
            s.len() == 64
                && s.bytes()
                    .all(|b| b.is_ascii_digit() || (b'a'..=b'f').contains(&b))
        };
        if self.version != 1
            || !matches!(
                self.kind.as_str(),
                "project-copy-only" | "steam-verified-by-user"
            )
            || !digest(&self.registry_sha256)
            || self.sources.is_empty()
            || self.sources.len() > 1024
            || self.executables.is_empty()
            || self
                .executables
                .iter()
                .any(|p| !self.sources.contains_key(p) || !p.to_ascii_lowercase().ends_with(".exe"))
        {
            return Err(bad("invalid project protection baseline"));
        }
        let mut names = BTreeSet::new();
        for (path, source) in &self.sources {
            let lower = path.to_ascii_lowercase();
            if !names.insert(lower.clone())
                || lower == "meta/0.papgt"
                || lower.starts_with(".workbench/")
                || !digest(&source.sha256)
                || path.split('/').any(|p| {
                    p.is_empty()
                        || p == "."
                        || p == ".."
                        || p.contains(['\\', ':'])
                        || p.ends_with(['.', ' '])
                })
            {
                return Err(bad("invalid protected source path or digest"));
            }
        }
        Ok(())
    }
    pub fn decode(bytes: &[u8], expected: &str) -> Result<Self> {
        if bytes.len() > 256 * 1024 || hash_bytes(bytes) != expected {
            return Err(bad("protection manifest differs from rehearsal marker"));
        }
        let value: Self = serde_json::from_slice(bytes)?;
        value.validate()?;
        if value.kind != "project-copy-only" {
            return Err(bad("rehearsal cannot import live admission"));
        }
        Ok(value)
    }
    pub fn file_count(&self) -> usize {
        self.sources.len()
    }
}

struct Lease {
    files: Vec<File>,
    executables: Vec<PathBuf>,
    // Deny rename/delete of the root and every protected parent until files close.
    _directories: Vec<File>,
}
impl Lease {
    #[cfg(windows)]
    fn acquire(
        root: &Path,
        baseline: &Baseline,
        check: &mut impl FnMut() -> Result<()>,
    ) -> Result<Self> {
        use sha2::{Digest, Sha256};
        use std::fs::OpenOptions;
        use std::{
            io::{Read, Seek},
            os::windows::fs::OpenOptionsExt,
        };
        baseline.validate()?;
        check()?;
        plain(root, true)?;
        let executables: Vec<_> = baseline.executables.iter().map(|p| root.join(p)).collect();
        if super::guard::paths_running(&executables)? {
            return Err(bad("protected executable is already running"));
        }
        let mut directories = BTreeSet::from([root.to_owned(), root.join("meta")]);
        // Resolve every candidate before taking exclusive file handles: plain()
        // itself opens a file to inspect its hardlink count.
        let mut paths: Vec<_> = baseline
            .sources
            .iter()
            .map(|(name, source)| Ok((name, source, checked(root, name)?)))
            .collect::<Result<_>>()?;
        // Lock all known launch paths before reading any large archive. Each
        // opened source remains held until the complete transaction finishes.
        paths.sort_by_key(|(name, _, _)| !baseline.executables.contains(*name));
        for (_, _, path) in &paths {
            let mut parent = path.parent().unwrap();
            while parent != root {
                if !parent.starts_with(root) {
                    return Err(bad("protected source escaped root"));
                }
                directories.insert(parent.to_owned());
                parent = parent
                    .parent()
                    .ok_or_else(|| bad("missing protected parent"))?;
            }
        }
        let mut held = Self {
            files: Vec::new(),
            executables,
            _directories: Vec::new(),
        };
        for dir in directories {
            plain(&dir, true)?;
            let file = OpenOptions::new()
                .read(true)
                .share_mode(3)
                .custom_flags(0x02200000)
                .open(&dir)?;
            verify_handle(&file, true)?;
            held._directories.push(file);
        }
        let mut opened = Vec::new();
        for (name, source, path) in paths {
            check()?;
            let file = OpenOptions::new()
                .read(true)
                .share_mode(if baseline.executables.contains(name) {
                    0
                } else {
                    1
                })
                .custom_flags(0x00200000)
                .open(path)?;
            verify_handle(&file, false)?;
            opened.push((name, source, file));
        }
        for (name, source, mut file) in opened {
            check()?;
            let mut hasher = Sha256::new();
            let mut count = 0u64;
            let mut buffer = [0; 128 * 1024];
            let mut last_check = std::time::Instant::now();
            loop {
                let n = file.read(&mut buffer)?;
                if n == 0 {
                    break;
                }
                hasher.update(&buffer[..n]);
                count += n as u64;
                if count > source.bytes {
                    return Err(bad(format!("protected source grew: {name}")));
                }
                if last_check.elapsed().as_millis() >= 250 {
                    check()?;
                    last_check = std::time::Instant::now();
                }
            }
            if count != source.bytes || format!("{:x}", hasher.finalize()) != source.sha256 {
                return Err(bad(format!(
                    "protected source mismatch: {name}; update or foreign change, no restore allowed"
                )));
            }
            file.rewind()?;
            held.files.push(file);
        }
        check()?;
        held.check()?;
        Ok(held)
    }
    #[cfg(windows)]
    fn hold_backup(&mut self, root: &Path, expected: &str) -> Result<()> {
        use std::{fs::OpenOptions, io::Read, os::windows::fs::OpenOptionsExt};
        let directory = root.join(".workbench");
        plain(&directory, true)?;
        let dir = OpenOptions::new()
            .read(true)
            .share_mode(3)
            .custom_flags(0x02200000)
            .open(&directory)?;
        verify_handle(&dir, true)?;
        let mut file = OpenOptions::new()
            .read(true)
            .share_mode(1)
            .custom_flags(0x00200000)
            .open(directory.join("baseline.papgt"))?;
        verify_handle(&file, false)?;
        let mut bytes = Vec::new();
        (&mut file)
            .take(crimson_format::MAX_FILE_BYTES as u64 + 1)
            .read_to_end(&mut bytes)?;
        if bytes.len() > crimson_format::MAX_FILE_BYTES || hash_bytes(&bytes) != expected {
            return Err(bad("protected registry backup mismatch"));
        }
        self.files.push(file);
        self._directories.push(dir);
        Ok(())
    }
    #[cfg(not(windows))]
    fn hold_backup(&mut self, _: &Path, _: &str) -> Result<()> {
        Err(bad("held protection unavailable"))
    }
    #[cfg(not(windows))]
    fn acquire(_: &Path, _: &Baseline, _: &mut impl FnMut() -> Result<()>) -> Result<Self> {
        Err(bad(
            "Held launch protection requires Windows; no unguarded fallback",
        ))
    }
    fn check(&self) -> Result<()> {
        if super::guard::paths_running(&self.executables)? {
            return Err(bad("protected executable is running; transaction stopped"));
        }
        for file in &self.files {
            verify_handle(file, false)?;
        }
        for file in &self._directories {
            verify_handle(file, true)?;
        }
        Ok(())
    }
}

#[cfg(windows)]
fn verify_handle(file: &File, directory: bool) -> Result<()> {
    use std::os::windows::io::AsRawHandle;
    use windows_sys::Win32::Storage::FileSystem::{
        BY_HANDLE_FILE_INFORMATION, GetFileInformationByHandle,
    };
    let mut info: BY_HANDLE_FILE_INFORMATION = unsafe { std::mem::zeroed() };
    // SAFETY: file owns this handle; info has the Win32 structure's exact layout.
    if unsafe { GetFileInformationByHandle(file.as_raw_handle(), &mut info) } == 0 {
        return Err(std::io::Error::last_os_error().into());
    }
    if info.dwFileAttributes & 0x400 != 0
        || (info.dwFileAttributes & 0x10 != 0) != directory
        || (!directory && info.nNumberOfLinks != 1)
    {
        return Err(bad("protected handle is linked or has an unexpected type"));
    }
    Ok(())
}
#[cfg(not(windows))]
fn verify_handle(_: &File, _: bool) -> Result<()> {
    Err(bad("held protection unavailable"))
}

pub(super) struct Session<F: FnMut() -> Result<()>> {
    engine: transaction::Engine,
    lease: Lease,
    check: F,
}
/// Pin already moved archive bytes until the new basis is published.
#[cfg(windows)]
pub(super) fn hold_archived(
    root: &Path,
    files: &[FileHash],
    check: &mut dyn FnMut() -> Result<()>,
) -> Result<Vec<File>> {
    use sha2::{Digest, Sha256};
    use std::{fs::OpenOptions, io::Read, os::windows::fs::OpenOptionsExt};
    let mut held = Vec::new();
    for source in files {
        check()?;
        let mut file = OpenOptions::new()
            .read(true)
            .share_mode(1)
            .custom_flags(0x00200000)
            .open(checked(root, &source.path)?)?;
        verify_handle(&file, false)?;
        let mut hash = Sha256::new();
        let mut count = 0;
        let mut buffer = [0; 128 * 1024];
        let mut last_check = std::time::Instant::now();
        loop {
            let n = file.read(&mut buffer)?;
            if n == 0 {
                break;
            }
            count += n as u64;
            if count > source.bytes {
                return Err(bad("archive source grew"));
            }
            hash.update(&buffer[..n]);
            if last_check.elapsed().as_millis() >= 250 {
                check()?;
                last_check = std::time::Instant::now();
            }
        }
        if count != source.bytes || format!("{:x}", hash.finalize()) != source.sha256 {
            return Err(bad("archive source changed"));
        }
        held.push(file);
    }
    Ok(held)
}
#[cfg(not(windows))]
pub(super) fn hold_archived(
    _: &Path,
    _: &[FileHash],
    _: &mut dyn FnMut() -> Result<()>,
) -> Result<Vec<File>> {
    Err(bad("held archive protection requires Windows"))
}
/// Source protection for a basis migration. No old backup handle is held because
/// the entire old private state is atomically moved, never overwritten.
pub(super) fn with_sources<T>(
    root: &Path,
    baseline: &Baseline,
    mut check: impl FnMut() -> Result<()>,
    action: impl FnOnce(&mut dyn FnMut() -> Result<()>) -> Result<T>,
) -> Result<T> {
    let lease = Lease::acquire(root, baseline, &mut check)?;
    #[cfg(windows)]
    let _registry = {
        use std::{fs::OpenOptions, io::Read, os::windows::fs::OpenOptionsExt};
        let mut file = OpenOptions::new()
            .read(true)
            .share_mode(1)
            .custom_flags(0x00200000)
            .open(root.join("meta/0.papgt"))?;
        verify_handle(&file, false)?;
        let mut bytes = Vec::new();
        (&mut file).take(1_048_577).read_to_end(&mut bytes)?;
        if bytes.len() > 1_048_576 || hash_bytes(&bytes) != baseline.registry_sha256 {
            return Err(bad("migration registry changed"));
        }
        file
    };
    let mut guarded = || {
        check()?;
        lease.check()
    };
    guarded()?;
    action(&mut guarded)
}
impl<F: FnMut() -> Result<()>> Session<F> {
    pub fn open(root: &Path, baseline: &Baseline, mut check: F) -> Result<Self> {
        Self::open_mode(root, baseline, &mut check, true).map(|(engine, lease)| Self {
            engine,
            lease,
            check,
        })
    }
    pub fn open_existing(root: &Path, baseline: &Baseline, mut check: F) -> Result<Self> {
        Self::open_mode(root, baseline, &mut check, false).map(|(engine, lease)| Self {
            engine,
            lease,
            check,
        })
    }
    fn open_mode(
        root: &Path,
        baseline: &Baseline,
        check: &mut F,
        initialize: bool,
    ) -> Result<(transaction::Engine, Lease)> {
        let mut lease = Lease::acquire(root, baseline, check)?;
        // Backup/lock/history creation can occur only after sources and guards pass.
        let engine = if initialize {
            transaction::Engine::new(root, &baseline.registry_sha256)?
        } else {
            transaction::Engine::open_existing(root, &baseline.registry_sha256)?
        };
        lease.hold_backup(root, &baseline.registry_sha256)?;
        Ok((engine, lease))
    }
    pub fn restore_review(&mut self) -> Result<transaction::RestoreReview> {
        (self.check)()?;
        self.lease.check()?;
        self.engine.restore_review()
    }
    pub fn plan(&mut self, files: &BTreeMap<String, Vec<u8>>) -> Result<Plan> {
        (self.check)()?;
        self.lease.check()?;
        self.engine.plan(files)
    }
    pub fn apply(&mut self, plan: &Plan, fault: Option<usize>) -> Result<Vec<&'static str>> {
        let Self {
            engine,
            lease,
            check,
        } = self;
        engine.apply(
            plan,
            || {
                check()?;
                lease.check()
            },
            fault,
        )
    }
    pub fn recover(&mut self, fault: Option<usize>) -> Result<Vec<&'static str>> {
        let Self {
            engine,
            lease,
            check,
        } = self;
        engine.recover(
            || {
                check()?;
                lease.check()
            },
            fault,
        )
    }
    pub fn restore(&mut self, fault: Option<usize>) -> Result<Vec<&'static str>> {
        let Self {
            engine,
            lease,
            check,
        } = self;
        engine.restore(
            || {
                check()?;
                lease.check()
            },
            fault,
        )
    }
}

#[cfg(test)]
#[path = "protected_tests.rs"]
mod tests;
