//! Bounded item edits in memory using the complete, pinned 2.03 item schema.
//! Gameplay eligibility and known status/buff references are checked by cd-core.
use crate::{
    binary::{BinaryRead, BinaryWrite},
    invalid,
    item_info::{
        ItemInfo,
        keys::{BuffKey, StatusKey},
        structs::{
            EnchantData, EnchantDataList, EnchantLevelChange, EnchantStatChange, EquipmentBuff,
        },
    },
};
use serde::{Deserialize, Serialize};
use std::{collections::BTreeSet, io};

#[derive(Clone, Debug, Default, PartialEq, Eq, Deserialize, Serialize)]
#[serde(default, deny_unknown_fields)]
pub struct ItemEdit {
    pub stack_size: Option<u64>,
    /// Retained for old requests/templates; true is rejected, never serialized as a mod.
    pub free_repair: bool,
    pub stats: Vec<StatEdit>,
    pub buffs: Vec<BuffEdit>,
    #[serde(skip_serializing_if = "Vec::is_empty")]
    pub enchant_copies: Vec<EnchantCopy>,
}
#[derive(Clone, Debug, PartialEq, Eq, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct EnchantCopy {
    pub source_level: u16,
    pub target_level: u16,
}
#[derive(Clone, Debug, PartialEq, Eq, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StatEdit {
    pub enchant_level: u16,
    pub list: StatList,
    pub stat: u32,
    pub value: String,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum StatList {
    Maximum,
    Regeneration,
    Static,
    PerLevel,
}
#[derive(Clone, Debug, PartialEq, Eq, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct BuffEdit {
    pub enchant_level: u16,
    pub buff: u32,
    pub level: u32,
}
#[derive(Clone, Debug, Serialize)]
pub struct ItemModInfo {
    pub key: u32,
    pub category: u16,
    pub equip_type: u32,
    pub stack_size: String,
    pub stack_risk: bool,
    pub max_endurance: u16,
    pub repair_entries: usize,
    pub enchant_levels: Vec<u16>,
    pub can_copy_enchants: bool,
    pub stats: Vec<StatEdit>,
    pub buffs: Vec<BuffEdit>,
}
#[derive(Debug)]
pub struct ItemFieldChange {
    pub field: String,
    pub before: String,
    pub after: String,
}

