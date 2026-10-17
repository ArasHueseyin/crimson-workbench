//! Opt-in, read-only verification against the user's pinned original archive.
//! Rebuilds plans in memory; never installs overlays or writes game/save files.
use cd_core::{
    GameData,
    mods::{ModCatalog, ModRequest, marketplace::Selection},
};
use std::{collections::BTreeMap, path::PathBuf};

#[test]
#[ignore = "requires CD_GAME_DIR; reads checked original item/localization archives only"]
fn selected_consumable_catalog_resolves_every_item_in_english_and_german()
-> Result<(), Box<dyn std::error::Error>> {
    use cd_core::config::preserve_consumables::ITEMS;
    let game = PathBuf::from(std::env::var_os("CD_GAME_DIR").expect("private game root required"));
    let registry_before = std::fs::read(game.join("meta/0.papgt"))?;
    let mut english = BTreeMap::new();
    for language in ["eng", "ger"] {
        let data = GameData::open(&game, language)?;
        for (key, kind) in ITEMS {
            let item = data.item_detail(key, false)?;
            let name = item["name"]
                .as_str()
                .expect("browser requires a string name");
            assert!(
                !name.trim().is_empty(),
                "{language}: empty {kind} item {key}"
            );
            if language == "eng" {
                english.insert(key, name.to_owned());
            }
        }
        println!(
            "{language}: all {} selected consumables have display names",
            ITEMS.len()
        );
        if language == "ger" {
            let translated = ITEMS
                .iter()
                .filter(|(key, _)| {
                    let item = data.item_detail(*key, false).unwrap();
                    item["name"].as_str().unwrap() != english[key]
                })
                .count();
            assert!(
                translated > 0,
                "German catalog must use its own localization"
            );
            println!(
                "{translated} consumable names differ between the checked English and German catalogs"
            );
        }
    }
    assert_eq!(std::fs::read(game.join("meta/0.papgt"))?, registry_before);
    Ok(())
}

#[test]
#[ignore = "requires CD_GAME_DIR; original archives read only, generated overlays in temporary fixtures"]
fn crafting_currency_component_builds_reversibly_without_changing_other_recipes()
-> Result<(), Box<dyn std::error::Error>> {
    use crimson_format::{Archive, RawTable, overlay};
    let game = PathBuf::from(std::env::var_os("CD_GAME_DIR").expect("private game root required"));
    let data = GameData::open(&game, "eng")?;
    let catalog = ModCatalog::open(&data)?;
    let build = |currency, inventory| {
        catalog.build(ModRequest {
            marketplace: Some(Selection {
                free_crafting_currency: currency,
                inventory_999: inventory,
                ..Default::default()
            }),
            ..Default::default()
        })
    };
    let selected = build(true, false)?;
    assert_eq!(selected.preview.changes.len(), 1);
    assert_eq!(selected.preview.changes[0].table, "multichangeinfo");
    assert_eq!(selected.preview.changes[0].key, 1010292);
    assert_eq!(selected.preview.changes[0].after, "0");
    assert_eq!(
        selected.preview.plan_id,
        catalog
            .build(selected.preview.request.clone())?
            .preview
            .plan_id
    );
    let temp = tempfile::tempdir()?;
    for (name, bytes) in &selected.files {
        let destination = temp.path().join(name);
        std::fs::create_dir_all(destination.parent().unwrap())?;
        std::fs::write(destination, bytes)?;
    }
    let staged = Archive::open(temp.path())?;
    let group = selected
        .files
        .keys()
        .find(|p| p.ends_with("/0.pamt"))
        .unwrap()
        .split('/')
        .next()
        .unwrap();
    let entries = staged.list_group(group)?;
    let read = |name: &str| staged.extract(entries.iter().find(|e| e.name == name).unwrap());
    let body = read("multichangeinfo.staticinfobody")?;
    let header = read("multichangeinfo.staticinfoheader")?;
    let original = Archive::with_registry(
        &game,
        std::fs::read(game.join(".workbench/baseline.papgt"))?,
    )?;
    let originals = original.list_group("0008")?;
    let read_original =
        |name: &str| original.extract(originals.iter().find(|e| e.name == name).unwrap());
    let before_body = read_original("multichangeinfo.staticinfobody")?;
    let before_header = read_original("multichangeinfo.staticinfoheader")?;
    let before = RawTable::parse(&before_body, &before_header, 4, 2)?;
    let after = RawTable::parse(&body, &header, 4, 2)?;
    assert_eq!(before.records().len(), after.records().len());
    for row in before.records() {
        if row.key != 1010292 {
            assert_eq!(before.record_bytes(row.key)?, after.record_bytes(row.key)?);
        } else {
            assert_eq!(
                before.record_bytes(row.key)?.len() - after.record_bytes(row.key)?.len(),
                30
            );
        }
    }
    assert_eq!(entries.len(), 2);
    assert!(build(false, false)?.files.is_empty());
    for currency in [true, false] {
        let combined = build(currency, true)?;
        let (name, pamt) = combined
            .files
            .iter()
            .find(|(name, _)| name.ends_with("/0.pamt"))
            .unwrap();
        let paths = overlay::validate_pair(pamt, &combined.files[&name.replace(".pamt", ".paz")])?;
        assert!(
            paths
                .iter()
                .any(|p| p.ends_with("inventory.staticinfobody"))
        );
        assert_eq!(
            paths
                .iter()
                .any(|p| p.ends_with("multichangeinfo.staticinfobody")),
            currency
        );
    }
    println!(
        "Currency component built, composed with inventory and removed from a new plan; all other 18,575 recipes and game/save files unchanged."
    );
    Ok(())
}

