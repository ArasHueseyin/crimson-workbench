//! Archived empty-recipe preparation retained for research; never install it.
//!
//! The runtime now uses immutable recipe views without changing the original
//! database or archive. Keep this alternative transform out of Selection and
//! the archive writer. No zero-quantity or invented inputs are used.
use super::crafting::validate_recipes;
use crate::{
    Result,
    crafting::formats::RecipeRow,
    fingerprint::hash_bytes,
    mods::{bad, table},
};
use std::collections::{BTreeMap, BTreeSet};

const FEE_RECIPE: u32 = 1010292;
const FEE_ITEM: u32 = 1000485;

/// The original UI's missing-knowledge fallback is the first fixed item's
/// knowledge key (native e27040/107d6e0, ItemInfo +f6). All nine such ordinary
/// recipes have a fixed item; no new recipe prerequisite is inferred here.
const KNOWLEDGE_FALLBACKS: [(u32, u32, u32); 9] = [
    (1000085, 1002870, 1000689),
    (1000087, 1002870, 1000689),
    (1000089, 1002870, 1000689),
    (1000091, 1002870, 1000689),
    (1000093, 1002870, 1000689),
    (1000160, 720001, 17047),
    (1000415, 75001, 1000165),
    (1000416, 75001, 1000165),
    (1000417, 75001, 1000165),
];

pub(super) struct PreparedMaterials {
    pub recipes: BTreeMap<u32, Vec<u8>>,
    /// Native helpers must support precisely these zero-input recipes. This is
    /// a required part of the plan, not evidence that they work unassisted.
    pub empty_recipes: BTreeSet<u32>,
    /// UI categorization only. Writing these keys into RecipeRow::knowledge
    /// would add a native unlock prerequisite that the original recipe lacks.
    pub ui_knowledge_fallbacks: BTreeMap<u32, u32>,
}

fn transform(original: &[u8], waive_currency: bool) -> Result<(Vec<u8>, bool, Option<u32>)> {
    let mut row = RecipeRow::parse(original)?;
    if row.consume != 1 || row.blocked != 0 || row.results.len() != 1 {
        return Err(bad("Unsupported ordinary crafting recipe"));
    }
    if row.fixed.is_empty() && row.groups.is_empty() {
        return Err(bad("Original crafting recipe has no requirements"));
    }
    let knowledge = if row.knowledge == 0 {
        let (_, item, knowledge) = KNOWLEDGE_FALLBACKS
            .iter()
            .find(|r| r.0 == row.key)
            .ok_or_else(|| bad("Unknown missing-knowledge crafting recipe"))?;
        if row.fixed.first().map(|f| (f.item, f.character, f.gimmick)) != Some((*item, 0, 0)) {
            return Err(bad("Original crafting knowledge fallback changed"));
        }
        Some(*knowledge)
    } else {
        None
    };
    // Only this positively identified native currency fee is retained. Item
    // type 32 is not a currency classifier: it also includes cog/material and
    // contribution records. The full recipe table is pinned before transform.
    if row.key == FEE_RECIPE {
        let fees: Vec<_> = row
            .fixed
            .iter()
            .filter(|f| f.item == FEE_ITEM)
            .cloned()
            .collect();
        if fees.len() != 1
            || fees[0].count != 300
            || fees[0].coupon != 0
            || fees[0].character != 0
            || fees[0].gimmick != 0
            || fees[0].enchant != 0
        {
            return Err(bad("Crafting currency fee changed"));
        }
        row.fixed = if waive_currency { vec![] } else { fees };
    } else {
        row.fixed.clear();
    }
    row.groups.clear();
    let empty = row.fixed.is_empty();
    let output = row.serialize();
    RecipeRow::parse(&output)?;
    Ok((output, empty, knowledge))
}