fn parse(bytes: &[u8]) -> io::Result<ItemInfo<'_>> {
    let mut at = 0;
    let item = ItemInfo::read_from(bytes, &mut at)?;
    let mut output = Vec::new();
    item.write_to(&mut output)?;
    if at != bytes.len() || output != bytes {
        return Err(invalid("item edit source failed complete roundtrip"));
    }
    Ok(item)
}
pub fn item_mod_info(bytes: &[u8]) -> io::Result<ItemModInfo> {
    let i = parse(bytes)?;
    let mut stats = Vec::new();
    let mut buffs = Vec::new();
    let mut levels = BTreeSet::new();
    for e in &i.enchant_data_list.items {
        if !levels.insert(e.level) {
            return Err(invalid("duplicate enchant level"));
        }
        let s = &e.enchant_stat_data;
        for (list, values) in [
            (StatList::Maximum, &s.max_stat_list.items),
            (StatList::Regeneration, &s.regen_stat_list.items),
            (StatList::Static, &s.stat_list_static.items),
        ] {
            for v in values {
                stats.push(StatEdit {
                    enchant_level: e.level,
                    list,
                    stat: v.stat.0,
                    value: v.change_mb.to_string(),
                });
            }
        }
        for v in &s.stat_list_static_level.items {
            stats.push(StatEdit {
                enchant_level: e.level,
                list: StatList::PerLevel,
                stat: v.stat.0,
                value: v.change_mb.to_string(),
            });
        }
        for b in &e.equip_buffs.items {
            buffs.push(BuffEdit {
                enchant_level: e.level,
                buff: b.buff.0,
                level: b.level,
            });
        }
    }
    Ok(ItemModInfo {
        key: i.key.0,
        category: i.category_info.0,
        equip_type: i.equip_type_info.0,
        stack_size: i.max_stack_count.to_string(),
        max_endurance: i.max_endurance,
        stack_risk: i.equip_type_info.0 != 0
            || (i.max_endurance != 0 && i.max_endurance != u16::MAX)
            || i.enchant_data_list.items.len() > 1
            || !i.equip_passive_skill_list.items.is_empty()
            || !buffs.is_empty(),
        repair_entries: i.repair_data_list.items.len(),
        enchant_levels: levels.into_iter().collect(),
        can_copy_enchants: copyable_enchants(&i.enchant_data_list),
        stats,
        buffs,
    }
    .with_instance_risk(
        i.drop_default_data.use_socket != 0
            || i.drop_default_data.socket_valid_count != 0
            || !i.drop_default_data.socket_item_list.items.is_empty()
            || i.item_charge_type != 2
            || i.max_charged_useable_count > 1
            || i.sharpness_data.max_sharpness != 0
            || !matches!(
                i.default_sub_item.value,
                crate::item_info::structs::SubItemValue::None
            )
            || !matches!(
                i.drop_default_data.default_sub_item.value,
                crate::item_info::structs::SubItemValue::None
            )
            || i.is_all_gimmick_sealable != 0
            || !i.sealable_item_info_list.items.is_empty()
            || !i.sealable_character_info_list.items.is_empty()
            || !i.sealable_gimmick_info_list.items.is_empty()
            || !i.sealable_gimmick_tag_list.items.is_empty()
            || !i.sealable_tribe_info_list.items.is_empty()
            || !i.sealable_money_info_list.items.is_empty(),
    ))
}
impl ItemModInfo {
    fn with_instance_risk(mut self, risk: bool) -> Self {
        self.stack_risk |= risk;
        self
    }
}
/// The pinned engine's item/endurance updater (0x14240d650) bypasses the
/// instance write when maxEndurance is 0xffff, including socketed sub-items.
/// Preserve items without endurance and those already using that sentinel.
pub fn prevent_durability_loss(bytes: &[u8]) -> io::Result<(Vec<u8>, Vec<ItemFieldChange>)> {
    let mut item = parse(bytes)?;
    if matches!(item.max_endurance, 0 | u16::MAX) {
        return Ok((bytes.to_vec(), Vec::new()));
    }
    let change = ItemFieldChange {
        field: "max_endurance".into(),
        before: item.max_endurance.to_string(),
        after: u16::MAX.to_string(),
    };
    item.max_endurance = u16::MAX;
    let mut output = Vec::new();
    item.write_to(&mut output)?;
    parse(&output)?;
    Ok((output, vec![change]))
}
pub fn edit_item(bytes: &[u8], edit: &ItemEdit) -> io::Result<(Vec<u8>, Vec<ItemFieldChange>)> {
    // The pinned engine divides by the resulting resource cost without a zero
    // guard (0x14240e0e5, 0x142be406d). Zeroing the list is not a free-repair path.
    if edit.free_repair {
        return Err(invalid(
            "free repair is unsupported: zero resource cost can be a divisor",
        ));
    }
    if edit.stats.len() > 256 || edit.buffs.len() > 64 {
        return Err(invalid("too many item edits"));
    }
    let mut i = parse(bytes)?;
    let mut changes = Vec::new();
    copy_enchants(&mut i.enchant_data_list, &edit.enchant_copies, &mut changes)?;
    let mut seen = BTreeSet::new();
    if let Some(n) = edit.stack_size {
        if n == 0 || n > 1_000_000 {
            return Err(invalid(
                "Workbench stack input limit: 1..1000000; engine maximum unverified",
            ));
        }
        if n != i.max_stack_count {
            changes.push(ItemFieldChange {
                field: "max_stack_count".into(),
                before: i.max_stack_count.to_string(),
                after: n.to_string(),
            });
            i.max_stack_count = n;
        }
    }
    for s in &edit.stats {
        if !seen.insert((s.enchant_level, s.list, s.stat)) {
            return Err(invalid("duplicate item stat edit"));
        }
        let v = s
            .value
            .parse::<i64>()
            .map_err(|_| invalid("stat value must be a signed decimal integer"))?;
        if v.unsigned_abs() > 1_000_000_000 {
            return Err(invalid("Workbench stat input limit exceeded"));
        }
        let e = i
            .enchant_data_list
            .items
            .iter_mut()
            .find(|e| e.level == s.enchant_level)
            .ok_or_else(|| invalid("item has no such enchant level"))?;
        let before = if s.list == StatList::PerLevel {
            let v = i8::try_from(v).map_err(|_| invalid("per-level stat needs -128..127"))?;
            let list = &mut e.enchant_stat_data.stat_list_static_level.items;
            if list.iter().filter(|x| x.stat.0 == s.stat).count() > 1 {
                return Err(invalid("ambiguous duplicate source stat"));
            }
            match list.iter_mut().find(|x| x.stat.0 == s.stat) {
                Some(x) => {
                    let old = x.change_mb.to_string();
                    x.change_mb = v;
                    old
                }
                None => {
                    list.push(EnchantLevelChange {
                        stat: StatusKey(s.stat),
                        change_mb: v,
                    });
                    "nicht vorhanden".into()
                }
            }
        } else {
            let list = match s.list {
                StatList::Maximum => &mut e.enchant_stat_data.max_stat_list.items,
                StatList::Regeneration => &mut e.enchant_stat_data.regen_stat_list.items,
                StatList::Static => &mut e.enchant_stat_data.stat_list_static.items,
                StatList::PerLevel => unreachable!(),
            };
            if list.iter().filter(|x| x.stat.0 == s.stat).count() > 1 {
                return Err(invalid("ambiguous duplicate source stat"));
            }
            match list.iter_mut().find(|x| x.stat.0 == s.stat) {
                Some(x) => {
                    let old = x.change_mb.to_string();
                    x.change_mb = v;
                    old
                }
                None => {
                    list.push(EnchantStatChange {
                        stat: StatusKey(s.stat),
                        change_mb: v,
                    });
                    "nicht vorhanden".into()
                }
            }
        };
        if before != v.to_string() {
            changes.push(ItemFieldChange {
                field: format!("enchant[{}].{:?}.stat[{}]", s.enchant_level, s.list, s.stat),
                before,
                after: v.to_string(),
            });
        }
    }
    let mut seen = BTreeSet::new();
    for b in &edit.buffs {
        if b.level == 0 || b.level > 100 || !seen.insert((b.enchant_level, b.buff)) {
            return Err(invalid("invalid/duplicate buff edit"));
        }
        let e = i
            .enchant_data_list
            .items
            .iter_mut()
            .find(|e| e.level == b.enchant_level)
            .ok_or_else(|| invalid("item has no such enchant level"))?;
        if e.equip_buffs
            .items
            .iter()
            .filter(|x| x.buff.0 == b.buff)
            .count()
            > 1
        {
            return Err(invalid("ambiguous source buff"));
        }
        let before = match e.equip_buffs.items.iter_mut().find(|x| x.buff.0 == b.buff) {
            Some(x) => {
                let old = x.level.to_string();
                x.level = b.level;
                old
            }
            None => {
                e.equip_buffs.items.push(EquipmentBuff {
                    buff: BuffKey(b.buff),
                    level: b.level,
                });
                "nicht vorhanden".into()
            }
        };
        if before != b.level.to_string() {
            changes.push(ItemFieldChange {
                field: format!("enchant[{}].buff[{}].level", b.enchant_level, b.buff),
                before,
                after: b.level.to_string(),
            });
        }
    }
    let mut output = Vec::new();
    i.write_to(&mut output)?;
    parse(&output)?;
    Ok((output, changes))
}