#[test]
#[ignore = "requires CD_GAME_DIR; original archives read only, generated overlays in temporary fixtures"]
fn free_upgrades_cover_every_upgrade_and_keep_crafting_currency_independent()
-> Result<(), Box<dyn std::error::Error>> {
    use crimson_format::{Archive, RawTable};
    let game = PathBuf::from(std::env::var_os("CD_GAME_DIR").expect("private game root required"));
    let data = GameData::open(&game, "eng")?;
    let catalog = ModCatalog::open(&data)?;
    let original = Archive::with_registry(
        &game,
        std::fs::read(game.join(".workbench/baseline.papgt"))?,
    )?;
    let originals = original.list_group("0008")?;
    let source = |name: &str| original.extract(originals.iter().find(|e| e.name == name).unwrap());
    let before_body = source("multichangeinfo.staticinfobody")?;
    let before_header = source("multichangeinfo.staticinfoheader")?;
    let before = RawTable::parse(&before_body, &before_header, 4, 2)?;
    let mut isolated: Option<(Vec<u8>, Vec<u8>)> = None;
    for currency in [false, true] {
        let built = catalog.build(ModRequest {
            marketplace: Some(Selection {
                free_upgrade_materials: true,
                free_crafting_currency: currency,
                ..Default::default()
            }),
            ..Default::default()
        })?;
        assert_eq!(built.preview.changes.len(), if currency { 2 } else { 1 });
        assert!(
            built
                .preview
                .changes
                .iter()
                .any(|c| c.field.contains("17,069"))
        );
        let temp = tempfile::tempdir()?;
        for (path, bytes) in &built.files {
            let destination = temp.path().join(path);
            std::fs::create_dir_all(destination.parent().unwrap())?;
            std::fs::write(destination, bytes)?;
        }
        let group = built
            .files
            .keys()
            .find(|p| p.ends_with("/0.pamt"))
            .unwrap()
            .split('/')
            .next()
            .unwrap();
        let staged = Archive::open(temp.path())?;
        let entries = staged.list_group(group)?;
        assert_eq!(entries.len(), 2);
        let read = |name: &str| staged.extract(entries.iter().find(|e| e.name == name).unwrap());
        let after_body = read("multichangeinfo.staticinfobody")?;
        let after_header = read("multichangeinfo.staticinfoheader")?;
        let after = RawTable::parse(&after_body, &after_header, 4, 2)?;
        let changed = before
            .records()
            .iter()
            .filter(|r| before.record_bytes(r.key).unwrap() != after.record_bytes(r.key).unwrap())
            .count();
        assert_eq!(changed, if currency { 17070 } else { 17069 });
        if currency {
            let (isolated_body, isolated_header) = isolated.as_ref().unwrap();
            let material_only = RawTable::parse(isolated_body, isolated_header, 4, 2)?;
            for row in before.records() {
                if row.key != 1010292 {
                    assert_eq!(
                        after.record_bytes(row.key)?,
                        material_only.record_bytes(row.key)?
                    );
                }
            }
        } else {
            assert_eq!(after.record_bytes(1010292)?, before.record_bytes(1010292)?);
            isolated = Some((after_body, after_header));
        }
        assert_eq!(
            built.preview.plan_id,
            catalog
                .build(built.preview.request.clone())?
                .preview
                .plan_id
        );
    }
    assert!(catalog.build(ModRequest::default())?.files.is_empty());
    println!(
        "All 17,069 upgrade recipes rebuilt; only 17,069 or 17,070 selected records changed. Currency toggle and removal remain independent. No game/save writes."
    );
    Ok(())
}

