//! Reads real game data, but writes only a cache/export in an isolated temp project.
use cd_core::{
    browser::{BrowserQuery, BrowserSession, BrowserSort},
    config::LocalConfig,
};
#[test]
fn desktop_catalog_uses_real_fts_icons_fields_and_guarded_exports()
-> Result<(), Box<dyn std::error::Error>> {
    let project = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
        .parent()
        .unwrap()
        .parent()
        .unwrap();
    let Some(game) = LocalConfig::load(project)?.game_dir else {
        eprintln!("SKIP desktop catalog: CD_GAME_DIR not configured");
        return Ok(());
    };
    let temp = tempfile::tempdir()?;
    let session = BrowserSession::open(temp.path(), Some(&game), "ger")?;
    let mut portraits = 0;
    let mut examples = 0;
    for mount in session.mounts()? {
        let icon = session.mount_icon(mount.key)?;
        if icon.data_url.is_some() {
            if let Some(caption) = icon.caption {
                examples += 1;
                assert!(caption.contains("Variante"));
                assert!(icon.source.unwrap().starts_with("ui/texture/image/"));
            } else {
                assert!(
                    icon.source
                        .unwrap()
                        .to_ascii_lowercase()
                        .ends_with(&format!("{}.dds", mount.internal.to_ascii_lowercase()))
                );
            }
            portraits += 1;
        }
    }
    assert!(
        portraits > 250 && examples > 100,
        "Matched and decoded mount portraits: {portraits}"
    );
    assert!(session.mount_icon(u32::MAX).is_err());
    assert_eq!(portraits - examples, 89);
    assert_eq!(examples, 210);
    // The same texture is used for exact and illustrative portraits. Captions
    // must stay per request even after the shared texture cache is populated.
    assert!(session.mount_icon(2002)?.caption.is_none());
    assert!(session.mount_icon(1003120)?.caption.is_some());
    assert!(session.mount_icon(2002)?.caption.is_none());
    println!("Real mount portraits decoded: {portraits}; labelled examples: {examples}");
    assert_eq!(session.info().item_count, 6816);
    for key in [1001314, 1001315, 1001316] {
        let detail = session.item(key)?;
        assert!(
            detail["fields"]
                .as_array()
                .unwrap()
                .iter()
                .any(|f| f["path"] == "item_memo"
                    && f["value"].as_str().is_some_and(|s| s.contains("몬스터용")))
        );
        assert!(
            session
                .validate_item_grant(key)
                .unwrap_err()
                .to_string()
                .contains("Monstermunition")
        );
    }
    for key in [50001, 50003, 1001321] {
        session.validate_item_grant(key)?;
    }
    assert!(session.validate_item_grant(u32::MAX).is_err());
    let page = session.search(BrowserQuery {
        text: "Stumpfpfeil".into(),
        ..Default::default()
    })?;
    assert!(page.items.iter().any(|i| i.key == 2200));
    let arrows = session.search(BrowserQuery {
        text: "pfeil".into(),
        ..Default::default()
    })?;
    assert!(
        arrows.items.iter().any(|i| i.key == 1001315),
        "Part-word search must find Blitzpfeil"
    );
    let arrows_regex = session.search(BrowserQuery {
        text: "(?i)blitz.*pfeil".into(),
        regex: true,
        ..Default::default()
    })?;
    assert!(arrows_regex.items.iter().any(|i| i.key == 1001315));
    assert!(
        session
            .search(BrowserQuery {
                text: "[".into(),
                regex: true,
                ..Default::default()
            })
            .is_err()
    );
    let detail = session.item(2200)?;
    assert_eq!(detail["name"], "Stumpfpfeil");
    assert!(detail["fields"].as_array().unwrap().len() > 100);
    assert_eq!(detail["fields"][0]["type_name"], "ItemKey");
    assert_eq!(detail["fields"][0]["value"], 2200);
    let icon = session.icon(2200)?;
    assert!(
        icon.data_url
            .as_deref()
            .is_some_and(|s| s.starts_with("data:image/png;base64,iVBOR")),
        "{icon:?}"
    );
    let info = session.info();
    assert!(info.group_error.is_none(), "{:?}", info.group_error);
    eprintln!(
        "CATEGORIES: {:?}",
        info.groups
            .iter()
            .filter(|g| g.order <= 5)
            .map(|g| &g.name)
            .collect::<Vec<_>>()
    );
    let swords = info
        .groups
        .iter()
        .find(|g| g.internal_key == "ItemGroup_SubCategory_Equip_Weapon_TwoHand")
        .ok_or("Missing real two-hand display group")?;
    assert!(!swords.name.is_empty());
    let sword_page = session.search(BrowserQuery {
        group: Some(swords.key),
        ..Default::default()
    })?;
    assert!(
        sword_page.total > 0
            && sword_page
                .items
                .iter()
                .all(|i| swords.items.contains(&i.key))
    );
    let matching = &sword_page.items[0];
    let intersection = session.search(BrowserQuery {
        group: Some(swords.key),
        text: matching.internal_key.clone(),
        ..Default::default()
    })?;
    assert!(intersection.items.iter().any(|i| i.key == matching.key));
    let exact_id = session.search(BrowserQuery {
        group: Some(swords.key),
        text: matching.key.to_string(),
        ..Default::default()
    })?;
    assert!(exact_id.items.iter().any(|i| i.key == matching.key));
    session.with_mods(|c| {
        assert!(!c.info().advanced.statuses.is_empty());
        c.advanced_item(matching.key)?;
        Ok(())
    })?;
    assert!(
        session
            .search(BrowserQuery {
                group: Some(u32::MAX),
                ..Default::default()
            })
            .is_err()
    );
    let stat = info.stats.first().ok_or("No real stat references")?.value;
    let stats = session.search(BrowserQuery {
        stat_key: Some(stat),
        sort: BrowserSort::Stack,
        descending: true,
        ..Default::default()
    })?;
    assert!(stats.total > 0 && stats.items.iter().all(|i| i.stat_keys.contains(&stat)));
    let first = session.search(BrowserQuery::default())?;
    let second = session.search(BrowserQuery {
        offset: 200,
        ..Default::default()
    })?;
    assert_eq!(first.total, 6816);
    assert_eq!(first.items.len(), 200);
    assert_eq!(second.items.len(), 200);
    assert!(
        !first
            .items
            .iter()
            .any(|a| second.items.iter().any(|b| a.key == b.key))
    );
    let export = session.export_item(2200)?;
    assert!(export.starts_with(temp.path().canonicalize()?.join("exports")));
    let read: serde_json::Value = serde_json::from_slice(&std::fs::read(export)?)?;
    assert_eq!(read, detail);
    assert!(session.item(u32::MAX).is_err());
    eprintln!(
        "LIVE DESKTOP PASS: 6816 items, FTS5, 31812 string records roundtripped, real DDS icon, stat filter, pagination, raw fields and guarded JSON export"
    );
    Ok(())
}
