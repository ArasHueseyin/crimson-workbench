#![cfg(target_os = "macos")]
use cd_core::{GameData, browser::BrowserSession};
use std::path::PathBuf;

#[test]
#[ignore = "explicit read-only test against CD_MAC_GAME_DIR; writes only a temporary search cache"]
fn native_mac_catalog_and_crafting_use_verified_data() {
    let game =
        PathBuf::from(std::env::var_os("CD_MAC_GAME_DIR").expect("native Mac packages directory"));
    let data = GameData::open(&game, "eng").unwrap();
    assert_eq!(data.items().len(), 6816);
    assert!(data.fingerprint().read_schema_supported);
    assert!(!data.fingerprint().metadata_matches);
    assert!(data.fingerprint().exe_version.is_none());
    assert!(cd_core::apply::game_running().is_err());
    let project = tempfile::tempdir().unwrap();
    let browser = BrowserSession::open(project.path(), Some(&game), "eng").unwrap();
    assert_eq!(browser.info().item_count, 6816);
    let crafting = browser.with_crafting(|c| Ok(c.info())).unwrap();
    assert_eq!(crafting.coverage.usable_recipes, 1108);
}
