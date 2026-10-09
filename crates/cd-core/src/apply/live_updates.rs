//! Immutable basis transitions. Archive old owned files on the same volume;
//! never copy an old registry over the updated game.
use super::*;
#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Tree {
    source: String,
    target: String,
    files: Vec<FileHash>,
    directories: BTreeSet<String>,
}
#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub(super) struct Plan {
    version: u32,
    sequence: String,
    before: Admission,
    pub(super) after: Admission,
    trees: Vec<Tree>,
}
#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Complete {
    intent_sha256: String,
}
pub(super) struct State {
    pub active: Admission,
    pub pending: Option<Plan>,
    plans: Vec<Plan>,
}
impl State {
    pub fn archived_files(&self) -> Vec<FileHash> {
        self.plans
            .iter()
            .flat_map(|p| p.trees.iter())
            .flat_map(archived_files)
            .collect()
    }
    pub fn archived_dirs(&self) -> BTreeSet<String> {
        self.plans
            .iter()
            .flat_map(|p| p.trees.iter())
            .flat_map(archived_dirs)
            .collect()
    }
}
fn archived_files(t: &Tree) -> Vec<FileHash> {
    t.files
        .iter()
        .map(|f| FileHash {
            path: format!("{}{}", t.target, &f.path[t.source.len()..]),
            bytes: f.bytes,
            sha256: f.sha256.clone(),
        })
        .collect()
}
fn archived_dirs(t: &Tree) -> BTreeSet<String> {
    let mut dirs: BTreeSet<_> = t
        .directories
        .iter()
        .map(|d| format!("{}{}", t.target, &d[t.source.len()..]))
        .collect();
    let mut parent = Path::new(&t.target).parent();
    while let Some(p) = parent.filter(|p| !p.as_os_str().is_empty()) {
        dirs.insert(p.to_string_lossy().replace('\\', "/"));
        parent = p.parent();
    }
    dirs
}
fn base(game: &Path) -> PathBuf {
    admission_path(game).parent().unwrap().join("updates")
}
fn read<T: serde::de::DeserializeOwned>(policy: &PathPolicy, path: &Path) -> Result<T> {
    let mut bytes = Vec::new();
    fs::File::open(policy.check_cache(path)?)?
        .take(8_388_609)
        .read_to_end(&mut bytes)?;
    if bytes.len() > 8_388_608 {
        return Err(bad("Basiswechsel-Protokoll zu groß"));
    }
    Ok(serde_json::from_slice(&bytes)?)
}
/// Deterministic partial files make interruption during project publication resumable.
fn publish_record(policy: &PathPolicy, path: &Path, bytes: &[u8]) -> Result<()> {
    use std::io::Write;
    let partial = path.with_extension("part");
    let target = policy.check_output(path)?;
    let partial = policy.check_cache(&partial)?;
    fs::create_dir_all(target.parent().unwrap())?;
    // Recheck after creating parents before opening a mutable output.
    let partial = policy.check_cache(&partial)?;
    let mut options = fs::OpenOptions::new();
    options.read(true).write(true).create(true).truncate(false);
    #[cfg(windows)]
    {
        use std::os::windows::fs::OpenOptionsExt;
        options.share_mode(0).custom_flags(0x00200000);
    }
    let mut file = options.open(&partial)?;
    if file.metadata()?.len() > bytes.len() as u64 {
        return Err(bad("Partielles Updateprotokoll zu groß"));
    }
    let mut previous = Vec::new();
    file.read_to_end(&mut previous)?;
    if !bytes.starts_with(&previous) {
        return Err(bad("Partielles Updateprotokoll wurde verändert"));
    }
    file.write_all(&bytes[previous.len()..])?;
    file.sync_all()?;
    drop(file);
    super::super::guard::publish(&partial, &target)?;
    Ok(())
}
fn plan_id(p: &Plan) -> Result<String> {
    Ok(hash_bytes(&serde_json::to_vec(p)?))
}
fn shape(p: &Plan) -> Result<()> {
    if p.version != 1
        || p.before.game != p.after.game
        || p.before == p.after
        || p.sequence.len() != 16
        || !p.sequence.bytes().all(|b| b.is_ascii_digit())
        || p.trees.is_empty()
        || p.trees.len() > 17
    {
        return Err(bad("Ungültiger Basiswechsel"));
    }
    let mut names = BTreeSet::new();
    for t in &p.trees {
        if (t.source != ".workbench"
            && !(t.source.len() == 4 && t.source.bytes().all(|b| b.is_ascii_digit())))
            || !names.insert(t.source.clone())
            || t.target != format!(".workbench-history/{}/{}", p.sequence, t.source)
            || t.files.len() > 16384
            || t.directories.len() > 16384
            || !t.directories.contains(&t.source)
        {
            return Err(bad("Ungültiges Archivziel"));
        }
        let valid = |s: &str| {
            (s == t.source || s.starts_with(&format!("{}/", t.source)))
                && s.split('/').all(|v| {
                    !v.is_empty()
                        && v != "."
                        && v != ".."
                        && !v.contains(['\\', ':'])
                        && !v.ends_with(['.', ' '])
                })
        };
        let mut paths = BTreeSet::new();
        for f in &t.files {
            if !valid(&f.path)
                || !digest(&f.sha256)
                || f.bytes > crimson_format::MAX_FILE_BYTES as u64
                || !paths.insert(f.path.to_lowercase())
            {
                return Err(bad("Ungültige Archivdatei"));
            }
        }
        if t.directories.iter().any(|d| !valid(d)) {
            return Err(bad("Ungültiger Archivordner"));
        }
    }
    if !names.contains(".workbench") {
        return Err(bad("Alte Transaktionshistorie fehlt"));
    }
    Ok(())
}
pub(super) fn state(policy: &PathPolicy, game: &Path) -> Result<State> {
    let root = game.canonicalize()?;
    let mut active = initial(policy, &root)?;
    let directory = policy
        .check_cache(&base(&root).join(".boundary"))?
        .parent()
        .unwrap()
        .to_owned();
    let mut plans = Vec::new();
    let mut pending = None;
    if directory.try_exists()? {
        transaction::plain(&directory, true)?;
        let mut entries = BTreeMap::new();
        for item in fs::read_dir(&directory)? {
            let item = item?;
            let name = item
                .file_name()
                .into_string()
                .map_err(|_| bad("Ungültige Updatehistorie"))?;
            if entries.len() >= 128 || name.len() != 16 || !name.bytes().all(|b| b.is_ascii_digit())
            {
                return Err(bad("Ungültige oder zu große Updatehistorie"));
            }
            transaction::plain(&item.path(), true)?;
            entries.insert(name, item.path());
        }
        let count = entries.len();
        for (i, (sequence, dir)) in entries.into_iter().enumerate() {
            if sequence != format!("{:016}", i + 1) || pending.is_some() {
                return Err(bad("Lücke oder offener Vorgang in der Updatehistorie"));
            }
            // A directory without an intent may be the result of an interrupted
            // atomic publication; only the matching partial intent may remain.
            if !dir.join("intent.json").try_exists()? {
                if i + 1 != count
                    || fs::read_dir(&dir)?
                        .any(|e| e.map_or(true, |e| e.file_name() != "intent.part"))
                {
                    return Err(bad("Unvollständiges Updateprotokoll"));
                }
                continue;
            }
            for file in fs::read_dir(&dir)? {
                if !matches!(
                    file?.file_name().to_str(),
                    Some("intent.json" | "complete.json" | "complete.part")
                ) {
                    return Err(bad("Fremde Datei in der Updatehistorie"));
                }
            }
            let p: Plan = read(policy, &dir.join("intent.json"))?;
            shape(&p)?;
            if p.sequence != sequence || p.before != active {
                return Err(bad("Basiswechselkette stimmt nicht"));
            }
            let expected = evidence_kind(policy, &root, &p.after.baseline_id, false)?;
            if expected.admission != p.after {
                return Err(bad("Gespeicherte neue Basis wurde verändert"));
            }
            if dir.join("complete.json").try_exists()? {
                let complete: Complete = read(policy, &dir.join("complete.json"))?;
                if complete.intent_sha256 != plan_id(&p)? {
                    return Err(bad("Updateabschluss stimmt nicht"));
                }
                active = p.after.clone();
                plans.push(p);
            } else {
                pending = Some(p);
            }
        }
    }
    Ok(State {
        active,
        pending,
        plans,
    })
}
#[derive(Serialize)]
pub struct Review {
    pub review_id: String,
    pub previous_baseline: String,
    pub next_baseline: String,
    pub resume: bool,
    pub game_path: PathBuf,
    pub archive_path: PathBuf,
    pub files: Vec<FileChange>,
    pub source_files: usize,
    pub total_bytes: String,
}
#[derive(Serialize)]
pub struct Receipt {
    pub action: &'static str,
    pub baseline_id: String,
    pub archive_path: PathBuf,
    pub registry_sha256: String,
    pub source_files_verified: usize,
}
fn to_review(p: &Plan, e: &Evidence, resume: bool) -> Result<Review> {
    let files = p
        .trees
        .iter()
        .flat_map(|t| {
            t.files.iter().map(|f| FileChange {
                path: f.path.clone(),
                action: "archive",
                before_sha256: Some(f.sha256.clone()),
                after_sha256: Some(f.sha256.clone()),
            })
        })
        .collect();
    Ok(Review {
        review_id: hash_bytes(&serde_json::to_vec(&(plan_id(p)?, &e.foreign_revision))?),
        previous_baseline: p.before.baseline_id.clone(),
        next_baseline: p.after.baseline_id.clone(),
        resume,
        game_path: p.after.game.clone(),
        archive_path: p
            .after
            .game
            .join(format!(".workbench-history/{}", p.sequence)),
        files,
        source_files: e.sources.len() - 1,
        total_bytes: e
            .sources
            .iter()
            .filter(|f| f.path != "meta/0.papgt")
            .map(|f| f.bytes)
            .sum::<u64>()
            .to_string(),
    })
}
fn registry(e: &Evidence) -> Result<()> {
    if crate::fingerprint::hash_file(&transaction::checked(&e.admission.game, "meta/0.papgt")?)?.1
        != e.admission.registry_sha256
    {
        return Err(bad(
            "Basiswechsel benötigt die bereits wiederhergestellte Original-Registry des neuen Builds. Steam-Dateiprüfung zuerst abschließen.",
        ));
    }
    Ok(())
}
fn collect_plan(old: &Admission, new: &Evidence, sequence: String) -> Result<Plan> {
    registry(new)?;
    let engine = transaction::Engine::inspect(&old.game, &old.registry_sha256)?;
    let ownership = engine.ownership()?;
    let mut roots = BTreeSet::from([".workbench".to_owned()]);
    for path in ownership.files.keys().chain(ownership.directories.iter()) {
        roots.insert(path.split('/').next().unwrap().to_owned());
    }
    let new_groups: BTreeSet<_> = crimson_format::overlay::registry_groups(&new.registry)?
        .into_iter()
        .map(|g| g.name)
        .collect();
    let mut trees = Vec::new();
    for source in roots {
        if new_groups.contains(&source)
            || new
                .sources
                .iter()
                .any(|f| f.path.starts_with(&format!("{source}/")))
        {
            return Err(bad(
                "Alte Overlaygruppe kollidiert mit dem neuen Originalbestand; nichts archiviert",
            ));
        }
        let mut files = Vec::new();
        for (path, expected) in &ownership.files {
            if !path.starts_with(&format!("{source}/")) {
                continue;
            }
            let (bytes, sha256) =
                crate::fingerprint::hash_file(&transaction::checked(&old.game, path)?)?;
            if let Some(expected) = expected
                && (expected.bytes != bytes || expected.sha256 != sha256)
            {
                return Err(bad(
                    "Eigene Overlaydatei wurde fremd verändert; nicht archiviert",
                ));
            }
            files.push(FileHash {
                path: path.clone(),
                bytes,
                sha256,
            });
        }
        let directories = ownership
            .directories
            .iter()
            .filter(|p| **p == source || p.starts_with(&format!("{source}/")))
            .cloned()
            .collect();
        trees.push(Tree {
            target: format!(".workbench-history/{sequence}/{source}"),
            source,
            files,
            directories,
        });
    }
    // Archive groups before moving their private journal.
    trees.sort_by_key(|t| t.source == ".workbench");
    let p = Plan {
        version: 1,
        sequence,
        before: old.clone(),
        after: new.admission.clone(),
        trees,
    };
    shape(&p)?;
    Ok(p)
}
fn tree_check(root: &Path, t: &Tree, archived: bool, contents: bool) -> Result<()> {
    let start = if archived { &t.target } else { &t.source };
    let files: Vec<_> = if archived {
        archived_files(t)
    } else {
        t.files.clone()
    };
    let expected: BTreeMap<_, _> = files.iter().map(|f| (f.path.as_str(), f)).collect();
    let dirs: BTreeSet<_> = t
        .directories
        .iter()
        .map(|d| {
            if archived {
                format!("{}{}", t.target, &d[t.source.len()..])
            } else {
                d.clone()
            }
        })
        .collect();
    let mut seen = BTreeSet::new();
    let mut seen_dirs = BTreeSet::new();
    let mut queue = vec![start.clone()];
    let mut count = 0;
    while let Some(dir) = queue.pop() {
        let absolute = transaction::checked(root, &dir)?;
        transaction::plain(&absolute, true)?;
        seen_dirs.insert(dir.clone());
        for item in fs::read_dir(absolute)? {
            let item = item?;
            count += 1;
            if count > 16384 {
                return Err(bad("Archivbaum zu groß"));
            }
            let name = item
                .file_name()
                .into_string()
                .map_err(|_| bad("Ungültiger Archivpfad"))?;
            let path = format!("{dir}/{name}");
            let meta = fs::symlink_metadata(item.path())?;
            transaction::plain(&item.path(), meta.is_dir())?;
            if meta.is_dir() && dirs.contains(&path) {
                queue.push(path);
            } else if let Some(f) = expected.get(path.as_str()).filter(|_| meta.is_file()) {
                if meta.len() != f.bytes
                    || (contents && crate::fingerprint::hash_file(&item.path())?.1 != f.sha256)
                {
                    return Err(bad(format!("Archivdatei verändert: {path}")));
                }
                seen.insert(path);
            } else {
                return Err(bad(format!("Fremder Inhalt im Archivbaum: {path}")));
            }
        }
    }
    if seen.len() != expected.len() || seen_dirs != dirs {
        return Err(bad("Archivbaum unvollständig"));
    }
    Ok(())
}
fn new_history(p: &Plan, source: &Path) -> Result<()> {
    transaction::plain(source, true)?;
    let registry = fs::read(transaction::checked(&p.after.game, "meta/0.papgt")?)?;
    if hash_bytes(&registry) != p.after.registry_sha256 {
        return Err(bad("Neue Registry verändert"));
    }
    for item in fs::read_dir(source)? {
        let item = item?;
        let path = item.path();
        match item.file_name().to_str() {
            Some("transactions") => {
                transaction::plain(&path, true)?;
                if fs::read_dir(path)?.next().is_some() {
                    return Err(bad("Neue Historie ist nicht leer"));
                }
            }
            Some("lock") => {
                transaction::plain(&path, false)?;
                if fs::metadata(path)?.len() != 0 {
                    return Err(bad("Fremder Lockinhalt"));
                }
            }
            Some("baseline.papgt" | "baseline.part") => {
                transaction::plain(&path, false)?;
                if fs::metadata(&path)?.len() > registry.len() as u64 {
                    return Err(bad("Neue Sicherung zu groß"));
                }
                let bytes = fs::read(path)?;
                if !registry.starts_with(&bytes)
                    || (item.file_name() == "baseline.papgt" && bytes != registry)
                {
                    return Err(bad("Neue Sicherung verändert"));
                }
            }
            _ => return Err(bad("Fremder Inhalt in neuer Historie")),
        }
    }
    Ok(())
}
fn locations(p: &Plan, contents: bool) -> Result<()> {
    for t in &p.trees {
        let source = transaction::checked(&p.after.game, &t.source)?;
        let target = transaction::checked(&p.after.game, &t.target)?;
        if target.try_exists()? {
            tree_check(&p.after.game, t, true, contents)?;
            if source.try_exists()? {
                if t.source != ".workbench" {
                    return Err(bad("Quelle und Archivziel gleichzeitig vorhanden"));
                }
                new_history(p, &source)?;
            }
        } else {
            tree_check(&p.after.game, t, false, contents)?;
        }
    }
    Ok(())
}
fn prepared(policy: &PathPolicy, game: &Path, id: &str) -> Result<(State, Evidence, Plan)> {
    let state = state(policy, game)?;
    let mut e = evidence(policy, game, id)?;
    e.archived_files = state.archived_files();
    e.archived_dirs = state.archived_dirs();
    registry(&e)?;
    let p = if let Some(p) = &state.pending {
        if p.after != e.admission {
            return Err(bad(
                "Offenen Basiswechsel mit der vorgesehenen Basis fortsetzen",
            ));
        }
        locations(p, true)?;
        p.clone()
    } else {
        if state.active == e.admission {
            return Err(bad("Diese Basis ist bereits aktiv"));
        }
        let current = crate::installation::baseline::inspect(policy, game, id)?;
        if current.current_state != "metadata_match" {
            return Err(bad(
                "Neuer Inhaltsbericht passt nicht zum aktuellen Originalbestand",
            ));
        }
        collect_plan(&state.active, &e, format!("{:016}", state.plans.len() + 1))?
    };
    for previous in &state.plans {
        for t in &previous.trees {
            tree_check(game, t, true, false)?;
        }
    }
    Ok((state, e, p))
}
pub fn preview(policy: &PathPolicy, game: &Path, id: &str) -> Result<Review> {
    let (state, e, p) = prepared(policy, game, id)?;
    to_review(&p, &e, state.pending.is_some())
}
pub fn execute(
    policy: &PathPolicy,
    game: &Path,
    id: &str,
    review_id: &str,
    steam_verified_before_audit: bool,
    cancel: &AtomicBool,
) -> Result<Receipt> {
    if !steam_verified_before_audit {
        return Err(bad(
            "Steam-Dateiprüfung vor dem neuen Inhaltsbericht muss bestätigt sein",
        ));
    }
    stopped(cancel)?;
    let (state, e, p) = prepared(policy, game, id)?;
    if !digest(review_id) || to_review(&p, &e, state.pending.is_some())?.review_id != review_id {
        return Err(bad("Basiswechsel-Vorschau veraltet; nichts geändert"));
    }
    migrate(
        policy,
        &state,
        &e,
        &p,
        || {
            stopped(cancel)?;
            foreign::unchanged(policy, &e)
        },
        None,
    )
}
fn migrate(
    policy: &PathPolicy,
    state: &State,
    e: &Evidence,
    p: &Plan,
    mut check: impl FnMut() -> Result<()>,
    fault: Option<usize>,
) -> Result<Receipt> {
    shape(p)?;
    registry(e)?;
    check()?;
    let path = base(&p.after.game).join(&p.sequence);
    if state.pending.is_none() {
        publish_record(
            policy,
            &path.join("intent.json"),
            &serde_json::to_vec_pretty(p)?,
        )?;
    }
    let mut step = 0;
    let mut point = || -> Result<()> {
        let n = step;
        step += 1;
        if fault == Some(n) {
            Err(bad(format!("injected migration interruption {n}")))
        } else {
            Ok(())
        }
    };
    point()?;
    protected::with_sources(&e.admission.game, &e.protection()?, &mut check, |guard| {
        locations(p, true)?;
        guard()?;
        for dir in [
            ".workbench-history".into(),
            format!(".workbench-history/{}", p.sequence),
        ] {
            let absolute = transaction::checked(&p.after.game, &dir)?;
            if !absolute.try_exists()? {
                fs::create_dir(&absolute)?;
            }
            transaction::plain(&absolute, true)?;
        }
        point()?;
        for t in &p.trees {
            guard()?;
            let target = transaction::checked(&p.after.game, &t.target)?;
            if !target.try_exists()? {
                tree_check(&p.after.game, t, false, true)?;
                super::super::guard::publish(
                    &transaction::checked(&p.after.game, &t.source)?,
                    &target,
                )?;
            }
            tree_check(&p.after.game, t, true, true)?;
            point()?;
        }
        // Windows cannot rename a directory with open child handles. Verify each
        // moved tree, then pin all archived bytes before accepting the new basis.
        let archive_sources: Vec<_> = p.trees.iter().flat_map(archived_files).collect();
        let _held_archive = protected::hold_archived(&p.after.game, &archive_sources, guard)?;
        guard()?;
        let engine = transaction::Engine::new(&p.after.game, &p.after.registry_sha256)?;
        if !engine.restore_review()?.files.is_empty() {
            return Err(bad("Neue Registry ist nicht original"));
        }
        drop(engine);
        point()?;
        let fresh = evidence_for_finish(e, state, p);
        layout(&fresh, &[])?;
        // Old state is archived verbatim. No old registry is written back.
        guard()?;
        point()?;
        publish_record(
            policy,
            &path.join("complete.json"),
            &serde_json::to_vec_pretty(&Complete {
                intent_sha256: plan_id(p)?,
            })?,
        )?;
        Ok(Receipt {
            action: "basis_update",
            baseline_id: p.after.baseline_id.clone(),
            archive_path: p
                .after
                .game
                .join(format!(".workbench-history/{}", p.sequence)),
            registry_sha256: p.after.registry_sha256.clone(),
            source_files_verified: e.sources.len() - 1,
        })
    })
}
fn evidence_for_finish(e: &Evidence, state: &State, p: &Plan) -> Evidence {
    let mut archived_files = state.archived_files();
    archived_files.extend(p.trees.iter().flat_map(archived_files_for_tree));
    let mut archived_dirs = state.archived_dirs();
    archived_dirs.extend(p.trees.iter().flat_map(archived_dirs_for_tree));
    Evidence {
        admission: e.admission.clone(),
        sources: e.sources.clone(),
        registry: e.registry.clone(),
        audited_at: e.audited_at,
        archived_files,
        archived_dirs,
        foreign_files: e.foreign_files.clone(),
        foreign_dirs: e.foreign_dirs.clone(),
        foreign_revision: e.foreign_revision.clone(),
    }
}
fn archived_files_for_tree(t: &Tree) -> Vec<FileHash> {
    archived_files(t)
}
fn archived_dirs_for_tree(t: &Tree) -> BTreeSet<String> {
    archived_dirs(t)
}