#[test]
#[ignore = "requires CD_GAME_DIR; read-only original tables, outputs in temporary fixtures only"]
fn native_weapon_effects_preserve_other_bytes_and_require_explicit_replacement()
-> Result<(), Box<dyn std::error::Error>> {
    use cd_core::mods::{advanced::AdvancedRequest, marketplace::WeaponEffectEdit};
    use crimson_format::{
        Archive, ItemTable, RawTable,
        item_mods::{self, ItemEdit},
    };
    let game = PathBuf::from(std::env::var_os("CD_GAME_DIR").expect("private game root required"));
    let data = GameData::open(&game, "eng")?;
    let catalog = ModCatalog::open(&data)?;
    let choices = catalog.marketplace_weapon_effects()?;
    assert_eq!(choices.len(), 2);
    let lightning = choices.iter().find(|s| s.source == 1001062).unwrap();
    let fire = choices.iter().find(|s| s.source == 1000451).unwrap();
    assert_eq!((lightning.skill, lightning.level), (91101, 3));
    assert_eq!((fire.skill, fire.level), (91105, 3));
    let sword = lightning
        .targets
        .iter()
        .find(|t| {
            t.existing.is_empty()
                && catalog.advanced_item(t.key).unwrap().item.equip_type == 1086980073
        })
        .unwrap()
        .key;
    let spear = fire
        .targets
        .iter()
        .find(|t| t.existing.is_empty())
        .unwrap()
        .key;
    let existing = lightning
        .targets
        .iter()
        .find(|t| t.existing.iter().any(|p| p.skill == 91105))
        .unwrap()
        .key;
    for source in &choices {
        assert!(
            !source
                .targets
                .iter()
                .any(|t| [1001062, 1000451, 1000806].contains(&t.key))
        );
    }
    for (target, source, replace_existing) in [
        (1000806, 1001062, true),   // Mask, never a compatible weapon.
        (1001062, 1001062, true),   // Original source remains protected.
        (sword, 7, false),          // Arbitrary passive sources are refused.
        (existing, 1001062, false), // Existing fire effect needs consent.
        (sword, 1000451, false),    // Fire is not observed on greatswords.
    ] {
        assert!(
            catalog
                .build(ModRequest {
                    marketplace: Some(Selection {
                        weapon_effects: BTreeMap::from([(
                            target,
                            WeaponEffectEdit {
                                source,
                                replace_existing
                            }
                        )]),
                        ..Default::default()
                    }),
                    ..Default::default()
                })
                .is_err()
        );
    }
    let mut stat = catalog
        .advanced_item(sword)?
        .item
        .stats
        .first()
        .unwrap()
        .clone();
    stat.value = (stat.value.parse::<i64>()? + 1).to_string();
    let workshop = ItemEdit {
        stats: vec![stat],
        ..Default::default()
    };
    let request = ModRequest {
        advanced: Some(AdvancedRequest {
            items: BTreeMap::from([(sword, workshop.clone())]),
            ..Default::default()
        }),
        marketplace: Some(Selection {
            weapon_effects: BTreeMap::from([
                (
                    sword,
                    WeaponEffectEdit {
                        source: 1001062,
                        replace_existing: false,
                    },
                ),
                (
                    spear,
                    WeaponEffectEdit {
                        source: 1000451,
                        replace_existing: false,
                    },
                ),
                (
                    existing,
                    WeaponEffectEdit {
                        source: 1001062,
                        replace_existing: true,
                    },
                ),
            ]),
            ..Default::default()
        }),
        ..Default::default()
    };
    let built = catalog.build(request.clone())?;
    assert_eq!(
        built
            .preview
            .changes
            .iter()
            .filter(|c| c.field.contains("equip_passive_skill_list"))
            .count(),
        3
    );
    let original = Archive::with_registry(
        &game,
        std::fs::read(game.join(".workbench/baseline.papgt"))?,
    )?;
    let original_entries = original.list_group("0008")?;
    let read_original =
        |name: &str| original.extract(original_entries.iter().find(|e| e.name == name).unwrap());
    let body = read_original("iteminfo.staticinfobody")?;
    let header = read_original("iteminfo.staticinfoheader")?;
    let before = RawTable::parse(&body, &header, 4, 2)?;
    let stage = |files: &BTreeMap<String, Vec<u8>>| -> Result<_, Box<dyn std::error::Error>> {
        let temp = tempfile::tempdir()?;
        for (path, bytes) in files {
            let dest = temp.path().join(path);
            std::fs::create_dir_all(dest.parent().unwrap())?;
            std::fs::write(dest, bytes)?;
        }
        let archive = Archive::open(temp.path())?;
        let group = files.keys().find(|p| p.ends_with("/0.pamt")).unwrap();
        let entries = archive.list_group(&group[..4])?;
        let read = |name: &str| archive.extract(entries.iter().find(|e| e.name == name).unwrap());
        Ok((
            read("iteminfo.staticinfobody")?,
            read("iteminfo.staticinfoheader")?,
        ))
    };
    let (output, index) = stage(&built.files)?;
    let after = RawTable::parse(&output, &index, 4, 2)?;
    assert_eq!(before.records().len(), after.records().len());
    for row in before.records() {
        let key = row.key as u32;
        let source = before.record_bytes(row.key)?;
        let result = after.record_bytes(row.key)?;
        if ![sword, spear, existing].contains(&key) {
            assert_eq!(result, source, "unselected item {key} changed");
            continue;
        }
        let expected = if key == sword {
            item_mods::edit_item(source, &workshop)?.0
        } else {
            source.to_vec()
        };
        let mut one = 1u16.to_le_bytes().to_vec();
        one.extend(key.to_le_bytes());
        one.extend(0u32.to_le_bytes());
        let old_fields = ItemTable::parse(&expected, &one)?.fields(key)?;
        let new_fields = ItemTable::parse(result, &one)?.fields(key)?;
        let path = "equip_passive_skill_list.__count__";
        let old = old_fields.iter().find(|f| f.path == path).unwrap();
        let new = new_fields.iter().find(|f| f.path == path).unwrap();
        assert_eq!((old.start, old.end), (new.start, new.end));
        let old_count = u32::from_le_bytes(expected[old.start..old.end].try_into()?);
        assert_eq!(&result[new.start..new.end], &1u32.to_le_bytes());
        let skill = if key == spear { 91105u32 } else { 91101u32 };
        assert_eq!(&result[new.end..new.end + 4], &skill.to_le_bytes());
        assert_eq!(&result[new.end + 4..new.end + 8], &3u32.to_le_bytes());
        let mut inverse = result.to_vec();
        inverse.splice(
            new.end..new.end + 8,
            expected[old.end..old.end + old_count as usize * 8]
                .iter()
                .copied(),
        );
        inverse[new.start..new.end].copy_from_slice(&old_count.to_le_bytes());
        assert_eq!(
            inverse, expected,
            "weapon {key} changed outside the reviewed passive list and Workshop edit"
        );
        assert_eq!(
            item_mods::item_mod_info(result)?.stats,
            item_mods::item_mod_info(&expected)?.stats
        );
    }
    // A subsequent plan without transfers derives each native ability from the
    // original snapshot, while retaining the unrelated, requested Workshop stat.
    let mut removed = request;
    removed.marketplace.as_mut().unwrap().weapon_effects.clear();
    let restored = catalog.build(removed)?;
    let (restored_body, restored_index) = stage(&restored.files)?;
    let restored = RawTable::parse(&restored_body, &restored_index, 4, 2)?;
    for key in [sword, spear, existing] {
        let source = before.record_bytes(key.into())?;
        let expected = if key == sword {
            item_mods::edit_item(source, &workshop)?.0
        } else {
            source.to_vec()
        };
        assert_eq!(restored.record_bytes(key.into())?, expected);
    }
    println!(
        "Native lightning/fire references transferred to compatible weapons; source/unselected bytes, upgrade stats and sockets preserved. Replacement consent and removal verified. No game writes."
    );
    Ok(())
}

