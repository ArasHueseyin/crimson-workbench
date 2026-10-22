//! Curated recipes contain offsets, identifiers and fingerprints, never game assets.
mod crafting;
// Staged data preparation: empty-input recipes still require verified native
// UI/database support. No selection or installation path enables this yet.
#[allow(dead_code)]
mod crafting_materials;
mod effects;
mod regions;
mod rokade;
use super::{Change, bad};
use crate::{Result, fingerprint::hash_bytes};
pub(super) use effects::choices as weapon_effect_choices;
pub use effects::{EffectSource, WeaponEffectEdit};
pub(crate) use regions::region_timers;
pub(super) use rokade::rokade;
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;

#[derive(Clone, Debug, Default, Deserialize, Serialize)]
#[serde(default, deny_unknown_fields)]
pub struct Selection {
    pub larger_lion: bool,
    pub larger_rokade: bool,
    pub faster_mounts: bool,
    pub no_mount_stamina: bool,
    pub inventory_999: bool,
    pub no_durability_loss: bool,
    pub region_return_30min: bool,
    pub automatic_fishing: bool,
    pub free_crafting_currency: bool,
    pub free_upgrade_materials: bool,
    pub item_stacks: BTreeMap<u32, u64>,
    pub weapon_effects: BTreeMap<u32, WeaponEffectEdit>,
}
impl Selection {
    pub fn active(&self) -> bool {
        self.larger_lion
            || self.larger_rokade
            || self.faster_mounts
            || self.no_mount_stamina
            || self.inventory_999
            || self.no_durability_loss
            || self.region_return_30min
            || self.automatic_fishing
            || self.free_crafting_currency
            || self.free_upgrade_materials
            || !self.item_stacks.is_empty()
            || !self.weapon_effects.is_empty()
    }
    pub(super) fn merge_advanced(
        &self,
        original: Option<&super::advanced::AdvancedRequest>,
    ) -> Result<Option<super::advanced::AdvancedRequest>> {
        if !self.inventory_999
            && !self.no_durability_loss
            && self.item_stacks.is_empty()
            && !self.automatic_fishing
        {
            return Ok(original.cloned());
        }
        let mut request = original.cloned().unwrap_or_default();
        if self.inventory_999 {
            for key in ["inventory/2/defaultSlotCount", "inventory/2/maxSlotCount"] {
                if request.fields.get(key).is_some_and(|value| value != "999") {
                    return Err(bad(
                        "999 inventory slots conflicts with a Workshop inventory setting. Align the settings before reviewing.",
                    ));
                }
                request.fields.insert(key.into(), "999".into());
            }
        }
        request.no_wear |= self.no_durability_loss;
        if self.automatic_fishing {
            // Same native rod equip type as the original Crab fishing rod
            // (1000088), which already carries Buff 1000195 at level 1.
            // The advanced editor independently checks the observed equip-type
            // compatibility and complete item roundtrip before building bytes.
            for key in [11001, 1000639, 1000640] {
                let item = request.items.entry(key).or_default();
                if let Some(buff) = item
                    .buffs
                    .iter()
                    .find(|b| b.enchant_level == 0 && b.buff == 1000195)
                {
                    if buff.level != 1 {
                        return Err(bad(
                            "Automatic fishing conflicts with a Workshop auto-reel buff setting",
                        ));
                    }
                } else {
                    item.buffs.push(crimson_format::item_mods::BuffEdit {
                        enchant_level: 0,
                        buff: 1000195,
                        level: 1,
                    });
                }
            }
        }
        if self.item_stacks.len() > 6816 {
            return Err(bad("Too many item stack overrides"));
        }
        for (key, limit) in &self.item_stacks {
            if *limit == 0 || *limit > 1_000_000 {
                return Err(bad("Item stack size must be between 1 and 1,000,000"));
            }
            let item = request.items.entry(*key).or_default();
            if item.stack_size.is_some_and(|size| size != *limit) {
                return Err(bad(format!(
                    "Item {key} has conflicting Marketplace and Workshop stack sizes"
                )));
            }
            item.stack_size = Some(*limit);
        }
        Ok(Some(request))
    }
}
#[derive(Deserialize)]
struct MountRow {
    key: u32,
    name: String,
    sha256: String,
    fields: Vec<Stat>,
}
#[derive(Deserialize)]
struct Stat {
    offset: usize,
    before: i64,
    after: i64,
    recipe: String,
    level: u32,
    name: String,
}

