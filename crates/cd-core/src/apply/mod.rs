//! B0: isolated rehearsals and explicit, reviewed live transactions.
pub(crate) mod guard;
pub mod live;
mod protected;
pub mod recovery;
#[path = "transaction_v2.rs"]
mod transaction;
// Preserve the previous implementation and its tests while the new kernel matures.
#[cfg(test)]
#[path = "transaction.rs"]
mod transaction_v1;
use crate::{Result, Workspace, fingerprint::hash_bytes, mods::BuiltMod};
use serde::{Deserialize, Serialize};
use std::{
    path::{Path, PathBuf},
    sync::atomic::{AtomicU64, Ordering},
};
pub use transaction::FileChange;
/// Accept an owned registry for reading only when backup, journal and overlay
/// files explain it completely. This grants no write admission.
pub(crate) fn original_registry(game: &Path) -> Result<Option<Vec<u8>>> {
    // Identify the build before interpreting a changed registry as an owned
    // overlay. A real game update must not surface as a missing rehearsal backup.
    let known = crate::fingerprint::known_for_game(game)?;
    let expected = known
        .files
        .iter()
        .find(|f| f.path == "meta/0.papgt")
        .ok_or_else(|| crate::Error::Invalid("Known registry missing".into()))?;
    let before = crate::fingerprint::hash_file(&game.join("meta/0.papgt"))?.1;
    if before == expected.sha256 {
        return Ok(None);
    }
    let engine = transaction::Engine::inspect(game, &expected.sha256)?;
    let registry = engine.original_registry()?;
    if before != crate::fingerprint::hash_file(&game.join("meta/0.papgt"))?.1 {
        return Err(crate::Error::Invalid(
            "Registry während des Einlesens geändert; erneut öffnen".into(),
        ));
    }
    Ok(Some(registry))
}
#[cfg(test)]
mod rehearsal_tests;