#[test]
#[ignore = "requires CD_GAME_DIR; original archive read only, staged output in a temporary fixture"]
fn rokade_size_is_isolated_and_combines_with_both_mount_stat_recipes()
-> Result<(), Box<dyn std::error::Error>> {
    use crimson_format::{Archive, overlay};
    let game = PathBuf::from(std::env::var_os("CD_GAME_DIR").expect("private game root required"));
    let data = GameData::open(&game, "eng")?;
    let catalog = ModCatalog::open(&data)?;
    let built = catalog.build(ModRequest {
        marketplace: Some(Selection {
            larger_lion: true,
            larger_rokade: true,
            faster_mounts: true,
            no_mount_stamina: true,
            ..Default::default()
        }),
        ..Default::default()
    })?;
    assert!(
        built
            .preview
            .changes
            .iter()
            .any(|c| c.table == "Rokade appearance" && c.key == 31378 && c.after == "1.469")
    );
    let original = Archive::with_registry(
        &game,
        std::fs::read(game.join(".workbench/baseline.papgt"))?,
    )?;
    let originals = original.list_group("0008")?;
    let read_original =
        |name: &str| original.extract(originals.iter().find(|e| e.name == name).unwrap());
    let temp = tempfile::tempdir()?;
    for (path, bytes) in &built.files {
        let dest = temp.path().join(path);
        std::fs::create_dir_all(dest.parent().unwrap())?;
        std::fs::write(dest, bytes)?;
    }
    let staged = Archive::open(temp.path())?;
    let mut entries = Vec::new();
    for path in built.files.keys().filter(|p| p.ends_with("/0.pamt")) {
        entries.extend(staged.list_group(&path[..4])?);
    }
    assert_eq!(entries.len(), 6, "two table pairs and two appearances");
    let read = |name: &str| staged.extract(entries.iter().find(|e| e.name == name).unwrap());
    let body = read_original("characterinfo.staticinfobody")?;
    let header = read_original("characterinfo.staticinfoheader")?;
    let output = read("characterinfo.staticinfobody")?;
    let index = read("characterinfo.staticinfoheader")?;
    let schema = cd_core::tables::schema("characterinfo").unwrap();
    let source = cd_core::tables::IndexedTable::parse(schema, &body, &header)?;
    let after = cd_core::tables::IndexedTable::parse(schema, &output, &index)?;
    let rows: BTreeMap<_, _> = after.records().iter().map(|r| (r.key, r)).collect();
    assert_eq!(source.records().len(), rows.len());
    let recipes: Vec<serde_json::Value> =
        serde_json::from_str(include_str!("../schemas/marketplace-mounts-2976.json"))?;
    for row in source.records() {
        let src = &body[row.offset as usize..row.offset as usize + row.length];
        let new = rows[&row.key];
        let mut inverse = output[new.offset as usize..new.offset as usize + new.length].to_vec();
        if row.key == 31378 {
            assert_eq!(&inverse[251..255], &2357531225u32.to_le_bytes());
            inverse[251..255].copy_from_slice(&3219883613u32.to_le_bytes());
        }
        if let Some(recipe) = recipes
            .iter()
            .find(|r| r["key"].as_u64() == Some(row.key.into()))
        {
            for field in recipe["fields"].as_array().unwrap() {
                let at = field["offset"].as_u64().unwrap() as usize;
                assert_eq!(
                    &inverse[at..at + 8],
                    &field["after"].as_i64().unwrap().to_le_bytes()
                );
                inverse[at..at + 8]
                    .copy_from_slice(&field["before"].as_i64().unwrap().to_le_bytes());
            }
        }
        assert_eq!(inverse, src, "unexpected character change in {}", row.key);
    }
    let strings = read_original("stringinfo.staticinfobody")?;
    let string_index = read_original("stringinfo.staticinfoheader")?;
    let updated = read("stringinfo.staticinfobody")?;
    let updated_index = read("stringinfo.staticinfoheader")?;
    assert_eq!(&updated[..strings.len()], strings);
    assert_eq!(&updated_index[2..string_index.len()], &string_index[2..]);
    assert_eq!(
        u16::from_le_bytes(updated_index[..2].try_into()?),
        u16::from_le_bytes(string_index[..2].try_into()?) + 1
    );
    assert_eq!(
        &updated[strings.len()..strings.len() + 4],
        &2357531225u32.to_le_bytes()
    );
    let path = format!("{}/{}", overlay::ROKADE_DIRECTORY, overlay::ROKADE_FILE);
    assert_eq!(&updated[strings.len() + 13..], path.as_bytes());
    let source_entries = original.list_group("0009")?;
    let original_appearance = original.extract(
        source_entries
            .iter()
            .find(|e| {
                e.directory == overlay::ROKADE_DIRECTORY
                    && e.name == "cd_r0002_00_horse_01007.app_xml"
            })
            .unwrap(),
    )?;
    let enlarged = String::from_utf8(read(overlay::ROKADE_FILE)?)?;
    assert_eq!(
        enlarged
            .replacen("CharacterScale=\"1.469\"", "CharacterScale=\"1.13\"", 1)
            .as_bytes(),
        original_appearance
    );
    assert!(
        !entries
            .iter()
            .any(|e| e.name == "cd_r0002_00_horse_01007.app_xml")
    );
    assert!(entries.iter().any(|e| e.name == overlay::LION_FILE));
    println!(
        "Rokade uses one new appearance; every original mapping and unrelated character byte preserved. Both mount stat recipes compose. No game writes."
    );
    Ok(())
}

