//! Output paths are confined to a workspace and kept separate from installed data.
//! No method in this module writes to a game or save directory.
use std::fs::{self, File, OpenOptions};
use std::io::{self, Write};
use std::path::{Component, Path, PathBuf};
use std::sync::atomic::{AtomicU64, Ordering};

#[derive(Debug, Clone)]
pub struct PathPolicy {
    output_root: PathBuf,
    protected_roots: Vec<PathBuf>,
    protected_resolved: Vec<PathBuf>,
}

impl PathPolicy {
    /// Confine generated files to the current workspace. Prefer an explicit root
    /// when the application's working directory is not controlled by the caller.
    pub fn new(game: PathBuf, saves: Vec<PathBuf>) -> io::Result<Self> {
        Self::with_output_root(game, saves, std::env::current_dir()?)
    }

    pub fn with_output_root(
        game: PathBuf,
        saves: Vec<PathBuf>,
        output_root: PathBuf,
    ) -> io::Result<Self> {
        Self::from_roots(std::iter::once(game).chain(saves).collect(), output_root)
    }

    /// Protect every discovered installation/save directory, including configured
    /// paths that do not exist yet. Discovery is never a reason to omit a root.
    pub fn from_roots(protected_roots: Vec<PathBuf>, output_root: PathBuf) -> io::Result<Self> {
        if protected_roots.iter().any(|p| p.as_os_str().is_empty())
            || output_root.as_os_str().is_empty()
        {
            return Err(invalid("empty policy root"));
        }
        let output_root = resolve_existing_ancestor(&output_root)?;
        let protected_resolved = protected_roots
            .iter()
            .map(|p| resolve_existing_ancestor(p))
            .collect::<io::Result<Vec<_>>>()?;
        if protected_resolved.iter().any(|p| contains(p, &output_root)) {
            return Err(denied("output root is inside a game or save directory"));
        }
        let protected_roots = protected_roots
            .into_iter()
            .map(|p| {
                if p.is_absolute() {
                    Ok(p)
                } else {
                    Ok(std::env::current_dir()?.join(p))
                }
            })
            .collect::<io::Result<Vec<_>>>()?;
        Ok(Self {
            output_root,
            protected_roots,
            protected_resolved,
        })
    }

    pub fn output_root(&self) -> &Path {
        &self.output_root
    }

    /// An explicitly inspected game addition must not enter any other protected
    /// root (for example a configured save directory nested inside the game).
    pub(crate) fn check_additional_read(&self, game: &Path, path: &Path) -> io::Result<PathBuf> {
        let game = resolve_existing_ancestor(game)?;
        let path = resolve_existing_ancestor(path)?;
        if !contains(&game, &path)
            || contains(&path, &game)
            || self.protected_resolved.iter().any(|root| {
                !(contains(root, &game) && contains(&game, root)) && contains(root, &path)
            })
        {
            return Err(denied("additional file enters another protected root"));
        }
        Ok(path)
    }

    /// Return the physical destination after resolving existing symlinks/junctions.
    /// Existing destinations are always refused, including hardlinks and dangling
    /// symlinks. A caller must write to this returned path, not the unchecked input.
    pub fn check_output(&self, path: &Path) -> io::Result<PathBuf> {
        self.check_path(path, false)
    }

    /// A private derived SQLite cache may be reopened. Only ordinary files with
    /// one link are accepted, including all SQLite sidecars that may be opened.
    /// This check is not a lock against a concurrently hostile filesystem actor.
    pub fn check_cache(&self, path: &Path) -> io::Result<PathBuf> {
        let resolved = self.check_path(path, true)?;
        for suffix in ["-wal", "-shm", "-journal"] {
            let mut sidecar = resolved.as_os_str().to_os_string();
            sidecar.push(suffix);
            self.check_path(Path::new(&sidecar), true)?;
        }
        Ok(resolved)
    }

