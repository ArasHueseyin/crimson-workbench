//! Exact, revocable consent to preserve foreign additions. Never writes game data
//! or accepts modifications to depot originals or foreign registry registrations.
use super::*;
use crate::installation::Inventory;
const MAX_FILES: usize = 512;
const MAX_BYTES: u64 = 512 * 1024 * 1024;
const MAX_RECORDS: usize = 128;
#[derive(Clone, Default, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub(super) struct Set {
    pub files: Vec<FileHash>,
    pub directories: BTreeSet<String>,
}
#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Record {
    version: u32,
    game: PathBuf,
    approved_at: u64,
    sequence: usize,
    set: Set,
}
#[derive(Default)]
pub(super) struct Accepted {
    pub revision: Option<String>,
    pub sequence: usize,
    pub set: Set,
}
#[derive(Serialize)]
pub struct Review {
    pub review_id: String,
    pub game_path: PathBuf,
    pub files: Vec<FileHash>,
    pub directories: BTreeSet<String>,
    pub total_bytes: String,
    pub previous_approval: Option<String>,
    pub game_running: Option<bool>,
}
#[derive(Serialize)]
pub struct Receipt {
    pub approval_id: String,
    pub files: usize,
    pub directories: usize,
    pub action: &'static str,
}
fn base(game: &Path) -> PathBuf {
    Path::new(".local/foreign").join(hash_bytes(game.to_string_lossy().as_bytes()))
}
fn safe(path: &str) -> bool {
    let lower = path.to_lowercase();
    !lower.starts_with(".workbench")
        && lower != "meta"
        && !lower.starts_with("meta/")
        && !lower.split('/').any(|p| {
            matches!(p, "save" | "saves" | "savegame" | "savegames")
                || p.is_empty()
                || p == "."
                || p == ".."
                || p.contains(['\\', ':'])
                || p.ends_with(['.', ' '])
        })
        && ![".sav", ".save", ".cdsav"]
            .iter()
            .any(|e| lower.ends_with(e))
        && path.len() <= 1024
}
fn validate(set: &Set) -> Result<()> {
    if set.files.len() > MAX_FILES || set.directories.len() > 1024 {
        return Err(bad("Too many third-party entries"));
    }
    let mut paths = BTreeSet::new();
    let mut total = 0u64;
    for file in &set.files {
        if !safe(&file.path) || !digest(&file.sha256) || !paths.insert(file.path.to_lowercase()) {
            return Err(bad("Invalid third-party file evidence"));
        }
        total = total
            .checked_add(file.bytes)
            .ok_or_else(|| bad("Third-party file size overflow"))?;
    }
    if total > MAX_BYTES {
        return Err(bad("Additional files exceed 512 MiB; review manually"));
    }
    for dir in &set.directories {
        if !safe(dir) || !paths.insert(dir.to_lowercase()) {
            return Err(bad("Invalid third-party folder"));
        }
    }
    Ok(())
}
pub(super) fn accepted(policy: &PathPolicy, game: &Path) -> Result<Accepted> {
    let root = game.canonicalize()?;
    let relative = base(&root);
    let folder = policy.output_root().join(&relative);
    match fs::symlink_metadata(&folder) {
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => return Ok(Accepted::default()),
        Err(e) => return Err(e.into()),
        Ok(_) => {}
    }
    let folder = policy
        .check_cache(&relative.join(".boundary"))?
        .parent()
        .unwrap()
        .to_owned();
    transaction::plain(&folder, true)?;
    let mut files = BTreeMap::new();
    for entry in fs::read_dir(&folder)? {
        let entry = entry?;
        let name = entry
            .file_name()
            .into_string()
            .map_err(|_| bad("Invalid approval log"))?;
        if files.len() >= MAX_RECORDS
            || name.len() != 21
            || !name.ends_with(".json")
            || !name[..16].bytes().all(|b| b.is_ascii_digit())
        {
            return Err(bad("Unknown entry in approval log"));
        }
        files.insert(name, entry.path());
    }
    let mut result = Accepted::default();
    for (i, (name, path)) in files.into_iter().enumerate() {
        if name != format!("{:016}.json", i + 1) {
            return Err(bad("Gap in approval log"));
        }
        let mut raw = Vec::new();
        fs::File::open(policy.check_cache(&path)?)?
            .take(1_048_577)
            .read_to_end(&mut raw)?;
        if raw.len() > 1_048_576 {
            return Err(bad("Approval log too large"));
        }
        let record: Record = serde_json::from_slice(&raw)?;
        validate(&record.set)?;
        if record.version != 1
            || record.game != root
            || record.approved_at == 0
            || record.sequence != i + 1
        {
            return Err(bad("Approval does not belong to this installation"));
        }
        result = Accepted {
            revision: Some(hash_bytes(&raw)),
            sequence: i + 1,
            set: record.set,
        };
    }
    Ok(result)
}
fn token(root: &Path, set: &Set, previous: &Option<String>) -> Result<String> {
    Ok(hash_bytes(&serde_json::to_vec(&(root, set, previous))?))
}
fn candidate(report: &Inventory) -> Result<Set> {
    if !report.directory_scan_complete || !report.depot_comparison_available {
        return Err(bad("A complete Steam file list is required"));
    }
    let extras: BTreeSet<_> = report
        .files
        .iter()
        .filter(|f| f.state == "additional" && f.expected_bytes.is_none())
        .map(|f| f.path.as_str())
        .collect();
    if report.issues.iter().any(|i| {
        !matches!(i.code, "unregistered_group" | "group_outside_depots")
            || !i.path.as_ref().is_some_and(|p| extras.contains(p.as_str()))
    }) || report.files.iter().any(|f| {
        !matches!(
            f.state,
            "size_matches" | "additional" | "workbench_registry"
        )
    }) {
        return Err(bad(
            "Original files, links or Workbench history differ. Approval blocked; check original state and file list first.",
        ));
    }
    if !report.managed_registry {
        use sha1::Digest;
        let expected = report
            .files
            .iter()
            .find(|f| f.path == "meta/0.papgt")
            .and_then(|f| f.expected_sha1.as_ref())
            .ok_or_else(|| bad("Registry missing from depot evidence"))?;
        let mut raw = Vec::new();
        fs::File::open(transaction::checked(&report.game_path, "meta/0.papgt")?)?
            .take(1_048_577)
            .read_to_end(&mut raw)?;
        if raw.len() > 1_048_576 || format!("{:x}", sha1::Sha1::digest(&raw)) != *expected {
            return Err(bad(
                "Registry is not verified as original or a Workbench mod. Remove registered third-party mods and complete Steam file verification first.",
            ));
        }
    }
    let mut set = Set::default();
    for file in &report.files {
        if !extras.contains(file.path.as_str()) {
            continue;
        }
        if !safe(&file.path) {
            return Err(bad(format!(
                "Additional path will not be read or approved: {}",
                file.path
            )));
        }
        let absolute = transaction::checked(&report.game_path, &file.path)?;
        if let Some(bytes) = &file.actual_bytes {
            transaction::plain(&absolute, false)?;
            set.files.push(FileHash {
                path: file.path.clone(),
                bytes: bytes
                    .parse()
                    .map_err(|_| bad("Invalid additional file size"))?,
                sha256: "0".repeat(64),
            });
        } else {
            transaction::plain(&absolute, true)?;
            set.directories.insert(file.path.clone());
        }
    }
    validate(&set)?;
    Ok(set)
}
fn hash_observed(path: &Path, expected: u64) -> Result<(u64, String)> {
    use sha2::{Digest, Sha256};
    let mut input = fs::File::open(path)?.take(expected + 1);
    let mut hash = Sha256::new();
    let mut buffer = [0; 128 * 1024];
    let mut bytes = 0;
    loop {
        let n = input.read(&mut buffer)?;
        if n == 0 {
            break;
        }
        bytes += n as u64;
        hash.update(&buffer[..n]);
    }
    Ok((bytes, format!("{:x}", hash.finalize())))
}
#[derive(Serialize)]
pub struct Inspection {
    pub approval_id: Option<String>,
    pub approved_files: usize,
    pub review: Option<Review>,
    pub issue: Option<String>,
}
pub fn inspect(policy: &PathPolicy, game: &Path) -> Result<Inspection> {
    let result = preview(policy, game);
    let state = accepted(policy, game)?;
    let (review, issue) = match result {
        Ok(r) if r.previous_approval == state.revision => (Some(r), None),
        Ok(_) => (
            None,
            Some("Approval changed in the meantime; check again".into()),
        ),
        Err(e) => (None, Some(e.to_string())),
    };
    Ok(Inspection {
        approval_id: if state.set.files.is_empty() && state.set.directories.is_empty() {
            None
        } else {
            state.revision
        },
        approved_files: state.set.files.len(),
        review,
        issue,
    })
}
pub(super) fn unchanged(policy: &PathPolicy, e: &Evidence) -> Result<()> {
    if accepted(policy, &e.admission.game)?.revision != e.foreign_revision {
        return Err(bad(
            "Third-party file approval changed or revoked; live operation stopped",
        ));
    }
    Ok(())
}
fn prepared(policy: &PathPolicy, game: &Path) -> Result<(Set, Accepted)> {
    let report = crate::installation::inspect_owned(policy, game)?;
    let mut set = candidate(&report)?;
    let previous = accepted(policy, game)?;
    for dir in &set.directories {
        policy.check_additional_read(
            &report.game_path,
            &transaction::checked(&report.game_path, dir)?,
        )?;
    }
    for file in &mut set.files {
        let (bytes, sha256) = hash_observed(
            &policy.check_additional_read(
                &report.game_path,
                &transaction::checked(&report.game_path, &file.path)?,
            )?,
            file.bytes,
        )?;
        if bytes != file.bytes {
            return Err(bad("Additional file changed during preview"));
        }
        file.sha256 = sha256;
    }
    let after = crate::installation::inspect_owned(policy, game)?;
    if serde_json::to_vec(&candidate(&after)?)? != serde_json::to_vec(&candidate(&report)?)?
        || previous.revision != accepted(policy, game)?.revision
    {
        return Err(bad("Third-party file inventory changed during preview"));
    }
    Ok((set, previous))
}
pub fn preview(policy: &PathPolicy, game: &Path) -> Result<Review> {
    let root = game.canonicalize()?;
    let (set, previous) = prepared(policy, &root)?;
    Ok(Review {
        review_id: token(&root, &set, &previous.revision)?,
        game_path: root,
        previous_approval: previous.revision,
        game_running: super::super::guard::game_running().ok(),
        total_bytes: set.files.iter().map(|f| f.bytes).sum::<u64>().to_string(),
        files: set.files,
        directories: set.directories,
    })
}
fn save(
    policy: &PathPolicy,
    game: &Path,
    set: Set,
    previous: Accepted,
    action: &'static str,
) -> Result<Receipt> {
    validate(&set)?;
    if previous.sequence >= MAX_RECORDS {
        return Err(bad("Approval log full"));
    }
    // Never consume the last slot with consent that could no longer be revoked.
    if previous.sequence == MAX_RECORDS - 1
        && (!set.files.is_empty() || !set.directories.is_empty())
    {
        return Err(bad(
            "Approval log full; final entry reserved for revocation",
        ));
    }
    if accepted(policy, game)?.revision != previous.revision {
        return Err(bad(
            "Approval changed in the meantime; create a new preview",
        ));
    }
    let files = set.files.len();
    let directories = set.directories.len();
    let record = Record {
        version: 1,
        game: game.canonicalize()?,
        sequence: previous.sequence + 1,
        approved_at: std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap_or_default()
            .as_secs(),
        set,
    };
    let bytes = serde_json::to_vec_pretty(&record)?;
    crate::write_generated(
        policy,
        &base(&record.game).join(format!("{:016}.json", record.sequence)),
        &bytes,
    )?;
    Ok(Receipt {
        approval_id: hash_bytes(&bytes),
        files,
        directories,
        action,
    })
}
pub fn confirm(
    policy: &PathPolicy,
    game: &Path,
    review_id: &str,
    preserve_confirmed: bool,
) -> Result<Receipt> {
    if !preserve_confirmed {
        return Err(bad("Additional files require explicit approval"));
    }
    let root = game.canonicalize()?;
    let (set, previous) = prepared(policy, &root)?;
    if !digest(review_id) || token(&root, &set, &previous.revision)? != review_id {
        return Err(bad("Third-party file preview is stale; nothing approved"));
    }
    if set.files.is_empty() && set.directories.is_empty() {
        return Err(bad("No additional files to approve"));
    }
    save(policy, &root, set, previous, "preserve_foreign")
}
pub fn revoke(policy: &PathPolicy, game: &Path, approval_id: &str) -> Result<Receipt> {
    let previous = accepted(policy, game)?;
    if previous.set.files.is_empty() && previous.set.directories.is_empty() {
        return Err(bad("No active third-party file approval"));
    }
    if previous.revision.as_deref() != Some(approval_id) {
        return Err(bad("Approval is no longer current"));
    }
    save(policy, game, Set::default(), previous, "revoke_foreign")
}
/// Metadata attribution only. Contents are freshly hashed under held protection
/// before B0 writes. Depot originals are never removed from their comparison.
pub(crate) fn classify(policy: &PathPolicy, report: &mut Inventory) -> Result<()> {
    let accepted = accepted(policy, &report.game_path)?;
    for path in accepted
        .set
        .files
        .iter()
        .map(|f| &f.path)
        .chain(accepted.set.directories.iter())
    {
        policy.check_additional_read(&report.game_path, &report.game_path.join(path))?;
    }
    report.foreign_approval = accepted.revision;
    let files: BTreeMap<_, _> = accepted
        .set
        .files
        .iter()
        .map(|f| (f.path.as_str(), f))
        .collect();
    let mut original = Vec::new();
    let mut attributed = BTreeSet::new();
    for mut file in std::mem::take(&mut report.files) {
        let matches = file.state == "additional"
            && file.expected_bytes.is_none()
            && (files.get(file.path.as_str()).is_some_and(|f| {
                file.actual_bytes.as_deref() == Some(f.bytes.to_string().as_str())
            }) || accepted.set.directories.contains(&file.path) && file.actual_bytes.is_none());
        if matches {
            attributed.insert(file.path.clone());
            file.state = "foreign_acknowledged";
            report.foreign_files.push(file);
        } else {
            original.push(file);
        }
    }
    report.files = original;
    report.issues.retain(|i| {
        !matches!(i.code, "unregistered_group" | "group_outside_depots")
            || !i.path.as_ref().is_some_and(|p| attributed.contains(p))
    });
    if !report.foreign_files.is_empty() {
        report.limitations.push("Third-party additions were explicitly approved for keeping. Metadata mapping without a fresh content comparison; no vanilla or compatibility confirmation. These files are hashed again and protected before live writes.".into());
    }
    Ok(())
}
pub(super) fn decorate(policy: &PathPolicy, e: &mut Evidence) -> Result<()> {
    let accepted = accepted(policy, &e.admission.game)?;
    let originals: BTreeSet<_> = e.sources.iter().map(|f| f.path.to_lowercase()).collect();
    e.foreign_files = accepted
        .set
        .files
        .into_iter()
        .filter(|f| !originals.contains(&f.path.to_lowercase()))
        .collect();
    e.foreign_dirs = accepted.set.directories;
    e.foreign_revision = accepted.revision;
    for path in e
        .foreign_files
        .iter()
        .map(|f| &f.path)
        .chain(e.foreign_dirs.iter())
    {
        policy.check_additional_read(&e.admission.game, &e.admission.game.join(path))?;
    }
    Ok(())
}

#[cfg(all(test, windows))]
#[path = "live_foreign_tests.rs"]
mod tests;