#[test]
#[ignore = "requires CD_GAME_DIR; original archive read only, output only in a temporary fixture"]
fn automatic_fishing_preserves_every_other_item_and_rod_field()
-> Result<(), Box<dyn std::error::Error>> {
    use crimson_format::{Archive, ItemTable};
    let game = PathBuf::from(std::env::var_os("CD_GAME_DIR").expect("private game root required"));
    let data = GameData::open(&game, "eng")?;
    let catalog = ModCatalog::open(&data)?;
    let crab = catalog.advanced_item(1000088)?.item;
    assert!(
        crab.buffs
            .iter()
            .any(|b| (b.enchant_level, b.buff, b.level) == (0, 1000195, 1))
    );
    for key in [11001, 1000639, 1000640] {
        let rod = catalog.advanced_item(key)?;
        assert_eq!(rod.item.equip_type, crab.equip_type);
        assert_eq!(rod.item.category, crab.category);
        assert!(rod.compatible_buffs.contains(&(1000195, 1)));
        assert!(!rod.item.buffs.iter().any(|b| b.buff == 1000195));
    }
    let built = catalog.build(ModRequest {
        marketplace: Some(Selection {
            automatic_fishing: true,
            ..Default::default()
        }),
        ..Default::default()
    })?;
    assert_eq!(built.preview.changes.len(), 3);
    assert!(built.preview.changes.iter().all(|c| c.table == "iteminfo"
        && [11001, 1000639, 1000640].contains(&c.key)
        && c.field == "enchant[0].buff[1000195].level"
        && c.after == "1"));
    let original = Archive::with_registry(
        &game,
        std::fs::read(game.join(".workbench/baseline.papgt"))?,
    )?;
    let entries = original.list_group("0008")?;
    let body = original.extract(
        entries
            .iter()
            .find(|e| e.name == "iteminfo.staticinfobody")
            .unwrap(),
    )?;
    let header = original.extract(
        entries
            .iter()
            .find(|e| e.name == "iteminfo.staticinfoheader")
            .unwrap(),
    )?;
    let schema = cd_core::tables::schema("iteminfo").unwrap();
    let before = cd_core::tables::IndexedTable::parse(schema, &body, &header)?;
    let temp = tempfile::tempdir()?;
    for (path, bytes) in &built.files {
        let dest = temp.path().join(path);
        std::fs::create_dir_all(dest.parent().unwrap())?;
        std::fs::write(dest, bytes)?;
    }
    let staged = Archive::open(temp.path())?;
    let group = built.files.keys().find(|p| p.ends_with("/0.pamt")).unwrap();
    let entries = staged.list_group(&group[..4])?;
    let new_body = staged.extract(
        entries
            .iter()
            .find(|e| e.name == "iteminfo.staticinfobody")
            .unwrap(),
    )?;
    let new_header = staged.extract(
        entries
            .iter()
            .find(|e| e.name == "iteminfo.staticinfoheader")
            .unwrap(),
    )?;
    let after = cd_core::tables::IndexedTable::parse(schema, &new_body, &new_header)?;
    for row in before.records() {
        let source = before.record_bytes(row.key).unwrap();
        let result = after.record_bytes(row.key).unwrap();
        if [11001, 1000639, 1000640].contains(&row.key) {
            let mut index = 1u16.to_le_bytes().to_vec();
            index.extend(row.key.to_le_bytes());
            index.extend(0u32.to_le_bytes());
            let source_fields = ItemTable::parse(source, &index)?.fields(row.key)?;
            let result_fields = ItemTable::parse(result, &index)?.fields(row.key)?;
            let count = source_fields
                .iter()
                .find(|f| {
                    f.path.contains("enchant_data_list")
                        && f.path.contains("equip_buffs")
                        && f.type_name == "CArray.count"
                })
                .expect("tracked equipment buff count");
            let updated = result_fields.iter().find(|f| f.path == count.path).unwrap();
            assert_eq!((count.start, count.end), (updated.start, updated.end));
            assert_eq!(&source[count.start..count.end], &0u32.to_le_bytes());
            assert_eq!(&result[updated.start..updated.end], &1u32.to_le_bytes());
            assert_eq!(
                &result[updated.end..updated.end + 4],
                &1000195u32.to_le_bytes()
            );
            assert_eq!(
                &result[updated.end + 4..updated.end + 8],
                &1u32.to_le_bytes()
            );
            let mut inverse = result.to_vec();
            inverse.drain(updated.end..updated.end + 8);
            inverse[count.start..count.end].copy_from_slice(&0u32.to_le_bytes());
            assert_eq!(inverse, source, "rod {} changed outside auto-reel", row.key);
        } else {
            assert_eq!(result, source, "unselected item {} changed", row.key);
        }
    }
    println!(
        "Automatic reeling added only to three rods; original Crab rod and all other items preserved. No game writes."
    );
    Ok(())
}

