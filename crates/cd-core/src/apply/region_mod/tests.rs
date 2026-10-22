use super::*;
use std::{collections::BTreeMap, fs};
struct Fixture {
    dir: tempfile::TempDir,
    before: Vec<u8>,
}
impl Fixture {
    fn new() -> Self {
        let dir = tempfile::tempdir().unwrap();
        let root = dir.path();
        fs::create_dir_all(root.join("meta")).unwrap();
        fs::create_dir_all(root.join("0044")).unwrap();
        fs::create_dir_all(root.join("bin64")).unwrap();
        let mut empty = vec![0; 16];
        empty[0] = 17;
        empty[11] = 23;
        empty[4..8].copy_from_slice(&crimson_format::checksum(&[0; 4]).to_le_bytes());
        let before = overlay::register(&empty, "0044", 0x7fff, 123).unwrap();
        fs::write(root.join("meta/0.papgt"), &before).unwrap();
        fs::write(root.join("0044/0.paz"), b"existing unrelated mod").unwrap();
        fs::write(root.join("bin64/personal.asi"), b"personal runtime").unwrap();
        Self { dir, before }
    }
    fn store(&self) -> Store {
        Store::new(self.dir.path()).unwrap()
    }
    fn registry(&self) -> Vec<u8> {
        fs::read(self.dir.path().join("meta/0.papgt")).unwrap()
    }
    fn intact(&self) {
        assert_eq!(
            fs::read(self.dir.path().join("0044/0.paz")).unwrap(),
            b"existing unrelated mod"
        );
        assert_eq!(
            fs::read(self.dir.path().join("bin64/personal.asi")).unwrap(),
            b"personal runtime"
        );
    }
    fn late(&self) -> Vec<u8> {
        let after = overlay::register(&self.registry(), "0046", 0x7fff, 789).unwrap();
        fs::write(self.dir.path().join("meta/0.papgt"), &after).unwrap();
        after
    }
}
fn package() -> Package {
    let files: BTreeMap<_, _> = overlay::REGION_TIMER_FILES
        .iter()
        .enumerate()
        .map(|(i, n)| (n.to_string(), vec![i as u8 + 1; 256]))
        .collect();
    let pack = overlay::build_region_timers(&files, [0; 4]).unwrap();
    Package {
        pamt: pack.pamt,
        paz: pack.paz,
        language: 0x1234,
        checksum: pack.checksum,
    }
}