    fn check_path(&self, path: &Path, allow_existing: bool) -> io::Result<PathBuf> {
        if path.as_os_str().is_empty() {
            return Err(invalid("empty output path"));
        }
        let absolute = if path.is_absolute() {
            path.to_path_buf()
        } else {
            self.output_root.join(path)
        };
        validate_components(&absolute)?;
        match fs::symlink_metadata(&absolute) {
            Ok(meta) => {
                if !allow_existing {
                    return Err(io::Error::new(
                        io::ErrorKind::AlreadyExists,
                        "output already exists",
                    ));
                }
                if !meta.is_file() || is_reparse(&meta) {
                    return Err(denied(
                        "cache must be a regular file, not a link or reparse point",
                    ));
                }
                if link_count(&absolute, &meta)? != 1 {
                    return Err(denied("cache has multiple hardlinks"));
                }
            }
            Err(e) if e.kind() == io::ErrorKind::NotFound => {}
            Err(e) => return Err(e),
        }
        let resolved = resolve_existing_ancestor(&absolute)?;
        if !contains(&self.output_root, &resolved) || equal_path(&self.output_root, &resolved) {
            return Err(denied(
                "output must be a file inside the workspace output root",
            ));
        }
        for (original, initial) in self.protected_roots.iter().zip(&self.protected_resolved) {
            // A configured missing root may later become a junction/symlink. Keep
            // both its initial location and its current physical target protected.
            let current = resolve_existing_ancestor(original)?;
            if contains(initial, &resolved) || contains(&current, &resolved) {
                return Err(denied(
                    "output is inside a protected game or save directory",
                ));
            }
        }
        Ok(resolved)
    }

    /// Commit a new output atomically without replacing any existing destination.
    /// The temporary file and final name are on the same filesystem. Linking a
    /// complete synced file is atomic and fails if the destination already exists.
    pub fn write_new(&self, path: &Path, bytes: &[u8]) -> io::Result<PathBuf> {
        let resolved = self.check_output(path)?;
        let parent = resolved
            .parent()
            .ok_or_else(|| invalid("output has no parent"))?;
        fs::create_dir_all(parent)?;
        let resolved = self.check_output(&resolved)?;
        let parent = resolved
            .parent()
            .ok_or_else(|| invalid("output has no parent"))?;
        let (temporary, mut file) = new_temporary(parent)?;
        let result = (|| {
            file.write_all(bytes)?;
            file.sync_all()?;
            // Revalidate after preparation, before linking the complete file.
            self.check_output(&resolved)?;
            fs::hard_link(&temporary, &resolved)?;
            Ok(resolved)
        })();
        drop(file);
        let cleanup = fs::remove_file(&temporary);
        match (result, cleanup) {
            (Ok(path), Ok(())) => Ok(path),
            (Ok(_), Err(e)) => Err(e),
            (Err(e), _) => Err(e),
        }
    }
}

fn new_temporary(parent: &Path) -> io::Result<(PathBuf, File)> {
    static NEXT: AtomicU64 = AtomicU64::new(0);
    for _ in 0..100 {
        let serial = NEXT.fetch_add(1, Ordering::Relaxed);
        let path = parent.join(format!(
            ".crimson-output-{}-{serial}.tmp",
            std::process::id()
        ));
        match OpenOptions::new().write(true).create_new(true).open(&path) {
            Ok(file) => return Ok((path, file)),
            Err(e) if e.kind() == io::ErrorKind::AlreadyExists => continue,
            Err(e) => return Err(e),
        }
    }
    Err(io::Error::new(
        io::ErrorKind::AlreadyExists,
        "cannot reserve temporary output",
    ))
}

fn invalid(message: &str) -> io::Error {
    io::Error::new(io::ErrorKind::InvalidInput, message)
}
fn denied(message: &str) -> io::Error {
    io::Error::new(io::ErrorKind::PermissionDenied, message)
}

// Reject parent traversals rather than changing their meaning around a symlink.
// On Windows ':' in a normal component would address an alternate data stream.
fn validate_components(path: &Path) -> io::Result<()> {
    for component in path.components() {
        match component {
            Component::ParentDir => return Err(invalid("parent traversal is not allowed")),
            Component::Normal(name) => {
                #[cfg(windows)]
                {
                    let name = name.to_string_lossy();
                    if name.contains(':') || name.ends_with('.') || name.ends_with(' ') {
                        return Err(invalid("ambiguous Windows output component"));
                    }
                    let base = name.split('.').next().unwrap_or("").to_ascii_uppercase();
                    if matches!(
                        base.as_str(),
                        "CON" | "PRN" | "AUX" | "NUL" | "CONIN$" | "CONOUT$"
                    ) || base.strip_prefix("COM").is_some_and(|n| {
                        matches!(n, "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9")
                    }) || base.strip_prefix("LPT").is_some_and(|n| {
                        matches!(n, "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9")
                    }) {
                        return Err(invalid("reserved Windows output component"));
                    }
                }
                #[cfg(not(windows))]
                let _ = name;
            }
            _ => {}
        }
    }
    Ok(())
}

