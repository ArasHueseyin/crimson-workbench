use super::*;
use crate::paths::PathPolicy;
use crimson_format::{checksum, overlay};
use std::fs;

#[test]
fn recovery_capability_requires_a_marked_unprotected_project_rehearsal() {
    let temp = tempfile::tempdir().unwrap();
    let project = temp.path();
    let game = project.join(".local/rehearsals/game");
    let policy = PathPolicy::from_roots(vec![game.clone()], project.to_owned()).unwrap();
    let mut empty = vec![0; 16];
    empty[4..8].copy_from_slice(&checksum(&[0; 4]).to_le_bytes());
    let baseline = overlay::register(&empty, "0008", 0x7fff, 123).unwrap();
    let marker = serde_json::to_vec(&RehearsalIdentity {
        format: 2,
        kind: "crimson-workbench-project-rehearsal".into(),
        baseline_sha256: hash_bytes(&baseline),
        plan_id: "synthetic-plan".into(),
        protection_sha256: None,
    })
    .unwrap();
    for directory in [
        &game,
        &project.join("exports/probe"),
        &project.join(".local/other/probe"),
    ] {
        fs::create_dir_all(directory.join("meta")).unwrap();
        fs::write(directory.join("meta/0.papgt"), &baseline).unwrap();
        fs::write(directory.join("rehearsal.json"), &marker).unwrap();
        assert!(recover_rehearsal(&policy, directory).is_err());
        assert!(!directory.join(".workbench").exists());
        assert_eq!(fs::read(directory.join("meta/0.papgt")).unwrap(), baseline);
    }
    let root = project.join(".local/rehearsals/probe");
    fs::create_dir_all(root.join("meta")).unwrap();
    fs::write(root.join("meta/0.papgt"), &baseline).unwrap();
    assert!(recover_rehearsal(&policy, &root).is_err());
    fs::write(root.join("rehearsal.json"), &marker).unwrap();
    let report = recover_rehearsal(&policy, Path::new(".local/rehearsals/probe")).unwrap();
    assert!(report.registry_restored);
    assert!(!report.pending_recovered);
    assert_eq!(report.plan_id, "synthetic-plan");
    assert!(recover_rehearsal(&policy, &root).unwrap().registry_restored);
    fs::hard_link(root.join("rehearsal.json"), project.join("linked-marker")).unwrap();
    assert!(recover_rehearsal(&policy, &root).is_err());
    let unknown = project.join(".local/rehearsals/unknown");
    fs::create_dir_all(&unknown).unwrap();
    let mut unknown_marker: serde_json::Value = serde_json::from_slice(&marker).unwrap();
    unknown_marker["format"] = 99.into();
    fs::write(
        unknown.join("rehearsal.json"),
        serde_json::to_vec(&unknown_marker).unwrap(),
    )
    .unwrap();
    assert!(recover_rehearsal(&policy, &unknown).is_err());
    assert!(!unknown.join(".workbench").exists());
}

#[cfg(windows)]
#[test]
fn protected_recovery_checks_marker_sources_downgrade_and_backup_before_restore() {
    let temp = tempfile::tempdir().unwrap();
    let policy = PathPolicy::from_roots(vec![], temp.path().to_owned()).unwrap();
    let root = temp.path().join(".local/rehearsals/protected");
    fs::create_dir_all(root.join("meta")).unwrap();
    fs::create_dir(root.join("probe")).unwrap();
    let mut empty = vec![0; 16];
    empty[4..8].copy_from_slice(&checksum(&[0; 4]).to_le_bytes());
    let registry = overlay::register(&empty, "0008", 0x7fff, 123).unwrap();
    fs::write(root.join("meta/0.papgt"), &registry).unwrap();
    fs::copy(
        std::env::current_exe().unwrap(),
        root.join("probe/launcher.exe"),
    )
    .unwrap();
    fs::write(root.join("probe/source.bin"), b"original synthetic source").unwrap();
    let baseline = protected::Baseline::capture(
        &root,
        &["probe/launcher.exe", "probe/source.bin"],
        &["probe/launcher.exe"],
    )
    .unwrap();
    let manifest = serde_json::to_vec(&baseline).unwrap();
    fs::write(root.join("protection.json"), &manifest).unwrap();
    let identity = RehearsalIdentity {
        format: 3,
        kind: "crimson-workbench-project-rehearsal".into(),
        baseline_sha256: hash_bytes(&registry),
        plan_id: "synthetic-protected".into(),
        protection_sha256: Some(hash_bytes(&manifest)),
    };
    let marker = serde_json::to_vec(&identity).unwrap();
    fs::write(root.join("rehearsal.json"), &marker).unwrap();
    assert!(recover_rehearsal(&policy, &root).unwrap().registry_restored);
    fs::write(root.join("protection.json"), b"changed manifest").unwrap();
    assert!(recover_rehearsal(&policy, &root).is_err());
    fs::write(root.join("protection.json"), &manifest).unwrap();
    fs::write(root.join("probe/source.bin"), b"synthetic update").unwrap();
    assert!(recover_rehearsal(&policy, &root).is_err());
    assert_eq!(
        fs::read(root.join("probe/source.bin")).unwrap(),
        b"synthetic update"
    );
    fs::write(root.join("probe/source.bin"), b"original synthetic source").unwrap();
    let mut downgrade: serde_json::Value = serde_json::from_slice(&marker).unwrap();
    downgrade["format"] = 2.into();
    downgrade["protection_sha256"] = serde_json::Value::Null;
    fs::write(
        root.join("rehearsal.json"),
        serde_json::to_vec(&downgrade).unwrap(),
    )
    .unwrap();
    assert!(recover_rehearsal(&policy, &root).is_err());
    fs::write(root.join("rehearsal.json"), &marker).unwrap();
    fs::write(root.join(".workbench/baseline.papgt"), b"corrupt backup").unwrap();
    assert!(recover_rehearsal(&policy, &root).is_err());
    assert_eq!(fs::read(root.join("meta/0.papgt")).unwrap(), registry);
    assert_eq!(
        fs::read(root.join(".workbench/baseline.papgt")).unwrap(),
        b"corrupt backup"
    );
}
