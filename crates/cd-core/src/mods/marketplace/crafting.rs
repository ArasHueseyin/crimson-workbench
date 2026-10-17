//! Independent upgrade-material and recipe-currency components.
//! General material-free crafting remains pending.
use crate::{
    Result,
    crafting::formats::RecipeRow,
    fingerprint::hash_bytes,
    mods::{Change, bad, table},
};
use std::collections::BTreeMap;

const RECIPE: u32 = 1010292;
const CURRENCY: u32 = 1000485;

pub(super) fn validate_recipes(blobs: &BTreeMap<String, Vec<u8>>) -> Result<()> {
    for (name, expected) in [
        (
            "multichangeinfo.staticinfobody",
            "b21dcbff4deb62a1c476158632d87b372694ca53e983e901e7cfe23f3a50e301",
        ),
        (
            "multichangeinfo.staticinfoheader",
            "27b8f2b37f0cd90c114e34240c6fe606b17ffbeb7277477b571f6ba468179a80",
        ),
    ] {
        if blobs.get(name).map(|bytes| hash_bytes(bytes)).as_deref() != Some(expected) {
            return Err(bad("Crafting definitions changed; nothing written"));
        }
    }
    Ok(())
}

fn without_upgrade_materials(original: &[u8]) -> Result<Vec<u8>> {
    let mut row = RecipeRow::parse(original)?;
    let target = row
        .fixed
        .first()
        .ok_or_else(|| bad("Upgrade equipment input is missing"))?;
    if row.consume != 3
        || row.blocked != 0
        || row.fixed.len() < 2
        || !row.groups.is_empty()
        || target.item == 0
        || target.character != 0
        || target.gimmick != 0
        || target.count != 1
        || target.coupon != 0
    {
        return Err(bad("Unsupported upgrade material layout"));
    }
    // In every pinned type-3 row the first entry is the owned equipment and
    // refinement level. Keeping it leaves a valid positive native input and
    // preserves the engine's normal equipment-improvement transaction.
    row.fixed.truncate(1);
    let bytes = row.serialize();
    RecipeRow::parse(&bytes)?;
    Ok(bytes)
}

pub(super) fn apply_upgrades(
    blobs: &BTreeMap<String, Vec<u8>>,
    replacements: &mut BTreeMap<&'static str, BTreeMap<u32, Vec<u8>>>,
    changes: &mut Vec<Change>,
) -> Result<()> {
    validate_recipes(blobs)?;
    let recipes = table(blobs, "multichangeinfo")?;
    let mut edits = BTreeMap::new();
    let mut total = 0;
    for record in recipes.records() {
        let bytes = recipes
            .record_bytes(record.key)
            .ok_or_else(|| bad("Upgrade recipe missing"))?;
        let row = RecipeRow::parse(bytes)?;
        if row.consume != 3 {
            continue;
        }
        total += 1;
        let edited = without_upgrade_materials(bytes)?;
        if let Some(existing) = replacements
            .get("multichangeinfo")
            .and_then(|rows| rows.get(&record.key))
        {
            if *existing == edited {
                continue;
            }
            if existing != bytes {
                return Err(bad(
                    "Another recipe edit conflicts with free equipment upgrades",
                ));
            }
        }
        edits.insert(record.key, edited);
    }
    // A repeated invocation is a no-op. Every first invocation must cover the
    // complete pinned upgrade set, rather than silently skipping other layouts.
    if total != 17069 {
        return Err(bad("Upgrade recipe set is incomplete; nothing written"));
    }
    if edits.is_empty() {
        return Ok(());
    }
    replacements
        .entry("multichangeinfo")
        .or_default()
        .extend(edits);
    changes.push(Change {
        module: "Marketplace",
        table: "multichangeinfo",
        key: 0,
        name: "Equipment upgrades".into(),
        field: "Material and token costs across 17,069 upgrade recipes".into(),
        before: "Original material and token requirements".into(),
        after: "No material/token costs; existing equipment still required".into(),
    });
    Ok(())
}

