use super::super::tests::Fixture;
use super::*;
use serde_json::json;

pub(in crate::apply::live) fn persist(policy: &PathPolicy, e: &mut Evidence, id: &str) {
    let report = json!({"version":1,"kind":"cache-content-audit","game_path":e.admission.game,"build_id":"1","started_at":1,"finished_at":1,
      "all_files_match_cache":true,"metadata_stable":true,"certified_vanilla":false,"can_apply":false,"manifest_sha256":["a".repeat(64)],"limitations":[],
      "files":e.sources.iter().map(|f|json!({"path":f.path,"bytes":f.bytes.to_string(),"expected_sha1":"0".repeat(40),"sha1":"0".repeat(40),"sha256":f.sha256,"matches_cache":true})).collect::<Vec<_>>()});
    let raw = serde_json::to_vec(&report).unwrap();
    e.admission.baseline_id = id.into();
    e.admission.report_sha256 = hash_bytes(&raw);
    let base = Path::new(".local/baselines").join(id);
    for (name,bytes) in [("audit.json",raw),("registry.papgt",e.registry.clone()),("snapshot.json",serde_json::to_vec(&json!({
       "version":1,"kind":"observed-installation-baseline","id":id,"created_at":1,"report_name":"installation-audit-synthetic.json",
       "report_sha256":e.admission.report_sha256,"registry_sha256":e.admission.registry_sha256,"registry_bytes":e.registry.len(),"inventory_sha256":"0".repeat(64)
    })).unwrap())] {crate::write_generated(policy,&base.join(name),&bytes).unwrap();}
}
struct Update {
    _project: tempfile::TempDir,
    _library: tempfile::TempDir,
    f: Fixture,
    policy: PathPolicy,
    next: Evidence,
}
impl Update {
    fn new() -> Self {
        let library = tempfile::tempdir().unwrap();
        let common = library.path().join("steamapps/common");
        fs::create_dir_all(&common).unwrap();
        let mut f = Fixture::new_at(tempfile::tempdir_in(common).unwrap());
        let project = tempfile::tempdir().unwrap();
        let policy =
            PathPolicy::with_output_root(f.dir.path().into(), vec![], project.path().into())
                .unwrap();
        persist(&policy, &mut f.evidence, "old");
        crate::write_generated(
            &policy,
            &admission_path(&f.evidence.admission.game),
            &serde_json::to_vec(&f.evidence.admission).unwrap(),
        )
        .unwrap();
        {
            let mut session = protected::Session::open_existing(
                f.dir.path(),
                &f.evidence.protection().unwrap(),
                || Ok(()),
            )
            .unwrap();
            let plan = session.plan(&f.files).unwrap();
            session.apply(&plan, None).unwrap();
        }
        let source = crimson_format::overlay::build(
            &BTreeMap::from([
                ("example.staticinfobody".into(), vec![4; 256]),
                ("example.staticinfoheader".into(), vec![2; 32]),
            ]),
            [0x32, 2, 14, 97],
        )
        .unwrap();
        let mut empty = vec![0; 16];
        empty[4..8].copy_from_slice(&crimson_format::checksum(&[0; 4]).to_le_bytes());
        let registry =
            crimson_format::overlay::register(&empty, "0008", 0x7fff, source.checksum).unwrap();
        for (path, bytes) in [
            ("0008/0.pamt", source.pamt),
            ("0008/0.paz", source.paz),
            ("meta/0.papgt", registry.clone()),
        ] {
            fs::write(f.dir.path().join(path), bytes).unwrap();
        }
        let sources = f
            .evidence
            .sources
            .iter()
            .map(|s| {
                let (bytes, sha256) =
                    crate::fingerprint::hash_file(&f.dir.path().join(&s.path)).unwrap();
                FileHash {
                    path: s.path.clone(),
                    bytes,
                    sha256,
                }
            })
            .collect();
        let mut next = Evidence {
            admission: Admission {
                registry_sha256: hash_bytes(&registry),
                ..f.evidence.admission.clone()
            },
            registry,
            sources,
            audited_at: 1,
            archived_files: vec![],
            archived_dirs: BTreeSet::new(),
            foreign_files: vec![],
            foreign_dirs: BTreeSet::new(),
            foreign_revision: None,
        };
        persist(&policy, &mut next, "new");
        Self {
            _project: project,
            _library: library,
            f,
            policy,
            next,
        }
    }
    fn plan(&self) -> Plan {
        collect_plan(
            &self.f.evidence.admission,
            &self.next,
            "0000000000000001".into(),
        )
        .unwrap()
    }
    fn state(&self) -> State {
        state(&self.policy, self.f.dir.path()).unwrap()
    }
    fn run(&self, p: &Plan, fault: Option<usize>) -> Result<Receipt> {
        migrate(&self.policy, &self.state(), &self.next, p, || Ok(()), fault)
    }
    fn finished(&self, p: &Plan) {
        let state = self.state();
        assert!(state.pending.is_none());
        assert_eq!(state.active.baseline_id, "new");
        assert_eq!(
            fs::read(self.f.dir.path().join("meta/0.papgt")).unwrap(),
            self.next.registry
        );
        assert_ne!(self.next.registry, self.f.evidence.registry);
        for t in &p.trees {
            tree_check(self.f.dir.path(), t, true, true).unwrap();
        }
        let engine =
            transaction::Engine::inspect(self.f.dir.path(), &self.next.admission.registry_sha256)
                .unwrap();
        assert!(engine.restore_review().unwrap().files.is_empty());
        assert!(
            claims(&self.policy, self.f.dir.path())
                .unwrap()
                .unwrap()
                .0
                .contains(".workbench-history/0000000000000001/0041/0.paz")
        );
        layout(
            &evidence_for_finish(
                &self.next,
                &State {
                    active: p.before.clone(),
                    pending: None,
                    plans: vec![],
                },
                p,
            ),
            &[],
        )
        .unwrap();
    }
}
#[test]
fn migration_preserves_every_old_byte_and_uses_new_original_registry() {
    let f = Update::new();
    let p = f.plan();
    assert!(
        claims(&f.policy, f.f.dir.path())
            .unwrap()
            .unwrap()
            .0
            .contains("0041/0.paz")
    );
    // Synthetic builds are intentionally unknown: production admission must refuse.
    assert!(preview(&f.policy, f.f.dir.path(), "new").is_err());
    f.run(&p, None).unwrap();
    f.finished(&p);
    let finished = evidence_for_finish(
        &f.next,
        &State {
            active: p.before.clone(),
            pending: None,
            plans: vec![],
        },
        &p,
    );
    let mut session =
        protected::Session::open_existing(f.f.dir.path(), &finished.protection().unwrap(), || {
            Ok(())
        })
        .unwrap();
    let files = BTreeMap::from([
        ("example.staticinfobody".into(), vec![7; 256]),
        ("example.staticinfoheader".into(), vec![2; 32]),
    ]);
    let overlay = crimson_format::overlay::build(&files, [0x32, 2, 14, 97]).unwrap();
    let files = BTreeMap::from([
        ("0041/0.pamt".into(), overlay.pamt),
        ("0041/0.paz".into(), overlay.paz),
        (
            "meta/0.papgt".into(),
            crimson_format::overlay::register(&f.next.registry, "0041", 0x7fff, overlay.checksum)
                .unwrap(),
        ),
    ]);
    let plan = session.plan(&files).unwrap();
    session.apply(&plan, None).unwrap();
    session.restore(None).unwrap();
    assert_eq!(
        fs::read(f.f.dir.path().join("meta/0.papgt")).unwrap(),
        f.next.registry
    );
}
#[test]
fn every_migration_boundary_resumes_without_replacing_updated_registry() {
    for point in 0..6 {
        let f = Update::new();
        let p = f.plan();
        assert!(f.run(&p, Some(point)).is_err(), "point {point}");
        let state = f.state();
        assert!(state.pending.is_some());
        assert!(claims(&f.policy, f.f.dir.path()).is_err());
        assert_eq!(
            fs::read(f.f.dir.path().join("meta/0.papgt")).unwrap(),
            f.next.registry
        );
        f.run(&p, None).unwrap();
        f.finished(&p);
    }
}
#[test]
fn partially_initialized_new_backup_resumes_and_foreign_partial_refuses() {
    for bad_partial in [false, true] {
        let f = Update::new();
        let p = f.plan();
        assert!(f.run(&p, Some(3)).is_err());
        let wb = f.f.dir.path().join(".workbench");
        fs::create_dir(&wb).unwrap();
        let bytes = if bad_partial {
            vec![0xff; 3]
        } else {
            f.next.registry[..3].to_vec()
        };
        fs::write(wb.join("baseline.part"), bytes).unwrap();
        if bad_partial {
            assert!(f.run(&p, None).is_err());
            assert!(f.state().pending.is_some());
        } else {
            f.run(&p, None).unwrap();
            f.finished(&p);
        }
    }
}
#[test]
fn collisions_changed_owned_files_and_foreign_private_files_fail_closed() {
    let f = Update::new();
    let mut next = evidence_for_finish(
        &f.next,
        &State {
            active: f.f.evidence.admission.clone(),
            pending: None,
            plans: vec![],
        },
        &f.plan(),
    );
    next.sources.push(FileHash {
        path: "0041/0.paz".into(),
        bytes: 1,
        sha256: "0".repeat(64),
    });
    assert!(collect_plan(&f.f.evidence.admission, &next, "0000000000000001".into()).is_err());
    let path = f.f.dir.path().join("0041/0.paz");
    let mut bytes = fs::read(&path).unwrap();
    bytes[0] ^= 1;
    fs::write(&path, bytes).unwrap();
    // Ownership is explicitly metadata-only, but migration must verify the bytes.
    assert!(claims(&f.policy, f.f.dir.path()).is_ok());
    assert!(collect_plan(&f.f.evidence.admission, &f.next, "0000000000000001".into()).is_err());
    fs::write(f.f.dir.path().join(".workbench/foreign.txt"), b"foreign").unwrap();
    assert!(claims(&f.policy, f.f.dir.path()).is_err());
    assert!(!f.f.dir.path().join(".workbench-history").exists());
}
#[test]
fn cancellation_source_changes_and_duplicate_locations_preserve_pending_plan() {
    let f = Update::new();
    let p = f.plan();
    assert!(
        migrate(
            &f.policy,
            &f.state(),
            &f.next,
            &p,
            || Err(bad("cancelled")),
            None
        )
        .is_err()
    );
    assert!(f.state().pending.is_none());
    fs::write(f.f.dir.path().join("bin/live-probe.exe"), vec![42; 256]).unwrap();
    assert!(f.run(&p, None).is_err());
    assert!(f.state().pending.is_some());
    assert!(!f.f.dir.path().join(".workbench-history").exists());
    fs::write(f.f.dir.path().join("bin/live-probe.exe"), vec![11; 256]).unwrap();
    assert!(f.run(&p, Some(2)).is_err());
    fs::create_dir(f.f.dir.path().join("0041")).unwrap();
    assert!(f.run(&p, None).is_err());
    assert_eq!(
        fs::read(f.f.dir.path().join("meta/0.papgt")).unwrap(),
        f.next.registry
    );
}
#[test]
fn ledger_tampering_archive_foreign_files_and_hardlinks_are_rejected() {
    let f = Update::new();
    let p = f.plan();
    f.run(&p, None).unwrap();
    let complete = f
        .policy
        .check_cache(
            &base(&f.f.evidence.admission.game)
                .join(&p.sequence)
                .join("complete.json"),
        )
        .unwrap();
    let saved = fs::read(&complete).unwrap();
    fs::write(&complete, b"{}").unwrap();
    assert!(f.state_checked().is_err());
    fs::write(complete, saved).unwrap();
    let target = f.f.dir.path().join(&p.trees[0].target).join("foreign");
    fs::hard_link(f.f.dir.path().join("0008/0.paz"), &target).unwrap();
    assert!(claims(&f.policy, f.f.dir.path()).is_err());
}
impl Update {
    fn state_checked(&self) -> Result<State> {
        state(&self.policy, self.f.dir.path())
    }
}

