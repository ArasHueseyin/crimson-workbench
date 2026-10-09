use super::super::{tests::Fixture, updates::tests::persist};
use super::*;
struct ForeignFixture {
    _library: tempfile::TempDir,
    _project: tempfile::TempDir,
    f: Fixture,
    policy: PathPolicy,
}
impl ForeignFixture {
    fn new() -> Self {
        let library = tempfile::tempdir().unwrap();
        let common = library.path().join("steamapps/common");
        fs::create_dir_all(&common).unwrap();
        let mut f = Fixture::new_at(tempfile::tempdir_in(common).unwrap());
        let project = tempfile::tempdir().unwrap();
        let policy =
            PathPolicy::with_output_root(f.dir.path().into(), vec![], project.path().into())
                .unwrap();
        persist(&policy, &mut f.evidence, "original");
        crate::write_generated(
            &policy,
            &admission_path(&f.evidence.admission.game),
            &serde_json::to_vec(&f.evidence.admission).unwrap(),
        )
        .unwrap();
        crate::installation::synthetic_depot(f.dir.path(), &f.evidence.sources);
        Self {
            _library: library,
            _project: project,
            f,
            policy,
        }
    }
    fn add(&self, path: &str, bytes: &[u8]) {
        let path = self.f.dir.path().join(path);
        fs::create_dir_all(path.parent().unwrap()).unwrap();
        fs::write(path, bytes).unwrap();
    }
    fn preview(&self) -> Review {
        preview(&self.policy, self.f.dir.path()).unwrap()
    }
    fn confirm(&mut self) -> Receipt {
        let p = self.preview();
        let r = confirm(&self.policy, self.f.dir.path(), &p.review_id, true).unwrap();
        decorate(&self.policy, &mut self.f.evidence).unwrap();
        r
    }
    fn review(&self) -> super::super::Review {
        let engine = transaction::Engine::inspect(
            self.f.dir.path(),
            &self.f.evidence.admission.registry_sha256,
        )
        .unwrap();
        let state = engine.restore_review().unwrap();
        let changes = engine.plan(&self.f.files).unwrap().changes();
        super::super::review(
            &self.f.evidence,
            &Request::Apply {
                settings: Default::default(),
            },
            &state,
            changes,
        )
        .unwrap()
    }
    fn apply(&self, id: &str) -> Result<super::super::Receipt> {
        execute_built(
            &self.f.evidence,
            &Request::Apply {
                settings: Default::default(),
            },
            &self.f.files,
            id,
            || Ok(()),
            None,
        )
    }
}
#[test]
fn exact_consent_is_required_for_inventory_audit_and_held_apply_restore() {
    let mut f = ForeignFixture::new();
    for (path, bytes) in [
        ("0041/0.paz", b"foreign archive".as_slice()),
        ("0041/0.pamt", b"foreign metadata"),
        ("dxgi.dll", b"foreign loader"),
        ("mods/empty.ini", b""),
        ("tools/cwb-foreign-test.EXE", b"synthetic binary"),
    ] {
        f.add(path, bytes);
    }
    assert!(layout(&f.f.evidence, &[]).is_err());
    assert!(
        crate::installation::audit::synthetic_managed_audit(&f.policy, f.f.dir.path()).is_err()
    );
    let p = f.preview();
    assert_eq!(p.files.len(), 5);
    assert!(confirm(&f.policy, f.f.dir.path(), &p.review_id, false).is_err());
    assert!(
        accepted(&f.policy, f.f.dir.path())
            .unwrap()
            .revision
            .is_none()
    );
    let receipt = f.confirm();
    assert_eq!(receipt.files, 5);
    let inventory = crate::installation::inspect_managed(&f.policy, f.f.dir.path()).unwrap();
    assert!(inventory.issues.is_empty());
    assert_eq!(inventory.files.len(), 4);
    assert!(inventory.foreign_files.iter().any(|v| v.path == "dxgi.dll"));
    let audit =
        crate::installation::audit::synthetic_managed_audit(&f.policy, f.f.dir.path()).unwrap();
    assert!(audit.all_files_match_cache);
    assert_eq!(audit.files.len(), 4);
    assert_eq!(
        audit.foreign_approval.as_deref(),
        Some(receipt.approval_id.as_str())
    );
    for _ in 0..2 {
        let review = f.review();
        assert_eq!(review.preserved_foreign_files, 5);
        f.apply(&review.review_id).unwrap();
    }
    let engine =
        transaction::Engine::inspect(f.f.dir.path(), &f.f.evidence.admission.registry_sha256)
            .unwrap();
    let state = engine.restore_review().unwrap();
    let review = super::super::review(
        &f.f.evidence,
        &Request::Restore,
        &state,
        state.files.clone(),
    )
    .unwrap();
    execute_built(
        &f.f.evidence,
        &Request::Restore,
        &BTreeMap::new(),
        &review.review_id,
        || Ok(()),
        None,
    )
    .unwrap();
    for file in p.files {
        assert_eq!(
            crate::fingerprint::hash_file(&f.f.dir.path().join(file.path)).unwrap(),
            (file.bytes, file.sha256)
        );
    }
    assert_eq!(
        fs::read(f.f.dir.path().join("meta/0.papgt")).unwrap(),
        f.f.evidence.registry
    );
}
#[test]
fn changed_foreign_bytes_stale_tokens_and_new_children_never_gain_implicit_consent() {
    let mut f = ForeignFixture::new();
    f.add("mods/settings.ini", b"before");
    let old = f.preview();
    f.add("mods/settings.ini", b"after!");
    assert!(confirm(&f.policy, f.f.dir.path(), &old.review_id, true).is_err());
    f.confirm();
    let review = f.review();
    f.add("mods/settings.ini", b"tamper");
    assert!(f.apply(&review.review_id).is_err());
    assert_eq!(
        fs::read(f.f.dir.path().join("meta/0.papgt")).unwrap(),
        f.f.evidence.registry
    );
    f.confirm();
    assert!(f.apply(&review.review_id).is_err());
    f.add("mods/new.ini", b"new");
    let next = f.review();
    assert!(f.apply(&next.review_id).is_err());
    let inventory = crate::installation::inspect_managed(&f.policy, f.f.dir.path()).unwrap();
    assert!(inventory.files.iter().any(|p| p.path == "mods/new.ini"));
}
#[test]
fn revocation_is_reviewed_and_preserves_every_foreign_byte() {
    let mut f = ForeignFixture::new();
    f.add("foreign.ini", b"unchanged");
    let receipt = f.confirm();
    assert!(revoke(&f.policy, f.f.dir.path(), &"0".repeat(64)).is_err());
    revoke(&f.policy, f.f.dir.path(), &receipt.approval_id).unwrap();
    decorate(&f.policy, &mut f.f.evidence).unwrap();
    assert!(f.f.evidence.foreign_files.is_empty());
    assert!(layout(&f.f.evidence, &[]).is_err());
    assert_eq!(
        fs::read(f.f.dir.path().join("foreign.ini")).unwrap(),
        b"unchanged"
    );
    assert!(revoke(&f.policy, f.f.dir.path(), &receipt.approval_id).is_err());
}
#[test]
fn originals_registered_mods_saves_links_and_private_paths_cannot_be_confirmed() {
    let f = ForeignFixture::new();
    f.add("unknown.ini", b"x");
    let registry = f.f.evidence.registry.clone();
    fs::write(
        f.f.dir.path().join("meta/0.papgt"),
        &f.f.files["meta/0.papgt"],
    )
    .unwrap();
    assert!(preview(&f.policy, f.f.dir.path()).is_err());
    fs::write(f.f.dir.path().join("meta/0.papgt"), registry).unwrap();
    f.add("saves/probe.cdsav", b"must not read");
    assert!(preview(&f.policy, f.f.dir.path()).is_err());
    let g = ForeignFixture::new();
    g.add(".workbench/foreign.ini", b"foreign");
    assert!(preview(&g.policy, g.f.dir.path()).is_err());
    let h = ForeignFixture::new();
    fs::hard_link(
        h.f.dir.path().join("0008/0.paz"),
        h.f.dir.path().join("foreign.paz"),
    )
    .unwrap();
    assert!(preview(&h.policy, h.f.dir.path()).is_err());
}
#[test]
fn confirmation_does_not_hide_original_content_changes_or_promote_a_build() {
    let mut f = ForeignFixture::new();
    f.add("foreign.ini", b"x");
    f.confirm();
    let path = f.f.dir.path().join("0008/0.paz");
    let mut bytes = fs::read(&path).unwrap();
    bytes[0] ^= 1;
    fs::write(path, bytes).unwrap();
    assert!(f.apply(&f.review().review_id).is_err());
    let audit =
        crate::installation::audit::synthetic_managed_audit(&f.policy, f.f.dir.path()).unwrap();
    assert!(!audit.all_files_match_cache);
    assert!(super::super::setup_preview(&f.policy, f.f.dir.path(), "original").is_err());
}
#[test]
fn consent_limits_tampering_and_original_path_collisions_fail_closed() {
    let mut f = ForeignFixture::new();
    f.add("foreign.ini", b"x");
    f.confirm();
    let relative = base(&f.f.evidence.admission.game).join("0000000000000001.json");
    let path = f.policy.check_cache(&relative).unwrap();
    let raw = fs::read(&path).unwrap();
    let mut record: Record = serde_json::from_slice(&raw).unwrap();
    record.set.files[0].path = "meta/0.papgt".into();
    fs::write(&path, serde_json::to_vec(&record).unwrap()).unwrap();
    assert!(accepted(&f.policy, f.f.dir.path()).is_err());
    fs::write(path, raw).unwrap();
    let mut too_large = Set::default();
    too_large.files.push(FileHash {
        path: "huge.paz".into(),
        bytes: MAX_BYTES + 1,
        sha256: "0".repeat(64),
    });
    assert!(validate(&too_large).is_err());
    // If Steam later owns this exact file it must stay in the depot comparison.
    let mut files = f.f.evidence.sources.clone();
    let (bytes, sha256) =
        crate::fingerprint::hash_file(&f.f.dir.path().join("foreign.ini")).unwrap();
    files.push(FileHash {
        path: "foreign.ini".into(),
        bytes,
        sha256,
    });
    crate::installation::synthetic_depot(f.f.dir.path(), &files);
    let inventory = crate::installation::inspect_managed(&f.policy, f.f.dir.path()).unwrap();
    assert!(
        inventory
            .files
            .iter()
            .any(|p| p.path == "foreign.ini" && p.expected_bytes.is_some())
    );
    assert!(inventory.foreign_files.is_empty());

    // Even at the history limit, the active approval can still be revoked.
    let mut g = ForeignFixture::new();
    g.add("foreign.ini", b"preserve");
    g.confirm();
    let folder = g
        .policy
        .output_root()
        .join(base(&g.f.evidence.admission.game));
    let mut record: Record =
        serde_json::from_slice(&fs::read(folder.join("0000000000000001.json")).unwrap()).unwrap();
    for sequence in 2..MAX_RECORDS {
        record.sequence = sequence;
        fs::write(
            folder.join(format!("{sequence:016}.json")),
            serde_json::to_vec(&record).unwrap(),
        )
        .unwrap();
    }
    let review = preview(&g.policy, g.f.dir.path()).unwrap();
    assert!(confirm(&g.policy, g.f.dir.path(), &review.review_id, true).is_err());
    revoke(
        &g.policy,
        g.f.dir.path(),
        review.previous_approval.as_deref().unwrap(),
    )
    .unwrap();
    let state = accepted(&g.policy, g.f.dir.path()).unwrap();
    assert_eq!(state.sequence, MAX_RECORDS);
    assert!(state.set.files.is_empty() && state.set.directories.is_empty());
    assert_eq!(
        fs::read(g.f.dir.path().join("foreign.ini")).unwrap(),
        b"preserve"
    );
}

