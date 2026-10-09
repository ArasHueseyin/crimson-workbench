//! Optional real-installation regression check.
//!
//! `.env` is read from the project root; process environment takes precedence.
//! Missing `CD_GAME_DIR` is an explicit skip so CI needs no proprietary fixtures.
//! A configured but missing, changed, unsupported or unreadable installation is
//! a FAILURE, never a skip. This test opens only read APIs: no cache database,
//! extracted files, save bytes, Apply, Restore, game launch or process control.

use std::{collections::BTreeMap, path::Path};

use cd_core::{GameData, config::LocalConfig};

#[test]
fn configured_game_roundtrips_current_tables_items_and_german_localization()
-> Result<(), Box<dyn std::error::Error>> {
    let project = Path::new(env!("CARGO_MANIFEST_DIR"))
        .parent()
        .and_then(Path::parent)
        .expect("cd-core must be located under crates/cd-core");
    let configuration = LocalConfig::load(project)?;
    let Some(game_dir) = configuration.game_dir else {
        eprintln!(
            "SKIP live game roundtrip: CD_GAME_DIR is not configured in the environment or project .env. No game files were opened."
        );
        return Ok(());
    };
    assert!(
        game_dir.is_dir(),
        "CD_GAME_DIR is configured but is not an accessible directory: {}",
        game_dir.display()
    );

    let game = GameData::open(&game_dir, "ger")?;
    let fingerprint = game.fingerprint();
    assert!(
        fingerprint.read_schema_supported,
        "Configured installation does not have a supported read schema: {:?}",
        fingerprint.diagnostics
    );
    let (schema_id, stage_count, quest_count) = match fingerprint.exe_version.as_deref() {
        Some("1.0.0.2944") => ("steam-25381195-gamedata-2.3-v2", 52_080, 1_097),
        Some("1.0.0.2949") => ("steam-25455892-gamedata-2.3-v2", 52_082, 1_098),
        Some("1.0.0.2976") => ("steam-25477059-gamedata-2.3-v2", 52_082, 1_098),
        other => panic!("No build-specific expectations for {other:?}"),
    };
    assert_eq!(fingerprint.schema_id.as_deref(), Some(schema_id));
    assert!(
        !fingerprint.certified_vanilla,
        "Read-only hash matching must not certify vanilla provenance"
    );
    assert_eq!(fingerprint.digest.len(), 64);
    assert!(fingerprint.digest.bytes().all(|b| b.is_ascii_hexdigit()));

    // Item bytes are identical in these admitted builds. Stage/quest additions
    // are checked explicitly so schema acceptance cannot silently drop rows.
    let tables = game.tables();
    assert_eq!(
        tables
            .iter()
            .find(|t| t.name == "stageinfo")
            .unwrap()
            .records,
        stage_count
    );
    assert_eq!(
        tables
            .iter()
            .find(|t| t.name == "questinfo")
            .unwrap()
            .records,
        quest_count
    );
    assert_eq!(
        game.items().len(),
        6_816,
        "All current item records must survive parsing"
    );
    assert_eq!(
        game.tables().len(),
        14,
        "Every tracked body/header pair must be checked"
    );
    let original: BTreeMap<_, _> = game.items().iter().map(|item| (item.key, item)).collect();
    assert_eq!(
        original.len(),
        6_816,
        "Duplicate item keys are not acceptable"
    );

    let localized = game.localized_items()?;
    assert_eq!(localized.len(), original.len());
    let localized_keys: BTreeMap<_, _> = localized.iter().map(|item| (item.key, ())).collect();
    assert_eq!(
        localized_keys.len(),
        original.len(),
        "Localized records must not duplicate keys"
    );
    let mut changed_localizations = 0usize;
    for item in &localized {
        let raw = original
            .get(&item.key)
            .expect("Localized record must refer to a parsed item");
        assert_eq!(item.internal_key, raw.string_key);
        assert_eq!(item.item_type, i64::from(raw.item_type));
        assert_eq!(item.max_stack_count, raw.max_stack_count);
        assert!(
            !item.name.trim().is_empty(),
            "Item {} has neither a localized name nor fallback",
            item.key
        );
        if item.name != raw.name.default && item.name != item.internal_key {
            changed_localizations += 1;
        }
        let detail: serde_json::Value = serde_json::from_str(&item.detail_json)?;
        assert!(
            detail.is_object(),
            "Indexed item detail must be structured JSON"
        );
    }
    assert!(
        changed_localizations > 0,
        "German PALOC must resolve real names instead of returning only default text"
    );

    // The all-fields mode must walk every typed item's ranges without gaps or
    // overlaps, in addition to item/index and PALOC lossless serialization.
    let report = game.roundtrip(true)?;
    assert!(
        report.checks.len() >= 31,
        "Roundtrip report must include every body/header, typed items and both PALOC layers"
    );
    assert_eq!(
        report.tracked_items, 6_816,
        "Every item must pass tracked-field coverage"
    );
    assert!(
        report.checks.iter().all(|check| check.byte_identical),
        "No individual roundtrip failure may be hidden by the report summary"
    );
    assert!(
        report.all_passed,
        "Live roundtrip failure: {}",
        serde_json::to_string_pretty(&report)?
    );
    let first = game.items().first().expect("Current table is nonempty");
    assert!(game.item_detail(first.key, true)?.is_object());
    assert!(
        game.item_detail(u32::MAX, true).is_err(),
        "Missing item key must return an error"
    );
    eprintln!(
        "LIVE PASS: {} items, {} table pairs, German localization and all-field roundtrips; source data read-only.",
        game.items().len(),
        game.tables().len()
    );
    Ok(())
}