/// Resolve as much of a path as exists; append only ordinary non-existing names.
/// Dangling links and inaccessible ancestors fail closed instead of being treated
/// as missing. Canonicalization also resolves Windows junctions and drive aliases.
fn resolve_existing_ancestor(path: &Path) -> io::Result<PathBuf> {
    let mut current = if path.is_absolute() {
        path.to_path_buf()
    } else {
        std::env::current_dir()?.join(path)
    };
    validate_components(&current)?;
    let mut missing = Vec::new();
    loop {
        match fs::symlink_metadata(&current) {
            Ok(_) => {
                let mut resolved = fs::canonicalize(&current)?;
                if !missing.is_empty() && !resolved.is_dir() {
                    return Err(invalid("output ancestor is not a directory"));
                }
                for part in missing.iter().rev() {
                    resolved.push(part);
                }
                return Ok(resolved);
            }
            Err(e) if e.kind() == io::ErrorKind::NotFound => {
                let name = current
                    .file_name()
                    .ok_or_else(|| invalid("path has no existing ancestor"))?
                    .to_os_string();
                missing.push(name);
                if !current.pop() {
                    return Err(invalid("path has no existing ancestor"));
                }
            }
            Err(e) => return Err(e),
        }
    }
}

fn equal_component(left: Component<'_>, right: Component<'_>) -> bool {
    #[cfg(windows)]
    {
        left.as_os_str().to_string_lossy().to_lowercase()
            == right.as_os_str().to_string_lossy().to_lowercase()
    }
    #[cfg(not(windows))]
    {
        left == right
    }
}
fn contains(root: &Path, candidate: &Path) -> bool {
    let mut candidate = candidate.components();
    root.components().all(|part| {
        candidate
            .next()
            .is_some_and(|other| equal_component(part, other))
    })
}
fn equal_path(left: &Path, right: &Path) -> bool {
    left.components().count() == right.components().count() && contains(left, right)
}

fn is_reparse(meta: &fs::Metadata) -> bool {
    #[cfg(windows)]
    {
        use std::os::windows::fs::MetadataExt;
        meta.file_type().is_symlink() || meta.file_attributes() & 0x400 != 0
    }
    #[cfg(not(windows))]
    {
        meta.file_type().is_symlink()
    }
}
#[cfg(unix)]
pub(crate) fn link_count(_: &Path, meta: &fs::Metadata) -> io::Result<u64> {
    use std::os::unix::fs::MetadataExt;
    Ok(meta.nlink())
}
#[cfg(windows)]
pub(crate) fn link_count(path: &Path, _: &fs::Metadata) -> io::Result<u64> {
    use std::os::windows::io::AsRawHandle;
    #[repr(C)]
    struct FileInfo {
        attributes: u32,
        creation: [u32; 2],
        access: [u32; 2],
        write: [u32; 2],
        volume: u32,
        size_high: u32,
        size_low: u32,
        links: u32,
        index_high: u32,
        index_low: u32,
    }
    #[link(name = "kernel32")]
    unsafe extern "system" {
        fn GetFileInformationByHandle(handle: *mut std::ffi::c_void, info: *mut FileInfo) -> i32;
    }
    let file = File::open(path)?;
    let mut info = FileInfo {
        attributes: 0,
        creation: [0; 2],
        access: [0; 2],
        write: [0; 2],
        volume: 0,
        size_high: 0,
        size_low: 0,
        links: 0,
        index_high: 0,
        index_low: 0,
    };
    // SAFETY: file owns a live handle and info has the Win32 C structure layout.
    if unsafe { GetFileInformationByHandle(file.as_raw_handle(), &mut info) } == 0 {
        return Err(io::Error::last_os_error());
    }
    Ok(u64::from(info.links))
}
#[cfg(not(any(unix, windows)))]
pub(crate) fn link_count(_: &Path, _: &fs::Metadata) -> io::Result<u64> {
    Err(io::Error::new(
        io::ErrorKind::Unsupported,
        "hardlink verification unavailable on this platform",
    ))
}

#[cfg(test)]
mod tests {
    use super::*;
    struct Fixture(PathBuf);
    impl Fixture {
        fn new() -> Self {
            static NEXT: AtomicU64 = AtomicU64::new(0);
            let root = std::env::temp_dir().join(format!(
                "cd-paths-{}-{}",
                std::process::id(),
                NEXT.fetch_add(1, Ordering::Relaxed)
            ));
            fs::create_dir_all(&root).unwrap();
            Self(root)
        }
        fn policy(&self) -> PathPolicy {
            for name in ["workspace", "game", "saves"] {
                fs::create_dir_all(self.0.join(name)).unwrap();
            }
            PathPolicy::with_output_root(
                self.0.join("game"),
                vec![self.0.join("saves")],
                self.0.join("workspace"),
            )
            .unwrap()
        }
    }
    impl Drop for Fixture {
        fn drop(&mut self) {
            let _ = fs::remove_dir_all(&self.0);
        }
    }