#[test]
fn revocation_during_protected_work_stops_before_any_registry_write() {
    let mut f = ForeignFixture::new();
    f.add("foreign.ini", b"preserve");
    let receipt = f.confirm();
    let review = f.review();
    let mut checks = 0;
    let result = execute_built(
        &f.f.evidence,
        &Request::Apply {
            settings: Default::default(),
        },
        &f.f.files,
        &review.review_id,
        || {
            checks += 1;
            if checks == 5 {
                revoke(&f.policy, f.f.dir.path(), &receipt.approval_id)?;
            }
            unchanged(&f.policy, &f.f.evidence)
        },
        None,
    );
    assert!(result.is_err());
    assert!(checks >= 5);
    assert_eq!(
        fs::read(f.f.dir.path().join("meta/0.papgt")).unwrap(),
        f.f.evidence.registry
    );
    assert!(
        fs::read_dir(f.f.dir.path().join(".workbench/transactions"))
            .unwrap()
            .next()
            .is_none()
    );
    assert!(
        inspect(&f.policy, f.f.dir.path())
            .unwrap()
            .approval_id
            .is_none()
    );
    assert!(
        fs::OpenOptions::new()
            .write(true)
            .open(f.f.dir.path().join("foreign.ini"))
            .is_ok()
    );
}

#[test]
fn configured_save_directory_is_never_treated_as_a_foreign_mod() {
    let mut f = ForeignFixture::new();
    f.add("profiles/progress.dat", b"synthetic save contents");
    let policy = PathPolicy::with_output_root(
        f.f.dir.path().into(),
        vec![f.f.dir.path().join("profiles")],
        f.policy.output_root().into(),
    )
    .unwrap();
    assert!(preview(&policy, f.f.dir.path()).is_err());
    assert!(
        !policy
            .output_root()
            .join(base(&f.f.evidence.admission.game))
            .exists()
    );
    f.confirm();
    assert!(decorate(&policy, &mut f.f.evidence).is_err());
    let inventory = crate::installation::inspect_managed(&policy, f.f.dir.path()).unwrap();
    assert!(
        inventory
            .issues
            .iter()
            .any(|i| i.code == "foreign_consent_unavailable")
    );
}
