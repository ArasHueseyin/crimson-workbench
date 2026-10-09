//! Explicit read-only acceptance against the local installed extension.
use cd_core::{
    GameData,
    extra_sockets::{self, Request},
    fingerprint::hash_bytes,
};
use std::{fs, path::PathBuf};
#[test]
#[ignore = "requires explicit CD_EXTRA_SOCKETS_GAME and CD_EXTRA_SOCKETS_PROJECT; never edits data"]
fn installed_snapshot_and_stale_write_guard() {
    let game =
        PathBuf::from(std::env::var_os("CD_EXTRA_SOCKETS_GAME").expect("explicit game root"));
    let project =
        PathBuf::from(std::env::var_os("CD_EXTRA_SOCKETS_PROJECT").expect("explicit project root"));
    let data = GameData::open(&game, "ger").unwrap();
    let path = game.join("bin64/CrimsonExtraSockets.dat");
    let before = fs::read(&path).unwrap();
    let snap = extra_sockets::snapshot(&data).unwrap();
    assert_eq!(snap.revision, hash_bytes(&before));
    assert_eq!(snap.records.len(), 3);
    assert_eq!(
        snap.records
            .iter()
            .map(|r| r.baseline + r.additional)
            .collect::<Vec<_>>(),
        [10, 11, 15]
    );
    assert!(
        snap.records
            .iter()
            .all(|r| r.gems.len() == 10 && !r.name.is_empty())
    );
    let request = Request {
        revision: "stale".into(),
        uid: "1003664".into(),
        index: 9,
        gem_key: 1002787,
    };
    assert!(extra_sockets::set(&data, &project, &request).is_err());
    assert_eq!(fs::read(path).unwrap(), before);
    println!(
        "PASS installed snapshot: {} / {} / {}; stale write leaves config unchanged",
        snap.records[0].name, snap.records[1].name, snap.records[2].name
    );
}
#[test]
#[ignore = "requires explicit private fixture and local game root; never edits game/save/config"]
fn own_equipment_candidates() {
    let game = PathBuf::from(std::env::var_os("CD_EXTRA_SOCKETS_GAME").unwrap());
    let fixture = PathBuf::from(std::env::var_os("CD_EXTRA_SOCKETS_FIXTURE").unwrap());
    let before = fs::read(&fixture).unwrap();
    let data = GameData::open(&game, "ger").unwrap();
    let save_root = PathBuf::from(
        std::env::var_os("CD_SAVE_ROOT").expect("explicit save directory for this ignored test"),
    );
    let c = cd_core::extra_sockets_candidates::candidates(&data, &save_root, None).unwrap();
    assert!(!c.items.is_empty());
    assert!(c.save_revision.is_some());
    for (uid, base) in [("1001416", 0), ("1003059", 1), ("1003664", 5)] {
        let item = c.items.iter().find(|i| i.uid == uid).unwrap();
        assert_eq!(item.baseline, base);
        assert!(item.extended && !item.eligible);
    }
    assert!(c.items.iter().any(|i| i.eligible && !i.extended));
    let unique = c
        .items
        .iter()
        .map(|i| &i.uid)
        .collect::<std::collections::BTreeSet<_>>();
    assert_eq!(unique.len(), c.items.len());
    assert_eq!(fs::read(fixture).unwrap(), before);
    println!(
        "PASS {} own equipment instances, {} eligible, no save writes",
        c.items.len(),
        c.items.iter().filter(|i| i.eligible).count()
    );
}
