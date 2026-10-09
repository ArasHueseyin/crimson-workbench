//! Bounded read-only installation inventory and audits. Baseline persistence
//! writes only into the project; it grants no Apply capability or vanilla trust.
pub mod audit;
pub mod baseline;
mod depot;
#[cfg(test)]
mod tests;
use crate::{
    Result,
    discovery::{self, Vdf},
    fingerprint::hash_bytes,
};
use serde::Serialize;
use std::{
    collections::{BTreeMap, BTreeSet},
    fs,
    io::{self, Read},
    path::{Path, PathBuf},
};

const MAX_NODES: usize = 16_384;
#[derive(Serialize)]
pub struct Inventory {
    pub version: u32,
    pub mode: &'static str,
    pub game_path: PathBuf,
    pub observed_at: u64,
    pub build_id: Option<String>,
    pub game_running: Option<bool>,
    pub directory_scan_complete: bool,
    pub depot_comparison_available: bool,
    pub content_verified: bool,
    pub certified_vanilla: bool,
    pub can_apply: bool,
    pub expected_files: usize,
    pub actual_files: usize,
    pub expected_bytes: String,
    pub registry_sha256: Option<String>,
    pub registry_matches_observed_build: Option<bool>,
    pub depots: Vec<DepotInfo>,
    pub executables: Vec<String>,
    pub groups: Vec<GroupInfo>,
    pub files: Vec<FileInfo>,
    pub managed_files: Vec<FileInfo>,
    pub managed_registry: bool,
    pub foreign_files: Vec<FileInfo>,
    pub foreign_approval: Option<String>,
    pub issues: Vec<Issue>,
    pub limitations: Vec<String>,
}
#[derive(Serialize)]
pub struct DepotInfo {
    pub id: u32,
    pub manifest_id: String,
    pub manifest_sha256: String,
    pub files: usize,
    pub bytes: String,
    pub signature_bytes: usize,
    pub authenticated: bool,
}
#[derive(Serialize)]
pub struct GroupInfo {
    pub name: String,
    pub optional: bool,
    pub installed: bool,
    pub in_depots: bool,
}
#[derive(Serialize)]
pub struct FileInfo {
    pub path: String,
    pub state: &'static str,
    pub expected_bytes: Option<String>,
    pub actual_bytes: Option<String>,
    pub expected_sha1: Option<String>,
}
#[derive(Serialize)]
pub struct Issue {
    pub code: &'static str,
    pub path: Option<String>,
    pub message: String,
}
impl Inventory {
    fn issue(&mut self, code: &'static str, path: Option<String>, message: impl Into<String>) {
        self.issues.push(Issue {
            code,
            path,
            message: message.into(),
        });
    }
}
/// Metadata-only: never opens PAZ/EXE/DLL/save contents, never starts Steam
/// verification and never locks a game file against writing or execution.
pub fn inspect(game: &Path) -> Result<Inventory> {
    inspect_with(
        game,
        &discovery::DiscoveryOptions::from_environment().steam_roots,
    )
}
pub fn inspect_managed(policy: &crate::paths::PathPolicy, game: &Path) -> Result<Inventory> {
    classify(policy, inspect(game)?)
}
pub(crate) fn inspect_owned(policy: &crate::paths::PathPolicy, game: &Path) -> Result<Inventory> {
    classify_owned(policy, inspect(game)?)
}
pub(super) fn classify(policy: &crate::paths::PathPolicy, report: Inventory) -> Result<Inventory> {
    let mut report = classify_owned(policy, report)?;
    if let Err(e) = crate::apply::live::foreign::classify(policy, &mut report) {
        report.issue("foreign_consent_unavailable", None, e.to_string());
    }
    Ok(report)
}
fn classify_owned(policy: &crate::paths::PathPolicy, mut report: Inventory) -> Result<Inventory> {
    match crate::apply::live::updates::claims(policy, &report.game_path) {
        Ok(Some((paths, active_registry))) => {
            let mut originals = Vec::new();
            for mut file in report.files {
                if paths.contains(&file.path) && file.expected_bytes.is_none() {
                    file.state = "workbench_owned";
                    report.managed_files.push(file);
                } else {
                    if active_registry && file.path == "meta/0.papgt" {
                        file.state = "workbench_registry";
                        report.managed_registry = true;
                    }
                    originals.push(file);
                }
            }
            report.files = originals;
            report.issues.retain(|i| {
                !i.path.as_ref().is_some_and(|p| {
                    paths.contains(p)
                        && matches!(i.code, "unregistered_group" | "group_outside_depots")
                        || report.managed_registry
                            && p == "meta/0.papgt"
                            && i.code == "size_mismatch"
                })
            });
            report.limitations.push("Workbench-Dateien werden anhand der eigenen Zulassung und Historie separat ausgewiesen. Dies ist eine Metadatenzuordnung, kein erneuter Inhaltsnachweis der Overlays. Für einen neuen Vanilla-Inhaltsbericht muss die Registry im Originalzustand sein.".into());
        }
        Ok(None) => {}
        Err(e) => report.issue("workbench_ownership_unavailable", None, e.to_string()),
    }
    Ok(report)
}
fn inspect_with(game: &Path, steam_roots: &[PathBuf]) -> Result<Inventory> {
    let root = game.canonicalize()?;
    if !root.is_dir() {
        return Err(bad("installation root is not a directory").into());
    }
    let status = crate::apply::status(&root);
    let mut report = Inventory {
        version:1, mode:"metadata_inventory", game_path:root.clone(),
        observed_at:std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).unwrap_or_default().as_secs(),
        build_id:None, game_running:status.game_running, directory_scan_complete:true,
        depot_comparison_available:false, content_verified:false, certified_vanilla:false, can_apply:false,
        expected_files:0, actual_files:0, expected_bytes:"0".into(), registry_sha256:None,
        registry_matches_observed_build:None, depots:Vec::new(), executables:Vec::new(),
        groups:Vec::new(), files:Vec::new(), managed_files:Vec::new(), managed_registry:false, foreign_files:Vec::new(),foreign_approval:None, issues:Vec::new(),
        limitations:vec![
            "Dateinamen und Größen sind geprüft; Dateiinhalte der Archive und Startprogramme wurden nicht gehasht.".into(),
            "Lokale Steam-Depotlisten sind nicht unabhängig authentifiziert. Auch ein passender Cache bestätigt kein Vanilla.".into(),
            "Momentaufnahme ohne Spielsperren. Gleich große Änderungen und Änderungen während oder nach der Prüfung können unbemerkt bleiben.".into(),
            "EXE-Liste umfasst Depot- und Verzeichnisfunde innerhalb der Installation; externe Startpfade und der tatsächliche Ladevorgang sind nicht verifiziert.".into(),
            "Diese Dateiliste erteilt keine Live-Freigabe. Live-B0 prüft Herkunftsbestätigung, Basis und Quellen separat. Steam-Dateiprüfung nur nach dem Spielen manuell ausführen.".into(),
        ],
    };
    let mut disk = BTreeMap::new();
    if let Err(e) = walk(&root, Path::new(""), &mut disk) {
        report.directory_scan_complete = false;
        report.issue("scan_incomplete", None, e.to_string());
    }
    let expected = match steam_inventory(&root, steam_roots) {
        Ok((build, depots, entries)) => {
            report.build_id = Some(build);
            report.depots = depots;
            report.depot_comparison_available = true;
            entries
        }
        Err(e) => {
            report.issue("steam_metadata_unavailable", None, e.to_string());
            BTreeMap::new()
        }
    };
    report.expected_files = expected.values().filter(|e| !e.directory).count();
    report.expected_bytes = expected
        .values()
        .filter(|e| !e.directory)
        .map(|e| e.bytes as u128)
        .sum::<u128>()
        .to_string();
    report.actual_files = disk.values().filter(|e| e.kind == Kind::File).count();
    let paths: BTreeSet<_> = expected.keys().chain(disk.keys()).cloned().collect();
    for key in paths {
        let want = expected.get(&key);
        let actual = disk.get(&key);
        let path = actual
            .map(|d| d.path.clone())
            .unwrap_or_else(|| want.unwrap().path.clone());
        if key.ends_with(".exe") {
            report.executables.push(path.clone());
        }
        let state = match (want, actual) {
            (_, Some(d)) if matches!(d.kind, Kind::Link | Kind::Other) => "unsupported_entry",
            (Some(_), None) => "missing",
            (None, Some(_)) if !report.depot_comparison_available => "uncompared",
            (None, Some(_)) => "additional",
            (Some(w), Some(d)) if w.directory != (d.kind == Kind::Directory) => "type_mismatch",
            (Some(w), Some(_)) if w.directory => "directory_matches",
            (Some(w), Some(d)) if w.bytes != d.bytes => "size_mismatch",
            (Some(_), Some(_)) => "size_matches",
            _ => unreachable!(),
        };
        if matches!(
            state,
            "missing" | "type_mismatch" | "size_mismatch" | "unsupported_entry"
        ) {
            report.issue(
                state,
                Some(path.clone()),
                match state {
                    "missing" => {
                        "In den Depotlisten vorhanden, auf dem Datenträger nicht gefunden."
                    }
                    "type_mismatch" => {
                        "Datei und Verzeichnis stimmen nicht mit der Depotliste überein."
                    }
                    "size_mismatch" => "Dateigröße weicht von der Depotliste ab.",
                    _ => "Verknüpfung oder besonderer Dateityp; nicht verfolgt und nicht geprüft.",
                },
            );
        }
        // Matching implicit directories add noise; keep all anomalous directories.
        if state != "directory_matches" {
            report.files.push(FileInfo {
                path,
                state,
                expected_bytes: want.filter(|e| !e.directory).map(|e| e.bytes.to_string()),
                actual_bytes: actual
                    .filter(|e| e.kind == Kind::File)
                    .map(|e| e.bytes.to_string()),
                expected_sha1: want.and_then(|e| e.sha1.clone()),
            });
        }
    }
    // Only read a plain registry inside the scanned tree. All source archives stay unopened.
    let registry = disk.get("meta/0.papgt").filter(|d| d.kind == Kind::File);
    let registry_result = registry
        .ok_or_else(|| bad("plain meta/0.papgt missing"))
        .and_then(|entry| read_small(&root.join(&entry.path), 1024 * 1024))
        .and_then(|bytes| {
            let groups = crimson_format::overlay::registry_groups(&bytes)?;
            report.registry_sha256 = Some(hash_bytes(&bytes));
            report.registry_matches_observed_build =
                Some(crate::fingerprint::known_builds().iter().any(|build| {
                    build.files.iter().any(|file| {
                        file.path == "meta/0.papgt"
                            && Some(&file.sha256) == report.registry_sha256.as_ref()
                    })
                }));
            for group in groups {
                let key = format!("{}/0.pamt", group.name.to_lowercase());
                let installed = disk.get(&key).is_some_and(|d| d.kind == Kind::File);
                let in_depots = expected.get(&key).is_some_and(|e| !e.directory);
                if !installed && !group.is_optional {
                    report.issue(
                        "required_group_missing",
                        Some(group.name.clone()),
                        "Erforderliche registrierte Archivgruppe fehlt.",
                    );
                }
                if installed && !in_depots && report.depot_comparison_available {
                    report.issue(
                        "group_outside_depots",
                        Some(group.name.clone()),
                        "Installierte Registry-Gruppe fehlt in den gewählten Depotlisten.",
                    );
                }
                report.groups.push(GroupInfo {
                    name: group.name,
                    optional: group.is_optional,
                    installed,
                    in_depots,
                });
            }
            Ok(())
        });
    if let Err(e) = registry_result {
        report.issue(
            "registry_unavailable",
            Some("meta/0.papgt".into()),
            e.to_string(),
        );
    }
    for row in &report.files {
        if let Some(group) = row.path.strip_suffix("/0.pamt")
            && !report
                .groups
                .iter()
                .any(|g| g.name.eq_ignore_ascii_case(group))
        {
            report.issues.push(Issue {
                code: "unregistered_group",
                path: Some(group.into()),
                message: "Archivgruppe ist nicht in der gelesenen Registry registriert.".into(),
            });
        }
    }
    Ok(report)
}

