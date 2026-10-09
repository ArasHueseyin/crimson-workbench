//! Read-only source access; outputs go only to a fresh OS temporary directory.
use cd_core::{
    Workspace,
    config::LocalConfig,
    mods::{ModCatalog, ModRequest, VendorOptions},
    tables::{self, IndexedTable},
};
use std::{collections::BTreeMap, path::Path};
#[test]
fn actual_mod_tables_roundtrip_and_selected_changes_preserve_other_records()
-> Result<(), Box<dyn std::error::Error>> {
    let project = Path::new(env!("CARGO_MANIFEST_DIR"))
        .parent()
        .unwrap()
        .parent()
        .unwrap();
    if LocalConfig::load(project)?.game_dir.is_none() {
        eprintln!("SKIP mods_live: CD_GAME_DIR not configured");
        return Ok(());
    }
    let w = Workspace::open(project, None, "ger")?;
    let c = ModCatalog::open(w.data())?;
    let info = c.info();
    assert_eq!(
        (
            info.store_rows,
            info.vendors.len(),
            info.stock_rows,
            info.trust_rows,
            info.dropsets.len(),
            info.quantity_excluded
        ),
        (436, 397, 6376, 3, 12736, 299)
    );
    let noop = c.build(ModRequest::default())?;
    assert!(noop.files.is_empty());
    assert!(noop.preview.changes.is_empty());
    assert!(!noop.preview.gates.can_apply);
    assert_eq!(info.append_vendors.len(), 208);
    assert_eq!(info.daily_vendors.len(), 369);
    assert_eq!(info.chance_dropsets.len(), 12306);
    assert_eq!(info.guarantee_dropsets.len(), 12306);
    assert_eq!(info.items.len(), 6816);
    let daily = c.build(ModRequest {
        daily_refresh: true,
        ..Default::default()
    })?;
    assert_eq!(daily.preview.changes.len(), 246);
    assert!(daily.preview.changes.iter().all(|c| c.field == "reset_days"
        && c.after == "1"
        && ["3", "7"].contains(&c.before.as_str())));
    let global = c.build(ModRequest {
        quantity_multiplier: 2,
        ..Default::default()
    })?;
    let chosen = global
        .preview
        .changes
        .iter()
        .find(|c| c.module == "drops")
        .unwrap()
        .key;
    let request: ModRequest = serde_json::from_value(
        serde_json::json!({"shop_stock":999,"vendors":[3101],"quantity_multiplier":2,"dropsets":[chosen],"trust_multiplier":3}),
    )?;
    let built = c.build(request.clone())?;
    assert_eq!(built.preview.plan_id, c.build(request)?.preview.plan_id);
    assert!(
        built
            .preview
            .changes
            .iter()
            .filter(|c| c.module == "shops")
            .all(|c| c.key == 3101 && c.after == "999")
    );
    assert!(built.preview.changes.iter().any(|c| c.module == "drops"));
    assert!(
        built
            .preview
            .changes
            .iter()
            .filter(|c| c.module == "drops")
            .all(|c| c.key == chosen)
    );
    assert_eq!(
        built
            .preview
            .changes
            .iter()
            .filter(|c| c.module == "trust")
            .count(),
        4
    );
    assert!(!built.preview.changes.iter().any(|c| c.key == 9500003));
    let temp = tempfile::tempdir()?;
    for (p, b) in &built.files {
        let path = temp.path().join(p);
        std::fs::create_dir_all(path.parent().unwrap())?;
        std::fs::write(path, b)?;
    }
    let overlay = crimson_format::Archive::open(temp.path())?;
    let group = overlay.groups().first().unwrap();
    let generated: BTreeMap<_, _> = overlay
        .list_group(&group.name)?
        .iter()
        .map(|e| Ok((e.name.clone(), overlay.extract(e)?)))
        .collect::<Result<_, std::io::Error>>()?;
    let original = crimson_format::Archive::open(w.data().game_root())?;
    let entries = original.list_group("0008")?;
    for name in ["storeinfo", "dropsetinfo"] {
        let body_name = format!("{name}.staticinfobody");
        let header_name = format!("{name}.staticinfoheader");
        let before = original.extract(entries.iter().find(|e| e.name == body_name).unwrap())?;
        let header = original.extract(entries.iter().find(|e| e.name == header_name).unwrap())?;
        assert_eq!(
            generated[&header_name], header,
            "fixed-width edits retain exact index bytes"
        );
        let left = IndexedTable::parse(tables::schema(name).unwrap(), &before, &header)?;
        let right = IndexedTable::parse(
            tables::schema(name).unwrap(),
            &generated[&body_name],
            &generated[&header_name],
        )?;
        for r in left.records() {
            let allowed = if name == "storeinfo" {
                r.key == 3101
            } else {
                [chosen, 9500001, 9500002].contains(&r.key)
            };
            if !allowed {
                assert_eq!(
                    left.record_bytes(r.key),
                    right.record_bytes(r.key),
                    "{name}/{} changed unexpectedly",
                    r.key
                );
            }
            if name == "dropsetinfo" && [9500001, 9500002, 9500003].contains(&r.key) {
                let before = left.record_bytes(r.key).unwrap();
                let after = right.record_bytes(r.key).unwrap();
                let name_len = u32::from_le_bytes(before[4..8].try_into().unwrap()) as usize;
                for delta in [42, 50] {
                    let at = 8 + name_len + 18 + delta;
                    let value = |b: &[u8]| i64::from_le_bytes(b[at..at + 8].try_into().unwrap());
                    let expected = match r.key {
                        9500001 => 50,
                        9500002 => 5,
                        _ => -200,
                    };
                    assert_eq!(value(before), expected);
                    assert_eq!(
                        value(after),
                        if expected > 0 { expected * 3 } else { expected }
                    );
                }
            }
        }
    }
    assert!(
        c.build(serde_json::from_value(
            serde_json::json!({"dropsets":[100000],"quantity_multiplier":2})
        )?)
        .is_err()
    );
    assert!(serde_json::from_str::<ModRequest>(r#"{"unrecognized_option":2}"#).is_err());
    assert!(
        c.build(serde_json::from_value(
            serde_json::json!({"trust_multiplier":1001})
        )?)
        .is_err()
    );
    // Real growth: append a known item to one merchant, rebuild the header, and
    // independently inspect the entire result rather than trusting the preview.
    let grow_request = ModRequest {
        vendors: [3101].into(),
        shop_items: [2200, 50001].into(),
        daily_refresh: true,
        ..Default::default()
    };
    let grow = c.build(grow_request.clone())?;
    assert_eq!(grow.preview.plan_id, c.build(grow_request)?.preview.plan_id);
    let generated = unpack(&grow.files)?;
    let body_name = "storeinfo.staticinfobody";
    let header_name = "storeinfo.staticinfoheader";
    let before = original.extract(entries.iter().find(|e| e.name == body_name).unwrap())?;
    let header = original.extract(entries.iter().find(|e| e.name == header_name).unwrap())?;
    let left = IndexedTable::parse(tables::schema("storeinfo").unwrap(), &before, &header)?;
    let right = IndexedTable::parse(
        tables::schema("storeinfo").unwrap(),
        &generated[body_name],
        &generated[header_name],
    )?;
    let old = left.record_bytes(3101).unwrap();
    let new = right.record_bytes(3101).unwrap();
    let added = grow
        .preview
        .changes
        .iter()
        .filter(|c| c.field.ends_with(".item"))
        .count();
    assert!(added > 0);
    assert_eq!(new.len() - old.len(), added * 127);
    assert_ne!(header, generated[header_name]);
    for record in left.records() {
        if record.key != 3101 {
            assert_eq!(
                left.record_bytes(record.key),
                right.record_bytes(record.key)
            );
        }
    }
    let all_items = c.build(ModRequest {
        vendors: [3101].into(),
        shop_items: info.items.iter().map(|i| i.key).collect(),
        ..Default::default()
    })?;
    let all_generated = unpack(&all_items.files)?;
    let all_table = IndexedTable::parse(
        tables::schema("storeinfo").unwrap(),
        &all_generated[body_name],
        &all_generated[header_name],
    )?;
    let all_record = all_table.record_bytes(3101).unwrap();
    let new_items = all_items
        .preview
        .changes
        .iter()
        .filter(|c| c.field.ends_with(".item"))
        .count();
    assert!(new_items > 6700);
    assert_eq!(all_record.len() - old.len(), new_items * 127);
    for record in left.records() {
        if record.key != 3101 {
            assert_eq!(
                left.record_bytes(record.key),
                all_table.record_bytes(record.key)
            );
        }
    }
    // Per-vendor options can suppress globals and opt another vendor in.
    let targets: Vec<_> = daily
        .preview
        .changes
        .iter()
        .filter(|c| info.append_vendors.contains(&c.key))
        .take(2)
        .map(|c| c.key)
        .collect();
    assert_eq!(targets.len(), 2);
    let custom = c.build(ModRequest {
        vendors: [targets[0]].into(),
        shop_items: [2200, 50001].into(),
        daily_refresh: true,
        vendor_options: [
            (
                targets[0],
                VendorOptions {
                    items: Some(Default::default()),
                    daily_refresh: Some(false),
                },
            ),
            (
                targets[1],
                VendorOptions {
                    items: Some([2200, 50001].into()),
                    daily_refresh: Some(true),
                },
            ),
        ]
        .into(),
        ..Default::default()
    })?;
    assert!(
        custom
            .preview
            .changes
            .iter()
            .all(|change| change.key == targets[1])
    );
    assert!(
        custom
            .preview
            .changes
            .iter()
            .any(|change| change.field == "reset_days" && change.after == "1")
    );
    assert!(
        custom
            .preview
            .changes
            .iter()
            .any(|change| change.field.ends_with(".item"))
    );
    let custom_files = unpack(&custom.files)?;
    let custom_table = IndexedTable::parse(
        tables::schema("storeinfo").unwrap(),
        &custom_files[body_name],
        &custom_files[header_name],
    )?;
    for record in left.records() {
        if record.key != targets[1] {
            assert_eq!(
                left.record_bytes(record.key),
                custom_table.record_bytes(record.key)
            );
        }
    }
    for (key, item) in [(u32::MAX, 2200), (6, 2200), (3101, u32::MAX)] {
        assert!(
            c.build(ModRequest {
                vendor_options: [(
                    key,
                    VendorOptions {
                        items: Some([item].into()),
                        daily_refresh: None
                    }
                )]
                .into(),
                ..Default::default()
            })
            .is_err()
        );
    }
    // Independent-roll Goblin entry: 35,000 / 1,000,000 -> 70,000; scaling
    // quantities and chances together preserves the record's roll semantics.
    let chance = c.build(ModRequest {
        chance_multiplier: 2,
        quantity_multiplier: 3,
        dropsets: [175521].into(),
        ..Default::default()
    })?;
    let generated = unpack(&chance.files)?;
    let table = IndexedTable::parse(
        tables::schema("dropsetinfo").unwrap(),
        &generated["dropsetinfo.staticinfobody"],
        &generated["dropsetinfo.staticinfoheader"],
    )?;
    let row = table.record_bytes(175521).unwrap();
    let start = 8 + u32::from_le_bytes(row[4..8].try_into()?) as usize + 18;
    assert_eq!(
        u64::from_le_bytes(row[start + 26..start + 34].try_into()?),
        70000
    );
    assert_eq!(
        u64::from_le_bytes(row[start + 42..start + 50].try_into()?),
        3
    );
    let guaranteed = c.build(ModRequest {
        guaranteed_dropsets: [175521].into(),
        chance_overrides: [(175521, 0)].into(),
        ..Default::default()
    })?;
    assert!(
        guaranteed
            .preview
            .changes
            .iter()
            .any(|c| c.field.ends_with("chance_per_million") && c.after == "1000000")
    );
    assert!(
        c.build(ModRequest {
            guaranteed_dropsets: [175521].into(),
            quantity_multiplier: 0,
            ..Default::default()
        })
        .is_err()
    );
    assert!(
        c.build(ModRequest {
            guaranteed_dropsets: [175199].into(),
            ..Default::default()
        })
        .is_err()
    );
    assert!(
        c.build(ModRequest {
            chance_overrides: [(175199, 2)].into(),
            ..Default::default()
        })
        .is_err()
    );
    assert!(
        c.build(ModRequest {
            chance_multiplier: 2,
            dropsets: [175199].into(),
            ..Default::default()
        })
        .is_err()
    );
    assert!(
        c.build(ModRequest {
            vendors: [6].into(),
            shop_items: [2200].into(),
            ..Default::default()
        })
        .is_err()
    );
    assert!(
        c.build(ModRequest {
            shop_items: [u32::MAX].into(),
            ..Default::default()
        })
        .is_err()
    );
    assert!(
        c.build(ModRequest {
            shop_items: info.items.iter().map(|i| i.key).collect(),
            ..Default::default()
        })
        .is_err()
    );
    let advanced = cd_core::mods::advanced::AdvancedRequest {
        spawn_percent: 300,
        patrol_reset_percent: Some(50),
        reoccupation_delay_percent: Some(50),
        dragon_no_cooldown: true,
        dragon_duration: Some(1800),
        dragon_regions: true,
        town_running: true,
        no_wear: true,
        stack_size: Some(999),
        skill_cooldown_percent: 0,
        skill_buffs: BTreeMap::from([(
            30001,
            BTreeMap::from([("0/0/common.mem_24".into(), "123".into())]),
        )]),
        cost_percent: BTreeMap::from([
            ("stamina:climbing".into(), 0),
            ("spirit:jumping".into(), 50),
        ]),
        fields: BTreeMap::from([
            ("inventory/2/defaultSlotCount".into(), "300".into()),
            ("inventory/2/maxSlotCount".into(), "300".into()),
        ]),
        ..Default::default()
    };
    let built = c.build(ModRequest {
        advanced: Some(advanced),
        ..Default::default()
    })?;
    let payload = unpack(&built.files)?;
    for name in [
        "characterinfo",
        "regioninfo",
        "fieldinfo",
        "inventory",
        "iteminfo",
        "equiptypeinfo",
        "skill",
        "terrainregionautospawninfo",
        "spawningpoolautospawninfo",
        "stageinfo",
        "conditioninfo",
        "factionreblockadinginfo",
    ] {
        assert!(
            payload.contains_key(&format!("{name}.staticinfobody")),
            "{name}"
        );
        assert!(
            payload.contains_key(&format!("{name}.staticinfoheader")),
            "{name}"
        );
    }
    let stages = IndexedTable::parse(
        tables::schema("stageinfo").unwrap(),
        &payload["stageinfo.staticinfobody"],
        &payload["stageinfo.staticinfoheader"],
    )?;
    let expected_stage_count = match w.data().fingerprint().exe_version.as_deref() {
        Some("1.0.0.2944") => 52_080,
        Some("1.0.0.2949") => 52_082,
        version => panic!("unverified stage build: {version:?}"),
    };
    let stage_body = original.extract(
        entries
            .iter()
            .find(|entry| entry.name == "stageinfo.staticinfobody")
            .unwrap(),
    )?;
    let stage_header = original.extract(
        entries
            .iter()
            .find(|entry| entry.name == "stageinfo.staticinfoheader")
            .unwrap(),
    )?;
    let original_stages = IndexedTable::parse(
        tables::schema("stageinfo").unwrap(),
        &stage_body,
        &stage_header,
    )?;
    assert_eq!(stages.records().len(), expected_stage_count);
    assert_eq!(original_stages.records().len(), expected_stage_count);
    let after_by_key: BTreeMap<_, _> = stages
        .records()
        .iter()
        .map(|record| {
            let start = record.offset as usize;
            (
                record.key,
                &payload["stageinfo.staticinfobody"][start..start + record.length],
            )
        })
        .collect();
    let mut changed_stages = Vec::new();
    for record in original_stages.records() {
        let after = after_by_key.get(&record.key).expect("stage preserved");
        let start = record.offset as usize;
        if *after != &stage_body[start..start + record.length] {
            changed_stages.push(record.key);
        }
    }
    changed_stages.sort_unstable();
    assert_eq!(changed_stages, [1002224, 1017811]);
    if expected_stage_count == 52_082 {
        for key in [1001269, 1003266] {
            assert!(original_stages.record_bytes(key).is_some());
            assert_eq!(stages.record_bytes(key), original_stages.record_bytes(key));
        }
    }
    assert!(!payload.contains_key("questinfo.staticinfobody"));
    assert!(!payload.contains_key("questinfo.staticinfoheader"));
    assert_eq!(
        built
            .preview
            .changes
            .iter()
            .filter(|c| c.table == "conditioninfo" && c.key == 1011130 && c.after == "!CheckNone()")
            .count(),
        1
    );
    assert_eq!(
        built
            .preview
            .changes
            .iter()
            .filter(|c| c.table == "stageinfo" && c.field == "resetSecond" && c.after == "129600")
            .count(),
        2
    );
    assert_eq!(
        built
            .preview
            .changes
            .iter()
            .filter(|c| c.table == "factionreblockadinginfo"
                && c.field == "delayTime"
                && ["43200", "216000"].contains(&c.after.as_str()))
            .count(),
        108
    );
    let skills = IndexedTable::parse(
        tables::schema("skill").unwrap(),
        &payload["skill.staticinfobody"],
        &payload["skill.staticinfoheader"],
    )?;
    let detail =
        cd_core::mods::advanced::skill_detail::inspect(skills.record_bytes(30001).unwrap())?;
    assert_eq!(
        detail.buffs[0]
            .fields
            .iter()
            .find(|f| f.path == "common.mem_24")
            .unwrap()
            .value,
        "123"
    );
    assert!(
        built
            .preview
            .changes
            .iter()
            .any(|c| c.table == "characterinfo"
                && c.key == 1000799
                && c.field == "callMercenaryCoolTime"
                && c.before == "3600"
                && c.after == "0")
    );
    assert!(
        !built
            .preview
            .changes
            .iter()
            .any(|c| c.table == "characterinfo" && c.key == 60003)
    );
    let items = crimson_format::ItemTable::parse(
        &payload["iteminfo.staticinfobody"],
        &payload["iteminfo.staticinfoheader"],
    )?;
    assert_eq!(items.entries().len(), 6816);
    assert!(
        built
            .preview
            .changes
            .iter()
            .any(|c| c.table == "iteminfo" && c.field == "max_stack_count")
    );
    assert!(
        c.build(ModRequest {
            advanced: Some(cd_core::mods::advanced::AdvancedRequest {
                free_repair: true,
                ..Default::default()
            }),
            ..Default::default()
        })
        .is_err()
    );
    Ok(())
}
fn unpack(
    files: &BTreeMap<String, Vec<u8>>,
) -> Result<BTreeMap<String, Vec<u8>>, Box<dyn std::error::Error>> {
    let temp = tempfile::tempdir()?;
    for (p, b) in files {
        let path = temp.path().join(p);
        std::fs::create_dir_all(path.parent().unwrap())?;
        std::fs::write(path, b)?;
    }
    let archive = crimson_format::Archive::open(temp.path())?;
    let group = &archive.groups()[0].name;
    Ok(archive
        .list_group(group)?
        .iter()
        .map(|e| Ok((e.name.clone(), archive.extract(e)?)))
        .collect::<Result<_, std::io::Error>>()?)
}
