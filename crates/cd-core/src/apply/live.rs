//! Live B0 adapter. Persisted user provenance is distinct from hash evidence.
//! Every mutation requires fresh source hashes under held Windows protection.
use super::{FileChange, protected, transaction};
use crate::{
    Error, Result,
    fingerprint::{FileHash, hash_bytes},
    paths::PathPolicy,
};
use serde::{Deserialize, Serialize};
use std::{
    collections::{BTreeMap, BTreeSet},
    fs,
    io::Read,
    path::{Path, PathBuf},
    sync::atomic::{AtomicBool, Ordering},
};
#[path = "live_foreign.rs"]
pub mod foreign;
#[path = "live_updates.rs"]
pub mod updates;

fn bad(s: impl Into<String>) -> Error {
    Error::Invalid(s.into())
}
fn digest(s: &str) -> bool {
    s.len() == 64
        && s.bytes()
            .all(|b| b.is_ascii_digit() || (b'a'..=b'f').contains(&b))
}
#[derive(Clone, Deserialize, Serialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
struct Admission {
    version: u32,
    kind: String,
    game: PathBuf,
    baseline_id: String,
    report_sha256: String,
    registry_sha256: String,
}
struct Evidence {
    admission: Admission,
    sources: Vec<FileHash>,
    registry: Vec<u8>,
    audited_at: u64,
    archived_files: Vec<FileHash>,
    archived_dirs: BTreeSet<String>,
    foreign_files: Vec<FileHash>,
    foreign_dirs: BTreeSet<String>,
    foreign_revision: Option<String>,
}
impl Evidence {
    fn protection(&self) -> Result<protected::Baseline> {
        let mut files = self.sources.clone();
        files.extend(self.foreign_files.clone());
        protected::Baseline::admitted(self.admission.registry_sha256.clone(), &files)
    }
    fn identity(&self) -> Result<String> {
        Ok(hash_bytes(&serde_json::to_vec(&(
            &self.admission,
            &self.foreign_revision,
        ))?))
    }
}
#[derive(Serialize)]
pub struct SetupPreview {
    pub review_id: String,
    pub baseline_id: String,
    pub game_path: PathBuf,
    pub files: usize,
    pub total_bytes: String,
    pub audited_at: u64,
    pub preserved_foreign_files: usize,
    pub provenance: &'static str,
}
#[derive(Serialize)]
pub struct Status {
    pub configured: bool,
    pub initialized: bool,
    pub game_running: Option<bool>,
    pub baseline_id: Option<String>,
    pub pending_basis: Option<String>,
    pub active_overlay: bool,
    pub recovery_required: bool,
    pub issues: Vec<String>,
}
#[derive(Clone, Deserialize, Serialize)]
#[serde(tag = "action", rename_all = "snake_case", deny_unknown_fields)]
pub enum Request {
    Apply {
        settings: Box<crate::mods::ModRequest>,
    },
    Restore,
}
#[derive(Serialize)]
pub struct Review {
    pub review_id: String,
    pub action: &'static str,
    pub game_path: PathBuf,
    pub baseline_id: String,
    pub files: Vec<FileChange>,
    pub recovery_required: bool,
    pub protected_source_files: usize,
    pub preserved_foreign_files: usize,
    pub total_bytes: String,
}
#[derive(Debug, Serialize)]
pub struct Receipt {
    pub action: &'static str,
    pub review_id: String,
    pub registry_sha256: String,
    pub source_files_verified: usize,
    pub pending_recovered: bool,
}
fn admission_path(game: &Path) -> PathBuf {
    Path::new(".local/live")
        .join(hash_bytes(game.to_string_lossy().as_bytes()))
        .join("admission.json")
}
fn evidence(policy: &PathPolicy, game: &Path, id: &str) -> Result<Evidence> {
    let mut value = evidence_kind(policy, game, id, true)?;
    foreign::decorate(policy, &mut value)?;
    value.protection()?;
    Ok(value)
}
fn evidence_kind(policy: &PathPolicy, game: &Path, id: &str, known: bool) -> Result<Evidence> {
    let (report, registry, report_sha256) = if known {
        crate::installation::baseline::apply_evidence(policy, game, id)?
    } else {
        crate::installation::baseline::historical_evidence(policy, game, id)?
    };
    let sources = report
        .files
        .iter()
        .map(|f| {
            Ok(FileHash {
                path: f.path.clone(),
                bytes: f.bytes.parse().map_err(|_| bad("invalid source size"))?,
                sha256: f.sha256.clone(),
            })
        })
        .collect::<Result<_>>()?;
    let value = Evidence {
        admission: Admission {
            version: 1,
            kind: "steam-verified-by-user".into(),
            game: game.canonicalize()?,
            baseline_id: id.into(),
            report_sha256,
            registry_sha256: hash_bytes(&registry),
        },
        sources,
        registry,
        audited_at: report.finished_at,
        archived_files: Vec::new(),
        archived_dirs: BTreeSet::new(),
        foreign_files: Vec::new(),
        foreign_dirs: BTreeSet::new(),
        foreign_revision: None,
    };
    value.protection()?;
    Ok(value)
}
fn initial(policy: &PathPolicy, game: &Path) -> Result<Admission> {
    let root = game.canonicalize()?;
    let path = policy.check_cache(&admission_path(&root))?;
    let mut raw = Vec::new();
    fs::File::open(path)?.take(16385).read_to_end(&mut raw)?;
    if raw.len() > 16384 {
        return Err(bad("Live-Einrichtung überschreitet das Größenlimit"));
    }
    let admission: Admission = serde_json::from_slice(&raw)?;
    let value = evidence_kind(policy, &root, &admission.baseline_id, false)?;
    if serde_json::to_vec(&admission)? != serde_json::to_vec(&value.admission)? {
        return Err(bad(
            "Live-Einrichtung passt nicht zur gesicherten Ausgangsbasis",
        ));
    }
    Ok(admission)
}
fn load(policy: &PathPolicy, game: &Path) -> Result<Evidence> {
    let state = updates::state(policy, game)?;
    if state.pending.is_some() {
        return Err(bad(
            "Basiswechsel unterbrochen; zuerst Basiswechsel fortsetzen.",
        ));
    }
    let mut value = evidence(policy, game, &state.active.baseline_id)?;
    if value.admission != state.active {
        return Err(bad(
            "Aktive Basis stimmt nicht mit dem Ausgangsstand überein",
        ));
    }
    value.archived_files = state.archived_files();
    value.archived_dirs = state.archived_dirs();
    Ok(value)
}
// Count paths without opening source contents: held EXE handles intentionally
// deny other opens. Source identities/types are checked by the held lease.
fn layout(e: &Evidence, owned: &[String]) -> Result<()> {
    let root = &e.admission.game;
    transaction::plain(root, true)?;
    let expected: BTreeMap<_, _> = e
        .sources
        .iter()
        .chain(&e.archived_files)
        .chain(&e.foreign_files)
        .map(|f| (f.path.clone(), f.bytes))
        .collect();
    let mut dirs = e.archived_dirs.clone();
    dirs.extend(e.foreign_dirs.clone());
    for path in expected.keys() {
        let mut parent = Path::new(path).parent();
        while let Some(p) = parent.filter(|p| !p.as_os_str().is_empty()) {
            dirs.insert(p.to_string_lossy().replace('\\', "/"));
            parent = p.parent();
        }
    }
    let mut found = BTreeSet::new();
    let mut queue = vec![(root.to_owned(), String::new())];
    let mut count = 0;
    while let Some((dir, prefix)) = queue.pop() {
        for item in fs::read_dir(dir)? {
            let item = item?;
            count += 1;
            if count > 16384 {
                return Err(bad("Installationsbestand überschreitet das Limit"));
            }
            let name = item
                .file_name()
                .into_string()
                .map_err(|_| bad("Ungültiger Installationspfad"))?;
            let path = if prefix.is_empty() {
                name.clone()
            } else {
                format!("{prefix}/{name}")
            };
            let meta = fs::symlink_metadata(item.path())?;
            #[cfg(windows)]
            let linked = {
                use std::os::windows::fs::MetadataExt;
                meta.file_attributes() & 0x400 != 0
            };
            #[cfg(not(windows))]
            let linked = meta.file_type().is_symlink();
            if linked {
                return Err(bad(format!("Verknüpfter Installationspfad: {path}")));
            }
            if prefix.is_empty() && path == ".workbench" {
                transaction::plain(&item.path(), true)?;
                for file in fs::read_dir(item.path())? {
                    let file = file?;
                    if !matches!(
                        file.file_name().to_str(),
                        Some("lock" | "baseline.papgt" | "baseline.part" | "transactions")
                    ) {
                        return Err(bad("Fremde Dateien im Workbench-Transaktionsordner"));
                    }
                }
                continue;
            }
            if prefix.is_empty() && owned.contains(&path) {
                if !meta.is_dir() {
                    return Err(bad("Eigene Overlaygruppe ist kein Ordner"));
                }
                continue; // exact contents are verified by the transaction kernel
            }
            if meta.is_dir() && dirs.contains(&path) {
                queue.push((item.path(), path));
            } else if meta.is_file() && expected.contains_key(&path) {
                if path != "meta/0.papgt" && expected[&path] != meta.len() {
                    return Err(bad(format!("Dateigröße geändert: {path}")));
                }
                found.insert(path);
            } else {
                return Err(bad(format!(
                    "Fremder oder unerwarteter Installationspfad: {path}"
                )));
            }
        }
    }
    if found.len() != expected.len() {
        return Err(bad("Originaldateien fehlen; kein Apply oder Restore"));
    }
    Ok(())
}
fn stopped(cancel: &AtomicBool) -> Result<()> {
    if cancel.load(Ordering::SeqCst) {
        return Err(bad(
            "Live-Vorgang abgebrochen. Zustand erneut prüfen; eine begonnene Transaktion bleibt wiederherstellbar.",
        ));
    }
    match super::guard::game_running()? {
        false => Ok(()),
        true => Err(bad(
            "Crimson Desert läuft. Live-Einrichtung, Apply und Restore erst nach dem Spielen starten.",
        )),
    }
}
pub fn setup_preview(policy: &PathPolicy, game: &Path, id: &str) -> Result<SetupPreview> {
    let e = evidence(policy, game, id)?;
    let path = policy.check_cache(&admission_path(&e.admission.game))?;
    if path.try_exists()? {
        if load(policy, game)?.identity()? != e.identity()? {
            return Err(bad(
                "Andere Live-Basis bereits eingerichtet; keine automatische Überschreibung",
            ));
        }
    } else {
        let current = crate::installation::baseline::inspect(policy, game, id)?;
        if current.current_state != "metadata_match" {
            return Err(bad(
                "Installation weicht vom Ausgangsstand ab. Vor Einrichtung Steam-Dateiprüfung und neuen Inhaltsbericht erstellen.",
            ));
        }
        if e.admission.game.join(".workbench").try_exists()? {
            return Err(bad(
                "Vorhandener Transaktionsordner ohne passende Live-Einrichtung",
            ));
        }
    }
    layout(&e, &[])?;
    if crate::fingerprint::hash_file(&e.admission.game.join("meta/0.papgt"))?.1
        != e.admission.registry_sha256
    {
        return Err(bad("Einrichtung benötigt die originale Registry"));
    }
    Ok(SetupPreview {
        review_id: e.identity()?,
        baseline_id: id.into(),
        game_path: e.admission.game.clone(),
        files: e.sources.len(),
        total_bytes: e.sources.iter().map(|f| f.bytes).sum::<u64>().to_string(),
        audited_at: e.audited_at,
        preserved_foreign_files: e.foreign_files.len(),
        provenance: "Nutzerbestätigung der Steam-Dateiprüfung vor diesem Inhaltsbericht; anschließend frischer SHA-256-Vergleich unter Start- und Quellschutz. Keine unabhängige Herkunftszertifizierung.",
    })
}
pub fn setup(
    policy: &PathPolicy,
    game: &Path,
    id: &str,
    review_id: &str,
    steam_verified_before_audit: bool,
    cancel: &AtomicBool,
) -> Result<Status> {
    if !steam_verified_before_audit {
        return Err(bad(
            "Bestätigung fehlt: Steam-Dateiprüfung wurde vor dem gewählten Inhaltsbericht abgeschlossen.",
        ));
    }
    stopped(cancel)?;
    let preview = setup_preview(policy, game, id)?;
    if !digest(review_id) || preview.review_id != review_id {
        return Err(bad("Live-Einrichtungsvorschau veraltet; erneut prüfen"));
    }
    let e = evidence(policy, game, id)?;
    if e.identity()? != review_id {
        return Err(bad("Einrichtungsvorschau inzwischen geändert"));
    }
    let path = policy.check_cache(&admission_path(&e.admission.game))?;
    if !path.try_exists()? {
        // Persist the provenance claim before touching the installation so an
        // interrupted backup initialization can be explicitly resumed.
        crate::write_generated(policy, &path, &serde_json::to_vec_pretty(&e.admission)?)?;
    }
    let mut session = protected::Session::open(&e.admission.game, &e.protection()?, || {
        stopped(cancel)?;
        foreign::unchanged(policy, &e)?;
        layout(&e, &[])
    })?;
    if !session.restore_review()?.files.is_empty() {
        return Err(bad("Einrichtung erwartet unveränderte Registry"));
    }
    drop(session);
    status(policy, game)
}
pub fn status(policy: &PathPolicy, game: &Path) -> Result<Status> {
    let root = game.canonicalize()?;
    let mut value = Status {
        configured: false,
        initialized: false,
        game_running: None,
        baseline_id: None,
        pending_basis: None,
        active_overlay: false,
        recovery_required: false,
        issues: Vec::new(),
    };
    match super::guard::game_running() {
        Ok(v) => value.game_running = Some(v),
        Err(e) => value.issues.push(e.to_string()),
    }
    if !policy.check_cache(&admission_path(&root))?.try_exists()? {
        return Ok(value);
    }
    value.configured = true;
    let checked = (|| -> Result<()> {
        let history = updates::state(policy, &root)?;
        value.baseline_id = Some(history.active.baseline_id.clone());
        value.pending_basis = history
            .pending
            .as_ref()
            .map(|p| p.after.baseline_id.clone());
        let e = load(policy, &root)?;
        value.baseline_id = Some(e.admission.baseline_id.clone());
        let engine = transaction::Engine::inspect(&root, &e.admission.registry_sha256)?;
        let state = engine.restore_review()?;
        layout(&e, &state.remove_directories)?;
        value.initialized = true;
        value.active_overlay = state.current_registry_sha256 != e.admission.registry_sha256;
        value.recovery_required = state.pending_outcome.is_some();
        Ok(())
    })();
    if let Err(e) = checked {
        value.issues.push(e.to_string());
    }
    Ok(value)
}
fn built(game: &Path, request: &Request) -> Result<BTreeMap<String, Vec<u8>>> {
    match request {
        Request::Restore => Ok(BTreeMap::new()),
        Request::Apply { settings } => {
            let data = crate::GameData::open(game, "ger")?;
            Ok(crate::mods::ModCatalog::open(&data)?
                .build(settings.as_ref().clone())?
                .files)
        }
    }
}
fn review(
    e: &Evidence,
    request: &Request,
    state: &transaction::RestoreReview,
    files: Vec<FileChange>,
) -> Result<Review> {
    let review_id = hash_bytes(&serde_json::to_vec(&(
        &e.admission,
        &e.foreign_revision,
        request,
        state,
        &files,
    ))?);
    Ok(Review {
        review_id,
        action: match request {
            Request::Apply { .. } => "apply",
            Request::Restore => "restore",
        },
        game_path: e.admission.game.clone(),
        baseline_id: e.admission.baseline_id.clone(),
        files,
        recovery_required: state.pending_outcome.is_some(),
        protected_source_files: e.sources.len() - 1,
        preserved_foreign_files: e.foreign_files.len(),
        total_bytes: e
            .sources
            .iter()
            .filter(|f| f.path != "meta/0.papgt")
            .map(|f| f.bytes)
            .sum::<u64>()
            .to_string(),
    })
}
pub fn preview(policy: &PathPolicy, game: &Path, request: &Request) -> Result<Review> {
    let e = load(policy, game)?;
    let files = built(game, request)?;
    let engine = transaction::Engine::inspect(game, &e.admission.registry_sha256)?;
    let state = engine.restore_review()?;
    layout(&e, &state.remove_directories)?;
    let changes = match request {
        Request::Restore => state.files.clone(),
        Request::Apply { .. } => engine.plan(&files)?.changes(),
    };
    review(&e, request, &state, changes)
}
pub fn execute(
    policy: &PathPolicy,
    game: &Path,
    request: &Request,
    expected: &str,
    cancel: &AtomicBool,
) -> Result<Receipt> {
    stopped(cancel)?;
    if !digest(expected) {
        return Err(bad("Gültige Live-Vorschau erforderlich"));
    }
    let e = load(policy, game)?;
    // Table bytes are rebuilt before exclusive source handles are acquired.
    // The subsequent full hash pass must prove every byte still matches.
    let files = built(game, request)?;
    execute_built(
        &e,
        request,
        &files,
        expected,
        || {
            stopped(cancel)?;
            foreign::unchanged(policy, &e)
        },
        None,
    )
}
fn execute_built(
    e: &Evidence,
    request: &Request,
    files: &BTreeMap<String, Vec<u8>>,
    expected: &str,
    mut check: impl FnMut() -> Result<()>,
    fault: Option<usize>,
) -> Result<Receipt> {
    check()?;
    let initial = transaction::Engine::inspect(&e.admission.game, &e.admission.registry_sha256)?
        .restore_review()?;
    layout(e, &initial.remove_directories)?;
    let owned = std::cell::RefCell::new(initial.remove_directories);
    let mut session =
        protected::Session::open_existing(&e.admission.game, &e.protection()?, || {
            check()?;
            layout(e, &owned.borrow())
        })?;
    let state = session.restore_review()?;
    layout(e, &state.remove_directories)?;
    let plan = if matches!(request, Request::Apply { .. }) {
        Some(session.plan(files)?)
    } else {
        None
    };
    let changes = plan
        .as_ref()
        .map_or_else(|| state.files.clone(), |p| p.changes());
    let reviewed = review(e, request, &state, changes)?;
    if reviewed.review_id != expected {
        return Err(bad(
            "Live-Vorschau veraltet. Keine Änderung; erneut berechnen.",
        ));
    }
    if let Some(plan) = &plan {
        owned
            .borrow_mut()
            .extend(plan.group_names().map(str::to_owned));
    }
    let pending = state.pending_outcome.is_some();
    if let Some(plan) = plan {
        session.apply(&plan, fault)?;
    } else {
        session.recover(None)?;
        session.restore(fault)?;
    }
    let after = session.restore_review()?;
    layout(e, &after.remove_directories)?;
    if after.pending_outcome.is_some()
        || (matches!(request, Request::Restore)
            && (after.current_registry_sha256 != hash_bytes(&e.registry)
                || !after.files.is_empty()))
    {
        return Err(bad(
            "Live-Vorgang nicht vollständig abgeschlossen; Zustand erneut prüfen",
        ));
    }
    Ok(Receipt {
        action: reviewed.action,
        review_id: reviewed.review_id,
        registry_sha256: after.current_registry_sha256,
        source_files_verified: e.sources.len() - 1,
        pending_recovered: pending,
    })
}

#[cfg(all(test, windows))]
#[path = "live_tests.rs"]
mod tests;
