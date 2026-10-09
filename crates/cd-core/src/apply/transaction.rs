//! Private transaction machinery, reachable only through a guarded project rehearsal.
//! Recovery rolls back a pending transaction; no foreign bytes are overwritten.
use crate::{Error, Result, fingerprint::hash_bytes};
use serde::{Deserialize, Serialize};
use std::{
    collections::BTreeMap,
    fs::{self, File, OpenOptions},
    io::{Read, Write},
    path::{Path, PathBuf},
};
fn bad(s: &str) -> Error {
    Error::Invalid(s.into())
}
#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Journal {
    version: u32,
    phase: String,
    baseline: String,
    hashes: BTreeMap<String, String>,
}
pub(super) struct Engine {
    root: PathBuf,
    baseline: String,
    _lock: File,
}
impl Engine {
    pub fn new(root: &Path, baseline: &str) -> Result<Self> {
        let root = root.canonicalize()?;
        let dir = root.join(".workbench");
        if dir.exists() {
            plain(&dir, true)?;
        } else {
            fs::create_dir(&dir)?;
        }
        let lock_path = dir.join("lock");
        if lock_path.exists() {
            plain(&lock_path, false)?;
        }
        let lock = OpenOptions::new()
            .read(true)
            .write(true)
            .create(true)
            .truncate(false)
            .open(lock_path)?;
        lock.try_lock()
            .map_err(|e| bad(&format!("transaction busy: {e}")))?;
        let value = Self {
            root,
            baseline: baseline.into(),
            _lock: lock,
        };
        let backup = value.path(".workbench/baseline.papgt")?;
        if backup.exists() {
            if hash_bytes(&read(&backup)?) != baseline {
                return Err(bad("backup hash mismatch"));
            }
        } else {
            let bytes = read(&value.path("meta/0.papgt")?)?;
            if hash_bytes(&bytes) != baseline {
                return Err(bad("baseline mismatch"));
            }
            create(&backup, &bytes)?;
            if hash_bytes(&read(&backup)?) != baseline {
                return Err(bad("backup verification failed"));
            }
        }
        Ok(value)
    }
    fn path(&self, relative: &str) -> Result<PathBuf> {
        if relative
            .split('/')
            .any(|p| p.is_empty() || p == "." || p == ".." || p.contains(['\\', ':']))
        {
            return Err(bad("invalid transaction path"));
        }
        let mut path = self.root.clone();
        let parts: Vec<_> = relative.split('/').collect();
        for (i, part) in parts.iter().enumerate() {
            path.push(part);
            match fs::symlink_metadata(&path) {
                Ok(_) => plain(&path, i + 1 != parts.len())?,
                Err(e) if e.kind() == std::io::ErrorKind::NotFound => {}
                Err(e) => return Err(e.into()),
            }
        }
        Ok(path)
    }
    fn journal(&self, value: &Journal) -> Result<()> {
        self.atomic(
            ".workbench/journal.json",
            &serde_json::to_vec_pretty(value)?,
        )
    }
    fn atomic(&self, relative: &str, bytes: &[u8]) -> Result<()> {
        let target = self.path(relative)?;
        let temporary = self.path(&format!("{relative}.cw-new"))?;
        if temporary.exists() {
            return Err(bad("pending atomic temporary; recovery required"));
        }
        create(&temporary, bytes)?;
        super::guard::replace(&temporary, &target)?;
        if read(&target)? != bytes {
            return Err(bad("atomic replacement readback failed"));
        }
        Ok(())
    }
    pub fn apply(
        &mut self,
        files: &BTreeMap<String, Vec<u8>>,
        mut check: impl FnMut() -> Result<()>,
        fail_at: Option<usize>,
    ) -> Result<()> {
        check()?;
        if hash_bytes(&read(&self.path(".workbench/baseline.papgt")?)?) != self.baseline {
            return Err(bad("backup changed before apply"));
        }
        if files.len() != 3 || !files.contains_key("meta/0.papgt") {
            return Err(bad("expected one coherent overlay group"));
        }
        let mut group: Option<&str> = None;
        for path in files.keys().filter(|p| p.as_str() != "meta/0.papgt") {
            let (g, name) = path
                .split_once('/')
                .ok_or_else(|| bad("invalid overlay path"))?;
            if g.len() != 4
                || !g.bytes().all(|b| b.is_ascii_digit())
                || !matches!(name, "0.pamt" | "0.paz")
                || group.is_some_and(|v| v != g)
            {
                return Err(bad("invalid owned group"));
            }
            group = Some(g);
        }
        let current = read(&self.path("meta/0.papgt")?)?;
        if hash_bytes(&current) != self.baseline {
            return Err(bad(
                "registry changed; restore first or investigate foreign update",
            ));
        }
        let old_journal = self.path(".workbench/journal.json")?;
        if old_journal.exists() {
            let j: Journal = serde_json::from_slice(&read(&old_journal)?)?;
            if j.phase != "restored" {
                return Err(bad("pending/active transaction; recover before apply"));
            }
        }
        let group = group.unwrap();
        if self.root.join(group).exists() {
            return Err(bad("group collision; never overwrite foreign groups"));
        }
        let mut j = Journal {
            version: 1,
            phase: "prepared".into(),
            baseline: self.baseline.clone(),
            hashes: files
                .iter()
                .map(|(p, b)| (p.clone(), hash_bytes(b)))
                .collect(),
        };
        self.journal(&j)?;
        stop(fail_at, 0)?;
        check()?;
        fs::create_dir(self.root.join(group))?;
        stop(fail_at, 1)?;
        for (step, (path, bytes)) in files
            .iter()
            .filter(|(p, _)| p.as_str() != "meta/0.papgt")
            .enumerate()
        {
            check()?;
            create(&self.path(path)?, bytes)?;
            if hash_bytes(&read(&self.path(path)?)?) != j.hashes[path] {
                return Err(bad("overlay readback hash mismatch"));
            }
            stop(fail_at, step + 2)?;
        }
        check()?;
        if hash_bytes(&read(&self.path("meta/0.papgt")?)?) != self.baseline {
            return Err(bad("registry changed before commit"));
        }
        self.atomic("meta/0.papgt", &files["meta/0.papgt"])?;
        stop(fail_at, 4)?;
        j.phase = "applied".into();
        self.journal(&j)?;
        stop(fail_at, 5)?;
        Ok(())
    }
    pub fn restore(
        &mut self,
        mut check: impl FnMut() -> Result<()>,
        fail_at: Option<usize>,
    ) -> Result<()> {
        check()?;
        let path = self.path(".workbench/journal.json")?;
        let mut j: Journal = serde_json::from_slice(&read(&path)?)?;
        if j.version != 1
            || j.baseline != self.baseline
            || !matches!(j.phase.as_str(), "prepared" | "applied" | "restored")
            || j.hashes.len() != 3
            || !j.hashes.contains_key("meta/0.papgt")
        {
            return Err(bad("unknown journal"));
        }
        let original = read(&self.path(".workbench/baseline.papgt")?)?;
        if hash_bytes(&original) != self.baseline {
            return Err(bad("backup hash mismatch"));
        }
        let current = hash_bytes(&read(&self.path("meta/0.papgt")?)?);
        if current != self.baseline && current != j.hashes["meta/0.papgt"] {
            return Err(bad("foreign registry change; restore refused"));
        }
        let mut owned = Vec::new();
        let mut group: Option<String> = None;
        // Preflight ALL files before touching anything. The journal is untrusted input.
        for (name, expected) in j
            .hashes
            .iter()
            .filter(|(p, _)| p.as_str() != "meta/0.papgt")
        {
            let (g, base) = name
                .split_once('/')
                .ok_or_else(|| bad("bad journal path"))?;
            if g.len() != 4
                || !g.bytes().all(|b| b.is_ascii_digit())
                || !matches!(base, "0.paz" | "0.pamt")
                || group.as_ref().is_some_and(|v| v != g)
            {
                return Err(bad("bad owned path"));
            }
            group = Some(g.into());
            let path = self.path(name)?;
            if path.exists() && hash_bytes(&read(&path)?) != *expected {
                return Err(bad("owned file changed externally; restore refused"));
            }
            owned.push(path);
        }
        let dir = self.root.join(group.unwrap());
        if dir.exists() {
            plain(&dir, true)?;
            for entry in fs::read_dir(&dir)? {
                if !owned.contains(&entry?.path()) {
                    return Err(bad("foreign file in owned group; restore refused"));
                }
            }
        }
        check()?;
        if current != self.baseline {
            self.atomic("meta/0.papgt", &original)?;
        }
        stop(fail_at, 0)?;
        for (i, path) in owned.iter().enumerate() {
            check()?;
            if path.exists() {
                fs::remove_file(path)?;
            }
            stop(fail_at, i + 1)?;
        }
        if dir.exists() {
            fs::remove_dir(dir)?;
        }
        stop(fail_at, 3)?;
        if hash_bytes(&read(&self.path("meta/0.papgt")?)?) != self.baseline {
            return Err(bad("restored registry failed hash verification"));
        }
        j.phase = "restored".into();
        self.journal(&j)?;
        stop(fail_at, 4)?;
        Ok(())
    }
}
fn stop(at: Option<usize>, step: usize) -> Result<()> {
    if at == Some(step) {
        Err(bad("injected interruption"))
    } else {
        Ok(())
    }
}
fn read(path: &Path) -> Result<Vec<u8>> {
    let mut out = Vec::new();
    File::open(path)?
        .take(crimson_format::MAX_FILE_BYTES as u64 + 1)
        .read_to_end(&mut out)?;
    if out.len() > crimson_format::MAX_FILE_BYTES {
        return Err(bad("transaction file exceeds bound"));
    }
    Ok(out)
}
fn create(path: &Path, bytes: &[u8]) -> Result<()> {
    let mut f = OpenOptions::new().write(true).create_new(true).open(path)?;
    f.write_all(bytes)?;
    f.sync_all()?;
    Ok(())
}
fn plain(path: &Path, dir: bool) -> Result<()> {
    let m = fs::symlink_metadata(path)?;
    if m.file_type().is_symlink() || (dir && !m.is_dir()) || (!dir && !m.is_file()) {
        return Err(bad("non-ordinary transaction path"));
    }
    #[cfg(windows)]
    {
        use std::os::windows::fs::MetadataExt;
        if m.file_attributes() & 0x400 != 0 {
            return Err(bad("reparse transaction path"));
        }
    }
    if !dir && crate::paths::link_count(path, &m)? != 1 {
        return Err(bad("hardlinked transaction file"));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    fn fixture() -> (tempfile::TempDir, BTreeMap<String, Vec<u8>>) {
        let t = tempfile::tempdir().unwrap();
        fs::create_dir(t.path().join("meta")).unwrap();
        fs::write(t.path().join("meta/0.papgt"), b"baseline").unwrap();
        (
            t,
            BTreeMap::from([
                ("0041/0.pamt".into(), b"metadata".to_vec()),
                ("0041/0.paz".into(), b"payload".to_vec()),
                ("meta/0.papgt".into(), b"registered".to_vec()),
            ]),
        )
    }
    #[test]
    fn interruptions_at_each_apply_and_restore_boundary_recover() {
        for fail in 0..=5 {
            let (t, f) = fixture();
            let mut e = Engine::new(t.path(), &hash_bytes(b"baseline")).unwrap();
            assert!(e.apply(&f, || Ok(()), Some(fail)).is_err());
            drop(e);
            let mut e = Engine::new(t.path(), &hash_bytes(b"baseline")).unwrap();
            e.restore(|| Ok(()), None).unwrap();
            assert_eq!(
                fs::read(t.path().join("meta/0.papgt")).unwrap(),
                b"baseline"
            );
            assert!(!t.path().join("0041").exists());
            e.apply(&f, || Ok(()), None).unwrap();
            e.restore(|| Ok(()), None).unwrap();
        }
        for fail in 0..=4 {
            let (t, f) = fixture();
            let mut e = Engine::new(t.path(), &hash_bytes(b"baseline")).unwrap();
            e.apply(&f, || Ok(()), None).unwrap();
            assert!(e.restore(|| Ok(()), Some(fail)).is_err());
            drop(e);
            Engine::new(t.path(), &hash_bytes(b"baseline"))
                .unwrap()
                .restore(|| Ok(()), None)
                .unwrap();
            assert!(!t.path().join("0041").exists());
        }
    }
    #[test]
    fn refuses_foreign_updates_corrupt_backup_collisions_and_concurrency() {
        let (t, f) = fixture();
        let mut e = Engine::new(t.path(), &hash_bytes(b"baseline")).unwrap();
        assert!(Engine::new(t.path(), &hash_bytes(b"baseline")).is_err());
        e.apply(&f, || Ok(()), None).unwrap();
        fs::write(t.path().join("0041/0.paz"), b"foreign").unwrap();
        assert!(e.restore(|| Ok(()), None).is_err());
        assert_eq!(
            fs::read(t.path().join("meta/0.papgt")).unwrap(),
            b"registered"
        );
        let (t, f) = fixture();
        let mut e = Engine::new(t.path(), &hash_bytes(b"baseline")).unwrap();
        fs::create_dir(t.path().join("0041")).unwrap();
        assert!(e.apply(&f, || Ok(()), None).is_err());
        assert!(!t.path().join(".workbench/journal.json").exists());
        let (t, f) = fixture();
        let mut e = Engine::new(t.path(), &hash_bytes(b"baseline")).unwrap();
        fs::write(t.path().join(".workbench/baseline.papgt"), b"corrupt").unwrap();
        assert!(e.apply(&f, || Ok(()), None).is_err());
        assert!(!t.path().join("0041").exists());
        let (t, f) = fixture();
        let mut e = Engine::new(t.path(), &hash_bytes(b"baseline")).unwrap();
        e.apply(&f, || Ok(()), None).unwrap();
        fs::write(t.path().join("meta/0.papgt"), b"updated game").unwrap();
        assert!(e.restore(|| Ok(()), None).is_err());
        assert_eq!(fs::read(t.path().join("0041/0.paz")).unwrap(), b"payload");
        assert_eq!(
            fs::read(t.path().join("meta/0.papgt")).unwrap(),
            b"updated game"
        );
    }
    #[test]
    fn hardlinked_transaction_files_are_never_replaced() {
        let (t, f) = fixture();
        let original = t.path().join("meta/0.papgt");
        fs::hard_link(&original, t.path().join("linked.papgt")).unwrap();
        assert!(Engine::new(t.path(), &hash_bytes(b"baseline")).is_err());
        assert_eq!(fs::read(original).unwrap(), b"baseline");
        let (t, _) = fixture();
        let mut e = Engine::new(t.path(), &hash_bytes(b"baseline")).unwrap();
        e.apply(&f, || Ok(()), None).unwrap();
        fs::hard_link(t.path().join("0041/0.paz"), t.path().join("linked.paz")).unwrap();
        assert!(e.restore(|| Ok(()), None).is_err());
        assert_eq!(
            fs::read(t.path().join("meta/0.papgt")).unwrap(),
            b"registered"
        );
    }
    #[test]
    fn running_or_failed_process_check_prevents_commit_and_restore() {
        let (t, f) = fixture();
        let mut e = Engine::new(t.path(), &hash_bytes(b"baseline")).unwrap();
        assert!(e.apply(&f, || Err(bad("running")), None).is_err());
        assert!(!t.path().join("0041").exists());
        let mut n = 0;
        e.apply(
            &f,
            || {
                n += 1;
                if n == 5 {
                    Err(bad("process started"))
                } else {
                    Ok(())
                }
            },
            None,
        )
        .unwrap_err();
        assert_eq!(
            fs::read(t.path().join("meta/0.papgt")).unwrap(),
            b"baseline"
        );
        assert!(e.restore(|| Err(bad("query failed")), None).is_err());
        assert!(t.path().join("0041").exists());
        e.restore(|| Ok(()), None).unwrap();
    }
}
