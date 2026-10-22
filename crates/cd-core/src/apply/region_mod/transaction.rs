use super::super::{
    guard,
    transaction::{checked, plain},
};
use super::*;
use std::{
    fs::{self, File, OpenOptions},
    io::{Read, Write},
    time::{SystemTime, UNIX_EPOCH},
};
const STORE: &str = "bin64/CrimsonWorkbench-mods/region_return_30min";
const REGISTRY: &str = "meta/0.papgt";
const LIMIT: usize = 128 * 1024;

pub(super) struct Package {
    pub pamt: Vec<u8>,
    pub paz: Vec<u8>,
    pub language: u16,
    pub checksum: u32,
}
#[derive(Clone, Debug, Serialize, Deserialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
struct Digest {
    bytes: usize,
    sha256: String,
}
impl Digest {
    fn of(bytes: &[u8]) -> Self {
        Self {
            bytes: bytes.len(),
            sha256: hash_bytes(bytes),
        }
    }
    fn matches(&self, bytes: &[u8]) -> bool {
        self.bytes == bytes.len() && self.sha256 == hash_bytes(bytes)
    }
    fn valid(&self, max: usize) -> bool {
        self.bytes > 0
            && self.bytes <= max
            && self.sha256.len() == 64
            && self
                .sha256
                .bytes()
                .all(|b| b.is_ascii_hexdigit() && !b.is_ascii_uppercase())
    }
}
#[derive(Clone, Debug, Serialize, Deserialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub(super) struct Receipt {
    schema: u32,
    id: String,
    pub group: String,
    pub language: u16,
    pub checksum: u32,
    pamt: Digest,
    paz: Digest,
}
impl Receipt {
    fn validate(&self) -> Result<()> {
        if self.schema != 1
            || self.id != ID
            || self.group.len() != 4
            || !self.group.bytes().all(|b| b.is_ascii_digit())
            || !self.group.starts_with('9')
            || !self.pamt.valid(4096)
            || !self.paz.valid(65536)
        {
            return Err(bad("Invalid region-mod receipt. No files changed."));
        }
        Ok(())
    }
}
#[derive(Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
struct Pending {
    schema: u32,
    install: bool,
    transaction: String,
    receipt: Receipt,
    before: Vec<u8>,
    after: Vec<u8>,
}
impl Pending {
    fn validate(&self) -> Result<()> {
        self.receipt.validate()?;
        if self.schema != 1
            || self.transaction.len() != 32
            || !self.transaction.bytes().all(|b| b.is_ascii_hexdigit())
            || self.before.len() > 65536
            || self.after.len() > 65536
        {
            return Err(bad("Invalid region-mod transaction."));
        }
        let r = &self.receipt;
        let expected = if self.install {
            overlay::register(&self.before, &r.group, r.language, r.checksum)?
        } else {
            remove_registration(&self.before, r)?
        };
        if expected != self.after {
            return Err(bad("Region transaction changes other registrations."));
        }
        Ok(())
    }
}
fn read(path: &Path) -> Result<Vec<u8>> {
    plain(path, false)?;
    let mut bytes = Vec::new();
    File::open(path)?
        .take(LIMIT as u64 + 1)
        .read_to_end(&mut bytes)?;
    if bytes.len() > LIMIT {
        return Err(bad("Region-mod file exceeds its size limit."));
    }
    Ok(bytes)
}
fn write_new(path: &Path, bytes: &[u8]) -> Result<()> {
    let mut f = OpenOptions::new().write(true).create_new(true).open(path)?;
    f.write_all(bytes)?;
    f.sync_all()?;
    if read(path)? != bytes {
        return Err(bad("Region-mod file verification failed."));
    }
    Ok(())
}
fn remove_checked(path: &Path, digest: &Digest) -> Result<()> {
    if path.try_exists()? {
        if !digest.matches(&read(path)?) {
            return Err(bad("Changed mod file preserved; removal refused."));
        }
        fs::remove_file(path)?;
    }
    Ok(())
}
fn registration(bytes: &[u8], r: &Receipt) -> Result<bool> {
    let groups = overlay::registry_groups(bytes)?;
    if let Some(g) = groups.iter().find(|g| g.name == r.group) {
        if g.is_optional || g.language != r.language || g.pamt_checksum != r.checksum {
            return Err(bad(
                "The mod's registration was changed by another tool. It has been preserved.",
            ));
        }
        Ok(true)
    } else {
        Ok(false)
    }
}
fn remove_registration(bytes: &[u8], r: &Receipt) -> Result<Vec<u8>> {
    if registration(bytes, r)? {
        Ok(overlay::unregister(
            bytes, &r.group, r.language, r.checksum,
        )?)
    } else {
        Ok(bytes.to_vec())
    }
}
struct Fault {
    at: usize,
    fail: Option<usize>,
}
impl Fault {
    fn point(&mut self) -> Result<()> {
        self.at += 1;
        if self.fail == Some(self.at) {
            Err(bad("Simulated region transaction interruption."))
        } else {
            Ok(())
        }
    }
}

