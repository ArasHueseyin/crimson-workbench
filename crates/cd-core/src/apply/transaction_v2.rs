//! Private B0 kernel for isolated rehearsals. Immutable intents precede writes.
//! The registry is the commit boundary; recovery keeps its selected generation.
use crate::{Error, Result, fingerprint::hash_bytes};
use serde::{Deserialize, Serialize};
use std::{
    collections::{BTreeMap, BTreeSet},
    fs::{self, File, OpenOptions},
    io::{Read, Write},
    path::{Path, PathBuf},
};
const REGISTRY: &str = "meta/0.papgt";
const HISTORY: &str = ".workbench/transactions";
const MAX_GROUPS: usize = 8;
fn bad(s: impl Into<String>) -> Error {
    Error::Invalid(s.into())
}

#[derive(Clone, Debug, PartialEq, Eq, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Digest {
    sha256: String,
    bytes: usize,
}
impl Digest {
    fn of(bytes: &[u8]) -> Self {
        Self {
            sha256: hash_bytes(bytes),
            bytes: bytes.len(),
        }
    }
    fn matches(&self, bytes: &[u8]) -> bool {
        self.bytes == bytes.len() && self.sha256 == hash_bytes(bytes)
    }
    fn valid(&self) -> bool {
        self.bytes > 0
            && self.bytes <= crimson_format::MAX_FILE_BYTES
            && self.sha256.len() == 64
            && self
                .sha256
                .bytes()
                .all(|b| b.is_ascii_digit() || (b'a'..=b'f').contains(&b))
    }
}
#[derive(Clone, Debug, PartialEq, Eq, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Group {
    pamt: Digest,
    paz: Digest,
    checksum: u32,
}
#[derive(Clone, Debug, PartialEq, Eq, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Snapshot {
    registry: Vec<u8>,
    groups: BTreeMap<String, Group>,
}
#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Intent {
    version: u32,
    id: u64,
    /// Epoch seconds; zero for journals created by versions before 0.4.8.
    #[serde(default)]
    started_at: u64,
    baseline: String,
    before: Snapshot,
    after: Snapshot,
}
#[derive(Clone, Copy, PartialEq, Eq, Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
enum Outcome {
    Committed,
    RolledBack,
}
#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Complete {
    intent_sha256: String,
    outcome: Outcome,
}
struct Pending {
    intent: Intent,
    encoded: Vec<u8>,
    directory: String,
}
struct History {
    current: Snapshot,
    pending: Option<Pending>,
    next: u64,
}
pub(super) struct Plan {
    before: Snapshot,
    after: Snapshot,
    files: BTreeMap<String, Vec<u8>>,
}
#[derive(Clone, Serialize)]
pub struct FileChange {
    pub path: String,
    pub action: &'static str,
    pub before_sha256: Option<String>,
    pub after_sha256: Option<String>,
}
#[derive(Serialize)]
pub struct RestoreReview {
    pub backup_sha256: String,
    pub backup_bytes: usize,
    pub current_registry_sha256: String,
    pub pending_outcome: Option<&'static str>,
    pub pending_intent_sha256: Option<String>,
    pub next_transaction_id: String,
    pub files: Vec<FileChange>,
    pub remove_directories: Vec<String>,
}
/// Journal-derived ownership claims. Metadata inspection is deliberately not a
/// content verification of the overlay archives.
pub(super) struct Ownership {
    pub files: BTreeMap<String, Option<crate::fingerprint::FileHash>>,
    pub directories: BTreeSet<String>,
    pub registry_matches_history: bool,
}
impl Plan {
    pub fn changes(&self) -> Vec<FileChange> {
        if self.before == self.after {
            return Vec::new();
        }
        let mut out = Vec::new();
        for (state, action) in [(&self.after, "create"), (&self.before, "remove")] {
            for (name, g) in &state.groups {
                for (file, digest) in [("0.pamt", &g.pamt), ("0.paz", &g.paz)] {
                    out.push(FileChange {
                        path: format!("{name}/{file}"),
                        action,
                        before_sha256: (action == "remove").then(|| digest.sha256.clone()),
                        after_sha256: (action == "create").then(|| digest.sha256.clone()),
                    });
                }
            }
        }
        out.insert(
            self.after.groups.len() * 2,
            FileChange {
                path: REGISTRY.into(),
                action: "replace",
                before_sha256: Some(hash_bytes(&self.before.registry)),
                after_sha256: Some(hash_bytes(&self.after.registry)),
            },
        );
        out
    }
    pub fn group_names(&self) -> impl Iterator<Item = &str> {
        self.after.groups.keys().map(String::as_str)
    }
}
#[derive(Default)]
struct Fault {
    at: Option<usize>,
    trace: Vec<&'static str>,
}
impl Fault {
    fn point(&mut self, label: &'static str) -> Result<()> {
        let step = self.trace.len();
        self.trace.push(label);
        if self.at == Some(step) {
            Err(bad(format!("injected interruption {step}: {label}")))
        } else {
            Ok(())
        }
    }
}
pub(super) struct Engine {
    root: PathBuf,
    baseline: Snapshot,
    language: u16,
    reserved: BTreeSet<String>,
    _lock: Option<File>,
}
impl Engine {
    pub fn new(root: &Path, expected_baseline: &str) -> Result<Self> {
        Self::open(root, expected_baseline, true, true)
    }
    /// Existing state only: inspecting or restoring must never recreate a lost backup.
    pub fn open_existing(root: &Path, expected_baseline: &str) -> Result<Self> {
        Self::open(root, expected_baseline, false, true)
    }
    // Private to B0: a read-only engine is never exposed as a write session.
    pub fn inspect(root: &Path, expected_baseline: &str) -> Result<Self> {
        Self::open(root, expected_baseline, false, false)
    }
    fn open(
        root: &Path,
        expected_baseline: &str,
        initialize: bool,
        exclusive: bool,
    ) -> Result<Self> {
        let root = root.canonicalize()?;
        plain(&root, true)?;
        let directory = root.join(".workbench");
        if directory.try_exists()? {
            plain(&directory, true)?;
        } else if initialize {
            fs::create_dir(&directory)?;
        } else {
            return Err(bad(
                "Keine vorhandene Sicherung; Projektprobe nicht initialisiert.",
            ));
        }
        if directory.join("journal.json").try_exists()? {
            return Err(bad(
                "Legacy rehearsal journal: preserve the old directory and start a new rehearsal",
            ));
        }
        let lock_path = directory.join("lock");
        if lock_path.try_exists()? {
            plain(&lock_path, false)?;
        }
        let lock = if exclusive {
            let lock = OpenOptions::new()
                .read(true)
                .write(true)
                .create(initialize)
                .truncate(false)
                .open(lock_path)?;
            lock.try_lock()
                .map_err(|e| bad(format!("transaction busy: {e}")))?;
            Some(lock)
        } else {
            plain(&lock_path, false)?;
            None
        };
        let backup = directory.join("baseline.papgt");
        let registry = if backup.try_exists()? {
            read_plain(&backup)?
        } else if initialize {
            let bytes = read_plain(&checked(&root, REGISTRY)?)?;
            if hash_bytes(&bytes) != expected_baseline {
                return Err(bad("baseline mismatch"));
            }
            let partial = directory.join("baseline.part");
            resumable(&partial, &bytes, &mut Fault::default())?;
            super::guard::publish(&partial, &backup)?;
            read_plain(&backup)?
        } else {
            return Err(bad(
                "Registry-Sicherung fehlt; keine automatische Neuerstellung.",
            ));
        };
        if hash_bytes(&registry) != expected_baseline {
            return Err(bad("backup verification failed"));
        }
        let groups = crimson_format::overlay::registry_groups(&registry)?;
        let language = groups
            .iter()
            .find(|g| g.name == "0008")
            .map(|g| g.language)
            .ok_or_else(|| bad("source group 0008 absent from baseline"))?;
        let reserved = groups.iter().map(|g| g.name.clone()).collect();
        let value = Self {
            root,
            baseline: Snapshot {
                registry,
                groups: BTreeMap::new(),
            },
            language,
            reserved,
            _lock: lock,
        };
        if initialize {
            value.ensure_dir(HISTORY)?;
        } else {
            plain(&value.path(HISTORY)?, true)?;
        }
        value.history()?;
        Ok(value)
    }
    fn path(&self, p: &str) -> Result<PathBuf> {
        checked(&self.root, p)
    }
    pub fn original_registry(&self) -> Result<Vec<u8>> {
        self.restore_review()?;
        Ok(self.baseline.registry.clone())
    }
    pub fn ownership(&self) -> Result<Ownership> {
        let history = self.history()?;
        let current = read_plain(&self.path(REGISTRY)?)?;
        let mut groups = history.current.groups.clone();
        let mut matches = current == history.current.registry;
        if let Some(pending) = &history.pending {
            groups.extend(pending.intent.before.groups.clone());
            groups.extend(pending.intent.after.groups.clone());
            matches = current == pending.intent.before.registry
                || current == pending.intent.after.registry;
        }
        let mut out = Ownership {
            files: BTreeMap::new(),
            directories: BTreeSet::new(),
            registry_matches_history: matches,
        };
        let mut queue = vec![".workbench".to_owned()];
        let mut count = 0;
        while let Some(dir) = queue.pop() {
            plain(&self.path(&dir)?, true)?;
            out.directories.insert(dir.clone());
            for item in fs::read_dir(self.path(&dir)?)? {
                let item = item?;
                count += 1;
                if count > 16384 {
                    return Err(bad("ownership inventory limit exceeded"));
                }
                let name = item
                    .file_name()
                    .into_string()
                    .map_err(|_| bad("invalid private filename"))?;
                let path = format!("{dir}/{name}");
                let parts: Vec<_> = path.split('/').collect();
                let is_dir = fs::symlink_metadata(item.path())?.is_dir();
                plain(&item.path(), is_dir)?;
                let numeric = |s: &str, n| s.len() == n && s.bytes().all(|b| b.is_ascii_digit());
                let allowed = match parts.as_slice() {
                    [".workbench", "transactions"] => is_dir,
                    [".workbench", "lock" | "baseline.papgt" | "baseline.part"] => !is_dir,
                    [".workbench", "transactions", id] => is_dir && numeric(id, 16),
                    [
                        ".workbench",
                        "transactions",
                        id,
                        "intent.json" | "intent.part" | "complete.json" | "complete.part"
                        | "registry.part",
                    ] => !is_dir && numeric(id, 16),
                    [".workbench", "transactions", id, "stage"] => is_dir && numeric(id, 16),
                    [".workbench", "transactions", id, "stage", group] => {
                        is_dir && numeric(id, 16) && numeric(group, 4)
                    }
                    [
                        ".workbench",
                        "transactions",
                        id,
                        "stage",
                        group,
                        "0.pamt" | "0.paz",
                    ] => !is_dir && numeric(id, 16) && numeric(group, 4),
                    _ => false,
                };
                if !allowed {
                    return Err(bad(format!("unrecognized private path: {path}")));
                }
                if parts.len() >= 5 {
                    let intent: Intent = serde_json::from_slice(&read_limit(
                        &self.path(&format!("{HISTORY}/{}/intent.json", parts[2]))?,
                        1_048_576,
                    )?)?;
                    if !intent.after.groups.contains_key(parts[4]) {
                        return Err(bad("unrecognized private staging group"));
                    }
                }
                if is_dir {
                    queue.push(path);
                } else {
                    out.files.insert(path, None);
                }
            }
        }
        for (group, entry) in groups {
            let dir = self.path(&group)?;
            if !dir.try_exists()? {
                continue;
            }
            plain(&dir, true)?;
            out.directories.insert(group.clone());
            for item in fs::read_dir(&dir)? {
                let item = item?;
                let name = item.file_name();
                if !matches!(name.to_str(), Some("0.pamt" | "0.paz")) {
                    return Err(bad("foreign file in owned group"));
                }
                plain(&item.path(), false)?;
                let path = format!("{group}/{}", name.to_string_lossy());
                let digest = if name == "0.pamt" {
                    &entry.pamt
                } else {
                    &entry.paz
                };
                if item.metadata()?.len() != digest.bytes as u64 {
                    return Err(bad("owned file size changed"));
                }
                out.files.insert(
                    path.clone(),
                    Some(crate::fingerprint::FileHash {
                        path,
                        bytes: digest.bytes as u64,
                        sha256: digest.sha256.clone(),
                    }),
                );
            }
        }
        Ok(out)
    }
    fn ensure_dir(&self, p: &str) -> Result<()> {
        let path = self.path(p)?;
        if path.try_exists()? {
            plain(&path, true)?;
        } else {
            fs::create_dir(&path)?;
        }
        Ok(())
    }
    fn registry_for(&self, groups: &BTreeMap<String, Group>) -> Result<Vec<u8>> {
        if groups.len() > MAX_GROUPS {
            return Err(bad("too many owned overlay groups"));
        }
        let mut registry = self.baseline.registry.clone();
        for (name, g) in groups.iter().rev() {
            if name.len() != 4
                || !name.bytes().all(|b| b.is_ascii_digit())
                || self.reserved.contains(name)
                || !g.pamt.valid()
                || !g.paz.valid()
            {
                return Err(bad("invalid or reserved owned group"));
            }
            registry =
                crimson_format::overlay::register(&registry, name, self.language, g.checksum)?;
        }
        Ok(registry)
    }
    fn validate_state(&self, state: &Snapshot) -> Result<()> {
        if self.registry_for(&state.groups)? != state.registry {
            return Err(bad(
                "registry does not describe exactly the owned groups plus baseline",
            ));
        }
        Ok(())
    }
    fn verify_backup(&self) -> Result<()> {
        if read_plain(&self.path(".workbench/baseline.papgt")?)? != self.baseline.registry {
            return Err(bad("backup changed"));
        }
        Ok(())
    }
    fn history(&self) -> Result<History> {
        self.verify_backup()?;
        let mut directories = BTreeMap::new();
        for item in fs::read_dir(self.path(HISTORY)?)? {
            let item = item?;
            let name = item.file_name().to_string_lossy().into_owned();
            if name.len() != 16 || !name.bytes().all(|b| b.is_ascii_digit()) {
                return Err(bad("unknown transaction history entry"));
            }
            let id = name
                .parse::<u64>()
                .map_err(|_| bad("invalid transaction id"))?;
            if id == 0 || directories.len() >= 10000 {
                return Err(bad("history limit exceeded"));
            }
            plain(&item.path(), true)?;
            directories.insert(id, format!("{HISTORY}/{name}"));
        }
        let next = directories.last_key_value().map_or(1, |(n, _)| n + 1);
        if next > 9_999_999_999_999_999 {
            return Err(bad("transaction id exhausted"));
        }
        let mut current = self.baseline.clone();
        let mut pending = None;
        for (id, directory) in directories {
            let file = self.path(&format!("{directory}/intent.json"))?;
            if !file.try_exists()? {
                // No visible writes precede a complete durable intent. Keep partial
                // bytes for diagnosis instead of deleting private or foreign data.
                for entry in fs::read_dir(self.path(&directory)?)? {
                    let entry = entry?;
                    if entry.file_name() != "intent.part" {
                        return Err(bad("unpublished transaction contains unexpected files"));
                    }
                    plain(&entry.path(), false)?;
                }
                continue;
            }
            if pending.is_some() {
                return Err(bad("multiple pending or out-of-order transactions"));
            }
            let encoded = read_limit(&file, 1_048_576)?;
            let intent: Intent = serde_json::from_slice(&encoded)?;
            if intent.version != 2
                || intent.id != id
                || intent.baseline != hash_bytes(&self.baseline.registry)
                || intent.before != current
                || intent.before == intent.after
            {
                return Err(bad("invalid transaction chain"));
            }
            self.validate_state(&intent.before)?;
            self.validate_state(&intent.after)?;
            if intent
                .before
                .groups
                .keys()
                .any(|g| intent.after.groups.contains_key(g))
            {
                return Err(bad("reapply must use a separate generation"));
            }
            let completed = self.path(&format!("{directory}/complete.json"))?;
            if completed.try_exists()? {
                let result: Complete = serde_json::from_slice(&read_limit(&completed, 4096)?)?;
                if result.intent_sha256 != hash_bytes(&encoded) {
                    return Err(bad("completion/intent hash mismatch"));
                }
                current = match result.outcome {
                    Outcome::Committed => intent.after,
                    Outcome::RolledBack => intent.before,
                };
            } else {
                pending = Some(Pending {
                    intent,
                    encoded,
                    directory,
                });
            }
        }
        Ok(History {
            current,
            pending,
            next,
        })
    }
    fn verify_groups(&self, groups: &BTreeMap<String, Group>, missing_ok: bool) -> Result<()> {
        for (name, g) in groups {
            let dir = self.path(name)?;
            if !dir.try_exists()? {
                if missing_ok {
                    continue;
                }
                return Err(bad("active overlay group missing"));
            }
            plain(&dir, true)?;
            for entry in fs::read_dir(&dir)? {
                let entry = entry?;
                if !matches!(entry.file_name().to_str(), Some("0.pamt" | "0.paz")) {
                    return Err(bad("foreign file in owned group"));
                }
                plain(&entry.path(), false)?;
            }
            for (file, digest) in [("0.pamt", &g.pamt), ("0.paz", &g.paz)] {
                let path = self.path(&format!("{name}/{file}"))?;
                if !path.try_exists()? {
                    if missing_ok {
                        continue;
                    }
                    return Err(bad("active overlay file missing"));
                }
                if !digest.matches(&read_plain(&path)?) {
                    return Err(bad("owned file changed externally"));
                }
            }
        }
        Ok(())
    }
    fn verify_current(&self, state: &Snapshot) -> Result<()> {
        self.verify_backup()?;
        if read_plain(&self.path(REGISTRY)?)? != state.registry {
            return Err(bad(
                "foreign registry change or game update; no overwrite allowed",
            ));
        }
        self.verify_groups(&state.groups, false)
    }
    /// Rebuild from the original baseline; reserve new IDs without touching disk.
    /// The returned diff includes creation, registry replacement and old-group removal.
    pub fn plan(&self, files: &BTreeMap<String, Vec<u8>>) -> Result<Plan> {
        let history = self.history()?;
        if history.pending.is_some() {
            return Err(bad("pending transaction; recover before planning"));
        }
        self.verify_current(&history.current)?;
        if files.is_empty() {
            return Ok(Plan {
                before: history.current,
                after: self.baseline.clone(),
                files: BTreeMap::new(),
            });
        }
        let groups = self.parse_files(files)?;
        if self.registry_for(&groups)?
            != *files
                .get(REGISTRY)
                .ok_or_else(|| bad("overlay registry missing"))?
        {
            return Err(bad(
                "candidate registry differs from original baseline plus candidate groups",
            ));
        }
        let mut occupied = self.reserved.clone();
        for item in fs::read_dir(&self.root)? {
            occupied.insert(item?.file_name().to_string_lossy().into_owned());
        }
        let requested: BTreeSet<_> = groups.keys().cloned().collect();
        let mut after = BTreeMap::new();
        let mut output = BTreeMap::new();
        for (name, g) in groups {
            let selected = if !occupied.contains(&name) {
                name.clone()
            } else {
                (1..=9999)
                    .map(|n| format!("{n:04}"))
                    .find(|n| !occupied.contains(n) && !requested.contains(n))
                    .ok_or_else(|| bad("no free overlay group"))?
            };
            occupied.insert(selected.clone());
            for file in ["0.pamt", "0.paz"] {
                output.insert(
                    format!("{selected}/{file}"),
                    files[&format!("{name}/{file}")].clone(),
                );
            }
            after.insert(selected, g);
        }
        let registry = self.registry_for(&after)?;
        output.insert(REGISTRY.into(), registry.clone());
        Ok(Plan {
            before: history.current,
            after: Snapshot {
                registry,
                groups: after,
            },
            files: output,
        })
    }
    fn parse_files(&self, files: &BTreeMap<String, Vec<u8>>) -> Result<BTreeMap<String, Group>> {
        if files.len() < 3 || files.len() > MAX_GROUPS * 2 + 1 || !files.contains_key(REGISTRY) {
            return Err(bad("overlay requires registry and complete group pairs"));
        }
        let mut names = BTreeSet::new();
        for path in files.keys().filter(|p| p.as_str() != REGISTRY) {
            let (g, file) = path
                .split_once('/')
                .ok_or_else(|| bad("invalid overlay path"))?;
            if g.len() != 4
                || !g.bytes().all(|b| b.is_ascii_digit())
                || !matches!(file, "0.pamt" | "0.paz")
                || self.reserved.contains(g)
            {
                return Err(bad("invalid or reserved overlay path"));
            }
            names.insert(g.to_owned());
        }
        let mut groups = BTreeMap::new();
        let mut virtual_paths = BTreeSet::new();
        for name in names {
            let pamt = files
                .get(&format!("{name}/0.pamt"))
                .ok_or_else(|| bad("PAMT missing"))?;
            let paz = files
                .get(&format!("{name}/0.paz"))
                .ok_or_else(|| bad("PAZ missing"))?;
            for path in crimson_format::overlay::validate_pair(pamt, paz)? {
                if !virtual_paths.insert(path) {
                    return Err(bad("conflicting virtual table path across groups"));
                }
            }
            groups.insert(
                name,
                Group {
                    pamt: Digest::of(pamt),
                    paz: Digest::of(paz),
                    checksum: u32::from_le_bytes(pamt[..4].try_into().unwrap()),
                },
            );
        }
        Ok(groups)
    }
    pub fn apply(
        &mut self,
        plan: &Plan,
        mut check: impl FnMut() -> Result<()>,
        fail_at: Option<usize>,
    ) -> Result<Vec<&'static str>> {
        if self._lock.is_none() {
            return Err(bad("read-only transaction inspection"));
        }
        check()?;
        let history = self.history()?;
        if history.pending.is_some() || history.current != plan.before {
            return Err(bad("stale plan or pending transaction"));
        }
        self.verify_current(&plan.before)?;
        if plan.before == plan.after {
            return Ok(Vec::new());
        }
        for name in plan.after.groups.keys() {
            if self.path(name)?.try_exists()? {
                return Err(bad("group collision after preview"));
            }
        }
        let mut fault = Fault {
            at: fail_at,
            ..Default::default()
        };
        let directory = format!("{HISTORY}/{:016}", history.next);
        fs::create_dir(self.path(&directory)?)?;
        fault.point("transaction_directory")?;
        let intent = Intent {
            version: 2,
            id: history.next,
            started_at: std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap_or_default()
                .as_secs(),
            baseline: hash_bytes(&self.baseline.registry),
            before: plan.before.clone(),
            after: plan.after.clone(),
        };
        let encoded = serde_json::to_vec(&intent)?;
        let partial = self.path(&format!("{directory}/intent.part"))?;
        resumable(&partial, &encoded, &mut fault)?;
        super::guard::publish(&partial, &self.path(&format!("{directory}/intent.json"))?)?;
        fault.point("intent_published")?;
        let pending = Pending {
            intent,
            encoded,
            directory: directory.clone(),
        };
        self.ensure_dir(&format!("{directory}/stage"))?;
        fault.point("stage_directory")?;
        let registry_part = self.path(&format!("{directory}/registry.part"))?;
        resumable(&registry_part, &plan.after.registry, &mut fault)?;
        for name in plan.after.groups.keys() {
            check()?;
            let stage = format!("{directory}/stage/{name}");
            self.ensure_dir(&stage)?;
            fault.point("group_staging_directory")?;
            for file in ["0.pamt", "0.paz"] {
                check()?;
                resumable(
                    &self.path(&format!("{stage}/{file}"))?,
                    &plan.files[&format!("{name}/{file}")],
                    &mut fault,
                )?;
            }
            let pamt = read_plain(&self.path(&format!("{stage}/0.pamt"))?)?;
            let paz = read_plain(&self.path(&format!("{stage}/0.paz"))?)?;
            crimson_format::overlay::validate_pair(&pamt, &paz)?;
            check()?;
            self.verify_current(&plan.before)?;
            super::guard::publish(&self.path(&stage)?, &self.path(name)?)?;
            fault.point("group_published")?;
        }
        check()?;
        self.verify_current(&plan.before)?;
        self.verify_groups(&plan.after.groups, false)?;
        if read_plain(&registry_part)? != plan.after.registry {
            return Err(bad("staged registry changed"));
        }
        super::guard::replace(&registry_part, &self.path(REGISTRY)?)?;
        fault.point("registry_committed")?;
        self.finish(&pending, Outcome::Committed, &mut check, &mut fault)?;
        Ok(fault.trace)
    }
    pub fn recover(
        &mut self,
        mut check: impl FnMut() -> Result<()>,
        fail_at: Option<usize>,
    ) -> Result<Vec<&'static str>> {
        if self._lock.is_none() {
            return Err(bad("read-only transaction inspection"));
        }
        check()?;
        let history = self.history()?;
        let Some(pending) = history.pending else {
            self.verify_current(&history.current)?;
            return Ok(Vec::new());
        };
        let outcome = self.pending_outcome(&pending)?;
        let mut fault = Fault {
            at: fail_at,
            ..Default::default()
        };
        self.finish(&pending, outcome, &mut check, &mut fault)?;
        Ok(fault.trace)
    }
    fn pending_outcome(&self, pending: &Pending) -> Result<Outcome> {
        let registry = read_plain(&self.path(REGISTRY)?)?;
        if registry == pending.intent.after.registry {
            Ok(Outcome::Committed)
        } else if registry == pending.intent.before.registry {
            Ok(Outcome::RolledBack)
        } else {
            Err(bad(
                "foreign registry/update during recovery; no overwrite allowed",
            ))
        }
    }
    /// Exact visible-file cleanup followed by restore, without publishing a journal.
    pub fn restore_review(&self) -> Result<RestoreReview> {
        let history = self.history()?;
        let mut files = Vec::new();
        let mut remove_directories = Vec::new();
        let mut removal = |groups: &BTreeMap<String, Group>| -> Result<()> {
            for (name, group) in groups {
                for (file, digest) in [("0.pamt", &group.pamt), ("0.paz", &group.paz)] {
                    let path = format!("{name}/{file}");
                    if self.path(&path)?.try_exists()? {
                        files.push(FileChange {
                            path,
                            action: "remove",
                            before_sha256: Some(digest.sha256.clone()),
                            after_sha256: None,
                        });
                    }
                }
                if self.path(name)?.try_exists()? {
                    remove_directories.push(name.clone());
                }
            }
            Ok(())
        };
        let (keep, pending_outcome) = if let Some(pending) = &history.pending {
            let outcome = self.pending_outcome(pending)?;
            let (keep, remove, label) = match outcome {
                Outcome::Committed => (&pending.intent.after, &pending.intent.before, "committed"),
                Outcome::RolledBack => {
                    (&pending.intent.before, &pending.intent.after, "rolled_back")
                }
            };
            self.verify_current(keep)?;
            self.verify_groups(&remove.groups, true)?;
            removal(&remove.groups)?;
            (keep, Some(label))
        } else {
            self.verify_current(&history.current)?;
            (&history.current, None)
        };
        // The first removals finish a pending transaction. Restore then commits
        // the baseline registry before removing the currently selected overlays.
        let cleanup_count = files.len();
        let mut rest = Plan {
            before: keep.clone(),
            after: self.baseline.clone(),
            files: BTreeMap::new(),
        }
        .changes();
        files.append(&mut rest);
        remove_directories.extend(keep.groups.keys().cloned());
        debug_assert!(files[..cleanup_count].iter().all(|f| f.action == "remove"));
        Ok(RestoreReview {
            backup_sha256: hash_bytes(&self.baseline.registry),
            backup_bytes: self.baseline.registry.len(),
            current_registry_sha256: hash_bytes(&keep.registry),
            pending_outcome,
            pending_intent_sha256: history.pending.as_ref().map(|p| hash_bytes(&p.encoded)),
            next_transaction_id: history.next.to_string(),
            files,
            remove_directories,
        })
    }
    fn finish(
        &self,
        pending: &Pending,
        outcome: Outcome,
        check: &mut impl FnMut() -> Result<()>,
        fault: &mut Fault,
    ) -> Result<()> {
        let (keep, remove) = match outcome {
            Outcome::Committed => (&pending.intent.after, &pending.intent.before),
            Outcome::RolledBack => (&pending.intent.before, &pending.intent.after),
        };
        // Missing obsolete files are allowed after interrupted cleanup. Changed
        // bytes or foreign children are rejected before any further deletion.
        self.verify_current(keep)?;
        self.verify_groups(&remove.groups, true)?;
        for (name, g) in &remove.groups {
            for (file, digest) in [("0.pamt", &g.pamt), ("0.paz", &g.paz)] {
                check()?;
                self.verify_current(keep)?;
                let path = self.path(&format!("{name}/{file}"))?;
                if path.try_exists()? {
                    if !digest.matches(&read_plain(&path)?) {
                        return Err(bad("obsolete file changed during cleanup"));
                    }
                    fs::remove_file(path)?;
                    fault.point("obsolete_file_removed")?;
                }
            }
            check()?;
            let path = self.path(name)?;
            if path.try_exists()? {
                fs::remove_dir(path)?;
                fault.point("obsolete_directory_removed")?;
            }
        }
        check()?;
        self.verify_current(keep)?;
        let complete = serde_json::to_vec(&Complete {
            intent_sha256: hash_bytes(&pending.encoded),
            outcome,
        })?;
        let partial = self.path(&format!("{}/complete.part", pending.directory))?;
        resumable(&partial, &complete, fault)?;
        super::guard::publish(
            &partial,
            &self.path(&format!("{}/complete.json", pending.directory))?,
        )?;
        fault.point("completion_published")?;
        Ok(())
    }
    pub fn restore(
        &mut self,
        check: impl FnMut() -> Result<()>,
        fail_at: Option<usize>,
    ) -> Result<Vec<&'static str>> {
        let plan = self.plan(&BTreeMap::new())?;
        self.apply(&plan, check, fail_at)
    }
}
pub(super) fn checked(root: &Path, relative: &str) -> Result<PathBuf> {
    let parts: Vec<_> = relative.split('/').collect();
    if parts
        .iter()
        .any(|p| p.is_empty() || *p == "." || *p == ".." || p.contains(['\\', ':']))
    {
        return Err(bad("invalid transaction path"));
    }
    let mut path = root.to_owned();
    for (i, p) in parts.iter().enumerate() {
        path.push(p);
        match fs::symlink_metadata(&path) {
            Ok(meta) => plain(&path, i + 1 != parts.len() || meta.is_dir())?,
            Err(e) if e.kind() == std::io::ErrorKind::NotFound => {}
            Err(e) => return Err(e.into()),
        }
    }
    Ok(path)
}
fn read_limit(path: &Path, limit: usize) -> Result<Vec<u8>> {
    plain(path, false)?;
    let mut bytes = Vec::new();
    File::open(path)?
        .take(limit as u64 + 1)
        .read_to_end(&mut bytes)?;
    if bytes.len() > limit {
        return Err(bad("transaction file exceeds size limit"));
    }
    Ok(bytes)
}
fn read_plain(path: &Path) -> Result<Vec<u8>> {
    read_limit(path, crimson_format::MAX_FILE_BYTES)
}
fn resumable(path: &Path, bytes: &[u8], fault: &mut Fault) -> Result<()> {
    let mut written = 0;
    let mut file = if path.try_exists()? {
        let previous = read_limit(path, bytes.len())?;
        if !bytes.starts_with(&previous) {
            return Err(bad(
                "partial private file differs from expected prefix; preserved for inspection",
            ));
        }
        written = previous.len();
        OpenOptions::new().append(true).open(path)?
    } else {
        OpenOptions::new().write(true).create_new(true).open(path)?
    };
    let middle = written + (bytes.len() - written).div_ceil(2);
    file.write_all(&bytes[written..middle])?;
    fault.point("file_partial")?;
    file.write_all(&bytes[middle..])?;
    file.sync_all()?;
    fault.point("file_synced")?;
    drop(file);
    if read_plain(path)? != bytes {
        return Err(bad("staged file readback failed"));
    }
    Ok(())
}
pub(super) fn plain(path: &Path, dir: bool) -> Result<()> {
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
#[path = "transaction_v2_tests.rs"]
mod tests;
