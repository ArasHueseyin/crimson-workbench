//! Transfer complete, observed equipped-passive references, not cosmetic-only buffs.
use crate::{
    Result,
    fingerprint::hash_bytes,
    mods::{Change, Choice, advanced, bad, table},
};
use crimson_format::item_mods::{self, EquippedPassive, WeaponEffect};
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct WeaponEffectEdit {
    pub source: u32,
    #[serde(default)]
    pub replace_existing: bool,
}
#[derive(Serialize)]
pub struct EffectTarget {
    pub key: u32,
    pub name: String,
    pub existing: Vec<EquippedPassive>,
}
#[derive(Serialize)]
pub struct EffectSource {
    pub source: u32,
    pub source_name: &'static str,
    pub effect_name: &'static str,
    pub skill: u32,
    pub level: u32,
    pub targets: Vec<EffectTarget>,
}
const SOURCES: [(u32, &str, &str, &str, WeaponEffect); 2] = [
    (
        1001062,
        "Electro-Mecha Longsword",
        "Lightning weapon attack",
        "7c8ac99ca67e791608009418248a23506234cc8c2e02c811cea4cfa41892be54",
        WeaponEffect::Lightning,
    ),
    (
        1000451,
        "Flame Spear",
        "Fire weapon attack",
        "1ceb58b626662a132b6b409d04fb0c060a978e4ed17c2333a6d1eadccfe5ca03",
        WeaponEffect::Fire,
    ),
];

fn validate(blobs: &BTreeMap<String, Vec<u8>>) -> Result<()> {
    // Pins the skill-side passive/trigger chain and its native projectile,
    // condition and visual/material references. No skill body is changed.
    if blobs
        .get("skill.staticinfobody")
        .map(|b| hash_bytes(b))
        .as_deref()
        != Some("1c62b93ea8ec32445d4078becd8e8b451e13ab519bf638721a0ed8258ac01f41")
    {
        return Err(bad(
            "Native weapon effect definitions changed; nothing written",
        ));
    }
    let items = table(blobs, "iteminfo")?;
    for (key, _, _, hash, effect) in SOURCES {
        let row = items
            .record_bytes(key)
            .ok_or_else(|| bad("Native weapon effect source absent"))?;
        if hash_bytes(row) != hash
            || item_mods::item_mod_info(row)?.passive_skills != [effect.passive()]
        {
            return Err(bad("Native weapon effect source changed; nothing written"));
        }
    }
    for (key, hash, equip, category) in [
        (
            1001163,
            "8c1a4c364ce4c23bbd3dff293ee3d1d64270084d40b499b5881ec380c605b822",
            2914941932,
            205,
        ),
        (
            1002741,
            "0dba31e0fd8d8844e7f3f0a02f808bee14bd395b1c16afb30318e850f395e250",
            604374103,
            212,
        ),
    ] {
        let row = items
            .record_bytes(key)
            .ok_or_else(|| bad("Native lightning compatibility source absent"))?;
        let info = item_mods::item_mod_info(row)?;
        if hash_bytes(row) != hash
            || (info.equip_type, info.category) != (equip, category)
            || info.passive_skills != [WeaponEffect::Lightning.passive()]
        {
            return Err(bad("Native weapon compatibility changed; nothing written"));
        }
    }
    Ok(())
}

pub(crate) fn choices(
    blobs: &BTreeMap<String, Vec<u8>>,
    advanced: &advanced::Catalog,
    items: &[Choice],
) -> Result<Vec<EffectSource>> {
    validate(blobs)?;
    let mut result = Vec::new();
    for (source, source_name, effect_name, _, effect) in SOURCES {
        let mut targets = Vec::new();
        for item in items {
            if SOURCES.iter().any(|s| s.0 == item.key) {
                continue;
            }
            let info = advanced.item(item.key)?.item;
            if effect.compatible(&info) {
                targets.push(EffectTarget {
                    key: item.key,
                    name: item.name.clone(),
                    existing: info.passive_skills,
                });
            }
        }
        targets.sort_by(|a, b| a.name.cmp(&b.name).then(a.key.cmp(&b.key)));
        let passive = effect.passive();
        result.push(EffectSource {
            source,
            source_name,
            effect_name,
            skill: passive.skill,
            level: passive.level,
            targets,
        });
    }
    Ok(result)
}