#[test]
#[ignore = "requires CD_GAME_DIR; read-only original scripts and in-memory plan"]
fn restricted_region_timer_builds_all_four_pinned_scripts_in_one_plan()
-> Result<(), Box<dyn std::error::Error>> {
    let game = PathBuf::from(std::env::var_os("CD_GAME_DIR").expect("private game root required"));
    let data = GameData::open(&game, "eng")?;
    let catalog = ModCatalog::open(&data)?;
    let built = catalog.build(ModRequest {
        marketplace: Some(Selection {
            region_return_30min: true,
            ..Default::default()
        }),
        ..Default::default()
    })?;
    assert_eq!(built.preview.changes.len(), 4);
    for change in &built.preview.changes {
        assert_eq!(change.table, "region return script");
        assert_eq!(change.before, "30");
        assert_eq!(change.after, "1800");
        assert!([60001, 60101, 60201, 60301].contains(&change.key));
    }
    assert_eq!(built.files.len(), 3);
    let pamt = built
        .files
        .iter()
        .find(|(path, _)| path.ends_with("/0.pamt"))
        .unwrap();
    let paz = &built.files[&pamt.0.replace(".pamt", ".paz")];
    let paths = crimson_format::overlay::validate_pair(pamt.1, paz)?;
    assert_eq!(paths.len(), 4);
    assert!(
        paths
            .iter()
            .all(|path| path.starts_with(crimson_format::overlay::REGION_TIMER_DIRECTORY))
    );
    assert_eq!(
        built.preview.plan_id,
        catalog
            .build(built.preview.request.clone())?
            .preview
            .plan_id
    );
    println!(
        "Four original restricted-region scripts verified and rebuilt in memory; no game writes."
    );
    Ok(())
}