fn copyable_enchants(list: &EnchantDataList) -> bool {
    !list.items.is_empty()
        && list.items.len() < 256
        && list.separators.iter().all(|v| *v == 0)
        && list.items.windows(2).all(|p| p[0].level < p[1].level)
}

fn copy_enchants(
    list: &mut EnchantDataList,
    copies: &[EnchantCopy],
    changes: &mut Vec<ItemFieldChange>,
) -> io::Result<()> {
    if copies.is_empty() {
        return Ok(());
    }
    if copies.len() > 64 || list.items.len() + copies.len() > 256 || !copyable_enchants(list) {
        return Err(invalid(
            "enchant copy requires ordered original levels, zero separators and at most 64 new/256 total rows",
        ));
    }
    let mut targets: BTreeSet<_> = list.items.iter().map(|e| e.level).collect();
    let mut new_rows = Vec::new();
    for copy in copies {
        if !targets.insert(copy.target_level) {
            return Err(invalid("enchant target already exists or is duplicated"));
        }
        let source = list
            .items
            .iter()
            .find(|e| e.level == copy.source_level)
            .ok_or_else(|| invalid("enchant source must be an original level"))?;
        let mut bytes = Vec::new();
        source.write_to(&mut bytes)?;
        let mut at = 0;
        let mut row = EnchantData::read_from(&bytes, &mut at)?;
        if at != bytes.len() {
            return Err(invalid("enchant copy roundtrip failed"));
        }
        row.level = copy.target_level;
        new_rows.push(row);
    }
    for copy in copies {
        changes.push(ItemFieldChange {
            field: format!("enchant[{}]", copy.target_level),
            before: "nicht vorhanden".into(),
            after: format!("Kopie von Originalstufe {}", copy.source_level),
        });
    }
    list.items.extend(new_rows);
    list.items.sort_by_key(|e| e.level);
    list.separators = vec![0; list.items.len() - 1];
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn legacy_free_repair_requests_never_reach_the_item_writer() {
        let edit: ItemEdit =
            serde_json::from_str(r#"{"stack_size":500,"free_repair":true,"stats":[],"buffs":[]}"#)
                .unwrap();
        let error = edit_item(&[], &edit).unwrap_err();
        assert!(error.to_string().contains("free repair is unsupported"));
        // Keep old saved settings readable so the unsupported flag can be removed.
        let clean = ItemEdit {
            free_repair: false,
            ..edit
        };
        let encoded = serde_json::to_string(&clean).unwrap();
        let decoded: ItemEdit = serde_json::from_str(&encoded).unwrap();
        assert_eq!(decoded, clean);
        assert_eq!(decoded.stack_size, Some(500));
    }
    fn row(level: u16) -> EnchantData {
        let mut b = level.to_le_bytes().to_vec();
        b.extend(1u32.to_le_bytes()); // maximum stat
        b.extend(1000026u32.to_le_bytes());
        b.extend((-123i64).to_le_bytes());
        b.extend([0; 12]); // remaining stat lists
        b.extend(1u32.to_le_bytes()); // price list
        b.extend(2200u32.to_le_bytes());
        b.extend(234u64.to_le_bytes());
        b.extend(9u32.to_le_bytes());
        b.extend(2201u32.to_le_bytes());
        b.extend(1u32.to_le_bytes()); // buff list
        b.extend(42u32.to_le_bytes());
        b.extend(3u32.to_le_bytes());
        let mut at = 0;
        let row = EnchantData::read_from(&b, &mut at).unwrap();
        assert_eq!(at, b.len());
        row
    }
    fn bytes(row: &EnchantData) -> Vec<u8> {
        let mut v = vec![];
        row.write_to(&mut v).unwrap();
        v
    }
    #[test]
    fn enchant_copy_preserves_all_contents_and_rebuilds_sorted_separators() {
        let mut list = EnchantDataList {
            items: vec![row(0), row(2)],
            separators: vec![0],
        };
        let source = bytes(&list.items[0]);
        let other = bytes(&list.items[1]);
        let mut changes = vec![];
        copy_enchants(
            &mut list,
            &[
                EnchantCopy {
                    source_level: 0,
                    target_level: 3,
                },
                EnchantCopy {
                    source_level: 2,
                    target_level: 1,
                },
            ],
            &mut changes,
        )
        .unwrap();
        assert_eq!(
            list.items.iter().map(|v| v.level).collect::<Vec<_>>(),
            vec![0, 1, 2, 3]
        );
        assert_eq!(bytes(&list.items[0]), source);
        assert_eq!(bytes(&list.items[2]), other);
        assert_eq!(bytes(&list.items[3])[2..], source[2..]);
        assert_eq!(bytes(&list.items[1])[2..], other[2..]);
        assert_eq!(list.separators, vec![0, 0, 0]);
        assert_eq!(changes.len(), 2);
        let mut b = vec![];
        list.write_to(&mut b).unwrap();
        let mut at = 0;
        let decoded = EnchantDataList::read_from(&b, &mut at).unwrap();
        assert_eq!(at, b.len());
        assert_eq!(decoded.items.len(), 4);
        assert_eq!(decoded.separators, vec![0, 0, 0]);
    }
    #[test]
    fn enchant_copy_rejects_ambiguous_layouts_and_chained_or_duplicate_targets() {
        for copies in [
            vec![(0, 0)],
            vec![(9, 1)],
            vec![(0, 1), (0, 1)],
            vec![(0, 1), (1, 2)],
        ] {
            let mut list = EnchantDataList {
                items: vec![row(0)],
                separators: vec![],
            };
            let before = bytes(&list.items[0]);
            let q = copies
                .into_iter()
                .map(|(source_level, target_level)| EnchantCopy {
                    source_level,
                    target_level,
                })
                .collect::<Vec<_>>();
            assert!(copy_enchants(&mut list, &q, &mut vec![]).is_err());
            assert_eq!(list.items.len(), 1);
            assert_eq!(bytes(&list.items[0]), before);
        }
        for (levels, separators) in [
            (vec![0, 2], vec![7]),
            (vec![2, 0], vec![0]),
            (vec![0, 0], vec![0]),
            (vec![], vec![]),
        ] {
            let mut list = EnchantDataList {
                items: levels.into_iter().map(row).collect(),
                separators,
            };
            assert!(
                copy_enchants(
                    &mut list,
                    &[EnchantCopy {
                        source_level: 0,
                        target_level: 3
                    }],
                    &mut vec![]
                )
                .is_err()
            );
        }
        let old: ItemEdit = serde_json::from_str(
            r#"{"stack_size":null,"free_repair":false,"stats":[],"buffs":[]}"#,
        )
        .unwrap();
        assert_eq!(old, ItemEdit::default());
        assert!(
            !serde_json::to_string(&old)
                .unwrap()
                .contains("enchant_copies")
        );
    }
}