pub(crate) fn apply(
    blobs: &BTreeMap<String, Vec<u8>>,
    edits: &BTreeMap<u32, WeaponEffectEdit>,
    replacements: &mut BTreeMap<&'static str, BTreeMap<u32, Vec<u8>>>,
    changes: &mut Vec<Change>,
) -> Result<()> {
    if edits.is_empty() {
        return Ok(());
    }
    if edits.len() > 256 {
        return Err(bad("Too many native weapon effect transfers"));
    }
    validate(blobs)?;
    if let Some(skills) = replacements.get("skill") {
        let original = table(blobs, "skill")?;
        for key in [91101, 92011, 91105, 92013] {
            if let Some(bytes) = skills.get(&key)
                && Some(bytes.as_slice()) != original.record_bytes(key)
            {
                return Err(bad(
                    "A Workshop edit conflicts with a native weapon effect definition",
                ));
            }
        }
    }
    let items = table(blobs, "iteminfo")?;
    for (key, edit) in edits {
        let (_, _, label, _, effect) = SOURCES
            .iter()
            .find(|s| s.0 == edit.source)
            .ok_or_else(|| bad("Unknown native weapon effect source"))?;
        let original = items
            .record_bytes(*key)
            .ok_or_else(|| bad("Unknown target weapon"))?;
        let rows = replacements.entry("iteminfo").or_default();
        let bytes = rows.get(key).map(Vec::as_slice).unwrap_or(original);
        let (output, diff) =
            item_mods::transfer_weapon_effect(bytes, *effect, edit.replace_existing)?;
        if !diff.is_empty() {
            rows.insert(*key, output);
            for change in diff {
                changes.push(Change {
                    module: "Marketplace",
                    table: "iteminfo",
                    key: *key,
                    name: format!("Item {key}"),
                    field: format!("{label} · {}", change.field),
                    before: change.before,
                    after: change.after,
                });
            }
        }
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn missing_or_changed_native_definitions_refuse_without_edits() {
        let mut rows = BTreeMap::new();
        let mut changes = vec![];
        assert!(
            apply(
                &BTreeMap::new(),
                &BTreeMap::from([(
                    123,
                    WeaponEffectEdit {
                        source: 1001062,
                        replace_existing: false,
                    }
                )]),
                &mut rows,
                &mut changes
            )
            .is_err()
        );
        assert!(rows.is_empty() && changes.is_empty());
    }
    #[test]
    #[ignore = "requires private original table fixtures; read only"]
    fn workshop_cannot_silently_change_the_transferred_native_skill() {
        let root = std::path::PathBuf::from(
            std::env::var_os("CD_WEAPON_EFFECT_FIXTURE").expect("private fixtures required"),
        );
        let blobs: BTreeMap<_, _> = [
            "iteminfo.staticinfobody",
            "iteminfo.staticinfoheader",
            "skill.staticinfobody",
            "skill.staticinfoheader",
        ]
        .into_iter()
        .map(|name| (name.into(), std::fs::read(root.join(name)).unwrap()))
        .collect();
        let items = table(&blobs, "iteminfo").unwrap();
        let target = items
            .records()
            .iter()
            .find(|row| {
                let info = item_mods::item_mod_info(items.record_bytes(row.key).unwrap()).unwrap();
                info.equip_type == 1086980073 && info.passive_skills.is_empty()
            })
            .unwrap()
            .key;
        let original = table(&blobs, "skill").unwrap().record_bytes(91101).unwrap();
        let cooldown = advanced::skill_detail::inspect(original)
            .unwrap()
            .record_fields
            .into_iter()
            .find(|f| f.path == "cooltime")
            .unwrap();
        let mut changed = original.to_vec();
        changed[cooldown.offset..cooldown.offset + 4].copy_from_slice(&1u32.to_le_bytes());
        advanced::skill_detail::inspect(&changed).unwrap();
        let edits = BTreeMap::from([(
            target,
            WeaponEffectEdit {
                source: 1001062,
                replace_existing: false,
            },
        )]);
        let mut replacements =
            BTreeMap::from([("skill", BTreeMap::from([(91101, changed.clone())]))]);
        let mut changes = vec![];
        let error = apply(&blobs, &edits, &mut replacements, &mut changes).unwrap_err();
        assert!(error.to_string().contains("Workshop edit conflicts"));
        assert_eq!(replacements.len(), 1);
        assert_eq!(replacements["skill"][&91101], changed);
        assert!(changes.is_empty());
        replacements
            .get_mut("skill")
            .unwrap()
            .insert(91101, original.to_vec());
        apply(&blobs, &edits, &mut replacements, &mut changes).unwrap();
        assert_eq!(changes.len(), 1);
        assert_eq!(
            item_mods::item_mod_info(&replacements["iteminfo"][&target])
                .unwrap()
                .passive_skills,
            [WeaponEffect::Lightning.passive()]
        );
    }
}