#[test]
#[ignore = "requires CD_GAME_DIR; reads original game data, builds plans in memory only"]
fn marketplace_inventory_stacks_and_wear_use_the_verified_editors()
-> Result<(), Box<dyn std::error::Error>> {
    let game = PathBuf::from(std::env::var_os("CD_GAME_DIR").expect("private game root required"));
    let data = GameData::open(&game, "eng")?;
    let catalog = ModCatalog::open(&data)?;
    let mut chosen = None;
    for item in data
        .items()
        .iter()
        .filter(|i| i.max_stack_count > 1 && i.max_stack_count < 1_000_000)
    {
        if !catalog.advanced_item(item.key)?.item.stack_risk {
            chosen = Some((item.key, item.max_stack_count + 1));
            break;
        }
    }
    let (key, limit) = chosen.expect("verified ordinary stackable item required");
    let build = |selection| {
        catalog.build(ModRequest {
            marketplace: Some(selection),
            ..Default::default()
        })
    };
    let inventory = build(Selection {
        inventory_999: true,
        ..Default::default()
    })?;
    assert_eq!(inventory.preview.changes.len(), 2);
    assert!(
        inventory
            .preview
            .changes
            .iter()
            .all(|c| c.table == "inventory" && c.key == 2 && c.after == "999")
    );
    let stacks = build(Selection {
        item_stacks: BTreeMap::from([(key, limit)]),
        ..Default::default()
    })?;
    assert!(!stacks.preview.changes.is_empty());
    assert!(
        stacks
            .preview
            .changes
            .iter()
            .all(|c| c.table == "iteminfo" && c.key == key && c.after == limit.to_string())
    );
    let wear = build(Selection {
        no_durability_loss: true,
        ..Default::default()
    })?;
    assert!(!wear.preview.changes.is_empty());
    assert!(
        wear.preview
            .changes
            .iter()
            .all(|c| ["iteminfo", "equiptypeinfo"].contains(&c.table))
    );
    let combined = build(Selection {
        inventory_999: true,
        no_durability_loss: true,
        item_stacks: BTreeMap::from([(key, limit)]),
        ..Default::default()
    })?;
    assert_eq!(
        combined.preview.changes.len(),
        inventory.preview.changes.len() + stacks.preview.changes.len() + wear.preview.changes.len()
    );
    assert_eq!(
        combined.preview.plan_id,
        catalog
            .build(combined.preview.request.clone())?
            .preview
            .plan_id
    );
    assert!(!combined.files.is_empty());
    println!(
        "Read-only original-data verification: 999 slots, one selected item stack, wear and combined plan; no game/save writes."
    );
    Ok(())
}