#[test]
fn managed_inventory_and_audit_exclude_only_proven_own_files() {
    let f = Update::new();
    let root = f.f.dir.path();
    crate::installation::synthetic_depot(root, &f.next.sources);
    let inventory = crate::installation::inspect_managed(&f.policy, root).unwrap();
    assert!(
        inventory.issues.is_empty(),
        "{}",
        serde_json::to_string(&inventory.issues).unwrap()
    );
    assert_eq!(inventory.files.len(), 4);
    assert!(
        inventory
            .managed_files
            .iter()
            .any(|p| p.path == "0041/0.paz")
    );
    let audit = crate::installation::audit::synthetic_managed_audit(&f.policy, root).unwrap();
    assert!(audit.all_files_match_cache);
    assert_eq!(audit.files.len(), 4);
    // An active own registry is recognized, but cannot become a vanilla report.
    fs::write(root.join("meta/0.papgt"), &f.f.files["meta/0.papgt"]).unwrap();
    let active = crate::installation::inspect_managed(&f.policy, root).unwrap();
    assert!(active.managed_registry);
    assert!(crate::installation::audit::synthetic_managed_audit(&f.policy, root).is_err());
    fs::write(root.join("meta/0.papgt"), &f.next.registry).unwrap();
    let p = f.plan();
    f.run(&p, None).unwrap();
    let after = crate::installation::inspect_managed(&f.policy, root).unwrap();
    assert!(after.issues.is_empty());
    assert_eq!(after.files.len(), 4);
    assert!(after.managed_files.len() > inventory.managed_files.len());
    assert!(
        crate::installation::audit::synthetic_managed_audit(&f.policy, root)
            .unwrap()
            .all_files_match_cache
    );
    fs::write(root.join("unknown-manager.json"), b"foreign").unwrap();
    let foreign = crate::installation::inspect_managed(&f.policy, root).unwrap();
    assert!(
        foreign
            .files
            .iter()
            .any(|f| f.path == "unknown-manager.json" && f.state == "additional")
    );
    assert!(crate::installation::audit::synthetic_managed_audit(&f.policy, root).is_err());
}