fn without_currency(original: &[u8]) -> Result<Vec<u8>> {
    let mut row = RecipeRow::parse(original)?;
    if row.key != RECIPE
        || row.name != b"Kuku_Drone_Backpack_0"
        || row.tool != 205
        || row.consume != 1
    {
        return Err(bad("Unexpected crafting currency recipe"));
    }
    let fees: Vec<_> = row
        .fixed
        .iter()
        .enumerate()
        .filter(|(_, f)| f.item == CURRENCY)
        .collect();
    if fees.len() != 1 {
        return Err(bad("Crafting currency fee is missing or ambiguous"));
    }
    let (position, fee) = fees[0];
    if fee.count != 300
        || fee.coupon != 0
        || fee.character != 0
        || fee.gimmick != 0
        || fee.enchant != 0
        || row.fixed.len() <= 1
    {
        return Err(bad("Crafting currency fee changed"));
    }
    // Remove this requirement, rather than leaving a zero quantity that can
    // reach a native division. Other materials and all recipe gates survive.
    row.fixed.remove(position);
    let output = row.serialize();
    RecipeRow::parse(&output)?;
    Ok(output)
}

pub(super) fn apply_currency(
    blobs: &BTreeMap<String, Vec<u8>>,
    replacements: &mut BTreeMap<&'static str, BTreeMap<u32, Vec<u8>>>,
    changes: &mut Vec<Change>,
) -> Result<()> {
    validate_recipes(blobs)?;
    let items = table(blobs, "iteminfo")?;
    if items.record_bytes(CURRENCY).map(hash_bytes).as_deref()
        != Some("e10e89aeef53a7f6769059058d06a7363d28635f88d8c364fd3b150670435bae")
    {
        return Err(bad("Clockwork Insect currency definition changed"));
    }
    let recipes = table(blobs, "multichangeinfo")?;
    let original = recipes
        .record_bytes(RECIPE)
        .ok_or_else(|| bad("Crafting currency recipe is missing"))?;
    if hash_bytes(original) != "0317fd6d9f82a6d72a4008bdcbaaa5dcf9702334b44614c39ffc6716d26a744f" {
        return Err(bad("Crafting currency recipe changed"));
    }
    let edited = without_currency(original)?;
    let rows = replacements.entry("multichangeinfo").or_default();
    if let Some(existing) = rows.get(&RECIPE) {
        if *existing == edited {
            return Ok(());
        }
        if existing != original {
            return Err(bad(
                "Another recipe edit conflicts with the crafting currency mod",
            ));
        }
    }
    rows.insert(RECIPE, edited);
    changes.push(Change {
        module: "Marketplace",
        table: "multichangeinfo",
        key: RECIPE,
        name: "Kuku drone backpack".into(),
        field: "Clockwork Insect crafting fee".into(),
        before: "300".into(),
        after: "0".into(),
    });
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::crafting::formats::{Fixed, Text};
    fn original() -> RecipeRow {
        RecipeRow {
            key: RECIPE,
            name: b"Kuku_Drone_Backpack_0".to_vec(),
            blocked: 0,
            tool: 205,
            consume: 1,
            conditions: vec![(123, Text(0, 456, b"Keep this condition".to_vec()))],
            knowledge: 1003097,
            tag: b"keep".to_vec(),
            flags: [1, 1, 0, 1, 0],
            fixed: vec![
                Fixed {
                    item: 1002130,
                    character: 0,
                    gimmick: 0,
                    count: 1,
                    coupon: 0,
                    enchant: 0,
                },
                Fixed {
                    item: CURRENCY,
                    character: 0,
                    gimmick: 0,
                    count: 300,
                    coupon: 0,
                    enchant: 0,
                },
            ],
            groups: vec![],
            elemental: 0,
            states: vec![7],
            description: Text(13, 999, b"Original".to_vec()),
            name_text: Text(0, 0, vec![]),
            strings: [1, 2, 3],
            complete: Text(0, 0, vec![]),
            results: vec![1007688],
            additional: vec![111],
        }
    }
    #[test]
    fn free_upgrades_keep_the_exact_owned_target_and_all_non_cost_fields() {
        let mut row = original();
        row.consume = 3;
        row.fixed[0].enchant = 7;
        row.fixed[1].item = 0;
        row.fixed[1].count = 0;
        row.fixed[1].coupon = 1;
        let bytes = row.serialize();
        let after = without_upgrade_materials(&bytes).unwrap();
        let mut edited = RecipeRow::parse(&after).unwrap();
        assert_eq!(edited.fixed.len(), 1);
        assert_eq!(edited.fixed[0].count, 1);
        assert_eq!(edited.fixed[0].enchant, 7);
        edited.fixed.extend_from_slice(&row.fixed[1..]);
        assert_eq!(
            edited.serialize(),
            bytes,
            "only the non-target cost requirements may change"
        );
        for case in 0..8 {
            let mut invalid = row.clone();
            match case {
                0 => invalid.consume = 1,
                1 => invalid.fixed[0].count = 2,
                2 => invalid.fixed[0].coupon = 1,
                3 => invalid.fixed[0].character = 1,
                4 => invalid.fixed[0].item = 0,
                5 => invalid.blocked = 1,
                6 => invalid.fixed.truncate(1),
                _ => invalid
                    .groups
                    .push(crate::crafting::formats::GroupIngredient {
                        group: 1,
                        count: 1,
                        unknown: 0,
                    }),
            }
            assert!(without_upgrade_materials(&invalid.serialize()).is_err());
        }
    }
    #[test]
    fn removing_a_fee_preserves_all_other_recipe_bytes_and_never_zeros_a_quantity() {
        let row = original();
        let bytes = row.serialize();
        let output = without_currency(&bytes).unwrap();
        let mut edited = RecipeRow::parse(&output).unwrap();
        assert_eq!(edited.fixed.len(), 1);
        assert_eq!(edited.fixed[0].count, 1);
        assert!(edited.fixed.iter().all(|f| f.item != CURRENCY));
        assert!(without_currency(&output).is_err());
        edited.fixed.insert(1, row.fixed[1].clone());
        assert_eq!(
            edited.serialize(),
            bytes,
            "inverse restores every non-currency field exactly"
        );
        for case in 0..7 {
            let mut invalid = original();
            match case {
                0 => invalid.consume = 3,
                1 => invalid.fixed[1].count = 0,
                2 => invalid.fixed[1].coupon = 1,
                3 => invalid.fixed.push(invalid.fixed[1].clone()),
                4 => invalid.fixed[1].item = 1,
                5 => invalid.fixed[1].enchant = 1,
                _ => {
                    invalid.fixed.remove(0);
                }
            };
            assert!(without_currency(&invalid.serialize()).is_err());
        }
    }
    #[test]
    #[ignore = "requires CD_CRAFT_FIXTURE, immutable private original tables only"]
    fn current_build_recipe_costs_roundtrip_and_currency_edit_is_isolated() {
        let root = std::path::PathBuf::from(
            std::env::var_os("CD_CRAFT_FIXTURE").expect("private fixture root"),
        );
        let mut blobs: BTreeMap<_, _> = [
            "multichangeinfo.staticinfobody",
            "multichangeinfo.staticinfoheader",
            "iteminfo.staticinfobody",
            "iteminfo.staticinfoheader",
        ]
        .into_iter()
        .map(|name| (name.into(), std::fs::read(root.join(name)).unwrap()))
        .collect();
        let source = blobs.clone();
        let recipes = table(&blobs, "multichangeinfo").unwrap();
        assert_eq!(recipes.records().len(), 18576);
        let mut upgrades = 0;
        let mut coupons = 0;
        for row in recipes.records() {
            let bytes = recipes.record_bytes(row.key).unwrap();
            let parsed = RecipeRow::parse(bytes).unwrap();
            assert_eq!(parsed.key, row.key);
            assert_eq!(parsed.serialize(), bytes);
            if parsed.consume == 3 {
                upgrades += 1;
                assert_eq!(parsed.fixed[0].count, 1);
                assert_eq!(parsed.fixed[0].coupon, 0);
                assert!(parsed.groups.is_empty());
            }
            coupons += parsed.fixed.iter().filter(|f| f.coupon != 0).count();
        }
        assert_eq!(upgrades, 17069);
        assert_eq!(coupons, 4032);
        let mut replacements = BTreeMap::new();
        let mut changes = vec![];
        apply_currency(&blobs, &mut replacements, &mut changes).unwrap();
        assert_eq!(changes.len(), 1);
        apply_currency(&blobs, &mut replacements, &mut changes).unwrap();
        assert_eq!(changes.len(), 1);
        let (body, header) = recipes.rebuild(&replacements["multichangeinfo"]).unwrap();
        let rebuilt = crate::tables::IndexedTable::parse(
            crate::tables::schema("multichangeinfo").unwrap(),
            &body,
            &header,
        )
        .unwrap();
        for row in recipes.records() {
            if row.key != RECIPE {
                assert_eq!(rebuilt.record_bytes(row.key), recipes.record_bytes(row.key));
            }
        }
        let mut inverse = RecipeRow::parse(rebuilt.record_bytes(RECIPE).unwrap()).unwrap();
        let original = RecipeRow::parse(recipes.record_bytes(RECIPE).unwrap()).unwrap();
        inverse.fixed.insert(3, original.fixed[3].clone());
        assert_eq!(inverse.serialize(), original.serialize());
        apply_upgrades(&blobs, &mut replacements, &mut changes).unwrap();
        assert_eq!(replacements["multichangeinfo"].len(), 17070);
        assert_eq!(changes.len(), 2);
        apply_upgrades(&blobs, &mut replacements, &mut changes).unwrap();
        assert_eq!(changes.len(), 2);
        let (upgraded_body, upgraded_header) =
            recipes.rebuild(&replacements["multichangeinfo"]).unwrap();
        let upgraded = crate::tables::IndexedTable::parse(
            crate::tables::schema("multichangeinfo").unwrap(),
            &upgraded_body,
            &upgraded_header,
        )
        .unwrap();
        for record in recipes.records() {
            let before = recipes.record_bytes(record.key).unwrap();
            let after = upgraded.record_bytes(record.key).unwrap();
            let original = RecipeRow::parse(before).unwrap();
            if original.consume == 3 {
                let mut inverse = RecipeRow::parse(after).unwrap();
                assert_eq!(inverse.fixed.len(), 1);
                assert_eq!(inverse.fixed[0].count, 1);
                inverse.fixed.extend_from_slice(&original.fixed[1..]);
                assert_eq!(
                    inverse.serialize(),
                    before,
                    "upgrade target, prerequisites and results for {}",
                    record.key
                );
            } else if record.key == RECIPE {
                assert_eq!(after, rebuilt.record_bytes(RECIPE).unwrap());
            } else {
                assert_eq!(
                    after, before,
                    "ordinary crafting recipe {} changed",
                    record.key
                );
            }
        }
        let mut only_upgrades = BTreeMap::new();
        apply_upgrades(&blobs, &mut only_upgrades, &mut vec![]).unwrap();
        assert!(!only_upgrades["multichangeinfo"].contains_key(&RECIPE));
        let key = *only_upgrades["multichangeinfo"].keys().next().unwrap();
        let mut conflict = RecipeRow::parse(recipes.record_bytes(key).unwrap()).unwrap();
        conflict.knowledge = 123456;
        let mut conflicts = BTreeMap::from([(
            "multichangeinfo",
            BTreeMap::from([(key, conflict.serialize())]),
        )]);
        let existing = conflicts.clone();
        let mut refused = vec![];
        assert!(apply_upgrades(&blobs, &mut conflicts, &mut refused).is_err());
        assert_eq!(conflicts, existing);
        assert!(refused.is_empty());
        assert_eq!(blobs, source);
        blobs.get_mut("multichangeinfo.staticinfobody").unwrap()[0] ^= 1;
        assert!(apply_currency(&blobs, &mut BTreeMap::new(), &mut vec![]).is_err());
    }
}
