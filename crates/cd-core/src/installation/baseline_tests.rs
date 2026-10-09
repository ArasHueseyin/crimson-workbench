use super::*;
use crate::installation::tests::Fixture;

struct Setup {
    game: Fixture,
    _project: tempfile::TempDir,
    policy: PathPolicy,
    report: audit::Report,
}
const REPORT: &str = "installation-audit-fixture.json";
#[test]
fn schema_evidence_requires_one_complete_build_and_its_identity() {
    let f = Setup::new();
    let builds = crate::fingerprint::known_builds();
    for known in builds {
        let mut report = f.report.clone();
        report.build_id = known.steam_buildid.clone();
        report.files = known
            .files
            .iter()
            .map(|file| audit::ContentFile {
                path: file.path.clone(),
                bytes: file.bytes.to_string(),
                expected_sha1: "0".repeat(40),
                sha1: "0".repeat(40),
                sha256: file.sha256.clone(),
                matches_cache: true,
            })
            .collect();
        assert!(matches_read_schema(&report));
        let mut incomplete = report.clone();
        incomplete.files.pop();
        assert!(!matches_read_schema(&incomplete));
        let mut wrong_size = report.clone();
        wrong_size.files[0].bytes = "1".into();
        assert!(!matches_read_schema(&wrong_size));
        let other = builds
            .iter()
            .find(|other| other.steam_buildid != known.steam_buildid)
            .unwrap();
        let other_registry = other
            .files
            .iter()
            .find(|file| file.path == REGISTRY)
            .unwrap();
        let mut mixed = report.clone();
        let registry = mixed
            .files
            .iter_mut()
            .find(|file| file.path == REGISTRY)
            .unwrap();
        registry.sha256 = other_registry.sha256.clone();
        registry.bytes = other_registry.bytes.to_string();
        assert!(!matches_read_schema(&mixed));
        report.build_id = other.steam_buildid.clone();
        assert!(!matches_read_schema(&report));
    }
}
impl Setup {
    fn new() -> Self {
        let game = Fixture::new();
        let project = tempfile::tempdir().unwrap();
        let policy =
            PathPolicy::from_roots(vec![game.root.clone()], project.path().to_owned()).unwrap();
        let inv = super::super::inspect_with(&game.root, &[]).unwrap();
        let files = inv
            .files
            .iter()
            .map(|f| {
                use sha1::Digest;
                let bytes = fs::read(game.root.join(&f.path)).unwrap();
                audit::ContentFile {
                    path: f.path.clone(),
                    bytes: bytes.len().to_string(),
                    expected_sha1: f.expected_sha1.clone().unwrap(),
                    sha1: format!("{:x}", sha1::Sha1::digest(&bytes)),
                    sha256: hash_bytes(&bytes),
                    matches_cache: true,
                }
            })
            .collect();
        let report = audit::Report {
            version: 1,
            kind: "cache-content-audit".into(),
            game_path: game.root.canonicalize().unwrap(),
            build_id: "123".into(),
            started_at: now() - 5,
            finished_at: now(),
            all_files_match_cache: true,
            metadata_stable: true,
            certified_vanilla: false,
            can_apply: false,
            manifest_sha256: inv
                .depots
                .iter()
                .map(|d| d.manifest_sha256.clone())
                .collect(),
            files,
            foreign_approval: None,
            limitations: vec!["Synthetic only".into()],
        };
        write_generated(
            &policy,
            &Path::new("exports").join(REPORT),
            &serde_json::to_vec(&report).unwrap(),
        )
        .unwrap();
        Self {
            game,
            _project: project,
            policy,
            report,
        }
    }
    fn save(&self) -> Saved {
        let p = preview(&self.policy, &self.game.root, REPORT).unwrap();
        capture(&self.policy, &self.game.root, REPORT, &p.review_id).unwrap()
    }
    fn output(&self, path: &str) -> PathBuf {
        self.policy.output_root().join(path)
    }
}
#[test]
fn audit_is_preserved_with_verified_registry_copy_and_no_trust_promotion() {
    let f = Setup::new();
    let original = fs::read(f.game.root.join(REGISTRY)).unwrap();
    assert_eq!(catalog(&f.policy).unwrap().reports, [REPORT]);
    let p = preview(&f.policy, &f.game.root, REPORT).unwrap();
    assert_eq!(p.file_count, 4);
    assert_eq!(p.archive_count, 1);
    assert!(!f.output(".local/baselines").exists());
    let saved = capture(&f.policy, &f.game.root, REPORT, &p.review_id).unwrap();
    assert_eq!(saved.current_state, "metadata_match");
    assert!(!saved.preview.can_apply && !saved.preview.certified_vanilla);
    assert_eq!(
        fs::read(saved.directory.join("registry.papgt")).unwrap(),
        original
    );
    assert_eq!(
        fs::read(saved.directory.join("audit.json")).unwrap(),
        fs::read(f.output(&format!("exports/{REPORT}"))).unwrap()
    );
    assert_eq!(fs::read(f.game.root.join(REGISTRY)).unwrap(), original);
    assert!(!f.game.root.join(".workbench").exists());
    assert_eq!(catalog(&f.policy).unwrap().snapshots, [saved.id]);
}
#[test]
fn incomplete_false_claims_duplicate_paths_unsafe_names_and_overflows_are_refused() {
    let f = Setup::new();
    for case in 0..12 {
        let mut r = f.report.clone();
        match case {
            0 => r.certified_vanilla = true,
            1 => r.can_apply = true,
            2 => r.all_files_match_cache = false,
            3 => r.files[0].matches_cache = false,
            4 => r.files[0].sha1 = "a".repeat(40),
            5 => r.files[0].path = "../save.bin".into(),
            6 => {
                r.files.push(r.files[0].clone());
            }
            7 => r.files[0].bytes = "18446744073709551615".into(),
            8 => r.files[0].path = "CON.txt".into(),
            9 => {
                r.files
                    .iter_mut()
                    .find(|f| f.path == REGISTRY)
                    .unwrap()
                    .path = "META/0.PAPGT".into()
            }
            10 => r.manifest_sha256.push(r.manifest_sha256[0].clone()),
            _ => r.finished_at = now() + 10000,
        }
        assert!(validate(&r).is_err(), "case {case}");
    }
    let raw = serde_json::to_string(&f.report).unwrap();
    assert!(
        serde_json::from_str::<audit::Report>(&raw.replacen("{", "{\"version\":1,", 1)).is_err()
    );
    assert!(
        serde_json::from_str::<audit::Report>(&raw.replacen("{", "{\"foreign\":1,", 1)).is_err()
    );
    for name in [
        "../installation-audit-other.json",
        "C:/save.json",
        "not-a-report.json",
    ] {
        assert!(preview(&f.policy, &f.game.root, name).is_err());
    }
    assert!(!f.output(".local/baselines").exists());
}
#[test]
fn stale_report_and_changed_installation_are_refused_before_snapshot_writes() {
    for change in 0..3 {
        let f = Setup::new();
        let p = preview(&f.policy, &f.game.root, REPORT).unwrap();
        match change {
            0 => {
                fs::write(f.game.root.join(REGISTRY), b"foreign registry").unwrap();
            }
            1 => {
                let text = fs::read_to_string(&f.game.app)
                    .unwrap()
                    .replace("123", "124");
                fs::write(&f.game.app, text).unwrap();
            }
            _ => {
                let mut r = f.report.clone();
                r.limitations.push("changed report bytes".into());
                fs::write(
                    f.output(&format!("exports/{REPORT}")),
                    serde_json::to_vec(&r).unwrap(),
                )
                .unwrap();
            }
        }
        assert!(capture(&f.policy, &f.game.root, REPORT, &p.review_id).is_err());
        assert!(!f.output(".local/baselines").exists());
    }
}
#[test]
fn interrupted_copies_never_load_and_retry_uses_a_new_directory() {
    let f = Setup::new();
    let p = preview(&f.policy, &f.game.root, REPORT).unwrap();
    for point in 0..3 {
        assert!(capture_with(&f.policy, &f.game.root, REPORT, &p.review_id, Some(point)).is_err());
    }
    let partial = catalog(&f.policy).unwrap();
    assert_eq!(partial.snapshots.len(), 3);
    for id in &partial.snapshots {
        assert!(inspect(&f.policy, &f.game.root, id).is_err());
    }
    let saved = f.save();
    assert_eq!(saved.current_state, "metadata_match");
    assert_eq!(catalog(&f.policy).unwrap().snapshots.len(), 4);
    for id in &partial.snapshots {
        assert!(
            !f.output(&format!(".local/baselines/{id}/snapshot.json"))
                .exists()
        );
    }
}
#[test]
fn corrupted_or_hardlinked_saved_files_never_become_an_accepted_backup() {
    for file in ["audit.json", "registry.papgt", "snapshot.json"] {
        let f = Setup::new();
        let saved = f.save();
        fs::write(saved.directory.join(file), b"corrupt").unwrap();
        assert!(inspect(&f.policy, &f.game.root, &saved.id).is_err());
        assert_eq!(fs::read(saved.directory.join(file)).unwrap(), b"corrupt");
    }
    let f = Setup::new();
    let saved = f.save();
    fs::hard_link(
        saved.directory.join("registry.papgt"),
        f.output("linked-backup"),
    )
    .unwrap();
    assert!(inspect(&f.policy, &f.game.root, &saved.id).is_err());
}
#[test]
fn update_status_keeps_the_old_backup_and_same_size_archives_are_not_rehashed() {
    let f = Setup::new();
    let saved = f.save();
    let old = fs::read(saved.directory.join("registry.papgt")).unwrap();
    let path = f.game.root.join("0000/0.paz");
    let count = fs::metadata(&path).unwrap().len() as usize;
    fs::write(path, vec![b'x'; count]).unwrap();
    let status = inspect(&f.policy, &f.game.root, &saved.id).unwrap();
    assert_eq!(status.current_state, "metadata_match");
    assert!(!status.preview.can_apply && !status.preview.certified_vanilla);
    fs::write(f.game.root.join(REGISTRY), b"updated registry").unwrap();
    let status = inspect(&f.policy, &f.game.root, &saved.id).unwrap();
    assert_eq!(status.current_state, "changed");
    assert!(!status.issues.is_empty());
    assert_eq!(
        fs::read(saved.directory.join("registry.papgt")).unwrap(),
        old
    );
    assert_eq!(
        fs::read(f.game.root.join(REGISTRY)).unwrap(),
        b"updated registry"
    );
}
#[test]
fn another_installation_protected_destinations_and_unbounded_inputs_are_refused() {
    let f = Setup::new();
    let other = Fixture::new();
    assert!(preview(&f.policy, &other.root, REPORT).is_err());
    let blocked =
        PathPolicy::from_roots(vec![f.output(".local")], f.policy.output_root().to_owned())
            .unwrap();
    let p = preview(&blocked, &f.game.root, REPORT).unwrap();
    assert!(capture(&blocked, &f.game.root, REPORT, &p.review_id).is_err());
    assert!(!f.output(".local").exists());
    fs::write(
        f.output(&format!("exports/{REPORT}")),
        vec![b' '; MAX_REPORT + 1],
    )
    .unwrap();
    assert!(preview(&f.policy, &f.game.root, REPORT).is_err());
    assert!(inspect(&f.policy, &f.game.root, "../../exports").is_err());
}