pub(super) fn apply(
    blobs: &BTreeMap<String, Vec<u8>>,
    selection: &Selection,
    replacements: &mut BTreeMap<&'static str, BTreeMap<u32, Vec<u8>>>,
    changes: &mut Vec<Change>,
) -> Result<()> {
    effects::apply(blobs, &selection.weapon_effects, replacements, changes)?;
    if selection.free_crafting_currency {
        crafting::apply_currency(blobs, replacements, changes)?;
    }
    if selection.free_upgrade_materials {
        crafting::apply_upgrades(blobs, replacements, changes)?;
    }
    if !selection.faster_mounts && !selection.no_mount_stamina {
        return Ok(());
    }
    let table = super::table(blobs, "characterinfo")?;
    let recipes: Vec<MountRow> =
        serde_json::from_str(include_str!("../../schemas/marketplace-mounts-2976.json"))?;
    for recipe in recipes {
        let original = table
            .record_bytes(recipe.key)
            .ok_or_else(|| bad("Marketplace mount missing"))?;
        if hash_bytes(original) != recipe.sha256 {
            return Err(bad("Marketplace mount schema changed; nothing written"));
        }
        let rows = replacements.entry("characterinfo").or_default();
        let row = rows.entry(recipe.key).or_insert_with(|| original.to_vec());
        for stat in recipe.fields {
            let selected = match stat.recipe.as_str() {
                "faster_mounts" => selection.faster_mounts,
                "no_mount_stamina" => selection.no_mount_stamina,
                _ => return Err(bad("Unknown marketplace recipe")),
            };
            if !selected {
                continue;
            }
            let bytes = row
                .get_mut(stat.offset..stat.offset + 8)
                .ok_or_else(|| bad("Marketplace stat outside row"))?;
            let before = i64::from_le_bytes(bytes.try_into().unwrap());
            if before != stat.before {
                return Err(bad(
                    "Another setting conflicts with the marketplace mount recipe",
                ));
            }
            bytes.copy_from_slice(&stat.after.to_le_bytes());
            changes.push(Change {
                module: "Marketplace",
                table: "characterinfo",
                key: recipe.key,
                name: recipe.name.clone(),
                field: format!("Level {} · {}", stat.level, stat.name),
                before: before.to_string(),
                after: stat.after.to_string(),
            });
        }
    }
    Ok(())
}