#[test]
#[ignore = "requires CD_GAME_DIR; reads original archives, writes generated output only to a temporary fixture"]
fn all_archive_marketplace_mods_compose_without_losing_shared_table_edits()
-> Result<(), Box<dyn std::error::Error>> {
    use cd_core::mods::marketplace::WeaponEffectEdit;
    use crimson_format::{Archive, overlay};
    let game = PathBuf::from(std::env::var_os("CD_GAME_DIR").expect("private game root required"));
    let registry_before = std::fs::read(game.join("meta/0.papgt"))?;
    let data = GameData::open(&game, "eng")?;
    let catalog = ModCatalog::open(&data)?;
    let target = catalog
        .marketplace_weapon_effects()?
        .into_iter()
        .find(|source| source.source == 1001062)
        .unwrap()
        .targets
        .into_iter()
        .find(|target| target.existing.is_empty())
        .unwrap()
        .key;
    let item = data
        .items()
        .iter()
        .find(|item| {
            item.max_stack_count > 1
                && item.max_stack_count < 999
                && !catalog.advanced_item(item.key).unwrap().item.stack_risk
        })
        .expect("ordinary stackable item");
    let built = catalog.build(ModRequest {
        marketplace: Some(Selection {
            larger_lion: true,
            larger_rokade: true,
            faster_mounts: true,
            no_mount_stamina: true,
            inventory_999: true,
            no_durability_loss: true,
            region_return_30min: true,
            automatic_fishing: true,
            free_crafting_currency: true,
            free_upgrade_materials: true,
            item_stacks: BTreeMap::from([(item.key, 999)]),
            weapon_effects: BTreeMap::from([(
                target,
                WeaponEffectEdit {
                    source: 1001062,
                    replace_existing: false,
                },
            )]),
        }),
        ..Default::default()
    })?;
    let fixture = tempfile::tempdir()?;
    let mut paths = std::collections::BTreeSet::new();
    for (name, bytes) in &built.files {
        if name.ends_with("/0.pamt") {
            for path in overlay::validate_pair(bytes, &built.files[&name.replace(".pamt", ".paz")])?
            {
                assert!(
                    paths.insert(path),
                    "two overlays must not shadow the same resource"
                );
            }
        }
        let output = fixture.path().join(name);
        std::fs::create_dir_all(output.parent().unwrap())?;
        std::fs::write(output, bytes)?;
    }
    let staged = Archive::open(fixture.path())?;
    let entries: Vec<_> = built
        .files
        .keys()
        .filter(|name| name.ends_with("/0.pamt"))
        .map(|name| staged.list_group(&name[..4]))
        .collect::<Result<Vec<_>, _>>()?
        .into_iter()
        .flatten()
        .collect();
    for resource in [
        "iteminfo.staticinfobody",
        "equiptypeinfo.staticinfobody",
        "inventory.staticinfobody",
        "characterinfo.staticinfobody",
        "stringinfo.staticinfobody",
        "multichangeinfo.staticinfobody",
        overlay::LION_FILE,
        overlay::ROKADE_FILE,
    ]
    .into_iter()
    .chain(overlay::REGION_TIMER_FILES)
    {
        assert_eq!(
            entries
                .iter()
                .filter(|entry| entry.name == resource)
                .count(),
            1,
            "{resource}"
        );
    }
    for entry in &entries {
        assert!(!staged.extract(entry)?.is_empty());
    }
    let read =
        |name: &str| staged.extract(entries.iter().find(|entry| entry.name == name).unwrap());
    let body = read("iteminfo.staticinfobody")?;
    let index = read("iteminfo.staticinfoheader")?;
    let table = crimson_format::RawTable::parse(&body, &index, 4, 2)?;
    let info = |key: u32| {
        crimson_format::item_mods::item_mod_info(table.record_bytes(key.into()).unwrap())
    };
    assert_eq!(info(item.key)?.stack_size, "999");
    let weapon = info(target)?;
    assert_eq!(
        weapon.passive_skills,
        [crimson_format::item_mods::EquippedPassive {
            skill: 91101,
            level: 3
        }]
    );
    assert!(matches!(weapon.max_endurance, 0 | u16::MAX));
    for rod in [11001, 1000639, 1000640] {
        let rod = info(rod)?;
        assert!(
            rod.buffs
                .iter()
                .any(|buff| buff.buff == 1000195 && buff.level == 1)
        );
        assert!(matches!(rod.max_endurance, 0 | u16::MAX));
    }
    let changes = &built.preview.changes;
    assert_eq!(
        changes
            .iter()
            .filter(|c| c.table == "inventory" && c.key == 2 && c.after == "999")
            .count(),
        2
    );
    assert!(
        changes
            .iter()
            .any(|c| c.table == "iteminfo" && c.key == item.key && c.after == "999")
    );
    assert!(
        changes
            .iter()
            .any(|c| c.table == "iteminfo" && c.key == target && c.field.contains("Lightning"))
    );
    assert_eq!(
        changes
            .iter()
            .filter(|c| c.table == "region return script" && c.after == "1800")
            .count(),
        4
    );
    assert!(
        changes
            .iter()
            .any(|c| c.table == "multichangeinfo" && c.key == 1010292)
    );
    assert!(
        changes
            .iter()
            .any(|c| c.table == "multichangeinfo" && c.key == 0)
    );
    assert_eq!(std::fs::read(game.join("meta/0.papgt"))?, registry_before);
    println!(
        "All twelve archive choices compose into readable, non-overlapping overlays; original registry unchanged."
    );
    Ok(())
}
