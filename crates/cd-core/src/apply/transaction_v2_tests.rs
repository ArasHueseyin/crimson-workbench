use super::*;
use crimson_format::{Archive, checksum, overlay};

struct Fixture {
    temp: tempfile::TempDir,
    baseline: Vec<u8>,
    files: BTreeMap<String, Vec<u8>>,
}
impl Fixture {
    fn new(groups: usize) -> Self {
        let temp = tempfile::tempdir().unwrap();
        fs::create_dir(temp.path().join("meta")).unwrap();
        let mut empty = vec![0; 16];
        empty[0] = 17;
        empty[11] = 23;
        empty[4..8].copy_from_slice(&checksum(&[0; 4]).to_le_bytes());
        let base = overlay::register(&empty, "0008", 0x7fff, 123).unwrap();
        let baseline = overlay::register(&base, "0040", 0x7fff, 321).unwrap();
        fs::write(temp.path().join(REGISTRY), &baseline).unwrap();
        fs::create_dir(temp.path().join("0008")).unwrap();
        fs::write(
            temp.path().join("0008/0.paz"),
            b"original archive untouched",
        )
        .unwrap();
        let files = payload(&baseline, groups, 3);
        Self {
            temp,
            baseline,
            files,
        }
    }
    fn open(&self) -> Engine {
        Engine::new(self.temp.path(), &hash_bytes(&self.baseline)).unwrap()
    }
    fn baseline_restored(&self) {
        assert_eq!(
            fs::read(self.temp.path().join(REGISTRY)).unwrap(),
            self.baseline
        );
        assert_eq!(
            fs::read(self.temp.path().join("0008/0.paz")).unwrap(),
            b"original archive untouched"
        );
        for entry in fs::read_dir(self.temp.path()).unwrap() {
            let name = entry.unwrap().file_name();
            let name = name.to_str().unwrap();
            assert!(
                matches!(name, "0008" | "meta" | ".workbench"),
                "unexpected leftover {name}"
            );
        }
    }
}
fn payload(base: &[u8], count: usize, value: u8) -> BTreeMap<String, Vec<u8>> {
    let mut files = BTreeMap::new();
    let mut registry = base.to_vec();
    for index in (0..count).rev() {
        let name = format!("{:04}", 41 + index);
        let table = format!("table{}", (b'a' + index as u8) as char);
        let data = BTreeMap::from([
            (format!("{table}.staticinfobody"), vec![value; 512]),
            (format!("{table}.staticinfoheader"), vec![value; 42]),
        ]);
        let overlay = overlay::build(&data, [0x32, 2, 14, 97]).unwrap();
        registry = overlay::register(&registry, &name, 0x7fff, overlay.checksum).unwrap();
        files.insert(format!("{name}/0.pamt"), overlay.pamt);
        files.insert(format!("{name}/0.paz"), overlay.paz);
    }
    files.insert(REGISTRY.into(), registry);
    files
}
fn read_generation(f: &Fixture, plan: &Plan, value: u8) {
    let archive = Archive::open(f.temp.path()).unwrap();
    for name in plan.group_names() {
        for entry in archive.list_group(name).unwrap() {
            assert!(archive.extract(&entry).unwrap().iter().all(|b| *b == value));
        }
    }
}
#[test]
fn multi_group_reapply_switches_registry_before_removing_the_previous_generation() {
    let f = Fixture::new(2);
    let mut e = f.open();
    let first = e.plan(&f.files).unwrap();
    assert_eq!(first.changes().len(), 5);
    e.apply(&first, || Ok(()), None).unwrap();
    read_generation(&f, &first, 3);
    let other = payload(&f.baseline, 2, 9);
    let second = e.plan(&other).unwrap();
    assert_eq!(second.changes().len(), 9);
    assert!(
        second
            .group_names()
            .all(|n| !first.after.groups.contains_key(n) && n != "0040" && n != "0008")
    );
    let before = first.after.registry.clone();
    e.apply(
        &second,
        || {
            if fs::read(f.temp.path().join(REGISTRY))? == before {
                for name in first.group_names() {
                    assert!(f.temp.path().join(name).join("0.paz").exists());
                }
            }
            Ok(())
        },
        None,
    )
    .unwrap();
    read_generation(&f, &second, 9);
    assert!(first.group_names().all(|n| !f.temp.path().join(n).exists()));
    e.restore(|| Ok(()), None).unwrap();
    f.baseline_restored();
    let again = e.plan(&f.files).unwrap();
    assert_eq!(again.after, first.after);
    e.apply(&again, || Ok(()), None).unwrap();
    e.restore(|| Ok(()), None).unwrap();
    f.baseline_restored();
}
#[test]
fn every_partial_write_publish_commit_and_cleanup_boundary_recovers() {
    // Apply, replacement generation, and restore each have their own state space.
    let mut tested = 0;
    for mode in 0..3 {
        let f = Fixture::new(2);
        let mut e = f.open();
        if mode > 0 {
            let p = e.plan(&f.files).unwrap();
            e.apply(&p, || Ok(()), None).unwrap();
        }
        let source = if mode == 2 {
            BTreeMap::new()
        } else {
            payload(&f.baseline, 2, 9)
        };
        let plan = e.plan(&source).unwrap();
        let trace = e.apply(&plan, || Ok(()), None).unwrap();
        assert!(trace.contains(&"registry_committed"));
        assert!(trace.contains(&"file_partial"));
        for cut in 0..trace.len() {
            let f = Fixture::new(2);
            let mut e = f.open();
            if mode > 0 {
                let p = e.plan(&f.files).unwrap();
                e.apply(&p, || Ok(()), None).unwrap();
            }
            let source = if mode == 2 {
                BTreeMap::new()
            } else {
                payload(&f.baseline, 2, 9)
            };
            let plan = e.plan(&source).unwrap();
            let err = e.apply(&plan, || Ok(()), Some(cut)).unwrap_err();
            assert!(err.to_string().contains("injected interruption"), "{err}");
            drop(e);
            let mut e = f.open();
            let review = e.restore_review().unwrap();
            for change in &review.files {
                assert_eq!(
                    hash_bytes(&fs::read(f.temp.path().join(&change.path)).unwrap()),
                    *change.before_sha256.as_ref().unwrap()
                );
            }
            e.recover(|| Ok(()), None).unwrap();
            assert!(e.history().unwrap().pending.is_none());
            let state = e.history().unwrap().current;
            assert!(state == plan.before || state == plan.after);
            e.verify_current(&state).unwrap();
            e.restore(|| Ok(()), None).unwrap();
            for change in &review.files {
                let path = f.temp.path().join(&change.path);
                if let Some(hash) = &change.after_sha256 {
                    assert_eq!(&hash_bytes(&fs::read(path).unwrap()), hash);
                } else {
                    assert!(!path.exists());
                }
            }
            f.baseline_restored();
            tested += 1;
        }
    }
    assert!(tested >= 60);
    eprintln!("V2 interruption matrix: {tested} apply/reapply/restore checkpoints");
}
#[test]
fn recovery_can_itself_be_interrupted_before_or_after_registry_commit() {
    for label in ["group_published", "registry_committed"] {
        let f = Fixture::new(2);
        let mut e = f.open();
        let plan = e.plan(&f.files).unwrap();
        let trace = e.apply(&plan, || Ok(()), None).unwrap();
        let cut = trace.iter().position(|s| *s == label).unwrap();
        let seed = || {
            let f = Fixture::new(2);
            let mut e = f.open();
            let p = e.plan(&f.files).unwrap();
            e.apply(&p, || Ok(()), Some(cut)).unwrap_err();
            drop(e);
            f
        };
        let f = seed();
        let recovery_trace = f.open().recover(|| Ok(()), None).unwrap();
        for at in 0..recovery_trace.len() {
            let f = seed();
            let mut e = f.open();
            assert!(e.recover(|| Ok(()), Some(at)).is_err());
            drop(e);
            let mut e = f.open();
            e.recover(|| Ok(()), None).unwrap();
            e.restore(|| Ok(()), None).unwrap();
            f.baseline_restored();
        }
    }
}
#[test]
fn game_start_or_failed_query_refuses_apply_restore_and_recovery() {
    let f = Fixture::new(1);
    let mut e = f.open();
    let p = e.plan(&f.files).unwrap();
    assert!(e.apply(&p, || Err(bad("game running")), None).is_err());
    assert_eq!(fs::read_dir(e.path(HISTORY).unwrap()).unwrap().count(), 0);
    let mut checks = 0;
    assert!(
        e.apply(
            &p,
            || {
                checks += 1;
                if checks == 6 {
                    Err(bad("process started"))
                } else {
                    Ok(())
                }
            },
            None
        )
        .is_err()
    );
    assert_eq!(fs::read(f.temp.path().join(REGISTRY)).unwrap(), f.baseline);
    assert!(e.recover(|| Err(bad("query failed")), None).is_err());
    e.recover(|| Ok(()), None).unwrap();
    let p = e.plan(&f.files).unwrap();
    e.apply(&p, || Ok(()), None).unwrap();
    let bytes = fs::read(f.temp.path().join(REGISTRY)).unwrap();
    assert!(e.restore(|| Err(bad("running")), None).is_err());
    assert_eq!(fs::read(f.temp.path().join(REGISTRY)).unwrap(), bytes);
}
#[test]
fn stale_plan_foreign_changes_and_tampered_backup_are_refused_without_cleanup() {
    let f = Fixture::new(1);
    let mut e = f.open();
    assert!(Engine::new(f.temp.path(), &hash_bytes(&f.baseline)).is_err());
    let p = e.plan(&f.files).unwrap();
    fs::create_dir(f.temp.path().join("0041")).unwrap();
    assert!(e.apply(&p, || Ok(()), None).is_err());
    assert_eq!(fs::read_dir(e.path(HISTORY).unwrap()).unwrap().count(), 0);
    let p = e.plan(&f.files).unwrap();
    assert!(!p.after.groups.contains_key("0041"));
    e.apply(&p, || Ok(()), None).unwrap();
    let old = fs::read(f.temp.path().join(REGISTRY)).unwrap();
    let group = p.group_names().next().unwrap();
    fs::write(f.temp.path().join(group).join("foreign.txt"), b"keep me").unwrap();
    assert!(e.restore(|| Ok(()), None).is_err());
    assert_eq!(fs::read(f.temp.path().join(REGISTRY)).unwrap(), old);
    let f = Fixture::new(1);
    let mut e = f.open();
    let p = e.plan(&f.files).unwrap();
    e.apply(&p, || Ok(()), None).unwrap();
    fs::write(f.temp.path().join(REGISTRY), b"updated game").unwrap();
    assert!(e.restore(|| Ok(()), None).is_err());
    assert!(e.recover(|| Ok(()), None).is_err());
    assert!(f.temp.path().join("0041/0.paz").exists());
    let f = Fixture::new(1);
    let mut e = f.open();
    let p = e.plan(&f.files).unwrap();
    fs::write(f.temp.path().join(".workbench/baseline.papgt"), b"corrupt").unwrap();
    assert!(e.apply(&p, || Ok(()), None).is_err());
    assert!(!f.temp.path().join("0041").exists());
}
#[test]
fn incomplete_backup_resumes_but_changed_private_prefix_and_hardlinks_are_preserved() {
    let f = Fixture::new(1);
    fs::create_dir(f.temp.path().join(".workbench")).unwrap();
    fs::write(
        f.temp.path().join(".workbench/baseline.part"),
        &f.baseline[..10],
    )
    .unwrap();
    let e = f.open();
    assert_eq!(
        read_plain(&e.path(".workbench/baseline.papgt").unwrap()).unwrap(),
        f.baseline
    );
    drop(e);
    let f = Fixture::new(1);
    fs::create_dir(f.temp.path().join(".workbench")).unwrap();
    let path = f.temp.path().join(".workbench/baseline.part");
    fs::write(&path, b"foreign bytes").unwrap();
    assert!(Engine::new(f.temp.path(), &hash_bytes(&f.baseline)).is_err());
    assert_eq!(fs::read(path).unwrap(), b"foreign bytes");
    let f = Fixture::new(1);
    fs::hard_link(f.temp.path().join(REGISTRY), f.temp.path().join("linked")).unwrap();
    assert!(Engine::new(f.temp.path(), &hash_bytes(&f.baseline)).is_err());
    let f = Fixture::new(1);
    let mut e = f.open();
    let p = e.plan(&f.files).unwrap();
    e.apply(&p, || Ok(()), None).unwrap();
    fs::hard_link(
        f.temp.path().join("0041/0.paz"),
        f.temp.path().join("linked"),
    )
    .unwrap();
    assert!(e.restore(|| Ok(()), None).is_err());
}
#[test]
fn rejects_malformed_packs_reserved_groups_and_registry_edits() {
    let f = Fixture::new(1);
    let e = f.open();
    let mut altered = f.files.clone();
    altered.get_mut("0041/0.paz").unwrap()[0] ^= 1;
    assert!(e.plan(&altered).is_err());
    let mut altered = f.files.clone();
    altered.get_mut(REGISTRY).unwrap()[0] ^= 1;
    assert!(e.plan(&altered).is_err());
    let mut altered = f.files.clone();
    let bytes = altered.remove("0041/0.pamt").unwrap();
    altered.insert("0040/0.pamt".into(), bytes);
    assert!(e.plan(&altered).is_err());
    let mut altered = f.files.clone();
    altered.remove("0041/0.paz");
    assert!(e.plan(&altered).is_err());
    let mut altered = f.files.clone();
    altered.insert("../outside/0.paz".into(), vec![1]);
    assert!(e.plan(&altered).is_err());
    // A valid second archive must not shadow the same virtual table pair.
    let f = Fixture::new(2);
    let e = f.open();
    let mut altered = f.files.clone();
    for suffix in ["pamt", "paz"] {
        altered.insert(
            format!("0042/0.{suffix}"),
            altered[&format!("0041/0.{suffix}")].clone(),
        );
    }
    assert!(
        matches!(e.plan(&altered), Err(crate::Error::Invalid(message)) if message.contains("conflicting virtual table path"))
    );
}