type Expected = BTreeMap<String, depot::Entry>;
fn steam_inventory(
    root: &Path,
    steam_roots: &[PathBuf],
) -> io::Result<(String, Vec<DepotInfo>, Expected)> {
    let common = root
        .parent()
        .ok_or_else(|| bad("Steam common directory missing"))?;
    if !common
        .file_name()
        .is_some_and(|n| n.eq_ignore_ascii_case("common"))
    {
        return Err(bad("installation is not inside a Steam library"));
    }
    let steamapps = common
        .parent()
        .ok_or_else(|| bad("Steam library missing"))?;
    if !steamapps
        .file_name()
        .is_some_and(|n| n.eq_ignore_ascii_case("steamapps"))
    {
        return Err(bad("Steam library missing"));
    }
    let app_path = steamapps.join("appmanifest_3321460.acf");
    let app = read_small(&app_path, 1024 * 1024)?;
    let vdf = discovery::parse_vdf(
        std::str::from_utf8(&app).map_err(|_| bad("Steam app manifest is not UTF-8"))?,
    )?;
    let app_state = object(&vdf, "AppState")?;
    if discovery::string(app_state, "appid") != Some(discovery::STEAM_APP_ID) {
        return Err(bad("Steam app ID mismatch"));
    }
    let dir = relative(
        discovery::string(app_state, "installdir")
            .ok_or_else(|| bad("Steam install name missing"))?,
    )?;
    if common.join(dir).canonicalize()? != root {
        return Err(bad("Steam app manifest belongs to another installation"));
    }
    let build = decimal(discovery::string(app_state, "buildid"))?.to_string();
    if let Some(target) = discovery::string(app_state, "TargetBuildID")
        && decimal(Some(target))?.to_string() != build
    {
        return Err(bad("Steam build update pending"));
    }
    let installed = object(app_state, "InstalledDepots")?;
    if installed.is_empty() || installed.len() > 32 {
        return Err(bad("invalid installed depot count"));
    }
    let mut caches: Vec<_> = steam_roots.iter().map(|p| p.join("depotcache")).collect();
    caches.push(steamapps.join("depotcache"));
    if let Some(library) = steamapps.parent() {
        caches.push(library.join("depotcache"));
    }
    caches.sort();
    caches.dedup();
    let mut depots = Vec::new();
    let mut expected = Expected::new();
    for (id, fields) in installed {
        let id = u32::try_from(decimal(Some(id))?).map_err(|_| bad("depot ID overflow"))?;
        let Vdf::Object(fields) = fields else {
            return Err(bad("invalid installed depot"));
        };
        let gid = decimal(discovery::string(fields, "manifest"))?;
        let size = decimal(discovery::string(fields, "size"))?;
        let filename = format!("{id}_{gid}.manifest");
        let candidates: Vec<_> = caches
            .iter()
            .map(|p| p.join(&filename))
            .filter(|p| p.try_exists().unwrap_or(true))
            .collect();
        if candidates.is_empty() {
            return Err(bad(format!("Depot {id}: cached manifest missing")));
        }
        let mut chosen = None;
        for path in candidates {
            let bytes = read_small(&path, 16 * 1024 * 1024)?;
            if chosen.as_ref().is_some_and(|old| old != &bytes) {
                return Err(bad(format!("Depot {id}: conflicting cached manifests")));
            }
            chosen = Some(bytes);
        }
        let bytes = chosen.unwrap();
        let depot = depot::parse(&bytes)?;
        if depot.id != id || depot.manifest != gid || depot.total_bytes != size {
            return Err(bad(format!(
                "Depot {id}: identity or size differs from installed app manifest"
            )));
        }
        depots.push(DepotInfo {
            id,
            manifest_id: gid.to_string(),
            manifest_sha256: hash_bytes(&bytes),
            files: depot.entries.iter().filter(|e| !e.directory).count(),
            bytes: size.to_string(),
            signature_bytes: depot.signature_bytes,
            authenticated: false,
        });
        for entry in depot.entries {
            let key = entry.path.to_lowercase();
            if let Some(old) = expected.get(&key) {
                if old.bytes != entry.bytes
                    || old.directory != entry.directory
                    || old.sha1 != entry.sha1
                {
                    return Err(bad("conflicting entries across depots"));
                }
            } else {
                expected.insert(key, entry);
                if expected.len() > MAX_NODES {
                    return Err(bad("combined depot inventory entry limit"));
                }
            }
        }
    }
    for entry in expected.values().cloned().collect::<Vec<_>>() {
        let mut parent = Path::new(&entry.path).parent();
        while let Some(p) = parent.filter(|p| !p.as_os_str().is_empty()) {
            let path = p.to_string_lossy().replace('\\', "/");
            let key = path.to_lowercase();
            if expected.get(&key).is_some_and(|e| !e.directory) {
                return Err(bad("depot file used as directory"));
            }
            expected.entry(key).or_insert(depot::Entry {
                path,
                bytes: 0,
                directory: true,
                sha1: None,
            });
            if expected.len() > MAX_NODES {
                return Err(bad("combined depot inventory directory limit"));
            }
            parent = p.parent();
        }
    }
    if read_small(&app_path, 1024 * 1024)? != app {
        return Err(bad("Steam app manifest changed during inventory"));
    }
    Ok((build, depots, expected))
}
fn object<'a>(v: &'a BTreeMap<String, Vdf>, key: &str) -> io::Result<&'a BTreeMap<String, Vdf>> {
    match discovery::lookup(v, key) {
        Some(Vdf::Object(o)) => Ok(o),
        _ => Err(bad(format!("Steam object {key} missing"))),
    }
}
fn decimal(s: Option<&str>) -> io::Result<u64> {
    let s = s
        .filter(|s| !s.is_empty() && s.len() <= 20 && s.bytes().all(|b| b.is_ascii_digit()))
        .ok_or_else(|| bad("invalid Steam numeric metadata"))?;
    s.parse().map_err(|_| bad("Steam number overflow"))
}
#[derive(PartialEq, Eq)]
enum Kind {
    File,
    Directory,
    Link,
    Other,
}
struct DiskEntry {
    path: String,
    kind: Kind,
    bytes: u64,
}
fn walk(
    root: &Path,
    relative_path: &Path,
    out: &mut BTreeMap<String, DiskEntry>,
) -> io::Result<()> {
    if relative_path.components().count() > 32 {
        return Err(bad("installation directory nesting limit"));
    }
    for entry in fs::read_dir(root.join(relative_path))? {
        if out.len() >= MAX_NODES {
            return Err(bad("installation inventory entry limit"));
        }
        let entry = entry?;
        let path = relative_path.join(entry.file_name());
        let name = path
            .to_str()
            .ok_or_else(|| bad("non-UTF-8 installation path"))?
            .replace('\\', "/");
        let meta = fs::symlink_metadata(entry.path())?;
        let kind = if linked(&meta) {
            Kind::Link
        } else if meta.is_file() {
            Kind::File
        } else if meta.is_dir() {
            Kind::Directory
        } else {
            Kind::Other
        };
        let recurse = kind == Kind::Directory;
        if out
            .insert(
                name.to_lowercase(),
                DiskEntry {
                    path: name,
                    kind,
                    bytes: meta.len(),
                },
            )
            .is_some()
        {
            return Err(bad("case-insensitive installation path collision"));
        }
        if recurse {
            walk(root, &path, out)?;
        }
    }
    Ok(())
}
fn linked(meta: &fs::Metadata) -> bool {
    #[cfg(windows)]
    {
        use std::os::windows::fs::MetadataExt;
        meta.file_attributes() & 0x400 != 0
    }
    #[cfg(not(windows))]
    {
        meta.file_type().is_symlink()
    }
}
fn read_small(path: &Path, max: usize) -> io::Result<Vec<u8>> {
    // Check each component without acquiring deny-write/deny-execute handles.
    for part in path.ancestors() {
        let meta = fs::symlink_metadata(part)?;
        if linked(&meta) {
            return Err(bad("linked metadata path unsupported"));
        }
    }
    let meta = fs::symlink_metadata(path)?;
    if !meta.is_file() || meta.len() > max as u64 {
        return Err(bad("metadata is not a bounded regular file"));
    }
    let mut bytes = Vec::new();
    fs::File::open(path)?
        .take(max as u64 + 1)
        .read_to_end(&mut bytes)?;
    if bytes.len() > max {
        return Err(bad("metadata read limit exceeded"));
    }
    Ok(bytes)
}
fn relative(path: &str) -> io::Result<String> {
    let path = path.replace('\\', "/");
    if path.is_empty() || path.len() > 1024 {
        return Err(bad("invalid relative depot path"));
    }
    for part in path.split('/') {
        let device = part.split('.').next().unwrap_or("").to_ascii_uppercase();
        if part.is_empty()
            || part == "."
            || part == ".."
            || part.ends_with(['.', ' '])
            || part
                .chars()
                .any(|c| c.is_control() || "<>:\"|?*".contains(c))
            || matches!(device.as_str(), "CON" | "PRN" | "AUX" | "NUL")
            || (device.len() == 4
                && (device.starts_with("COM") || device.starts_with("LPT"))
                && matches!(device.as_bytes()[3], b'1'..=b'9'))
        {
            return Err(bad("unsafe relative depot path"));
        }
    }
    Ok(path)
}
fn bad(message: impl Into<String>) -> io::Error {
    io::Error::new(io::ErrorKind::InvalidData, message.into())
}

#[cfg(all(test, windows))]
pub(crate) fn synthetic_depot(root: &Path, paths: &[crate::fingerprint::FileHash]) {
    use sha1::Digest;
    let apps = root.parent().unwrap().parent().unwrap();
    let cache = apps.join("depotcache");
    fs::create_dir_all(&cache).unwrap();
    let entries: Vec<_> = paths
        .iter()
        .map(|p| {
            (
                p.path.as_str(),
                p.bytes,
                sha1::Sha1::digest(fs::read(root.join(&p.path)).unwrap()).into(),
            )
        })
        .collect();
    fs::write(
        cache.join("1_2.manifest"),
        depot::tests::fixture_hashes(&entries, 1, 2),
    )
    .unwrap();
    fs::write(apps.join("appmanifest_3321460.acf"),format!(r#""AppState" {{ "appid" "3321460" "installdir" "{}" "buildid" "1" "InstalledDepots" {{ "1" {{ "manifest" "2" "size" "{}" }} }} }}"#,root.file_name().unwrap().to_string_lossy(),paths.iter().map(|p|p.bytes).sum::<u64>())).unwrap();
}