#[test]
fn an_unconfigured_folder_is_not_a_game_installation() {
    let dir = tempfile::tempdir().unwrap();
    fs::create_dir(dir.path().join("bin64")).unwrap();
    fs::write(dir.path().join("bin64/CrimsonDesert.exe"), b"synthetic").unwrap();
    assert!(root(dir.path()).is_err());
    assert!(!dir.path().join("bin64/CrimsonWorkbench-mods").exists());
}
fn owned(root: &Path, r: &transaction::Receipt) -> Result<()> {
    let p = package();
    assert_eq!(fs::read(root.join(&r.group).join("0.pamt"))?, p.pamt);
    assert_eq!(fs::read(root.join(&r.group).join("0.paz"))?, p.paz);
    Ok(())
}
#[test]
fn additive_install_and_remove_preserve_old_and_later_mods() {
    let f = Fixture::new();
    let store = f.store();
    let _lock = store.lock().unwrap();
    store.install(package(), None).unwrap();
    let r = store.receipt().unwrap().unwrap();
    assert_eq!(r.group, "9000");
    assert!(store.registered(&r).unwrap());
    store.verify(&r, false).unwrap();
    let archive = Archive::with_registry(f.dir.path(), f.registry()).unwrap();
    let entries = archive.list_group("9000").unwrap();
    assert_eq!(entries.len(), 4);
    for e in entries {
        assert_eq!(archive.extract(&e).unwrap().len(), 256);
    }
    f.late();
    store.remove(owned, None).unwrap();
    assert_eq!(
        f.registry(),
        overlay::register(&f.before, "0046", 0x7fff, 789).unwrap()
    );
    assert!(!f.dir.path().join("9000").exists());
    assert!(store.receipt().unwrap().is_none());
    f.intact();
}
#[test]
fn every_install_interruption_recovers_to_a_complete_registration_or_no_mod() {
    for point in 1..=7 {
        let f = Fixture::new();
        let store = f.store();
        let _lock = store.lock().unwrap();
        assert!(store.install(package(), Some(point)).is_err());
        if store.pending().unwrap() {
            store.recover(owned).unwrap();
        }
        let installed = store.receipt().unwrap().is_some();
        assert_eq!(installed, point >= 5);
        if installed {
            let r = store.receipt().unwrap().unwrap();
            assert!(store.registered(&r).unwrap());
            store.verify(&r, false).unwrap();
        } else {
            assert_eq!(f.registry(), f.before);
            assert!(!f.dir.path().join("9000").exists());
        }
        assert!(!store.pending().unwrap());
        f.intact();
    }
}
#[test]
fn every_remove_interruption_recovers_without_restoring_a_stale_registry() {
    for point in 1..=7 {
        let f = Fixture::new();
        let store = f.store();
        let _lock = store.lock().unwrap();
        store.install(package(), None).unwrap();
        f.late();
        assert!(store.remove(owned, Some(point)).is_err());
        if store.pending().unwrap() {
            store.recover(owned).unwrap();
        }
        assert_eq!(store.receipt().unwrap().is_some(), point == 1);
        if point != 1 {
            assert_eq!(
                f.registry(),
                overlay::register(&f.before, "0046", 0x7fff, 789).unwrap()
            );
        }
        assert!(!store.pending().unwrap());
        f.intact();
    }
}
#[test]
fn recovery_keeps_new_registrations_added_before_an_interrupted_commit() {
    let f = Fixture::new();
    let store = f.store();
    let _lock = store.lock().unwrap();
    assert!(store.install(package(), Some(4)).is_err());
    let later = f.late();
    store.recover(owned).unwrap();
    assert_eq!(f.registry(), later);
    assert!(!f.dir.path().join("9000").exists());
    f.intact();
}
#[test]
fn modified_owned_files_and_unexpected_children_are_never_removed() {
    for extra in [false, true] {
        let f = Fixture::new();
        let store = f.store();
        let _lock = store.lock().unwrap();
        store.install(package(), None).unwrap();
        let before = f.registry();
        let path = f.dir.path().join(if extra {
            "9000/foreign.txt"
        } else {
            "9000/0.paz"
        });
        fs::write(&path, b"foreign bytes").unwrap();
        assert!(store.remove(owned, None).is_err());
        assert_eq!(fs::read(path).unwrap(), b"foreign bytes");
        assert_eq!(f.registry(), before);
        f.intact();
    }
}
#[test]
fn changed_receipts_registrations_and_hardlinks_fail_closed() {
    for mode in 0..3 {
        let f = Fixture::new();
        let store = f.store();
        let _lock = store.lock().unwrap();
        store.install(package(), None).unwrap();
        if mode == 0 {
            let path = f
                .dir
                .path()
                .join("bin64/CrimsonWorkbench-mods/region_return_30min/receipt.json");
            let mut value: serde_json::Value =
                serde_json::from_slice(&fs::read(&path).unwrap()).unwrap();
            value["group"] = "../0044".into();
            fs::write(path, serde_json::to_vec(&value).unwrap()).unwrap();
        } else if mode == 1 {
            let r = store.receipt().unwrap().unwrap();
            let old = overlay::unregister(&f.registry(), &r.group, r.language, r.checksum).unwrap();
            let changed = overlay::register(&old, &r.group, r.language, 999).unwrap();
            fs::write(f.dir.path().join("meta/0.papgt"), changed).unwrap();
        } else {
            fs::hard_link(
                f.dir.path().join("9000/0.paz"),
                f.dir.path().join("kept-hardlink"),
            )
            .unwrap();
        }
        let before = f.registry();
        assert!(store.remove(owned, None).is_err());
        assert_eq!(f.registry(), before);
        f.intact();
    }
}
#[test]
fn an_existing_reserved_group_is_skipped_and_never_adopted() {
    let f = Fixture::new();
    fs::create_dir(f.dir.path().join("9000")).unwrap();
    fs::write(f.dir.path().join("9000/foreign"), b"keep").unwrap();
    let store = f.store();
    let _lock = store.lock().unwrap();
    store.install(package(), None).unwrap();
    assert_eq!(store.receipt().unwrap().unwrap().group, "9001");
    store.remove(owned, None).unwrap();
    assert_eq!(
        fs::read(f.dir.path().join("9000/foreign")).unwrap(),
        b"keep"
    );
    assert_eq!(f.registry(), f.before);
}

#[test]
#[ignore = "Requires CD_REGION_INSTALLER_GAME_ROOT; reads original scripts, writes only an owned temporary fixture"]
fn real_region_payload_installs_and_removes_in_an_owned_fixture() {
    let game = PathBuf::from(
        std::env::var_os("CD_REGION_INSTALLER_GAME_ROOT")
            .expect("private source game root required"),
    );
    let f = Fixture::new();
    let store = f.store();
    let _lock = store.lock().unwrap();
    store.install(build(&game).unwrap(), None).unwrap();
    let receipt = store.receipt().unwrap().unwrap();
    curated(f.dir.path(), &receipt).unwrap();
    f.late();
    store.remove(curated, None).unwrap();
    assert_eq!(
        f.registry(),
        overlay::register(&f.before, "0046", 0x7fff, 789).unwrap()
    );
    f.intact();
}
