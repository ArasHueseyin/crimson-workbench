use super::*;
use crate::apply::{RehearsalIdentity, protected::Baseline};
use crimson_format::{checksum, overlay};
use std::collections::BTreeMap;

struct Fixture {
    _temp: tempfile::TempDir,
    policy: PathPolicy,
    root: PathBuf,
    baseline: Baseline,
    files: BTreeMap<String, Vec<u8>>,
}
impl Fixture {
    fn new() -> Self {
        let temp = tempfile::tempdir().unwrap();
        let root = temp.path().join(".local/rehearsals/test");
        fs::create_dir_all(root.join("meta")).unwrap();
        fs::create_dir(root.join("probe")).unwrap();
        fs::write(
            root.join("probe/restore-fixture.exe"),
            b"synthetic non-executable fixture",
        )
        .unwrap();
        fs::write(root.join("probe/source.bin"), b"unchanged source").unwrap();
        let mut empty = vec![0; 16];
        empty[4..8].copy_from_slice(&checksum(&[0; 4]).to_le_bytes());
        let registry = overlay::register(&empty, "0008", 0x7fff, 123).unwrap();
        fs::write(root.join("meta/0.papgt"), &registry).unwrap();
        let baseline = Baseline::capture(
            &root,
            &["probe/restore-fixture.exe", "probe/source.bin"],
            &["probe/restore-fixture.exe"],
        )
        .unwrap();
        let protection = serde_json::to_vec(&baseline).unwrap();
        fs::write(root.join("protection.json"), &protection).unwrap();
        fs::write(
            root.join("rehearsal.json"),
            serde_json::to_vec(&RehearsalIdentity {
                format: 3,
                kind: "crimson-workbench-project-rehearsal".into(),
                baseline_sha256: hash_bytes(&registry),
                plan_id: "fixture".into(),
                protection_sha256: Some(hash_bytes(&protection)),
            })
            .unwrap(),
        )
        .unwrap();
        let overlay = overlay::build(
            &BTreeMap::from([
                ("test.staticinfobody".into(), vec![7; 512]),
                ("test.staticinfoheader".into(), vec![7; 42]),
            ]),
            [1, 2, 3, 4],
        )
        .unwrap();
        let files = BTreeMap::from([
            ("0041/0.pamt".into(), overlay.pamt),
            ("0041/0.paz".into(), overlay.paz),
            (
                "meta/0.papgt".into(),
                overlay::register(&registry, "0041", 0x7fff, overlay.checksum).unwrap(),
            ),
        ]);
        let policy = PathPolicy::from_roots(vec![], temp.path().to_owned()).unwrap();
        Self {
            _temp: temp,
            policy,
            root,
            baseline,
            files,
        }
    }
    fn apply(&self, fault: Option<usize>) -> Result<Vec<&'static str>> {
        let mut session = protected::Session::open(&self.root, &self.baseline, || Ok(()))?;
        let plan = session.plan(&self.files)?;
        session.apply(&plan, fault)
    }
    fn snapshot(&self) -> BTreeMap<PathBuf, Vec<u8>> {
        fn walk(root: &Path, out: &mut BTreeMap<PathBuf, Vec<u8>>) {
            for item in fs::read_dir(root).unwrap() {
                let path = item.unwrap().path();
                if path.is_dir() {
                    out.insert(path.clone(), Vec::new());
                    walk(&path, out);
                } else {
                    out.insert(path.clone(), fs::read(path).unwrap());
                }
            }
        }
        let mut out = BTreeMap::new();
        walk(&self.root, &mut out);
        out
    }
}