/// Explicitly attributed metadata only; never hides depot originals, content
/// mismatches, foreign children, malformed journals, or an incomplete migration.
pub(crate) fn claims(policy: &PathPolicy, game: &Path) -> Result<Option<(BTreeSet<String>, bool)>> {
    let root = game.canonicalize()?;
    // Absence needs no permission to create a private cache. If a record exists,
    // all policy/link/integrity checks still run before reading or trusting it.
    match fs::symlink_metadata(policy.output_root().join(admission_path(&root))) {
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => return Ok(None),
        Err(e) => return Err(e.into()),
        Ok(_) => {}
    }
    policy.check_cache(&admission_path(&root))?;
    let state = state(policy, &root)?;
    if state.pending.is_some() {
        return Err(bad("Basiswechsel offen; zuerst fortsetzen"));
    }
    let engine = transaction::Engine::inspect(&root, &state.active.registry_sha256)?;
    let owned = engine.ownership()?;
    let mut paths = owned.directories;
    paths.extend(owned.files.into_keys());
    for p in &state.plans {
        for t in &p.trees {
            tree_check(&root, t, true, false)?;
            paths.extend(archived_dirs(t));
            paths.extend(archived_files(t).into_iter().map(|f| f.path));
        }
    }
    Ok(Some((
        paths,
        owned.registry_matches_history
            && crate::fingerprint::hash_file(&root.join("meta/0.papgt"))?.1
                != state.active.registry_sha256,
    )))
}

#[cfg(all(test, windows))]
#[path = "live_updates_tests.rs"]
pub(super) mod tests;