#[derive(Serialize)]
pub struct ApplyStatus {
    pub game_running: Option<bool>,
    pub can_apply: bool,
    pub reasons: Vec<String>,
}
pub fn status(_game: &Path) -> ApplyStatus {
    let (game_running, mut reasons) = match guard::game_running() {
        Ok(true) => (
            Some(true),
            vec!["Crimson Desert läuft. Apply und Restore sind gesperrt.".into()],
        ),
        Ok(false) => (Some(false), Vec::new()),
        Err(e) => (
            None,
            vec![format!(
                "Prozessprüfung fehlgeschlagen; Schreibpfad gesperrt: {e}"
            )],
        ),
    };
    reasons.push("Dieser Tabellenplan allein erteilt keine Schreibfreigabe. Live-Einrichtung und aktuelle Live-Dateivorschau sind separat erforderlich.".into());
    reasons.push("Die manuelle Prüfung von Overlay-Vorrang und Wirkung ist bis zum Ende aller Entwicklungsphasen zurückgestellt.".into());
    ApplyStatus {
        game_running,
        can_apply: false,
        reasons,
    }
}
pub(crate) fn require_game_stopped() -> Result<()> {
    match guard::game_running() {
        Ok(false) => Ok(()),
        Ok(true) => Err(crate::Error::Invalid(
            "Crimson Desert läuft. Vollständige Inhaltsprüfung erst nach dem Spielen starten."
                .into(),
        )),
        Err(e) => Err(crate::Error::Invalid(format!(
            "Spielstatus unbekannt; Inhaltsprüfung gesperrt: {e}"
        ))),
    }
}
fn unique() -> String {
    static NEXT: AtomicU64 = AtomicU64::new(0);
    format!(
        "{}-{}-{}",
        std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap_or_default()
            .as_millis(),
        std::process::id(),
        NEXT.fetch_add(1, Ordering::Relaxed)
    )
}
pub fn export(workspace: &Workspace, built: &BuiltMod) -> Result<PathBuf> {
    let base = PathBuf::from(format!("exports/mod-preview-{}", unique()));
    // These are unapplied artifacts with explicit gates and attribution, not a certified deployable mod.
    for (path, bytes) in &built.files {
        crate::write_generated(
            workspace.policy(),
            &base.join("unapplied").join(path),
            bytes,
        )?;
    }
    crate::write_generated(
        workspace.policy(),
        &base.join("preview.json"),
        &serde_json::to_vec_pretty(&built.preview)?,
    )
}
#[derive(Serialize)]
pub struct Rehearsal {
    pub directory: PathBuf,
    pub plan_id: String,
    pub cycles: u32,
    pub registry_restored: bool,
    pub archive_files_untouched: bool,
    pub scope: &'static str,
    pub reapply_passed: bool,
    pub recovery_passed: bool,
    pub launch_guard_held: bool,
    pub protected_source_files: usize,
    pub update_refusal_passed: bool,
    pub transitions: Vec<RehearsalStep>,
}
#[derive(Serialize)]
pub struct RehearsalStep {
    pub name: String,
    pub files: Vec<FileChange>,
}
#[derive(Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
struct RehearsalIdentity {
    format: u32,
    kind: String,
    baseline_sha256: String,
    plan_id: String,
    #[serde(default)]
    protection_sha256: Option<String>,
}
#[derive(Serialize)]
pub struct RehearsalRecovery {
    pub directory: PathBuf,
    pub registry_restored: bool,
    pub pending_recovered: bool,
    pub plan_id: String,
}
/// This capability accepts only marked project rehearsals, never an installation.
pub fn recover_rehearsal(
    policy: &crate::paths::PathPolicy,
    directory: &Path,
) -> Result<RehearsalRecovery> {
    let (root, identity) = read_identity(policy, directory)?;
    let root = root.as_path();
    let pending_recovered = if identity.format == 3 {
        let baseline = read_protection(policy, root, &identity)?;
        let mut engine = protected::Session::open(root, &baseline, || Ok(()))?;
        let pending = !engine.recover(None)?.is_empty();
        engine.restore(None)?;
        pending
    } else {
        if identity.protection_sha256.is_some() || root.join("protection.json").try_exists()? {
            return Err(crate::Error::Invalid(
                "Protected rehearsal cannot be downgraded".into(),
            ));
        }
        let mut engine = transaction::Engine::new(root, &identity.baseline_sha256)?;
        let pending = !engine.recover(|| Ok(()), None)?.is_empty();
        engine.restore(|| Ok(()), None)?;
        pending
    };
    recovery_receipt(policy, root, &identity, pending_recovered)
}
fn read_identity(
    policy: &crate::paths::PathPolicy,
    directory: &Path,
) -> Result<(PathBuf, RehearsalIdentity)> {
    let marker = policy.check_cache(&directory.join("rehearsal.json"))?;
    let relative = marker
        .strip_prefix(policy.output_root())
        .map_err(|_| crate::Error::Invalid("Rehearsal must be inside the project".into()))?;
    let parts: Vec<_> = relative.components().collect();
    if parts.len() != 4
        || parts[0].as_os_str() != ".local"
        || parts[1].as_os_str() != "rehearsals"
        || parts[3].as_os_str() != "rehearsal.json"
    {
        return Err(crate::Error::Invalid(
            "Recovery only accepts .local/rehearsals/<id>".into(),
        ));
    }
    use std::io::Read;
    let mut bytes = Vec::new();
    std::fs::File::open(&marker)?
        .take(4097)
        .read_to_end(&mut bytes)?;
    if bytes.len() > 4096 {
        return Err(crate::Error::Invalid(
            "Rehearsal marker exceeds limit".into(),
        ));
    }
    let identity: RehearsalIdentity = serde_json::from_slice(&bytes)?;
    if ![2, 3].contains(&identity.format) || identity.kind != "crimson-workbench-project-rehearsal"
    {
        return Err(crate::Error::Invalid("Unknown rehearsal marker".into()));
    }
    Ok((marker.parent().unwrap().to_owned(), identity))
}
fn recovery_receipt(
    policy: &crate::paths::PathPolicy,
    root: &Path,
    identity: &RehearsalIdentity,
    pending_recovered: bool,
) -> Result<RehearsalRecovery> {
    let registry = std::fs::read(root.join("meta/0.papgt"))?;
    if hash_bytes(&registry) != identity.baseline_sha256 {
        return Err(crate::Error::Invalid(
            "Recovered rehearsal registry mismatch".into(),
        ));
    }
    let report = RehearsalRecovery {
        directory: root.to_owned(),
        registry_restored: true,
        pending_recovered,
        plan_id: identity.plan_id.clone(),
    };
    crate::write_generated(
        policy,
        &root.join(format!("recovery-{}.json", unique())),
        &serde_json::to_vec_pretty(&report)?,
    )?;
    Ok(report)
}
fn read_protection(
    policy: &crate::paths::PathPolicy,
    root: &Path,
    identity: &RehearsalIdentity,
) -> Result<protected::Baseline> {
    use std::io::Read;
    let path = policy.check_cache(&root.join("protection.json"))?;
    let mut bytes = Vec::new();
    std::fs::File::open(path)?
        .take(256 * 1024 + 1)
        .read_to_end(&mut bytes)?;
    let expected = identity
        .protection_sha256
        .as_deref()
        .ok_or_else(|| crate::Error::Invalid("Missing protection manifest identity".into()))?;
    let baseline = protected::Baseline::decode(&bytes, expected)?;
    if baseline.registry_sha256 != identity.baseline_sha256 {
        return Err(crate::Error::Invalid(
            "Protection/registry identity mismatch".into(),
        ));
    }
    Ok(baseline)
}
pub fn rehearse(workspace: &Workspace, built: &BuiltMod) -> Result<Rehearsal> {
    if !cfg!(windows) {
        return Err(crate::Error::Invalid(
            "Geschützte Projektproben benötigen Windows; kein ungesicherter Ersatzpfad.".into(),
        ));
    }
    if built.files.is_empty() {
        return Err(crate::Error::Invalid(
            "Die Vorschau enthält keine Änderungen.".into(),
        ));
    }
    let relative = PathBuf::from(format!(".local/rehearsals/{}", unique()));
    let registry =
        crimson_format::Archive::open(workspace.data().game_root())?.serialize_registry()?;
    let root_file = crate::write_generated(
        workspace.policy(),
        &relative.join("meta/0.papgt"),
        &registry,
    )?;
    let root = root_file.parent().unwrap().parent().unwrap().to_path_buf();
    // A copy of OUR executable and an explicit synthetic source sentinel.
    // Never take exclusive handles on the game executable or its archives here.
    crate::write_generated(
        workspace.policy(),
        &relative.join("probe/workbench-guard-probe.exe"),
        &std::fs::read(std::env::current_exe()?)?,
    )?;
    const SOURCE_PROBE: &[u8] = b"Crimson Workbench synthetic update probe v1";
    crate::write_generated(
        workspace.policy(),
        &relative.join("probe/source.bin"),
        SOURCE_PROBE,
    )?;
    let baseline = protected::Baseline::capture(
        &root,
        &["probe/workbench-guard-probe.exe", "probe/source.bin"],
        &["probe/workbench-guard-probe.exe"],
    )?;
    let protection = serde_json::to_vec_pretty(&baseline)?;
    crate::write_generated(
        workspace.policy(),
        &relative.join("protection.json"),
        &protection,
    )?;
    crate::write_generated(
        workspace.policy(),
        &relative.join("rehearsal.json"),
        &serde_json::to_vec_pretty(&RehearsalIdentity {
            format: 3,
            kind: "crimson-workbench-project-rehearsal".into(),
            baseline_sha256: hash_bytes(&registry),
            plan_id: built.preview.plan_id.clone(),
            protection_sha256: Some(hash_bytes(&protection)),
        })?,
    )?;
    // Rehearsal has no source PAZ files at all and cannot write the installation.
    let mut engine = protected::Session::open(&root, &baseline, || Ok(()))?;
    let mut transitions = Vec::new();
    for cycle in 0..2 {
        engine.recover(None)?;
        let plan = engine.plan(&built.files)?;
        transitions.push(RehearsalStep {
            name: format!("Apply {}", cycle + 1),
            files: plan.changes(),
        });
        engine.apply(&plan, None)?;
        let archive = crimson_format::Archive::open(&root)?;
        for group in plan.group_names() {
            for entry in archive.list_group(group)? {
                archive.extract(&entry)?;
            }
        }
        let reapply = engine.plan(&built.files)?;
        transitions.push(RehearsalStep {
            name: format!("Reapply {}", cycle + 1),
            files: reapply.changes(),
        });
        engine.apply(&reapply, None)?;
        let archive = crimson_format::Archive::open(&root)?;
        for group in reapply.group_names() {
            for entry in archive.list_group(group)? {
                archive.extract(&entry)?;
            }
        }
        transitions.push(RehearsalStep {
            name: format!("Restore {}", cycle + 1),
            files: engine.plan(&std::collections::BTreeMap::new())?.changes(),
        });
        engine.restore(None)?;
    }
    // Exercise a real pending intent without publishing an overlay, then reopen
    // the engine just as a later process would. All writes remain in this copy.
    let recovery_plan = engine.plan(&built.files)?;
    let interruption = engine.apply(&recovery_plan, Some(3));
    match interruption {
        Err(crate::Error::Invalid(message))
            if message == "injected interruption 3: intent_published" => {}
        Err(other) => return Err(other),
        Ok(_) => {
            return Err(crate::Error::Invalid(
                "Rehearsal interruption was not triggered".into(),
            ));
        }
    }
    drop(engine);
    let recovery = recover_rehearsal(workspace.policy(), &root)?;
    if !recovery.pending_recovered {
        return Err(crate::Error::Invalid(
            "Expected pending rehearsal was not recovered".into(),
        ));
    }
    // Model an update ONLY to our own sentinel after releasing all handles.
    let source_probe = workspace
        .policy()
        .check_cache(&root.join("probe/source.bin"))?;
    std::fs::write(&source_probe, b"synthetic updated source")?;
    let refused = recover_rehearsal(workspace.policy(), &root);
    let update_refusal_passed = matches!(refused, Err(crate::Error::Invalid(ref message)) if message.starts_with("protected source mismatch:"));
    std::fs::write(&source_probe, SOURCE_PROBE)?;
    if !update_refusal_passed || std::fs::read(&root_file)? != registry {
        return Err(crate::Error::Invalid("Update refusal probe failed".into()));
    }
    recover_rehearsal(workspace.policy(), &root)?;
    let registry_restored = std::fs::read(&root_file)? == registry;
    let report = Rehearsal {
        directory: root.clone(),
        plan_id: built.preview.plan_id.clone(),
        cycles: 2,
        registry_restored,
        archive_files_untouched: true,
        scope: "Isolierte Registry-/Overlayprobe; Startschutz und Update-Sperre betreffen eine Kopie unserer eigenen EXE und synthetische Quelldaten. Keine Live-Installation, keine Vanilla-Zertifizierung und kein In-game-Test.",
        reapply_passed: true,
        recovery_passed: recovery.pending_recovered && recovery.registry_restored,
        launch_guard_held: true,
        protected_source_files: baseline.file_count(),
        update_refusal_passed,
        transitions,
    };
    crate::write_generated(
        workspace.policy(),
        &relative.join("result.json"),
        &serde_json::to_vec_pretty(&report)?,
    )?;
    Ok(report)
}