#[test]
fn inspection_never_initializes_or_repairs_missing_backup_and_history() {
    let f = Fixture::new();
    let before = f.snapshot();
    assert!(inspect(&f.policy, &f.root).is_err());
    assert_eq!(f.snapshot(), before);
    f.apply(None).unwrap();
    fs::remove_file(f.root.join(".workbench/baseline.papgt")).unwrap();
    let before = f.snapshot();
    assert!(inspect(&f.policy, &f.root).is_err());
    assert!(restore(&f.policy, &f.root, &"a".repeat(64)).is_err());
    assert_eq!(f.snapshot(), before);
}
#[test]
fn preview_is_read_only_and_restore_matches_displayed_paths_and_hashes() {
    let f = Fixture::new();
    f.apply(None).unwrap();
    let before = f.snapshot();
    let review = inspect(&f.policy, &f.root).unwrap();
    assert_eq!(before, f.snapshot());
    assert!(review.can_restore);
    assert_eq!(review.restore.files.len(), 3);
    assert_eq!(review.restore.files[0].path, "meta/0.papgt");
    assert_eq!(review.restore.remove_directories, ["0041"]);
    let result = restore(&f.policy, &f.root, &review.review_id).unwrap();
    assert!(result.registry_restored && !result.pending_recovered);
    for file in review.restore.files {
        let path = f.root.join(file.path);
        assert_eq!(hash_bytes(&before[&path]), file.before_sha256.unwrap());
        if let Some(hash) = file.after_sha256 {
            assert_eq!(hash_bytes(&fs::read(path).unwrap()), hash);
        } else {
            assert!(!path.exists());
        }
    }
    let after = inspect(&f.policy, &f.root).unwrap();
    assert!(!after.can_restore && after.restore.files.is_empty());
    assert_eq!(after.restore.backup_sha256, f.baseline.registry_sha256);
    assert_eq!(
        fs::read(f.root.join("probe/source.bin")).unwrap(),
        b"unchanged source"
    );
}
#[test]
fn pending_before_and_after_commit_can_be_previewed_and_finished() {
    let trace = Fixture::new().apply(None).unwrap();
    for label in ["intent_published", "group_published", "registry_committed"] {
        let f = Fixture::new();
        let cut = trace.iter().position(|s| *s == label).unwrap();
        assert!(f.apply(Some(cut)).is_err());
        let before = f.snapshot();
        let review = inspect(&f.policy, &f.root).unwrap();
        assert_eq!(f.snapshot(), before);
        assert!(review.can_restore);
        assert_eq!(
            review.restore.pending_outcome,
            Some(if label == "registry_committed" {
                "committed"
            } else {
                "rolled_back"
            })
        );
        assert!(
            restore(&f.policy, &f.root, &review.review_id)
                .unwrap()
                .pending_recovered
        );
        assert!(!inspect(&f.policy, &f.root).unwrap().can_restore);
    }
}
#[test]
fn stale_preview_or_another_directory_never_restores() {
    let f = Fixture::new();
    f.apply(None).unwrap();
    let review = inspect(&f.policy, &f.root).unwrap();
    let other = Fixture::new();
    other.apply(None).unwrap();
    let before = other.snapshot();
    assert!(restore(&other.policy, &other.root, &review.review_id).is_err());
    assert_eq!(other.snapshot(), before);
    f.apply(None).unwrap();
    let before = f.snapshot();
    assert!(restore(&f.policy, &f.root, &review.review_id).is_err());
    assert_eq!(f.snapshot(), before);
    let latest = inspect(&f.policy, &f.root).unwrap();
    assert_ne!(review.review_id, latest.review_id);
    assert!(
        restore(&f.policy, &f.root, &latest.review_id)
            .unwrap()
            .registry_restored
    );
}
#[test]
fn update_foreign_files_corrupt_backup_and_links_preserve_all_evidence() {
    for path in [
        "probe/source.bin",
        ".workbench/baseline.papgt",
        "meta/0.papgt",
        "0041/foreign.txt",
        "0041/0.paz",
    ] {
        let f = Fixture::new();
        f.apply(None).unwrap();
        let review = inspect(&f.policy, &f.root).unwrap();
        fs::write(f.root.join(path), b"update or foreign content").unwrap();
        let before = f.snapshot();
        assert!(inspect(&f.policy, &f.root).is_err(), "{path}");
        assert!(
            restore(&f.policy, &f.root, &review.review_id).is_err(),
            "{path}"
        );
        assert_eq!(f.snapshot(), before, "{path}");
    }
    let f = Fixture::new();
    f.apply(None).unwrap();
    fs::hard_link(
        f.root.join(".workbench/baseline.papgt"),
        f.root.join("linked-backup"),
    )
    .unwrap();
    let before = f.snapshot();
    assert!(inspect(&f.policy, &f.root).is_err());
    assert_eq!(f.snapshot(), before);
}
#[test]
fn listing_is_bounded_marker_only_and_protected_paths_are_refused() {
    let empty = tempfile::tempdir().unwrap();
    let policy = PathPolicy::from_roots(vec![], empty.path().to_owned()).unwrap();
    assert!(list(&policy).unwrap().entries.is_empty());
    assert!(!empty.path().join(".local").exists());
    let f = Fixture::new();
    let before = f.snapshot();
    let entries = list(&f.policy).unwrap();
    assert_eq!(entries.entries.len(), 1);
    assert_eq!(entries.entries[0].format, Some(3));
    assert_eq!(before, f.snapshot());
    let denied = PathPolicy::from_roots(vec![f.root.clone()], f._temp.path().to_owned()).unwrap();
    assert!(inspect(&denied, &f.root).is_err());
    assert!(list(&denied).unwrap().entries[0].error.is_some());
    for i in 0..201 {
        fs::create_dir(f.root.parent().unwrap().join(format!("z-{i:03}"))).unwrap();
    }
    let entries = list(&f.policy).unwrap();
    assert!(entries.truncated);
    assert_eq!(entries.entries.len(), 200);
    assert!(entries.entries.iter().all(|e| e.error.is_some()));
}