#[test]
fn interrupted_project_intent_and_completion_publications_resume() {
    let f = Update::new();
    let p = f.plan();
    let relative = base(&p.after.game).join(&p.sequence);
    let intent = serde_json::to_vec_pretty(&p).unwrap();
    crate::write_generated(&f.policy, &relative.join("intent.part"), &intent[..25]).unwrap();
    assert!(f.state().pending.is_none());
    assert!(f.run(&p, Some(0)).is_err());
    assert!(f.state().pending.is_some());
    let bytes = serde_json::to_vec_pretty(&Complete {
        intent_sha256: plan_id(&p).unwrap(),
    })
    .unwrap();
    crate::write_generated(&f.policy, &relative.join("complete.part"), &bytes[..20]).unwrap();
    f.run(&p, None).unwrap();
    f.finished(&p);
    assert!(
        !f.policy
            .check_cache(&relative.join("intent.part"))
            .unwrap()
            .exists()
    );
    assert!(
        !f.policy
            .check_cache(&relative.join("complete.part"))
            .unwrap()
            .exists()
    );
}

#[test]
fn basis_migration_preserves_acknowledged_foreign_files_and_their_consent() {
    let mut f = Update::new();
    let root = f.f.dir.path();
    crate::installation::synthetic_depot(root, &f.next.sources);
    fs::write(root.join("foreign-loader.dll"), b"foreign bytes").unwrap();
    let review = foreign::preview(&f.policy, root).unwrap();
    let receipt = foreign::confirm(&f.policy, root, &review.review_id, true).unwrap();
    foreign::decorate(&f.policy, &mut f.next).unwrap();
    let p = f.plan();
    f.run(&p, None).unwrap();
    f.finished(&p);
    assert_eq!(
        fs::read(root.join("foreign-loader.dll")).unwrap(),
        b"foreign bytes"
    );
    assert_eq!(
        foreign::inspect(&f.policy, root)
            .unwrap()
            .approval_id
            .as_deref(),
        Some(receipt.approval_id.as_str())
    );
    let inventory = crate::installation::inspect_managed(&f.policy, root).unwrap();
    assert!(inventory.issues.is_empty());
    assert_eq!(inventory.files.len(), 4);
    assert_eq!(inventory.foreign_files.len(), 1);
}