pub(super) fn prepare(
    blobs: &BTreeMap<String, Vec<u8>>,
    waive_currency: bool,
) -> Result<PreparedMaterials> {
    validate_recipes(blobs)?;
    // Pins the ItemInfo knowledge values used for the exact fallback above.
    for (name, expected) in [
        (
            "iteminfo.staticinfobody",
            "87a1bbcd77bcaf64fd8d615531bd868a3ed0ca1d3f410ce6dbcb51a6f2c628ce",
        ),
        (
            "iteminfo.staticinfoheader",
            "b88092f185269215f118d43d35397d64d8dbf9469c85a3d2e15e69990b258dd0",
        ),
    ] {
        if blobs.get(name).map(|b| hash_bytes(b)).as_deref() != Some(expected) {
            return Err(bad("Crafting knowledge fallback definitions changed"));
        }
    }
    let original = table(blobs, "multichangeinfo")?;
    let mut prepared = PreparedMaterials {
        recipes: BTreeMap::new(),
        empty_recipes: BTreeSet::new(),
        ui_knowledge_fallbacks: BTreeMap::new(),
    };
    for record in original.records() {
        let bytes = original
            .record_bytes(record.key)
            .ok_or_else(|| bad("Crafting recipe missing"))?;
        if RecipeRow::parse(bytes)?.consume != 1 {
            continue;
        }
        let (edited, empty, knowledge) = transform(bytes, waive_currency)?;
        prepared.recipes.insert(record.key, edited);
        if empty {
            prepared.empty_recipes.insert(record.key);
        }
        if let Some(key) = knowledge {
            prepared.ui_knowledge_fallbacks.insert(record.key, key);
        }
    }
    if prepared.recipes.len() != 1507
        || prepared.ui_knowledge_fallbacks.len() != 9
        || prepared.empty_recipes.len() != if waive_currency { 1507 } else { 1506 }
    {
        return Err(bad("Material-free crafting preparation is incomplete"));
    }
    Ok(prepared)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        crafting::formats::{Fixed, Text},
        tables::{IndexedTable, schema},
    };

    #[test]
    fn unfinished_empty_input_support_cannot_be_selected_through_marketplace_requests() {
        assert!(
            serde_json::from_value::<super::super::Selection>(serde_json::json!({
                "free_crafting_materials": true
            }))
            .is_err()
        );
    }

    fn sample(key: u32, knowledge: u32) -> RecipeRow {
        RecipeRow {
            key,
            name: b"Retain original recipe identity".to_vec(),
            blocked: 0,
            tool: 30001,
            consume: 1,
            conditions: vec![(123, Text(0, 4, b"Preserve prerequisite".to_vec()))],
            knowledge,
            tag: b"keep".to_vec(),
            flags: [1, 0, 0, 1, 0],
            fixed: vec![Fixed {
                item: 1002870,
                character: 0,
                gimmick: 0,
                count: 2,
                coupon: 0,
                enchant: 0,
            }],
            groups: vec![],
            elemental: 0,
            states: vec![55],
            description: Text(13, 999, vec![]),
            name_text: Text(0, 0, vec![]),
            strings: [1, 2, 3],
            complete: Text(0, 0, vec![]),
            results: vec![1007688],
            additional: vec![45],
        }
    }
    #[test]
    fn material_preparation_preserves_gates_and_does_not_invent_zero_cost_inputs() {
        let original = sample(1000085, 0);
        let bytes = original.serialize();
        let (edited, empty, fallback) = transform(&bytes, false).unwrap();
        let mut inverse = RecipeRow::parse(&edited).unwrap();
        assert!(empty);
        assert_eq!(fallback, Some(1000689));
        assert!(inverse.fixed.is_empty() && inverse.groups.is_empty());
        assert_eq!(
            inverse.knowledge, 0,
            "UI fallback must not become a new unlock prerequisite"
        );
        inverse.fixed = original.fixed.clone();
        assert_eq!(inverse.serialize(), bytes, "all other fields remain exact");
        for case in 0..5 {
            let mut row = original.clone();
            match case {
                0 => row.consume = 3,
                1 => row.blocked = 1,
                2 => row.results.push(777),
                3 => row.fixed[0].item = 75001,
                _ => row.key = 987654,
            }
            assert!(transform(&row.serialize(), false).is_err());
        }
    }
    #[test]
    fn material_preparation_retains_the_currency_fee_unless_independently_waived() {
        let mut row = sample(FEE_RECIPE, 1003097);
        row.fixed.push(Fixed {
            item: FEE_ITEM,
            character: 0,
            gimmick: 0,
            count: 300,
            coupon: 0,
            enchant: 0,
        });
        let (materials, empty, _) = transform(&row.serialize(), false).unwrap();
        let edited = RecipeRow::parse(&materials).unwrap();
        assert!(!empty);
        assert_eq!(edited.fixed.len(), 1);
        assert_eq!(edited.fixed[0].item, FEE_ITEM);
        assert_eq!(edited.fixed[0].count, 300);
        let (both, empty, _) = transform(&row.serialize(), true).unwrap();
        assert!(empty && RecipeRow::parse(&both).unwrap().fixed.is_empty());
        row.fixed[1].coupon = 1;
        assert!(transform(&row.serialize(), false).is_err());
    }
    #[test]
    #[ignore = "requires CD_CRAFT_FIXTURE; preparation only, never installs an empty-input recipe"]
    fn all_ordinary_material_plans_preserve_original_recipe_semantics_and_currency_choice() {
        let root = std::path::PathBuf::from(
            std::env::var_os("CD_CRAFT_FIXTURE").expect("private original tables"),
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
        let count_source = include_str!(concat!(
            env!("CARGO_MANIFEST_DIR"),
            "/../../runtime/marketplace/crafting/crafting_source_costs.h"
        ));
        let encoded = count_source
            .split("kSourceCostCounts{{")
            .nth(1)
            .unwrap()
            .split("}};")
            .next()
            .unwrap();
        let native_counts: Vec<u8> = encoded
            .split(',')
            .filter_map(|word| {
                let word = word.trim();
                (!word.is_empty())
                    .then(|| u8::from_str_radix(word.strip_prefix("0x").unwrap(), 16).unwrap())
            })
            .collect();
        let mut original_counts = Vec::new();
        for record in recipes.records() {
            let recipe = RecipeRow::parse(recipes.record_bytes(record.key).unwrap()).unwrap();
            if recipe.consume == 1 {
                assert!(recipe.fixed.len() <= 5 && recipe.groups.len() <= 4);
                original_counts.push((
                    record.key,
                    ((recipe.fixed.len() << 4) | recipe.groups.len()) as u8,
                ));
            }
        }
        original_counts.sort_unstable_by_key(|entry| entry.0);
        assert_eq!(original_counts.len(), 1507);
        assert_eq!(
            native_counts,
            original_counts
                .into_iter()
                .map(|entry| entry.1)
                .collect::<Vec<_>>()
        );
        let items = crimson_format::ItemTable::parse(
            &blobs["iteminfo.staticinfobody"],
            &blobs["iteminfo.staticinfoheader"],
        )
        .unwrap();
        for (recipe, item, knowledge) in KNOWLEDGE_FALLBACKS {
            let fields = items.fields(item).unwrap();
            let native_knowledge = fields
                .iter()
                .find(|f| f.path == "knowledge_info")
                .unwrap()
                .value
                .as_u64()
                .unwrap();
            assert_eq!(
                native_knowledge,
                u64::from(knowledge),
                "original first-material fallback for {recipe}"
            );
        }
        for waive_currency in [false, true] {
            let prepared = prepare(&blobs, waive_currency).unwrap();
            assert_eq!(prepared.recipes.len(), 1507);
            let (body, header) = recipes.rebuild(&prepared.recipes).unwrap();
            let after =
                IndexedTable::parse(schema("multichangeinfo").unwrap(), &body, &header).unwrap();
            let mut group_recipes = 0;
            for record in recipes.records() {
                let raw = recipes.record_bytes(record.key).unwrap();
                let old = RecipeRow::parse(raw).unwrap();
                let edited = after.record_bytes(record.key).unwrap();
                if old.consume != 1 {
                    assert_eq!(edited, raw, "upgrades remain untouched");
                    continue;
                }
                group_recipes += usize::from(!old.groups.is_empty());
                let mut inverse = RecipeRow::parse(edited).unwrap();
                assert!(inverse.groups.is_empty());
                let keep_fee = record.key == FEE_RECIPE && !waive_currency;
                assert_eq!(inverse.fixed.len(), usize::from(keep_fee));
                if keep_fee {
                    assert_eq!(inverse.fixed[0].item, FEE_ITEM);
                    assert_eq!(inverse.fixed[0].count, 300);
                }
                assert_eq!(
                    inverse.knowledge, old.knowledge,
                    "native knowledge prerequisites are never added or removed"
                );
                if old.knowledge == 0 {
                    assert!(prepared.ui_knowledge_fallbacks.contains_key(&record.key));
                }
                inverse.fixed = old.fixed;
                inverse.groups = old.groups;
                assert_eq!(
                    inverse.serialize(),
                    raw,
                    "tool, identity, conditions, flags, result and additional fields for {}",
                    record.key
                );
            }
            assert_eq!(group_recipes, 717);
        }
        assert_eq!(blobs, source);
        blobs.get_mut("iteminfo.staticinfobody").unwrap()[0] ^= 1;
        assert!(prepare(&blobs, false).is_err());
        assert_eq!(
            blobs["multichangeinfo.staticinfobody"],
            source["multichangeinfo.staticinfobody"]
        );
    }
}