pub(super) struct Store {
    root: PathBuf,
}
impl Store {
    pub fn new(root: &Path) -> Result<Self> {
        checked(root, STORE)?;
        Ok(Self {
            root: root.to_owned(),
        })
    }
    fn path(&self, relative: &str) -> Result<PathBuf> {
        checked(&self.root, &format!("{STORE}/{relative}"))
    }
    fn registry(&self) -> Result<Vec<u8>> {
        read(&checked(&self.root, REGISTRY)?)
    }
    pub fn lock(&self) -> Result<File> {
        let path = checked(&self.root, STORE)?;
        fs::create_dir_all(&path)?;
        plain(&path, true)?;
        let path = self.path("lock")?;
        let mut options = OpenOptions::new();
        options.read(true).write(true).create(true).truncate(false);
        #[cfg(windows)]
        {
            use std::os::windows::fs::OpenOptionsExt;
            options.share_mode(0);
        }
        Ok(options.open(path)?)
    }
    pub fn pending(&self) -> Result<bool> {
        Ok(self.path("pending.json")?.try_exists()?)
    }
    pub fn receipt(&self) -> Result<Option<Receipt>> {
        let path = self.path("receipt.json")?;
        if !path.try_exists()? {
            return Ok(None);
        }
        let r: Receipt = serde_json::from_slice(&read(&path)?)?;
        r.validate()?;
        Ok(Some(r))
    }
    pub fn registered(&self, r: &Receipt) -> Result<bool> {
        registration(&self.registry()?, r)
    }
    pub fn verify(&self, r: &Receipt, missing: bool) -> Result<()> {
        verify_files(&self.root, r, missing)
    }
    fn prepare(&self, install: bool, r: Receipt, package: &Package) -> Result<Pending> {
        r.validate()?;
        if !r.pamt.matches(&package.pamt) || !r.paz.matches(&package.paz) {
            return Err(bad("Region package does not match its receipt."));
        }
        let before = self.registry()?;
        let after = if install {
            overlay::register(&before, &r.group, r.language, r.checksum)?
        } else {
            remove_registration(&before, &r)?
        };
        let transaction = format!(
            "{:032x}",
            SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .map_err(|_| bad("System clock unavailable."))?
                .as_nanos()
        );
        let p = Pending {
            schema: 1,
            install,
            transaction,
            receipt: r,
            before,
            after,
        };
        p.validate()?;
        let tx = self.path(&format!("backups/{}", p.transaction))?;
        fs::create_dir_all(tx.parent().unwrap())?;
        fs::create_dir(&tx)?;
        fs::create_dir(tx.join(&p.receipt.group))?;
        write_new(&tx.join(&p.receipt.group).join("0.pamt"), &package.pamt)?;
        write_new(&tx.join(&p.receipt.group).join("0.paz"), &package.paz)?;
        write_new(&tx.join("before.papgt"), &p.before)?;
        write_new(&tx.join("after.papgt"), &p.after)?;
        write_new(&tx.join("pending.json"), &serde_json::to_vec(&p)?)?;
        guard::publish(&tx.join("pending.json"), &self.path("pending.json")?)?;
        Ok(p)
    }
    pub fn install(&self, package: Package, fail: Option<usize>) -> Result<()> {
        let before = self.registry()?;
        let groups = overlay::registry_groups(&before)?;
        let group = (9000..=9999)
            .map(|n| n.to_string())
            .find(|n| {
                !groups.iter().any(|g| &g.name == n)
                    && checked(&self.root, n).is_ok_and(|p| !p.exists())
            })
            .ok_or_else(|| bad("No free archive group is available."))?;
        let r = Receipt {
            schema: 1,
            id: ID.into(),
            group,
            language: package.language,
            checksum: package.checksum,
            pamt: Digest::of(&package.pamt),
            paz: Digest::of(&package.paz),
        };
        let p = self.prepare(true, r, &package)?;
        let mut fault = Fault { at: 0, fail };
        fault.point()?;
        let target = checked(&self.root, &p.receipt.group)?;
        fs::create_dir(&target)?;
        fault.point()?;
        let tx = self.path(&format!("backups/{}", p.transaction))?;
        for (name, bytes) in [("0.pamt", &package.pamt), ("0.paz", &package.paz)] {
            let stage = tx.join(format!("publish-{name}"));
            write_new(&stage, bytes)?;
            guard::publish(&stage, &target.join(name))?;
            fault.point()?;
        }
        self.publish(&p)?;
        fault.point()?;
        self.finish(&p, &mut fault)?;
        Ok(())
    }
    pub fn remove(
        &self,
        validate: impl Fn(&Path, &Receipt) -> Result<()>,
        fail: Option<usize>,
    ) -> Result<()> {
        let Some(r) = self.receipt()? else {
            return Ok(());
        };
        self.verify(&r, false)?;
        validate(&self.root, &r)?;
        let package = Package {
            pamt: read(&checked(&self.root, &format!("{}/0.pamt", r.group))?)?,
            paz: read(&checked(&self.root, &format!("{}/0.paz", r.group))?)?,
            language: r.language,
            checksum: r.checksum,
        };
        let p = self.prepare(false, r, &package)?;
        let mut fault = Fault { at: 0, fail };
        fault.point()?;
        self.publish(&p)?;
        fault.point()?;
        self.finish(&p, &mut fault)?;
        Ok(())
    }
    fn publish(&self, p: &Pending) -> Result<()> {
        if self.registry()? != p.before {
            return Err(bad(
                "Archive registrations changed during installation. Retry after recovery.",
            ));
        }
        if p.before == p.after {
            return Ok(());
        }
        let tx = self.path(&format!("backups/{}", p.transaction))?;
        let staged = tx.join("after.papgt");
        let backup = tx.join("published-before.papgt");
        let registry = checked(&self.root, REGISTRY)?;
        // ReplaceFile captures the bytes actually replaced. If an independent
        // writer raced publication, restore those bytes rather than a stale snapshot.
        atomic_replace(&staged, &registry, &backup)?;
        if read(&backup)? != p.before {
            self.undo_raced_publication(p, &backup)?;
            return Err(bad(
                "Another tool changed archive registrations. Its changes were preserved.",
            ));
        }
        if self.registry()? != p.after {
            return Err(bad(
                "Archive registrations changed after publication; backups preserved.",
            ));
        }
        Ok(())
    }
    fn undo_raced_publication(&self, p: &Pending, backup: &Path) -> Result<()> {
        if self.registry()? == p.after {
            let bytes = read(backup)?;
            overlay::registry_groups(&bytes)?;
            let stage = backup.with_file_name("restore-raced.papgt");
            if !stage.try_exists()? {
                write_new(&stage, &bytes)?;
            } else if read(&stage)? != bytes {
                return Err(bad("Registry recovery staging file changed."));
            }
            if self.registry()? != p.after {
                return Err(bad(
                    "Registry changed during recovery; all backups preserved.",
                ));
            }
            guard::replace(&stage, &checked(&self.root, REGISTRY)?)?;
        }
        Ok(())
    }
    pub fn recover(&self, validate: impl Fn(&Path, &Receipt) -> Result<()>) -> Result<()> {
        let bytes = read(&self.path("pending.json")?)?;
        let p: Pending = serde_json::from_slice(&bytes)?;
        p.validate()?;
        let tx = self.path(&format!("backups/{}", p.transaction))?;
        verify_files(&tx, &p.receipt, false)?;
        validate(&tx, &p.receipt)?;
        let backup = tx.join("published-before.papgt");
        if backup.try_exists()? && read(&backup)? != p.before {
            self.undo_raced_publication(&p, &backup)?;
        }
        self.finish(&p, &mut Fault { at: 0, fail: None })
    }
    fn finish(&self, p: &Pending, fault: &mut Fault) -> Result<()> {
        let present = self.registered(&p.receipt)?;
        let receipt = self.path("receipt.json")?;
        if present {
            self.verify(&p.receipt, false)?;
            if let Some(existing) = self.receipt()? {
                if existing != p.receipt {
                    return Err(bad("Installed receipt changed."));
                }
            } else {
                let next = self.path(&format!("backups/{}/receipt.next", p.transaction))?;
                let bytes = serde_json::to_vec(&p.receipt)?;
                if !next.try_exists()? {
                    write_new(&next, &bytes)?;
                } else if read(&next)? != bytes {
                    return Err(bad("Staged receipt changed."));
                }
                guard::publish(&next, &receipt)?;
            }
            fault.point()?;
        } else {
            self.verify(&p.receipt, true)?;
            let dir = checked(&self.root, &p.receipt.group)?;
            for (name, digest) in [("0.pamt", &p.receipt.pamt), ("0.paz", &p.receipt.paz)] {
                remove_checked(
                    &checked(&self.root, &format!("{}/{name}", p.receipt.group))?,
                    digest,
                )?;
                fault.point()?;
            }
            if dir.try_exists()? {
                fs::remove_dir(&dir)?;
            }
            fault.point()?;
            if let Some(existing) = self.receipt()? {
                if existing != p.receipt {
                    return Err(bad("Installed receipt changed."));
                }
                fs::remove_file(&receipt)?;
            }
            fault.point()?;
        }
        let journal = self.path("pending.json")?;
        if read(&journal)? != serde_json::to_vec(p)? {
            return Err(bad("Region transaction changed during cleanup."));
        }
        fs::remove_file(journal)?;
        fault.point()?;
        Ok(())
    }
}
fn verify_files(root: &Path, r: &Receipt, missing: bool) -> Result<()> {
    r.validate()?;
    let dir = checked(root, &r.group)?;
    if dir.try_exists()? {
        plain(&dir, true)?;
        for entry in fs::read_dir(&dir)? {
            let entry = entry?;
            if !["0.pamt", "0.paz"].contains(&entry.file_name().to_str().unwrap_or("")) {
                return Err(bad(
                    "Unexpected files in the owned mod group. They have been preserved.",
                ));
            }
        }
    }
    for (name, digest) in [("0.pamt", &r.pamt), ("0.paz", &r.paz)] {
        let path = checked(root, &format!("{}/{name}", r.group))?;
        if missing && !path.try_exists()? {
            continue;
        }
        if !digest.matches(&read(&path)?) {
            return Err(bad(
                "The region mod's files changed. Nothing will be removed.",
            ));
        }
    }
    Ok(())
}
fn atomic_replace(staged: &Path, destination: &Path, backup: &Path) -> Result<()> {
    #[cfg(windows)]
    {
        use std::os::windows::ffi::OsStrExt;
        use windows_sys::Win32::Storage::FileSystem::ReplaceFileW;
        let wide = |p: &Path| {
            p.as_os_str()
                .encode_wide()
                .chain(Some(0))
                .collect::<Vec<_>>()
        };
        if unsafe {
            ReplaceFileW(
                wide(destination).as_ptr(),
                wide(staged).as_ptr(),
                wide(backup).as_ptr(),
                0,
                std::ptr::null(),
                std::ptr::null(),
            )
        } == 0
        {
            return Err(std::io::Error::last_os_error().into());
        }
    }
    #[cfg(not(windows))]
    {
        write_new(backup, &read(destination)?)?;
        guard::replace(staged, destination)?;
    }
    Ok(())
}
