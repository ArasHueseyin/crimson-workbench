use super::*;
use crimson_format::{checksum, overlay};

pub(super) struct Fixture {
    pub(super) dir: tempfile::TempDir,
    pub(super) evidence: Evidence,
    pub(super) files: BTreeMap<String, Vec<u8>>,
}
impl Fixture {
    pub(super) fn new() -> Self {
        Self::new_at(tempfile::tempdir().unwrap())
    }
    pub(super) fn new_at(dir: tempfile::TempDir) -> Self {
        let root = dir.path().canonicalize().unwrap();
        for d in ["meta", "bin", "0008"] {
            fs::create_dir(root.join(d)).unwrap();
        }
        let mut empty = vec![0; 16];
        empty[4..8].copy_from_slice(&checksum(&[0; 4]).to_le_bytes());
        let source = overlay::build(
            &BTreeMap::from([
                ("example.staticinfobody".into(), vec![3; 256]),
                ("example.staticinfoheader".into(), vec![2; 32]),
            ]),
            [0x32, 2, 14, 97],
        )
        .unwrap();
        let registry = overlay::register(&empty, "0008", 0x7fff, source.checksum).unwrap();
        for (p, b) in [
            ("meta/0.papgt", registry.clone()),
            ("0008/0.pamt", source.pamt),
            ("0008/0.paz", source.paz),
            ("bin/live-probe.exe", vec![11; 256]),
        ] {
            fs::write(root.join(p), b).unwrap();
        }
        let sources = [
            "meta/0.papgt",
            "0008/0.pamt",
            "0008/0.paz",
            "bin/live-probe.exe",
        ]
        .iter()
        .map(|p| {
            let (bytes, sha256) = crate::fingerprint::hash_file(&root.join(p)).unwrap();
            FileHash {
                path: (*p).into(),
                bytes,
                sha256,
            }
        })
        .collect();
        let evidence = Evidence {
            admission: Admission {
                version: 1,
                kind: "steam-verified-by-user".into(),
                game: root,
                baseline_id: "synthetic-only".into(),
                report_sha256: "0".repeat(64),
                registry_sha256: hash_bytes(&registry),
            },
            sources,
            registry: registry.clone(),
            audited_at: 1,
            archived_files: Vec::new(),
            archived_dirs: BTreeSet::new(),
            foreign_files: vec![],
            foreign_dirs: BTreeSet::new(),
            foreign_revision: None,
        };
        let edit = overlay::build(
            &BTreeMap::from([
                ("example.staticinfobody".into(), vec![7; 256]),
                ("example.staticinfoheader".into(), vec![2; 32]),
            ]),
            [0x32, 2, 14, 97],
        )
        .unwrap();
        let files = BTreeMap::from([
            ("0041/0.pamt".into(), edit.pamt),
            ("0041/0.paz".into(), edit.paz),
            (
                "meta/0.papgt".into(),
                overlay::register(&registry, "0041", 0x7fff, edit.checksum).unwrap(),
            ),
        ]);
        drop(
            protected::Session::open(
                &evidence.admission.game,
                &evidence.protection().unwrap(),
                || Ok(()),
            )
            .unwrap(),
        );
        Self {
            dir,
            evidence,
            files,
        }
    }
    fn engine(&self) -> transaction::Engine {
        transaction::Engine::inspect(self.dir.path(), &self.evidence.admission.registry_sha256)
            .unwrap()
    }
    fn review(&self, request: &Request) -> Review {
        let engine = self.engine();
        let state = engine.restore_review().unwrap();
        let files = if matches!(request, Request::Restore) {
            state.files.clone()
        } else {
            engine.plan(&self.files).unwrap().changes()
        };
        review(&self.evidence, request, &state, files).unwrap()
    }
    fn run(&self, request: &Request, id: &str) -> Result<Receipt> {
        execute_built(&self.evidence, request, &self.files, id, || Ok(()), None)
    }
}
fn apply() -> Request {
    Request::Apply {
        settings: Default::default(),
    }
}
#[test]
fn live_adapter_rebuilds_reopens_original_archive_and_restores() {
    let f = Fixture::new();
    let req = apply();
    let originals: Vec<_> = f
        .evidence
        .sources
        .iter()
        .filter(|s| s.path != "meta/0.papgt")
        .map(|s| {
            (
                s.path.clone(),
                crate::fingerprint::hash_file(&f.dir.path().join(&s.path)).unwrap(),
            )
        })
        .collect();
    for _ in 0..3 {
        let review = f.review(&req);
        let receipt = f.run(&req, &review.review_id).unwrap();
        assert_eq!(receipt.source_files_verified, 3);
        let original = f.engine().original_registry().unwrap();
        assert_eq!(original, f.evidence.registry);
        let archive = crimson_format::Archive::with_registry(f.dir.path(), original).unwrap();
        let entry = archive.list_group("0008").unwrap().remove(0);
        assert_eq!(archive.extract(&entry).unwrap(), vec![3; 256]);
        assert_eq!(archive.groups().len(), 1);
    }
    let preview = f.review(&Request::Restore);
    assert!(preview.files.iter().any(|f| f.action == "remove"));
    let receipt = f.run(&Request::Restore, &preview.review_id).unwrap();
    assert_eq!(
        receipt.registry_sha256,
        f.evidence.admission.registry_sha256
    );
    for (path, hash) in originals {
        assert_eq!(
            crate::fingerprint::hash_file(&f.dir.path().join(path)).unwrap(),
            hash
        );
    }
    layout(&f.evidence, &[]).unwrap();
}
#[test]
fn stale_reviews_foreign_files_and_changed_sources_refuse_without_writes() {
    let f = Fixture::new();
    let req = apply();
    let stale = f.review(&req);
    f.run(&req, &stale.review_id).unwrap();
    let registry = fs::read(f.dir.path().join("meta/0.papgt")).unwrap();
    assert!(
        f.run(&req, &stale.review_id)
            .unwrap_err()
            .to_string()
            .contains("veraltet")
    );
    let fresh = f.review(&req);
    fs::write(f.dir.path().join("foreign.dll"), b"foreign").unwrap();
    assert!(f.run(&req, &fresh.review_id).is_err());
    fs::remove_file(f.dir.path().join("foreign.dll")).unwrap();
    fs::write(f.dir.path().join("bin/live-probe.exe"), vec![12; 256]).unwrap();
    assert!(f.run(&req, &fresh.review_id).is_err());
    assert_eq!(
        fs::read(f.dir.path().join("meta/0.papgt")).unwrap(),
        registry
    );
}
#[test]
fn cancelled_guard_never_mutates_and_releases_handles() {
    let f = Fixture::new();
    let req = apply();
    let review = f.review(&req);
    let mut count = 0;
    assert!(
        execute_built(
            &f.evidence,
            &req,
            &f.files,
            &review.review_id,
            || {
                count += 1;
                if count > 6 {
                    Err(bad("cancelled fixture"))
                } else {
                    Ok(())
                }
            },
            None
        )
        .is_err()
    );
    assert_eq!(
        fs::read(f.dir.path().join("meta/0.papgt")).unwrap(),
        f.evidence.registry
    );
    fs::write(f.dir.path().join("bin/live-probe.exe"), vec![11; 256]).unwrap();
    assert_eq!(
        fs::read_dir(f.dir.path().join(".workbench/transactions"))
            .unwrap()
            .count(),
        0
    );
}
#[test]
fn live_restore_handles_faults_before_and_after_commit() {
    let reference = Fixture::new();
    let req = apply();
    let mut session = protected::Session::open_existing(
        reference.dir.path(),
        &reference.evidence.protection().unwrap(),
        || Ok(()),
    )
    .unwrap();
    let plan = session.plan(&reference.files).unwrap();
    let trace = session.apply(&plan, None).unwrap();
    drop(session);
    for point in [
        trace.iter().position(|s| *s == "group_published").unwrap(),
        trace
            .iter()
            .position(|s| *s == "registry_committed")
            .unwrap(),
    ] {
        let f = Fixture::new();
        let review = f.review(&req);
        assert!(
            execute_built(
                &f.evidence,
                &req,
                &f.files,
                &review.review_id,
                || Ok(()),
                Some(point)
            )
            .is_err()
        );
        let restore = f.review(&Request::Restore);
        assert!(restore.recovery_required);
        let receipt = f.run(&Request::Restore, &restore.review_id).unwrap();
        assert!(receipt.pending_recovered);
        assert_eq!(
            receipt.registry_sha256,
            f.evidence.admission.registry_sha256
        );
        layout(&f.evidence, &[]).unwrap();
    }
}
#[test]
fn read_only_inspection_cannot_mutate_and_damaged_backup_is_refused() {
    let f = Fixture::new();
    let mut engine = f.engine();
    let plan = engine.plan(&f.files).unwrap();
    assert!(engine.apply(&plan, || Ok(()), None).is_err());
    assert!(engine.recover(|| Ok(()), None).is_err());
    drop(engine);
    fs::write(f.dir.path().join(".workbench/baseline.papgt"), b"corrupt").unwrap();
    assert!(
        transaction::Engine::inspect(f.dir.path(), &f.evidence.admission.registry_sha256).is_err()
    );
    assert_eq!(
        fs::read(f.dir.path().join("meta/0.papgt")).unwrap(),
        f.evidence.registry
    );
}