    #[test]
    fn outputs_are_scoped_and_never_overwrite() {
        let f = Fixture::new();
        let policy = f.policy();
        assert!(policy.check_output(&f.0.join("game/new/file")).is_err());
        assert!(policy.check_output(&f.0.join("saves/new.save")).is_err());
        assert!(policy.check_output(Path::new("../escape")).is_err());
        assert!(policy.check_output(&f.0.join("outside")).is_err());
        let path = policy
            .write_new(Path::new("nested/report.json"), b"first")
            .unwrap();
        assert_eq!(fs::read(&path).unwrap(), b"first");
        assert!(policy.write_new(&path, b"second").is_err());
        assert_eq!(fs::read(&path).unwrap(), b"first");
    }

    #[test]
    fn hardlinked_cache_and_sidecars_are_refused() {
        let f = Fixture::new();
        let policy = f.policy();
        let secret = f.0.join("game/original");
        fs::write(&secret, b"game").unwrap();
        let linked = f.0.join("workspace/linked.db");
        fs::hard_link(&secret, &linked).unwrap();
        assert!(policy.check_cache(&linked).is_err());
        assert!(policy.check_output(&linked).is_err());
        let cache = f.0.join("workspace/cache.db");
        fs::write(&cache, b"cache").unwrap();
        assert!(policy.check_cache(&cache).is_ok());
        fs::hard_link(&secret, f.0.join("workspace/cache.db-wal")).unwrap();
        assert!(policy.check_cache(&cache).is_err());
        assert_eq!(fs::read(secret).unwrap(), b"game");
    }

    #[test]
    fn missing_protected_roots_still_protect_future_files() {
        let f = Fixture::new();
        fs::create_dir_all(f.0.join("workspace")).unwrap();
        let policy = PathPolicy::with_output_root(
            f.0.join("workspace/future-game"),
            vec![],
            f.0.join("workspace"),
        )
        .unwrap();
        assert!(policy.check_output(Path::new("future-game/a/b")).is_err());
        assert!(
            policy
                .check_output(Path::new("future-game-backup/report"))
                .is_ok()
        );
    }

    #[cfg(unix)]
    #[test]
    fn symlink_ancestors_and_dangling_destinations_fail_closed() {
        use std::os::unix::fs::symlink;
        let f = Fixture::new();
        let policy = f.policy();
        symlink(f.0.join("game"), f.0.join("workspace/link")).unwrap();
        assert!(
            policy
                .check_output(Path::new("link/not-yet-created/file"))
                .is_err()
        );
        symlink(f.0.join("absent"), f.0.join("workspace/dangling")).unwrap();
        assert!(policy.check_output(Path::new("dangling")).is_err());
        assert!(policy.check_output(Path::new("dangling/file")).is_err());
    }

    #[cfg(windows)]
    #[test]
    fn rejects_windows_device_and_stream_paths() {
        let f = Fixture::new();
        let policy = f.policy();
        for name in [
            "report:stream",
            "NUL",
            "CON.txt",
            "COM1",
            "LPT9.log",
            "trailing.",
            "trailing ",
        ] {
            assert!(policy.check_output(Path::new(name)).is_err(), "{name}");
        }
    }

    #[cfg(windows)]
    #[test]
    fn junctions_cannot_redirect_outputs_into_protected_roots() {
        use std::os::windows::process::CommandExt;
        fn junction(link: &Path, target: &Path) {
            let output = std::process::Command::new("cmd.exe")
                .args(["/C", "mklink", "/J"])
                .arg(link.to_string_lossy().replace('/', "\\"))
                .arg(target.to_string_lossy().replace('/', "\\"))
                .creation_flags(0x0800_0000)
                .output()
                .unwrap();
            assert!(
                output.status.success(),
                "junction fixture: {}",
                String::from_utf8_lossy(&output.stderr)
            );
        }
        let f = Fixture::new();
        let policy = f.policy();
        junction(&f.0.join("workspace/alias"), &f.0.join("game"));
        assert!(
            policy
                .check_output(Path::new("alias/missing/file"))
                .is_err()
        );
        let future = f.0.join("workspace/future-save");
        let policy = PathPolicy::from_roots(vec![future.clone()], f.0.join("workspace")).unwrap();
        fs::create_dir_all(f.0.join("workspace/new-save-target")).unwrap();
        junction(&future, &f.0.join("workspace/new-save-target"));
        assert!(
            policy
                .check_output(Path::new("future-save/a.save"))
                .is_err()
        );
        assert!(
            policy
                .check_output(Path::new("new-save-target/a.save"))
                .is_err()
        );
    }
}