pub(super) fn lion(archive: &crimson_format::Archive) -> Result<crimson_format::overlay::Overlay> {
    let entry = archive
        .list_group("0009")?
        .into_iter()
        .find(|e| {
            e.directory == crimson_format::overlay::LION_DIRECTORY
                && e.name == crimson_format::overlay::LION_FILE
        })
        .ok_or_else(|| bad("Verified lion appearance absent"))?;
    let original = archive.extract(&entry)?;
    if hash_bytes(&original) != "19e0ea0f566156db3b60beffe961b366b7bdb97b5537351f76057962436f1adf" {
        return Err(bad("Lion appearance changed; nothing written"));
    }
    let bytes = scale_lion(&original)?;
    let pamt = archive.serialize_group("0009")?;
    Ok(crimson_format::overlay::build_lion(
        bytes,
        pamt[8..12].try_into().unwrap(),
    )?)
}
fn scale_lion(original: &[u8]) -> Result<Vec<u8>> {
    let text = std::str::from_utf8(original).map_err(|_| bad("Lion appearance is not UTF-8"))?;
    if text.matches("CharacterScale=").count() != 1
        || text.matches("CharacterScale=\"0.9\"").count() != 1
    {
        return Err(bad("Unexpected lion scale"));
    }
    Ok(text
        .replacen("CharacterScale=\"0.9\"", "CharacterScale=\"1.17\"", 1)
        .into_bytes())
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn automatic_fishing_preserves_other_item_edits_and_refuses_conflicting_levels() {
        let selection = Selection {
            automatic_fishing: true,
            ..Default::default()
        };
        let mut workshop = super::super::advanced::AdvancedRequest::default();
        workshop.items.entry(11001).or_default().stack_size = Some(1);
        workshop.items.entry(123).or_default().stack_size = Some(50);
        let merged = selection.merge_advanced(Some(&workshop)).unwrap().unwrap();
        assert_eq!(merged.items[&123].stack_size, Some(50));
        assert_eq!(merged.items[&11001].stack_size, Some(1));
        assert!(
            !merged.items.contains_key(&1000088),
            "the original auto-reel rod is unchanged"
        );
        for key in [11001, 1000639, 1000640] {
            assert_eq!(merged.items[&key].buffs.len(), 1);
            let buff = &merged.items[&key].buffs[0];
            assert_eq!((buff.enchant_level, buff.buff, buff.level), (0, 1000195, 1));
        }
        assert!(workshop.items[&11001].buffs.is_empty());
        assert_eq!(
            selection.merge_advanced(Some(&merged)).unwrap().unwrap(),
            merged
        );
        let mut conflict = merged;
        conflict.items.get_mut(&11001).unwrap().buffs[0].level = 2;
        assert!(selection.merge_advanced(Some(&conflict)).is_err());
    }
    #[test]
    fn marketplace_presets_merge_without_discarding_workshop_edits() {
        let mut workshop = super::super::advanced::AdvancedRequest::default();
        workshop
            .fields
            .insert("inventory/3/maxSlotCount".into(), "500".into());
        workshop.items.entry(123).or_default().free_repair = false;
        let selection = Selection {
            inventory_999: true,
            no_durability_loss: true,
            item_stacks: BTreeMap::from([(123, 999)]),
            ..Default::default()
        };
        let merged = selection.merge_advanced(Some(&workshop)).unwrap().unwrap();
        assert_eq!(merged.fields["inventory/2/defaultSlotCount"], "999");
        assert_eq!(merged.fields["inventory/2/maxSlotCount"], "999");
        assert_eq!(merged.fields["inventory/3/maxSlotCount"], "500");
        assert!(merged.no_wear);
        assert_eq!(merged.items[&123].stack_size, Some(999));
        assert!(!workshop.no_wear && workshop.items[&123].stack_size.is_none());
        assert_eq!(
            Selection::default()
                .merge_advanced(Some(&workshop))
                .unwrap()
                .unwrap(),
            workshop
        );
        workshop
            .fields
            .insert("inventory/2/maxSlotCount".into(), "300".into());
        assert!(selection.merge_advanced(Some(&workshop)).is_err());
        workshop.fields.remove("inventory/2/maxSlotCount");
        workshop.items.get_mut(&123).unwrap().stack_size = Some(50);
        assert!(selection.merge_advanced(Some(&workshop)).is_err());
        for size in [0, 1_000_001] {
            assert!(
                Selection {
                    item_stacks: BTreeMap::from([(123, size)]),
                    ..Default::default()
                }
                .merge_advanced(None)
                .is_err()
            );
        }
    }
    #[test]
    #[ignore = "requires read-only private game archive root"]
    fn real_lion_group_extracts_and_validates_the_overlay() {
        let game = std::path::PathBuf::from(
            std::env::var_os("CD_MARKET_LION_GAME").expect("private game root required"),
        );
        let registry = std::fs::read(game.join(".workbench/baseline.papgt")).unwrap();
        let archive = crimson_format::Archive::with_registry(&game, registry).unwrap();
        let result = lion(&archive).unwrap();
        assert_eq!(
            crimson_format::overlay::validate_pair(&result.pamt, &result.paz)
                .unwrap()
                .len(),
            1
        );
    }
    #[test]
    #[ignore = "requires private original character table fixtures; never writes the game"]
    fn mount_combinations_change_only_selected_fields() {
        let dir = std::path::PathBuf::from(
            std::env::var_os("CD_MARKET_MOUNT_FIXTURE").expect("private fixture required"),
        );
        let blobs: BTreeMap<_, _> = ["staticinfobody", "staticinfoheader"]
            .into_iter()
            .map(|ext| {
                let name = format!("characterinfo.{ext}");
                (name.clone(), std::fs::read(dir.join(name)).unwrap())
            })
            .collect();
        let original = super::super::table(&blobs, "characterinfo").unwrap();
        for (speed, stamina, expected) in [(true, false, 6), (false, true, 12), (true, true, 18)] {
            let mut replacements = BTreeMap::new();
            let mut changes = Vec::new();
            apply(
                &blobs,
                &Selection {
                    faster_mounts: speed,
                    no_mount_stamina: stamina,
                    ..Default::default()
                },
                &mut replacements,
                &mut changes,
            )
            .unwrap();
            assert_eq!(changes.len(), expected);
            let rows = &replacements["characterinfo"];
            assert_eq!(
                rows.keys().copied().collect::<Vec<_>>(),
                vec![31378, 1002316]
            );
            let (body, header) = original.rebuild(rows).unwrap();
            let rebuilt = crate::tables::IndexedTable::parse(
                crate::tables::schema("characterinfo").unwrap(),
                &body,
                &header,
            )
            .unwrap();
            for record in original.records() {
                if !rows.contains_key(&record.key) {
                    assert_eq!(
                        original.record_bytes(record.key),
                        rebuilt.record_bytes(record.key)
                    );
                }
            }
            let recipes: Vec<MountRow> =
                serde_json::from_str(include_str!("../../schemas/marketplace-mounts-2976.json"))
                    .unwrap();
            for recipe in recipes {
                let mut inverse = rows[&recipe.key].clone();
                for stat in recipe.fields {
                    if (stat.recipe == "faster_mounts" && speed)
                        || (stat.recipe == "no_mount_stamina" && stamina)
                    {
                        inverse[stat.offset..stat.offset + 8]
                            .copy_from_slice(&stat.before.to_le_bytes());
                    }
                }
                assert_eq!(inverse, original.record_bytes(recipe.key).unwrap());
            }
        }
    }
    #[test]
    fn scale_preserves_every_other_byte_and_rejects_ambiguous_input() {
        let original = b"\xef\xbb\xbf<Appearance CharacterScale=\"0.9\" Other=\"0.9\"/>\r\n";
        let after = scale_lion(original).unwrap();
        assert_eq!(
            String::from_utf8(after.clone())
                .unwrap()
                .replace("CharacterScale=\"1.17\"", "CharacterScale=\"0.9\"")
                .as_bytes(),
            original
        );
        let overlay = crimson_format::overlay::build_lion(after, [0; 4]).unwrap();
        assert_eq!(
            crimson_format::overlay::validate_pair(&overlay.pamt, &overlay.paz).unwrap(),
            vec![format!(
                "{}/{}",
                crimson_format::overlay::LION_DIRECTORY,
                crimson_format::overlay::LION_FILE
            )]
        );
        for source in [
            b"CharacterScale=\"1.17\"".as_slice(),
            b"CharacterScale=\"0.9\" CharacterScale=\"0.9\"",
        ] {
            assert!(scale_lion(source).is_err());
        }
    }
}
