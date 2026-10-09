//! Persist an observed audit and a verified registry copy inside the project.
//! This is evidence for a later Apply admission, never a live-write capability.
use super::{Inventory, audit, bad, relative};
use crate::{Result, fingerprint::hash_bytes, paths::PathPolicy, write_generated};
use serde::{Deserialize, Serialize};
use std::{
    collections::BTreeSet,
    fs,
    io::Read,
    path::{Path, PathBuf},
    sync::atomic::{AtomicU64, Ordering},
};

const MAX_REPORT: usize = 8 * 1024 * 1024;
const REGISTRY: &str = "meta/0.papgt";
#[derive(Serialize)]
pub struct Catalog {
    pub reports: Vec<String>,
    pub snapshots: Vec<String>,
    pub truncated: bool,
}
#[derive(Serialize)]
pub struct Preview {
    pub report_name: String,
    pub review_id: String,
    pub game_path: PathBuf,
    pub build_id: String,
    pub audited_at: u64,
    pub file_count: usize,
    pub archive_count: usize,
    pub total_bytes: String,
    pub registry_bytes: u64,
    pub registry_sha256: String,
    pub executables: Vec<String>,
    pub read_schema_matches: bool,
    pub certified_vanilla: bool,
    pub can_apply: bool,
}
#[derive(Serialize)]
pub struct Saved {
    pub id: String,
    pub directory: PathBuf,
    pub created_at: u64,
    pub preview: Preview,
    pub current_state: &'static str,
    pub issues: Vec<String>,
}
#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Manifest {
    version: u32,
    kind: String,
    id: String,
    created_at: u64,
    report_name: String,
    report_sha256: String,
    registry_sha256: String,
    registry_bytes: u64,
    inventory_sha256: String,
}
struct Prepared {
    manifest: Manifest,
    report: audit::Report,
    raw: Vec<u8>,
    registry: Vec<u8>,
}
fn now() -> u64 {
    std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs()
}
fn valid_digest(s: &str, len: usize) -> bool {
    s.len() == len
        && s.bytes()
            .all(|b| b.is_ascii_digit() || (b'a'..=b'f').contains(&b))
}
fn name(s: &str) -> Result<()> {
    if s.is_empty()
        || s.len() > 160
        || !s
            .bytes()
            .all(|b| b.is_ascii_alphanumeric() || b"-_.".contains(&b))
        || s == "."
        || s == ".."
    {
        return Err(bad("Ungültiger Projektdateiname").into());
    }
    Ok(())
}
fn report_name(s: &str) -> Result<()> {
    name(s)?;
    if !s.starts_with("installation-audit-") || !s.ends_with(".json") {
        return Err(bad("Nur exportierte Inhaltsprüfberichte werden akzeptiert").into());
    }
    Ok(())
}
fn read(policy: &PathPolicy, path: &Path, limit: usize) -> Result<Vec<u8>> {
    let path = policy.check_cache(path)?;
    let mut bytes = Vec::new();
    fs::File::open(path)?
        .take(limit as u64 + 1)
        .read_to_end(&mut bytes)?;
    if bytes.len() > limit {
        return Err(bad("Projektdatei überschreitet das Größenlimit").into());
    }
    Ok(bytes)
}
fn validate(report: &audit::Report) -> Result<()> {
    if report.version != 1
        || report.kind != "cache-content-audit"
        || !report.all_files_match_cache
        || !report.metadata_stable
        || report.certified_vanilla
        || report.can_apply
        || report
            .foreign_approval
            .as_ref()
            .is_some_and(|v| !valid_digest(v, 64))
        || report.files.is_empty()
        || report.files.len() > super::MAX_NODES
        || report.manifest_sha256.is_empty()
        || report.manifest_sha256.len() > 32
        || report.started_at == 0
        || report.finished_at < report.started_at
        || report.finished_at > now().saturating_add(300)
        || report.build_id.is_empty()
        || !report.build_id.bytes().all(|b| b.is_ascii_digit())
    {
        return Err(bad(
            "Kein vollständiger, konsistenter Inhaltsprüfbericht ohne Schreibfreigabe",
        )
        .into());
    }
    let mut names = BTreeSet::new();
    let mut total = 0u64;
    for file in &report.files {
        if relative(&file.path)? != file.path
            || !names.insert(file.path.to_lowercase())
            || !file.matches_cache
            || file.sha1 != file.expected_sha1
            || !valid_digest(&file.sha1, 40)
            || !valid_digest(&file.sha256, 64)
        {
            return Err(
                bad("Ungültiger oder widersprüchlicher Dateieintrag im Prüfbericht").into(),
            );
        }
        let bytes = file
            .bytes
            .parse::<u64>()
            .map_err(|_| bad("Ungültige Dateigröße im Prüfbericht"))?;
        if bytes.to_string() != file.bytes {
            return Err(bad("Nicht kanonische Dateigröße").into());
        }
        total = total
            .checked_add(bytes)
            .ok_or_else(|| bad("Gesamtgröße läuft über"))?;
    }
    let manifests: BTreeSet<_> = report.manifest_sha256.iter().collect();
    if manifests.len() != report.manifest_sha256.len()
        || manifests.iter().any(|s| !valid_digest(s, 64))
        || !report.files.iter().any(|f| f.path == REGISTRY)
    {
        return Err(bad("Depotidentität oder Registry im Prüfbericht fehlt").into());
    }
    Ok(())
}
fn compare(report: &audit::Report, inventory: &Inventory) -> Result<()> {
    audit::validate_inventory(inventory)?;
    if report.game_path != inventory.game_path
        || Some(&report.build_id) != inventory.build_id.as_ref()
        || report.files.len() != inventory.files.len()
    {
        return Err(bad(
            "Prüfbericht gehört nicht zur aktuellen Installation oder zum aktuellen Build",
        )
        .into());
    }
    let mut manifest_hashes: Vec<_> = inventory
        .depots
        .iter()
        .map(|d| d.manifest_sha256.as_str())
        .collect();
    let mut expected: Vec<_> = report.manifest_sha256.iter().map(String::as_str).collect();
    manifest_hashes.sort();
    expected.sort();
    if manifest_hashes != expected {
        return Err(bad("Steam-Depotlisten seit dem Prüfbericht geändert").into());
    }
    let by_path: std::collections::BTreeMap<_, _> =
        report.files.iter().map(|f| (f.path.as_str(), f)).collect();
    for file in &inventory.files {
        let previous = by_path
            .get(file.path.as_str())
            .ok_or_else(|| bad("Dateibestand seit dem Prüfbericht geändert"))?;
        if file.expected_bytes.as_ref() != Some(&previous.bytes)
            || file.actual_bytes.as_ref() != Some(&previous.bytes)
            || file.expected_sha1.as_ref() != Some(&previous.expected_sha1)
        {
            return Err(bad(format!("Datei oder Depotwert geändert: {}", file.path)).into());
        }
    }
    if inventory.registry_sha256.as_ref() != Some(&by_path[REGISTRY].sha256) {
        return Err(bad("Registry seit dem Prüfbericht geändert").into());
    }
    Ok(())
}
fn matches_read_schema(report: &audit::Report) -> bool {
    crate::fingerprint::known_builds().iter().any(|known| {
        report.build_id == known.steam_buildid
            && known.files.iter().all(|k| {
                report.files.iter().any(|f| {
                    f.path == k.path && f.sha256 == k.sha256 && f.bytes == k.bytes.to_string()
                })
            })
    })
}
fn preview_of(manifest: &Manifest, report: &audit::Report) -> Result<Preview> {
    let matches = matches_read_schema(report);
    let total: u64 = report
        .files
        .iter()
        .map(|f| f.bytes.parse::<u64>().unwrap())
        .sum();
    Ok(Preview {
        report_name: manifest.report_name.clone(),
        review_id: hash_bytes(&serde_json::to_vec(&(
            &manifest.report_sha256,
            &manifest.inventory_sha256,
            &manifest.registry_sha256,
            manifest.registry_bytes,
        ))?),
        game_path: report.game_path.clone(),
        build_id: report.build_id.clone(),
        audited_at: report.finished_at,
        file_count: report.files.len(),
        archive_count: report
            .files
            .iter()
            .filter(|f| f.path.ends_with(".paz"))
            .count(),
        total_bytes: total.to_string(),
        registry_bytes: manifest.registry_bytes,
        registry_sha256: manifest.registry_sha256.clone(),
        executables: report
            .files
            .iter()
            .filter(|f| f.path.to_ascii_lowercase().ends_with(".exe"))
            .map(|f| f.path.clone())
            .collect(),
        read_schema_matches: matches,
        certified_vanilla: false,
        can_apply: false,
    })
}
fn prepare(policy: &PathPolicy, game: &Path, filename: &str) -> Result<Prepared> {
    report_name(filename)?;
    let raw = read(policy, &Path::new("exports").join(filename), MAX_REPORT)?;
    let report: audit::Report = serde_json::from_slice(&raw)?;
    validate(&report)?;
    let before = super::inspect_managed(policy, game)?;
    compare(&report, &before)?;
    let mut source = audit::open_source(&before.game_path, REGISTRY)?;
    let stamp = audit::stamp(&source)?;
    let mut registry = Vec::new();
    (&mut source).take(1_048_577).read_to_end(&mut registry)?;
    if registry.len() > 1_048_576 || stamp != audit::stamp(&source)? {
        return Err(bad("Registry während der Sicherungsvorbereitung geändert").into());
    }
    let registry_sha256 = hash_bytes(&registry);
    let wanted = report.files.iter().find(|f| f.path == REGISTRY).unwrap();
    if registry_sha256 != wanted.sha256 || registry.len().to_string() != wanted.bytes {
        return Err(bad("Registry-Inhalt passt nicht zum Prüfbericht").into());
    }
    let after = super::inspect_managed(policy, game)?;
    compare(&report, &after)?;
    let identity = audit::identity(&before)?;
    if identity != audit::identity(&after)? {
        return Err(bad("Installation während der Vorbereitung geändert").into());
    }
    let manifest = Manifest {
        version: 1,
        kind: "observed-installation-baseline".into(),
        id: String::new(),
        created_at: 0,
        report_name: filename.into(),
        report_sha256: hash_bytes(&raw),
        registry_sha256,
        registry_bytes: registry.len() as u64,
        inventory_sha256: hash_bytes(&identity),
    };
    Ok(Prepared {
        manifest,
        report,
        raw,
        registry,
    })
}
pub fn preview(policy: &PathPolicy, game: &Path, filename: &str) -> Result<Preview> {
    let p = prepare(policy, game, filename)?;
    preview_of(&p.manifest, &p.report)
}
pub fn capture(policy: &PathPolicy, game: &Path, filename: &str, review_id: &str) -> Result<Saved> {
    capture_with(policy, game, filename, review_id, None)
}
fn capture_with(
    policy: &PathPolicy,
    game: &Path,
    filename: &str,
    review_id: &str,
    fault: Option<usize>,
) -> Result<Saved> {
    let mut p = prepare(policy, game, filename)?;
    if !valid_digest(review_id, 64) || preview_of(&p.manifest, &p.report)?.review_id != review_id {
        return Err(
            bad("Prüfbericht oder Installation seit der Vorschau geändert; erneut prüfen").into(),
        );
    }
    static NEXT: AtomicU64 = AtomicU64::new(0);
    p.manifest.created_at = now();
    p.manifest.id = format!(
        "{}-{}-{}",
        now(),
        std::process::id(),
        NEXT.fetch_add(1, Ordering::Relaxed)
    );
    let base = Path::new(".local/baselines").join(&p.manifest.id);
    let point = |n| -> Result<()> {
        if fault == Some(n) {
            Err(bad("injected baseline interruption").into())
        } else {
            Ok(())
        }
    };
    write_generated(policy, &base.join("audit.json"), &p.raw)?;
    point(0)?;
    write_generated(policy, &base.join("registry.papgt"), &p.registry)?;
    point(1)?;
    if read(policy, &base.join("audit.json"), MAX_REPORT)? != p.raw
        || read(policy, &base.join("registry.papgt"), 1_048_576)? != p.registry
    {
        return Err(bad("Gespeicherte Ausgangsbasis konnte nicht verifiziert werden").into());
    }
    let current = super::inspect_managed(policy, game)?;
    compare(&p.report, &current)?;
    if hash_bytes(&audit::identity(&current)?) != p.manifest.inventory_sha256 {
        return Err(bad("Installation vor Abschluss geändert; unvollständige Kopie bleibt zur Diagnose erhalten").into());
    }
    point(2)?;
    // The manifest is the last, atomic publication point. Partial copies cannot load.
    write_generated(
        policy,
        &base.join("snapshot.json"),
        &serde_json::to_vec_pretty(&p.manifest)?,
    )?;
    inspect(policy, game, &p.manifest.id)
}
pub fn inspect(policy: &PathPolicy, game: &Path, id: &str) -> Result<Saved> {
    let (m, report, _) = load(policy, id)?;
    let base = Path::new(".local/baselines").join(id);
    let (state, issues) = match super::inspect_managed(policy, game) {
        Ok(current) => match compare(&report, &current).and_then(|_| {
            if hash_bytes(&audit::identity(&current)?) != m.inventory_sha256 {
                Err(
                    bad("Aktuelle Installationsmetadaten weichen vom gespeicherten Stand ab")
                        .into(),
                )
            } else {
                Ok(())
            }
        }) {
            Ok(()) => ("metadata_match", Vec::new()),
            Err(e) => ("changed", vec![e.to_string()]),
        },
        Err(e) => ("unavailable", vec![e.to_string()]),
    };
    let directory = policy
        .check_cache(&base.join("snapshot.json"))?
        .parent()
        .unwrap()
        .to_owned();
    Ok(Saved {
        id: id.into(),
        directory,
        created_at: m.created_at,
        preview: preview_of(&m, &report)?,
        current_state: state,
        issues,
    })
}
fn load(policy: &PathPolicy, id: &str) -> Result<(Manifest, audit::Report, Vec<u8>)> {
    name(id)?;
    let base = Path::new(".local/baselines").join(id);
    let m: Manifest = serde_json::from_slice(&read(policy, &base.join("snapshot.json"), 16_384)?)?;
    if m.version != 1
        || m.kind != "observed-installation-baseline"
        || m.id != id
        || m.created_at == 0
        || !valid_digest(&m.report_sha256, 64)
        || !valid_digest(&m.registry_sha256, 64)
        || !valid_digest(&m.inventory_sha256, 64)
    {
        return Err(bad("Ungültiges Ausgangsbasis-Manifest").into());
    }
    report_name(&m.report_name)?;
    let raw = read(policy, &base.join("audit.json"), MAX_REPORT)?;
    let registry = read(policy, &base.join("registry.papgt"), 1_048_576)?;
    if hash_bytes(&raw) != m.report_sha256
        || hash_bytes(&registry) != m.registry_sha256
        || registry.len() as u64 != m.registry_bytes
    {
        return Err(
            bad("Gespeicherter Prüfbericht oder Registry-Sicherung wurde verändert").into(),
        );
    }
    let report: audit::Report = serde_json::from_slice(&raw)?;
    validate(&report)?;
    let wanted = report.files.iter().find(|f| f.path == REGISTRY).unwrap();
    if wanted.sha256 != m.registry_sha256 || wanted.bytes != m.registry_bytes.to_string() {
        return Err(bad("Registry-Sicherung gehört nicht zum gespeicherten Prüfbericht").into());
    }
    Ok((m, report, registry))
}
/// Integrity-checked evidence for B0. This does not certify origin or current
/// contents: B0 must require explicit provenance and rehash held source handles.
pub(crate) fn apply_evidence(
    policy: &PathPolicy,
    game: &Path,
    id: &str,
) -> Result<(audit::Report, Vec<u8>, String)> {
    let (report, registry, report_sha256) = historical_evidence(policy, game, id)?;
    if !matches_read_schema(&report) {
        return Err(bad("Ausgangsstand besitzt kein bekanntes Leseschema").into());
    }
    Ok((report, registry, report_sha256))
}
pub(crate) fn historical_evidence(
    policy: &PathPolicy,
    game: &Path,
    id: &str,
) -> Result<(audit::Report, Vec<u8>, String)> {
    let (manifest, report, registry) = load(policy, id)?;
    if report.game_path != game.canonicalize()? {
        return Err(bad(
            "Ausgangsstand gehört nicht zur Installation oder zu einem bekannten Leseschema",
        )
        .into());
    }
    Ok((report, registry, manifest.report_sha256))
}
pub fn catalog(policy: &PathPolicy) -> Result<Catalog> {
    fn names(policy: &PathPolicy, relative: &str, reports: bool) -> Result<(Vec<String>, bool)> {
        let checked = policy.check_cache(&Path::new(relative).join(".inventory-boundary"))?;
        let base = checked.parent().unwrap();
        if !base.try_exists()? {
            return Ok((Vec::new(), false));
        }
        if super::linked(&fs::symlink_metadata(base)?) {
            return Err(bad("Verknüpftes Ausgangsbasis-Verzeichnis").into());
        }
        let mut names = Vec::new();
        for (count, item) in fs::read_dir(base)?.enumerate() {
            if count >= 2000 {
                return Err(
                    bad("Mehr als 2.000 Einträge; Ausgangsbasis-Liste nicht vollständig").into(),
                );
            }
            let item = item?;
            let Some(value) = item.file_name().to_str().map(str::to_owned) else {
                continue;
            };
            let meta = fs::symlink_metadata(item.path())?;
            if super::linked(&meta) {
                continue;
            }
            if (reports && meta.is_file() && report_name(&value).is_ok())
                || (!reports && meta.is_dir() && name(&value).is_ok())
            {
                names.push(value);
            }
        }
        names.sort_by(|a, b| b.cmp(a));
        let truncated = names.len() > 200;
        names.truncate(200);
        Ok((names, truncated))
    }
    let (reports, a) = names(policy, "exports", true)?;
    let (snapshots, b) = names(policy, ".local/baselines", false)?;
    Ok(Catalog {
        reports,
        snapshots,
        truncated: a || b,
    })
}

#[cfg(test)]
#[path = "baseline_tests.rs"]
mod tests;
