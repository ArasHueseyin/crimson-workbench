//! B4–B11 edits share B0's original snapshot, diff, archive builder and write gates.
mod flight;
pub(crate) mod reader;
pub mod skill_detail;
use super::{Change, Choice, bad, table};
use crate::{Result, tables::TableSchema};
use crimson_format::item_mods::{self, ItemEdit, ItemModInfo};
use serde::{Deserialize, Serialize};
use std::collections::{BTreeMap, BTreeSet};

#[derive(Clone, Debug, PartialEq, Eq, Deserialize, Serialize)]
#[serde(default, deny_unknown_fields)]
pub struct AdvancedRequest {
    pub spawn_percent: u32,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub patrol_reset_percent: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub reoccupation_delay_percent: Option<u32>,
    pub mount_cooldown: Option<u32>,
    pub mount_duration: Option<u32>,
    pub town_running: bool,
    pub dragon_no_cooldown: bool,
    pub dragon_regions: bool,
    pub dragon_duration: Option<u32>,
    pub stack_size: Option<u64>,
    pub stack_categories: BTreeSet<u16>,
    pub experimental_stacks: bool,
    pub no_wear: bool,
    pub free_repair: bool,
    pub cost_percent: BTreeMap<String, u32>,
    pub skill_cooldown_percent: u32,
    pub skill_cooldowns: BTreeMap<u32, u32>,
    #[serde(skip_serializing_if = "BTreeMap::is_empty")]
    pub skill_buffs: BTreeMap<u32, BTreeMap<String, String>>,
    pub fields: BTreeMap<String, String>,
    pub items: BTreeMap<u32, ItemEdit>,
}
impl Default for AdvancedRequest {
    fn default() -> Self {
        Self {
            spawn_percent: 100,
            patrol_reset_percent: None,
            reoccupation_delay_percent: None,
            mount_cooldown: None,
            mount_duration: None,
            town_running: false,
            dragon_no_cooldown: false,
            dragon_regions: false,
            dragon_duration: None,
            stack_size: None,
            stack_categories: BTreeSet::new(),
            experimental_stacks: false,
            no_wear: false,
            free_repair: false,
            cost_percent: BTreeMap::new(),
            skill_cooldown_percent: 100,
            skill_cooldowns: BTreeMap::new(),
            skill_buffs: BTreeMap::new(),
            fields: BTreeMap::new(),
            items: BTreeMap::new(),
        }
    }
}
#[derive(Clone, Serialize)]
pub struct Field {
    pub name: String,
    pub value: String,
    pub min: String,
    pub max: String,
    pub rule: String,
    #[serde(skip)]
    offset: usize,
    #[serde(skip)]
    width: usize,
    #[serde(skip)]
    signed: bool,
}
#[derive(Clone, Serialize)]
pub struct Record {
    pub table: &'static str,
    pub key: u32,
    pub name: String,
    pub module: &'static str,
    pub display: String,
    pub description: String,
    pub category: String,
    pub fields: Vec<Field>,
    #[serde(skip)]
    dragon_forbidden: Option<(usize, Vec<u8>)>,
    #[serde(skip)]
    town_dismount: bool,
}
impl Record {
    fn new(
        table: &'static str,
        key: u32,
        name: String,
        module: &'static str,
        fields: Vec<Field>,
    ) -> Self {
        let lower = name.to_lowercase();
        let category = if lower.contains("climb") {
            "climbing"
        } else if lower.contains("swim") {
            "swimming"
        } else if lower.contains("jump") {
            "jumping"
        } else if lower.contains("sprint") || lower.contains("run") {
            "running"
        } else if lower.contains("attack") || lower.contains("guard") || lower.contains("dash") {
            "combat"
        } else {
            "other"
        };
        Self {
            table,
            key,
            name,
            module,
            display: String::new(),
            description: String::new(),
            category: category.into(),
            fields,
            dragon_forbidden: None,
            town_dismount: false,
        }
    }
}
#[derive(Clone, Serialize)]
pub struct AdvancedInfo {
    pub records: Vec<Record>,
    pub statuses: Vec<Choice>,
    pub categories: Vec<Choice>,
    pub repair_items: usize,
    pub stack_safe_items: usize,
    pub finite_endurance_items: usize,
    pub limitations: Vec<String>,
}
#[derive(Serialize)]
pub struct ItemDetail {
    pub item: ItemModInfo,
    pub compatible_buffs: Vec<(u32, u32)>,
}
pub(super) struct Catalog {
    records: Vec<Record>,
    items: BTreeMap<u32, ItemModInfo>,
    statuses: Vec<Choice>,
    buffs: BTreeMap<u32, BTreeSet<(u32, u32)>>,
}
pub(super) fn schema(name: &str) -> Option<TableSchema> {
    let (name, count_bytes) = match name {
        "equiptypeinfo" => ("equiptypeinfo", 2),
        "buffinfo" => ("buffinfo", 2),
        "statusinfo" => ("statusinfo", 2),
        "conditioninfo" => ("conditioninfo", 2),
        "factionreblockadinginfo" => ("factionreblockadinginfo", 2),
        "spawningpoolautospawninfo" => ("spawningpoolautospawninfo", 4),
        "terrainregionautospawninfo" => ("terrainregionautospawninfo", 4),
        _ => return None,
    };
    Some(TableSchema {
        name,
        count_bytes,
        key_bytes: if name == "factionreblockadinginfo" {
            2
        } else {
            4
        },
        interpretation: "pinned advanced fields; opaque bytes preserved",
    })
}
impl Catalog {
    pub fn open(blobs: &BTreeMap<String, Vec<u8>>) -> Result<Self> {
        type Parse = fn(&[u8]) -> Result<Record>;
        let mut records = Vec::new();
        for (name, expected, parse) in [
            ("characterinfo", 7250, reader::mount as Parse),
            ("regioninfo", 1007, reader::region),
            ("inventory", 21, reader::inventory),
            ("skill", 2069, reader::skill),
            ("buffinfo", 292, reader::buff_info),
            ("equiptypeinfo", 117, reader::equip),
            ("fieldinfo", 8, reader::field),
            ("factionreblockadinginfo", 108, reader::reoccupation),
            ("spawningpoolautospawninfo", 140, reader::pool),
            ("terrainregionautospawninfo", 134, reader::terrain),
        ] {
            let t = table(blobs, name)?;
            if t.records().len() != expected {
                return Err(bad(format!("{name}: coverage changed")));
            }
            for entry in t.records() {
                let r = parse(t.record_bytes(entry.key).unwrap())
                    .map_err(|e| bad(format!("{name}/{}: {e}", entry.key)))?;
                if r.key != entry.key {
                    return Err(bad("advanced key mismatch"));
                }
                if !r.fields.is_empty() || r.dragon_forbidden.is_some() {
                    records.push(r);
                }
            }
        }
        let conditions = table(blobs, "conditioninfo")?;
        if conditions.records().len() != 10798 {
            return Err(bad("condition coverage changed"));
        }
        records.push(flight::inspect(
            conditions
                .record_bytes(1011130)
                .ok_or_else(|| bad("missing town-flight rule"))?,
        )?);
        let stages = table(blobs, "stageinfo")?;
        // The two patrol records are byte-identical across these hash-admitted
        // builds; 25455892 adds two unrelated stage records without changing any
        // existing stage. Preserve those records when rebuilding the table.
        if ![52080, 52082].contains(&stages.records().len()) {
            return Err(bad("stage coverage changed"));
        }
        for (key, name) in [
            (1017811, "Faction_Byron_HiddenEstate_Block_FactionPatrol"),
            (1002224, "Watergate_Block_Patrol"),
        ] {
            let row = reader::patrol_stage(
                stages
                    .record_bytes(key)
                    .ok_or_else(|| bad("missing patrol stage"))?,
            )?;
            if row.key != key || row.name != name {
                return Err(bad("patrol identity changed"));
            }
            records.push(row);
        }
        let mut items = BTreeMap::new();
        let mut buffs: BTreeMap<u32, BTreeSet<(u32, u32)>> = BTreeMap::new();
        let t = table(blobs, "iteminfo")?;
        for entry in t.records() {
            let i = item_mods::item_mod_info(t.record_bytes(entry.key).unwrap())?;
            if i.key != entry.key {
                return Err(bad("item key mismatch"));
            }
            for b in &i.buffs {
                buffs
                    .entry(i.equip_type)
                    .or_default()
                    .insert((b.buff, b.level));
            }
            items.insert(i.key, i);
        }
        let t = table(blobs, "statusinfo")?;
        let mut statuses = Vec::new();
        for e in t.records() {
            let (key, name) = reader::Reader::new(t.record_bytes(e.key).unwrap()).head(4)?;
            if key != e.key {
                return Err(bad("status key mismatch"));
            }
            statuses.push(Choice {
                key,
                name,
                entries: 0,
            });
        }
        Ok(Self {
            records,
            items,
            statuses,
            buffs,
        })
    }
    pub fn info(&self) -> AdvancedInfo {
        let mut categories: BTreeMap<u16, usize> = BTreeMap::new();
        for i in self.items.values() {
            *categories.entry(i.category).or_default() += 1;
        }
        AdvancedInfo{records:self.records.clone(),statuses:self.statuses.clone(),categories:categories.into_iter().map(|(key,entries)|Choice{key:key.into(),name:format!("Kategorie {key}"),entries}).collect(),repair_items:self.items.values().filter(|i|i.repair_entries>0).count(),stack_safe_items:self.items.values().filter(|i|!i.stack_risk).count(),finite_endurance_items:self.items.values().filter(|i|!matches!(i.max_endurance,0|u16::MAX)).count(),limitations:vec![
            "Steam-Builds 25381195 und 25455892. Wirkung im Spiel noch nicht abgenommen. Jede Einstellung wird aus dem Original neu aufgebaut.".into(),
            "Spawns: bestätigte Charakterzahlen in Terrain-/Pool-Gruppen. Zwei Spawn-Patrouillen haben bearbeitbare Stage-Reset-Zeiten, 108 Fraktionsgebiete eine getrennte Wiederbesetzungswartezeit; allgemeine NPC-Respawn-Zeitgeber sind nicht freigegeben; ein Spawn-Maximum von 0 bleibt bei globaler Skalierung unverändert.".into(),
            "Städte: Laufbeschränkung und die gemeinsame Stadtflug-Abstiegsregel werden aufgehoben; alwaysCallVehicle_dev nur im MainField aktiviert. Die Drachenoption entfernt Regionssperre 79 und dieselbe Stadtflug-Regel, die auch andere Flugreittiere betrifft. Quest-/Zwischensequenz-Abstiege und Flughöhe bleiben unverändert. Spielabnahme offen.".into(),
            "Slots: Workbench-Grenze 1460 aus dem Standarddeckel des geprüften Inventar-Codepfads. Weitere Engine-/Save-Grenzen und Konfigurationsabweichungen bleiben ungeprüft. Stacks: 1000000 ist die Workbench-Grenze, kein bewiesenes Engine-Maximum. Verkleinerungen benötigen Spieltests.".into(),
            "Kostenfaktoren erfassen negative Skill-/Fahrerkosten, 31 Geistverbrauchswerte zweier Eigenverbrauchs-Skills und 36 Zusatzkosten in 33 Ausrüstungs-Buffs. Ausrüstung ist separat regelbar; Skill-Kategorien folgen internen Namen. Positive Regeneration, gegnerische Drain-Buffs und andere Effekte bleiben erhalten. Keine universelle Unlimited-Zusage.".into(),
            "Graph val0..val2 sind rohe Zahlen ohne bestätigte Einheit. Item-Buffs nur mit beobachteter ID/Level-Kombination desselben Equip-Typs. Neue Enchant-Zeilen kopieren eine Originalstufe; Upgrade-Freischaltungen und bestehende Iteminstanzen werden dadurch nicht angehoben.".into(),
            "Kein Verschleiß: Equip-Typ-Faktoren auf 0 und endliche Item-Haltbarkeit auf den Engine-Sentinel 65535. Der gemeinsame Haltbarkeits-Updater überspringt diese Items. Kein Wiederherstellen bereits verbrauchter Items; Spielabnahme offen.".into(),
            "Kostenfreie Reparatur ist nicht verfügbar: Es gibt noch keinen bestätigten Tabellenpfad. Die aktuellen Item-Reparaturlisten sind leer; ein Nullsetzen von Materialkosten ist auch bei vorhandenen Regeln nicht freigegeben.".into(),
        ]}
    }
    pub fn item(&self, key: u32) -> Result<ItemDetail> {
        let item = self
            .items
            .get(&key)
            .ok_or_else(|| bad("unknown item"))?
            .clone();
        let compatible_buffs = self
            .buffs
            .get(&item.equip_type)
            .map(|v| v.iter().copied().collect())
            .unwrap_or_default();
        Ok(ItemDetail {
            item,
            compatible_buffs,
        })
    }
    pub fn apply(
        &self,
        blobs: &BTreeMap<String, Vec<u8>>,
        q: &AdvancedRequest,
        replacements: &mut BTreeMap<&'static str, BTreeMap<u32, Vec<u8>>>,
        changes: &mut Vec<Change>,
    ) -> Result<()> {
        if q.free_repair || q.items.values().any(|edit| edit.free_repair) {
            return Err(bad(
                "Kostenfreie Reparatur ist nicht verfügbar. Bitte die Reparaturoption aus den Einstellungen oder der Item-Vorlage entfernen.",
            ));
        }
        if q.spawn_percent > 10000
            || q.patrol_reset_percent.is_some_and(|v| v == 0 || v > 10000)
            || q.reoccupation_delay_percent
                .is_some_and(|v| v == 0 || v > 10000)
            || q.skill_cooldown_percent > 10000
            || q.skill_cooldowns.values().any(|v| *v > 10000)
            || q.cost_percent.values().any(|v| *v > 10000)
            || q.items.len() > 1000
            || q.fields.len() > 10000
            || q.skill_buffs.len() > 1000
            || q.skill_buffs.values().map(BTreeMap::len).sum::<usize>() > 10000
        {
            return Err(bad("advanced input limit exceeded"));
        }
        for k in q.cost_percent.keys() {
            if !["stamina", "spirit"].iter().any(|resource| {
                [
                    "climbing",
                    "swimming",
                    "jumping",
                    "running",
                    "combat",
                    "other",
                    "equipment",
                ]
                .iter()
                .any(|category| *k == format!("{resource}:{category}"))
            }) {
                return Err(bad("unknown resource/action category"));
            }
        }
        for key in q.skill_cooldowns.keys() {
            if !self
                .records
                .iter()
                .any(|r| r.table == "skill" && r.key == *key)
            {
                return Err(bad("unknown cooldown skill"));
            }
        }
        if q.mount_duration == Some(0) || q.dragon_duration == Some(0) {
            return Err(bad(
                "ride duration must be positive; zero is not a proven unlimited sentinel",
            ));
        }
        if q.stack_size.is_some_and(|v| v == 0 || v > 1000000) {
            return Err(bad("stack input limit: 1..1000000"));
        }
        for c in &q.stack_categories {
            if !self.items.values().any(|i| i.category == *c) {
                return Err(bad("unknown stack category"));
            }
        }
        let mut used = BTreeSet::new();
        for row in &self.records {
            let t = table(blobs, row.table)?;
            let original = t.record_bytes(row.key).unwrap();
            let mut bytes = original.to_vec();
            for field in &row.fields {
                let before = field
                    .value
                    .parse::<i64>()
                    .map_err(|_| bad("invalid source number"))?;
                let mut value = before;
                match field.rule.as_str() {
                    "spawn_count" if before > 0 && before < 255 => {
                        value = scale(before, q.spawn_percent)?
                    }
                    "spawn_limit" if before > 0 && before < 65535 => {
                        value = scale(before, q.spawn_percent)?
                    }
                    "patrol_reset" => {
                        if let Some(percent) = q.patrol_reset_percent {
                            value = scale(before, percent)?;
                        }
                    }
                    "reoccupation_delay" => {
                        if let Some(percent) = q.reoccupation_delay_percent {
                            value = scale(before, percent)?;
                        }
                    }
                    "mount_cooldown" => {
                        if let Some(v) = q.mount_cooldown {
                            value = v.into();
                        }
                        if row.key == 1000799 && q.dragon_no_cooldown {
                            value = 0;
                        }
                    }
                    "mount_duration" => {
                        if let Some(v) = q.mount_duration {
                            value = v.into();
                        }
                        if row.key == 1000799
                            && let Some(v) = q.dragon_duration
                        {
                            value = v.into();
                        }
                    }
                    "town_run" if q.town_running => value = 0,
                    "call_vehicle" if q.town_running => value = 1,
                    "wear" if q.no_wear => value = 0,
                    "skill_cooldown" => {
                        value = scale(
                            before,
                            *q.skill_cooldowns
                                .get(&row.key)
                                .unwrap_or(&q.skill_cooldown_percent),
                        )?
                    }
                    "stamina" | "spirit" if before < 0 => {
                        value = scale(
                            before,
                            *q.cost_percent
                                .get(&format!("{}:{}", field.rule, row.category))
                                .unwrap_or(&100),
                        )?
                    }
                    _ => {}
                }
                let id = format!("{}/{}/{}", row.table, row.key, field.name);
                if let Some(v) = q.fields.get(&id) {
                    value = v
                        .parse::<i64>()
                        .map_err(|_| bad(format!("{id}: decimal integer required")))?;
                    used.insert(id);
                }
                if value == before {
                    continue;
                }
                let min = field.min.parse::<i64>().unwrap();
                let max = field.max.parse::<i64>().unwrap();
                if value < min || value > max || (!field.signed && value < 0) {
                    return Err(bad(format!(
                        "{}/{}/{}: allowed {min}..{max}",
                        row.table, row.key, field.name
                    )));
                }
                bytes[field.offset..field.offset + field.width]
                    .copy_from_slice(&value.to_le_bytes()[..field.width]);
                changes.push(Change {
                    module: row.module,
                    table: row.table,
                    key: row.key,
                    name: row.name.clone(),
                    field: field.name.clone(),
                    before: before.to_string(),
                    after: value.to_string(),
                });
            }
            if row.table == "inventory" && row.fields.len() == 2 {
                let a = &row.fields[0];
                let b = &row.fields[1];
                let default = u16::from_le_bytes(bytes[a.offset..a.offset + 2].try_into().unwrap());
                let max = u16::from_le_bytes(bytes[b.offset..b.offset + 2].try_into().unwrap());
                if default > max {
                    return Err(bad("inventory default slots exceed maximum"));
                }
            }
            if q.dragon_regions
                && let Some((offset, old)) = &row.dragon_forbidden
            {
                let filtered: Vec<_> = old.iter().copied().filter(|v| *v != 79).collect();
                let mut replacement = (filtered.len() as u32).to_le_bytes().to_vec();
                replacement.extend_from_slice(&filtered);
                bytes.splice(*offset..offset + 4 + old.len(), replacement);
                changes.push(Change {
                    module: "dragon",
                    table: row.table,
                    key: row.key,
                    name: row.name.clone(),
                    field: "forbiddenMercenaryKeyList".into(),
                    before: format!("{old:?}"),
                    after: format!("{filtered:?}"),
                });
                reader::region(&bytes)?;
            }
            if row.town_dismount && (q.dragon_regions || q.town_running) {
                bytes = flight::disable(original)?;
                changes.push(Change {
                    module: "dragon",
                    table: row.table,
                    key: row.key,
                    name: row.name.clone(),
                    field: "townFlightCondition".into(),
                    before: "IsInTown() && !IsAboveRoad(Bird,20)".into(),
                    after: "!CheckNone()".into(),
                });
            }
            if bytes != original {
                replacements
                    .entry(row.table)
                    .or_default()
                    .insert(row.key, bytes);
            }
        }
        if used.len() != q.fields.len() {
            return Err(bad("unknown or read-only advanced field override"));
        }
        let skills = table(blobs, "skill")?;
        let mut buff_edits = BTreeMap::new();
        if let Some(percent) = q.cost_percent.get("spirit:other").filter(|p| **p != 100) {
            for key in [40013, 10300] {
                let original = skills
                    .record_bytes(key)
                    .ok_or_else(|| bad("missing self-consumption skill"))?;
                buff_edits.insert(key, skill_detail::self_cost_overrides(original, *percent)?);
            }
        }
        for (key, overrides) in &q.skill_buffs {
            buff_edits
                .entry(*key)
                .or_default()
                .extend(overrides.clone());
        }
        for (key, overrides) in &buff_edits {
            let original = skills
                .record_bytes(*key)
                .ok_or_else(|| bad("unknown buff edit skill"))?;
            let current = replacements
                .get("skill")
                .and_then(|rows| rows.get(key))
                .map(Vec::as_slice)
                .unwrap_or(original);
            let (bytes, diff) = skill_detail::edit(current, overrides)?;
            if !diff.is_empty() {
                let name = self
                    .records
                    .iter()
                    .find(|r| r.table == "skill" && r.key == *key)
                    .ok_or_else(|| bad("unknown skill record"))?
                    .name
                    .clone();
                for (field, before, after) in diff {
                    changes.push(Change {
                        module: "skill",
                        table: "skill",
                        key: *key,
                        name: name.clone(),
                        field,
                        before,
                        after,
                    });
                }
                replacements.entry("skill").or_default().insert(*key, bytes);
            }
        }
        let t = table(blobs, "iteminfo")?;
        for key in q.items.keys() {
            if !self.items.contains_key(key) {
                return Err(bad("unknown item edit"));
            }
        }
        for (key, item) in &self.items {
            let mut edit = q.items.get(key).cloned().unwrap_or_default();
            let global_stack = q.stack_size.filter(|_| {
                item.stack_size.parse::<u64>().is_ok_and(|n| n > 1)
                    || q.stack_categories.contains(&item.category)
            });
            let stack_risk =
                item.stack_risk || !edit.buffs.is_empty() || !edit.enchant_copies.is_empty();
            if edit.stack_size.is_none() && (!stack_risk || q.experimental_stacks) {
                edit.stack_size = global_stack;
            }
            let final_stack = edit
                .stack_size
                .unwrap_or_else(|| item.stack_size.parse().unwrap());
            if final_stack > 1
                && stack_risk
                && !q.experimental_stacks
                && (edit.stack_size.is_some() || !item.stack_risk)
            {
                return Err(bad(
                    "equipment/stateful stacks require the experimental switch; new buffs/enchants also require stack size 1 or experimental stacks",
                ));
            }
            let no_wear = q.no_wear && !matches!(item.max_endurance, 0 | u16::MAX);
            if edit == ItemEdit::default() && !no_wear {
                continue;
            }
            for stat in &edit.stats {
                if !self.statuses.iter().any(|s| s.key == stat.stat) {
                    return Err(bad("unknown item status ID"));
                }
            }
            for buff in &edit.buffs {
                if !self
                    .buffs
                    .get(&item.equip_type)
                    .is_some_and(|s| s.contains(&(buff.buff, buff.level)))
                {
                    return Err(bad("buff/level not observed on this equip type"));
                }
            }
            let (mut bytes, mut diff) = item_mods::edit_item(t.record_bytes(*key).unwrap(), &edit)?;
            if no_wear {
                let (updated, durability_diff) = item_mods::prevent_durability_loss(&bytes)?;
                bytes = updated;
                diff.extend(durability_diff);
            }
            if !diff.is_empty() {
                for d in diff {
                    changes.push(Change {
                        module: "item",
                        table: "iteminfo",
                        key: *key,
                        name: format!("Item {key}"),
                        field: d.field,
                        before: d.before,
                        after: d.after,
                    });
                }
                replacements
                    .entry("iteminfo")
                    .or_default()
                    .insert(*key, bytes);
            }
        }
        Ok(())
    }
}
fn scale(value: i64, percent: u32) -> Result<i64> {
    value
        .checked_mul(i64::from(percent))
        .map(|v| v / 100)
        .ok_or_else(|| bad("advanced multiplier overflow"))
}

#[cfg(test)]
mod tests {
    use super::*;
    fn synthetic(values: &[(i64, usize, &str, i64, i64)]) -> (Catalog, BTreeMap<String, Vec<u8>>) {
        let mut bytes = Vec::new();
        let mut fields = Vec::new();
        for (index, (value, width, rule, min, max)) in values.iter().enumerate() {
            let offset = bytes.len();
            bytes.extend_from_slice(&value.to_le_bytes()[..*width]);
            fields.push(Field {
                name: format!("value{index}"),
                value: value.to_string(),
                min: min.to_string(),
                max: max.to_string(),
                rule: (*rule).into(),
                offset,
                width: *width,
                signed: *min < 0,
            });
        }
        bytes.extend_from_slice(&[81, 82, 83]);
        let row = Record::new("skill", 7, "Skill_Climb".into(), "skill", fields);
        let mut header = vec![1, 0];
        header.extend_from_slice(&7u32.to_le_bytes());
        header.extend_from_slice(&0u32.to_le_bytes());
        (
            Catalog {
                records: vec![row],
                items: BTreeMap::new(),
                statuses: vec![],
                buffs: BTreeMap::new(),
            },
            BTreeMap::from([
                ("skill.staticinfobody".into(), bytes),
                ("skill.staticinfoheader".into(), header),
                ("iteminfo.staticinfobody".into(), vec![]),
                ("iteminfo.staticinfoheader".into(), vec![0, 0]),
            ]),
        )
    }
    #[test]
    fn resource_gains_opaque_bytes_and_original_baseline_survive_overrides() {
        let (c, blobs) = synthetic(&[
            (-100, 8, "stamina", -1000000, 1000000),
            (200, 8, "stamina", -1000000, 1000000),
            (10, 4, "skill_cooldown", 0, 604800),
        ]);
        let q = AdvancedRequest {
            cost_percent: BTreeMap::from([("stamina:climbing".into(), 0)]),
            skill_cooldown_percent: 50,
            skill_cooldowns: BTreeMap::from([(7, 20)]),
            fields: BTreeMap::from([("skill/7/value2".into(), "3".into())]),
            ..Default::default()
        };
        let mut rows = BTreeMap::new();
        let mut changes = Vec::new();
        c.apply(&blobs, &q, &mut rows, &mut changes).unwrap();
        let b = &rows["skill"][&7];
        assert_eq!(&b[..8], &0i64.to_le_bytes());
        assert_eq!(&b[8..16], &200i64.to_le_bytes());
        assert_eq!(&b[16..20], &3u32.to_le_bytes());
        assert_eq!(&b[20..], &[81, 82, 83]);
        assert_eq!(changes.len(), 2);
        let mut repeated = BTreeMap::new();
        c.apply(&blobs, &q, &mut repeated, &mut Vec::new()).unwrap();
        assert_eq!(repeated, rows);
        let mut clean = BTreeMap::new();
        c.apply(
            &blobs,
            &AdvancedRequest::default(),
            &mut clean,
            &mut Vec::new(),
        )
        .unwrap();
        assert!(clean.is_empty());
    }
    #[test]
    fn spawn_sentinels_survive_and_overflows_are_errors() {
        let (c, b) = synthetic(&[
            (255, 1, "spawn_count", 0, 255),
            (65535, 2, "spawn_limit", 0, 65535),
            (0, 2, "spawn_limit", 0, 65535),
        ]);
        let q = AdvancedRequest {
            spawn_percent: 300,
            ..Default::default()
        };
        let mut rows = BTreeMap::new();
        c.apply(&b, &q, &mut rows, &mut Vec::new()).unwrap();
        assert!(rows.is_empty());
        let (c, b) = synthetic(&[(100, 1, "spawn_count", 0, 255)]);
        assert!(
            c.apply(&b, &q, &mut BTreeMap::new(), &mut Vec::new())
                .is_err()
        );
    }
    #[test]
    fn equipment_costs_use_separate_factors_preserve_gains_and_allow_exceptions() {
        let (mut c, mut blobs) = synthetic(&[
            (-100, 8, "stamina", -1000000, 1000000),
            (200, 8, "stamina", -1000000, 1000000),
            (-80, 8, "spirit", -1000000, 1000000),
            (-30, 8, "numeric", -1000000, 1000000),
        ]);
        c.records[0].table = "buffinfo";
        c.records[0].category = "equipment".into();
        for ext in ["staticinfobody", "staticinfoheader"] {
            blobs.insert(
                format!("buffinfo.{ext}"),
                blobs[&format!("skill.{ext}")].clone(),
            );
        }
        let q = AdvancedRequest {
            cost_percent: BTreeMap::from([
                ("stamina:climbing".into(), 0),
                ("stamina:equipment".into(), 50),
                ("spirit:equipment".into(), 0),
            ]),
            fields: BTreeMap::from([("buffinfo/7/value2".into(), "-7".into())]),
            ..Default::default()
        };
        let mut rows = BTreeMap::new();
        let mut changes = vec![];
        c.apply(&blobs, &q, &mut rows, &mut changes).unwrap();
        let mut expected = blobs["buffinfo.staticinfobody"].clone();
        expected[..8].copy_from_slice(&(-50i64).to_le_bytes());
        expected[16..24].copy_from_slice(&(-7i64).to_le_bytes());
        assert_eq!(rows["buffinfo"][&7], expected);
        assert_eq!(changes.len(), 2);
        assert_eq!(changes[1].before, "-80");
        assert_eq!(changes[1].after, "-7");
    }
    #[test]
    fn repair_requests_fail_before_other_edits_even_when_rules_exist() {
        let (mut c, blobs) = synthetic(&[(10, 4, "skill_cooldown", 0, 604800)]);
        c.items.insert(
            9,
            crimson_format::item_mods::ItemModInfo {
                key: 9,
                category: 1,
                equip_type: 1,
                stack_size: "1".into(),
                stack_risk: true,
                max_endurance: 100,
                repair_entries: 1,
                enchant_levels: vec![0],
                can_copy_enchants: false,
                stats: vec![],
                buffs: vec![],
            },
        );
        for key in [None, Some(9), Some(999)] {
            let mut q = AdvancedRequest {
                skill_cooldown_percent: 50,
                ..Default::default()
            };
            if let Some(key) = key {
                q.items.insert(
                    key,
                    ItemEdit {
                        free_repair: true,
                        ..Default::default()
                    },
                );
            } else {
                q.free_repair = true;
            }
            let mut replacements = BTreeMap::new();
            let mut changes = Vec::new();
            let error = c
                .apply(&blobs, &q, &mut replacements, &mut changes)
                .unwrap_err();
            assert!(
                error
                    .to_string()
                    .contains("Kostenfreie Reparatur ist nicht verfügbar")
            );
            assert!(replacements.is_empty());
            assert!(changes.is_empty());
        }
    }
    #[test]
    fn unknown_fields_and_categories_fail_closed() {
        let (c, b) = synthetic(&[(10, 4, "skill_cooldown", 0, 604800)]);
        for q in [
            AdvancedRequest {
                fields: BTreeMap::from([("skill/7/unknown".into(), "0".into())]),
                ..Default::default()
            },
            AdvancedRequest {
                cost_percent: BTreeMap::from([("stamina:unknown".into(), 0)]),
                ..Default::default()
            },
            AdvancedRequest {
                skill_cooldowns: BTreeMap::from([(999, 0)]),
                ..Default::default()
            },
            AdvancedRequest {
                dragon_duration: Some(0),
                ..Default::default()
            },
            AdvancedRequest {
                free_repair: true,
                ..Default::default()
            },
        ] {
            assert!(
                c.apply(&b, &q, &mut BTreeMap::new(), &mut Vec::new())
                    .is_err()
            );
        }
    }
    #[test]
    fn scaling_is_signed_and_checked() {
        assert_eq!(scale(-12345, 50).unwrap(), -6172);
        assert_eq!(scale(0, 300).unwrap(), 0);
        assert!(scale(i64::MAX, 10000).is_err());
    }
    #[test]
    fn patrol_factors_and_overrides_are_bounded_and_original_based() {
        let (c, b) = synthetic(&[(259200, 4, "patrol_reset", 1, 31536000)]);
        let mut q = AdvancedRequest {
            patrol_reset_percent: Some(50),
            ..Default::default()
        };
        let mut rows = BTreeMap::new();
        c.apply(&b, &q, &mut rows, &mut vec![]).unwrap();
        assert_eq!(&rows["skill"][&7][..4], &129600u32.to_le_bytes());
        q.fields.insert("skill/7/value0".into(), "600".into());
        c.apply(&b, &q, &mut rows, &mut vec![]).unwrap();
        assert_eq!(&rows["skill"][&7][..4], &600u32.to_le_bytes());
        for n in [0, 10001] {
            q.patrol_reset_percent = Some(n);
            assert!(c.apply(&b, &q, &mut rows, &mut vec![]).is_err());
        }
    }
    #[test]
    fn old_advanced_requests_keep_their_serialized_shape() {
        let request = AdvancedRequest::default();
        let value = serde_json::to_value(&request).unwrap();
        assert!(value.get("skill_buffs").is_none());
        assert!(value.get("patrol_reset_percent").is_none());
        assert!(value.get("reoccupation_delay_percent").is_none());
        assert_eq!(
            serde_json::from_value::<AdvancedRequest>(value).unwrap(),
            request
        );
    }
    #[test]
    fn reoccupation_factor_overrides_and_limits_do_not_change_patrols() {
        let (c, b) = synthetic(&[
            (86400, 4, "reoccupation_delay", 1, 31536000),
            (259200, 4, "patrol_reset", 1, 31536000),
        ]);
        let mut q = AdvancedRequest {
            reoccupation_delay_percent: Some(50),
            ..Default::default()
        };
        let mut rows = BTreeMap::new();
        c.apply(&b, &q, &mut rows, &mut vec![]).unwrap();
        assert_eq!(&rows["skill"][&7][..4], &43200u32.to_le_bytes());
        assert_eq!(&rows["skill"][&7][4..], &b["skill.staticinfobody"][4..]);
        q.fields.insert("skill/7/value0".into(), "12345".into());
        c.apply(&b, &q, &mut rows, &mut vec![]).unwrap();
        assert_eq!(&rows["skill"][&7][..4], &12345u32.to_le_bytes());
        for n in [0, 10001] {
            q.reoccupation_delay_percent = Some(n);
            assert!(c.apply(&b, &q, &mut rows, &mut vec![]).is_err());
        }
        q.reoccupation_delay_percent = Some(100);
        for n in ["0", "31536001", "4294967295"] {
            q.fields.insert("skill/7/value0".into(), n.into());
            assert!(c.apply(&b, &q, &mut rows, &mut vec![]).is_err());
        }
    }
    #[test]
    #[ignore = "requires local, previously extracted hash-pinned tables"]
    fn pinned_advanced_records_and_edits() {
        let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("../../.local/archive-probe/extracted");
        let mut blobs = BTreeMap::new();
        for name in [
            "characterinfo",
            "regioninfo",
            "inventory",
            "skill",
            "buffinfo",
            "fieldinfo",
            "stageinfo",
            "conditioninfo",
            "factionreblockadinginfo",
            "equiptypeinfo",
            "statusinfo",
            "spawningpoolautospawninfo",
            "terrainregionautospawninfo",
            "iteminfo",
        ] {
            for ext in ["staticinfobody", "staticinfoheader"] {
                let filename = format!("{name}.{ext}");
                blobs.insert(
                    filename.clone(),
                    std::fs::read(root.join(filename)).unwrap(),
                );
            }
        }
        let references: Vec<crate::fingerprint::TableHash> = serde_json::from_str(include_str!(
            "../../../schemas/steam-25381195.advanced.json"
        ))
        .unwrap();
        let mut references = references;
        references.extend(crate::fingerprint::known().tables);
        for (name, b) in &blobs {
            let reference = references.iter().find(|r| r.name == *name).unwrap();
            assert_eq!(crate::fingerprint::hash_bytes(b), reference.sha256);
        }
        let c = Catalog::open(&blobs).unwrap();
        // Every real item: the durability toggle changes exactly the known u16,
        // preserves zero/already-unlimited items, composes with other item edits,
        // and is idempotent. No saved instance is opened or rewritten.
        let items = table(&blobs, "iteminfo").unwrap();
        let typed_items = crimson_format::ItemTable::parse(
            &blobs["iteminfo.staticinfobody"],
            &blobs["iteminfo.staticinfoheader"],
        )
        .unwrap();
        let mut finite = 0;
        for entry in items.records() {
            let original = items.record_bytes(entry.key).unwrap();
            let mut ranges = typed_items.fields(entry.key).unwrap();
            let field = ranges
                .iter_mut()
                .find(|r| r.path == "max_endurance")
                .unwrap();
            field.start -= entry.offset as usize;
            field.end -= entry.offset as usize;
            assert_eq!(field.end - field.start, 2);
            let (changed, diff) = item_mods::prevent_durability_loss(original).unwrap();
            assert_eq!(&changed[..field.start], &original[..field.start]);
            assert_eq!(&changed[field.end..], &original[field.end..]);
            let old = c.items[&entry.key].max_endurance;
            if matches!(old, 0 | u16::MAX) {
                assert_eq!(changed, original);
                assert!(diff.is_empty());
            } else {
                finite += 1;
                assert_eq!(diff.len(), 1);
                assert_eq!(&changed[field.start..field.end], &[255, 255]);
                assert_eq!(
                    item_mods::item_mod_info(&changed).unwrap().max_endurance,
                    u16::MAX
                );
                let edit = ItemEdit {
                    stack_size: Some(777),
                    ..Default::default()
                };
                let (stacked, _) = item_mods::edit_item(original, &edit).unwrap();
                let (combined, _) = item_mods::prevent_durability_loss(&stacked).unwrap();
                let info = item_mods::item_mod_info(&combined).unwrap();
                assert_eq!(info.stack_size, "777");
                assert_eq!(info.max_endurance, u16::MAX);
            }
            let (again, diff) = item_mods::prevent_durability_loss(&changed).unwrap();
            assert_eq!(again, changed);
            assert!(diff.is_empty());
        }
        assert!(finite > 0);
        assert_eq!(c.info().finite_endurance_items, finite);
        println!(
            "durability: {finite} finite items; all 6816 records checked for exact preservation and idempotence"
        );
        assert_eq!(
            c.records.iter().filter(|r| r.module == "inventory").count(),
            9
        );
        let skills = table(&blobs, "skill").unwrap();
        let rebuild = |field: &skill_detail::Value| match field.kind {
            "opaque hex" => field
                .value
                .as_bytes()
                .chunks(2)
                .map(|v| u8::from_str_radix(std::str::from_utf8(v).unwrap(), 16).unwrap())
                .collect::<Vec<_>>(),
            "UTF-8 (u32 length)" => {
                let mut b = (field.value.len() as u32).to_le_bytes().to_vec();
                b.extend(field.value.as_bytes());
                b
            }
            "signed integer" => {
                field.value.parse::<i64>().unwrap().to_le_bytes()[..field.bytes].to_vec()
            }
            "unsigned integer" | "reference" => {
                field.value.parse::<u64>().unwrap().to_le_bytes()[..field.bytes].to_vec()
            }
            _ => panic!("unknown value kind"),
        };
        let mut buff_count = 0;
        let buff_info = table(&blobs, "buffinfo").unwrap();
        let mut round_tripped = BTreeMap::new();
        let mut cost_fields = 0;
        for entry in buff_info.records() {
            let source = buff_info.record_bytes(entry.key).unwrap();
            let detail = skill_detail::inspect_buff_info(source).unwrap();
            let mut rebuilt = Vec::new();
            for field in &detail.fields {
                assert_eq!(field.offset, rebuilt.len());
                rebuilt.extend(rebuild(field));
            }
            assert_eq!(rebuilt, source, "buff info {} round-trip", entry.key);
            cost_fields += detail.row.fields.len();
            round_tripped.insert(entry.key, rebuilt);
        }
        assert_eq!(cost_fields, 36);
        assert_eq!(
            c.records.iter().filter(|r| r.table == "buffinfo").count(),
            33
        );
        let (body, header) = buff_info.rebuild(&round_tripped).unwrap();
        assert_eq!(body, blobs["buffinfo.staticinfobody"]);
        assert_eq!(header, blobs["buffinfo.staticinfoheader"]);
        for percent in [0, 50, 200] {
            let q = AdvancedRequest {
                cost_percent: BTreeMap::from([
                    ("stamina:equipment".into(), percent),
                    ("spirit:equipment".into(), percent),
                ]),
                ..Default::default()
            };
            let mut edited = BTreeMap::new();
            let mut changes = vec![];
            c.apply(&blobs, &q, &mut edited, &mut changes).unwrap();
            assert_eq!(changes.len(), 36);
            assert_eq!(edited.len(), 1);
            for (key, changed) in &edited["buffinfo"] {
                let source = buff_info.record_bytes(*key).unwrap();
                let row = reader::buff_info(source).unwrap();
                let mut expected = source.to_vec();
                for field in row.fields {
                    let value = field.value.parse::<i64>().unwrap() * i64::from(percent) / 100;
                    expected[field.offset..field.offset + 8].copy_from_slice(&value.to_le_bytes());
                }
                assert_eq!(*changed, expected, "only resource amount may change");
                skill_detail::inspect_buff_info(changed).unwrap();
            }
            let (body, header) = buff_info.rebuild(&edited["buffinfo"]).unwrap();
            let rebuilt =
                crate::tables::IndexedTable::parse(schema("buffinfo").unwrap(), &body, &header)
                    .unwrap();
            for entry in buff_info.records() {
                if !edited["buffinfo"].contains_key(&entry.key) {
                    assert_eq!(
                        rebuilt.record_bytes(entry.key),
                        buff_info.record_bytes(entry.key)
                    );
                }
            }
        }
        println!(
            "buffinfo: 292 full record/table round-trips; 36 additional costs in 33 buffs; 0/50/200% preserve all other bytes"
        );
        let mut owned_costs = 0;
        for key in [40013, 10300] {
            let source = skills.record_bytes(key).unwrap();
            let edits = skill_detail::self_cost_overrides(source, 50).unwrap();
            owned_costs += edits.len();
            let (changed, diff) = skill_detail::edit(source, &edits).unwrap();
            assert_eq!(diff.len(), edits.len());
            let before = skill_detail::inspect(source).unwrap();
            let mut expected = source.to_vec();
            for buff in before.buffs {
                for field in buff.fields {
                    let id = format!("{}/{}/{}", buff.matrix_level, buff.index, field.path);
                    if let Some(value) = edits.get(&id) {
                        assert_eq!(field.path, "payload.part_1");
                        expected[field.offset..field.offset + 8]
                            .copy_from_slice(&value.parse::<i64>().unwrap().to_le_bytes());
                    }
                }
            }
            assert_eq!(changed, expected);
        }
        assert_eq!(owned_costs, 31);
        let mut owned_rows = BTreeMap::new();
        let mut owned_changes = vec![];
        c.apply(
            &blobs,
            &AdvancedRequest {
                cost_percent: BTreeMap::from([("spirit:other".into(), 0)]),
                skill_buffs: BTreeMap::from([(
                    40013,
                    BTreeMap::from([("0/0/payload.part_1".into(), "-123".into())]),
                )]),
                ..Default::default()
            },
            &mut owned_rows,
            &mut owned_changes,
        )
        .unwrap();
        let own_matrix: Vec<_> = owned_changes
            .iter()
            .filter(|c| c.field.starts_with("buffs/"))
            .collect();
        assert_eq!(own_matrix.len(), 31);
        let exception = own_matrix
            .iter()
            .filter(|c| c.after != "0")
            .collect::<Vec<_>>();
        assert_eq!(exception.len(), 1);
        assert_eq!(exception[0].key, 40013);
        assert_eq!(exception[0].before, "-1000");
        assert_eq!(exception[0].after, "-123");
        println!(
            "31 self-consumption matrix values in two skills: exact bytes and explicit override precedence verified"
        );
        let mut record_field_count = 0;
        let mut structured_payloads = [0usize; 2];
        for row in skills.records() {
            let b = skills.record_bytes(row.key).unwrap();
            let detail =
                skill_detail::inspect(b).unwrap_or_else(|e| panic!("skill {}: {e}", row.key));
            buff_count += detail.buffs.len();
            assert_eq!(detail.raw_hex.len(), b.len() * 2);
            let mut reconstructed = vec![0u8; b.len()];
            let mut covered = vec![false; b.len()];
            record_field_count += detail.record_fields.len();
            for field in &detail.record_fields {
                assert!(!field.editable);
                let end = field.offset + field.bytes;
                assert!(covered[field.offset..end].iter().all(|v| !v));
                reconstructed[field.offset..end].copy_from_slice(&rebuild(field));
                covered[field.offset..end].fill(true);
            }
            let mut at = detail.matrix_start;
            for (level, count) in detail.level_counts.iter().enumerate() {
                assert_eq!(
                    u32::from_le_bytes(b[at..at + 4].try_into().unwrap()) as usize,
                    *count
                );
                reconstructed[at..at + 4].copy_from_slice(&(*count as u32).to_le_bytes());
                covered[at..at + 4].fill(true);
                at += 4;
                for buff in detail.buffs.iter().filter(|v| v.matrix_level == level) {
                    if matches!(buff.type_id, Some(10 | 74)) {
                        let index = usize::from(buff.type_id == Some(74));
                        structured_payloads[index] += 1;
                        assert!(!buff.opaque_layout);
                        let payload: Vec<_> = buff
                            .fields
                            .iter()
                            .filter(|f| f.path.starts_with("payload."))
                            .collect();
                        assert!(payload.iter().all(|f| !f.editable));
                        assert_eq!(
                            payload.iter().map(|f| f.bytes).sum::<usize>(),
                            [181, 8][index]
                        );
                        assert_eq!(
                            payload[0].path,
                            [
                                "payload.summon.selectDataList.count",
                                "payload.subLevelReference"
                            ][index]
                        );
                    }
                    for field in &buff.fields {
                        assert_eq!(field.offset, at);
                        let rebuilt = rebuild(field);
                        reconstructed[at..at + field.bytes].copy_from_slice(&rebuilt);
                        covered[at..at + field.bytes].fill(true);
                        assert_eq!(rebuilt, b[at..at + field.bytes]);
                        at += field.bytes;
                    }
                }
            }
            assert_eq!(at, detail.matrix_end);
            assert!(
                covered.iter().all(|v| *v),
                "skill {}: uncovered bytes",
                row.key
            );
            assert_eq!(
                reconstructed, b,
                "skill {}: full record round-trip",
                row.key
            );
        }
        println!("complete skill matrices: 2069; buff entries including null: {buff_count}");
        println!(
            "named skill header/suffix fields: {record_field_count}; all 2069 complete records round-tripped"
        );
        assert_eq!(structured_payloads, [9, 1]);
        println!(
            "structured summon/sub-level payloads: {structured_payloads:?}; all bytes round-tripped"
        );
        assert_eq!(
            c.records.iter().filter(|r| r.table == "skill").count(),
            2069
        );
        println!(
            "stackable: {}, risk: {}, repair rows: {}",
            c.items.values().filter(|i| i.stack_size != "1").count(),
            c.items.values().filter(|i| i.stack_risk).count(),
            c.items.values().filter(|i| i.repair_entries > 0).count()
        );
        let mut replacements = BTreeMap::new();
        let mut changes = Vec::new();
        c.apply(
            &blobs,
            &AdvancedRequest::default(),
            &mut replacements,
            &mut changes,
        )
        .unwrap();
        assert!(replacements.is_empty());
        assert!(changes.is_empty());
        let q = AdvancedRequest {
            spawn_percent: 300,
            patrol_reset_percent: Some(50),
            reoccupation_delay_percent: Some(50),
            mount_cooldown: Some(0),
            dragon_no_cooldown: true,
            dragon_duration: Some(1800),
            dragon_regions: true,
            town_running: true,
            no_wear: true,
            free_repair: false,
            stack_size: Some(999),
            skill_cooldown_percent: 50,
            skill_buffs: BTreeMap::from([(
                30001,
                BTreeMap::from([("0/0/common.mem_24".into(), "123".into())]),
            )]),
            cost_percent: BTreeMap::from([
                ("stamina:climbing".into(), 0),
                ("spirit:jumping".into(), 0),
            ]),
            fields: BTreeMap::from([
                ("inventory/2/defaultSlotCount".into(), "300".into()),
                ("inventory/2/maxSlotCount".into(), "300".into()),
            ]),
            ..Default::default()
        };
        c.apply(&blobs, &q, &mut replacements, &mut changes)
            .unwrap();
        assert!(changes.len() > 500);
        let reoccupation = table(&blobs, "factionreblockadinginfo").unwrap();
        assert_eq!(replacements["factionreblockadinginfo"].len(), 108);
        for entry in reoccupation.records() {
            let original = reoccupation.record_bytes(entry.key).unwrap();
            let row = reader::reoccupation(original).unwrap();
            let field = &row.fields[0];
            let changed = &replacements["factionreblockadinginfo"][&entry.key];
            let before = field.value.parse::<u32>().unwrap();
            assert!([86400, 432000].contains(&before));
            assert_eq!(
                reader::reoccupation(changed).unwrap().fields[0].value,
                (before / 2).to_string()
            );
            assert_eq!(&original[..field.offset], &changed[..field.offset]);
            assert_eq!(&original[field.offset + 4..], &changed[field.offset + 4..]);
        }
        for key in [2, 8, 9, 13, 15, 16, 17, 18, 19] {
            let record = c
                .records
                .iter()
                .find(|r| r.table == "inventory" && r.key == key)
                .unwrap();
            assert!(record.fields.iter().all(|f| f.max == "1460"));
            let inventory_only = Catalog {
                records: vec![record.clone()],
                items: BTreeMap::new(),
                statuses: vec![],
                buffs: BTreeMap::new(),
            };
            for limit in [1460, 1461, 65535] {
                let input = AdvancedRequest {
                    fields: BTreeMap::from([
                        (
                            format!("inventory/{key}/defaultSlotCount"),
                            limit.to_string(),
                        ),
                        (format!("inventory/{key}/maxSlotCount"), limit.to_string()),
                    ]),
                    ..Default::default()
                };
                assert_eq!(
                    inventory_only
                        .apply(&blobs, &input, &mut BTreeMap::new(), &mut vec![])
                        .is_ok(),
                    limit == 1460
                );
            }
        }
        assert_eq!(replacements["conditioninfo"].len(), 1);
        assert_eq!(
            changes
                .iter()
                .filter(|c| c.table == "conditioninfo" && c.key == 1011130)
                .count(),
            1
        );
        assert!(
            changes.iter().any(|c| c.key == 30001
                && c.field == "buffs/0/0/common.mem_24"
                && c.after == "123")
        );
        assert!(
            changes
                .iter()
                .any(|c| c.key == 30001 && c.field == "cooltime" && c.after == "5")
        );
        let patched_skill = skill_detail::inspect(&replacements["skill"][&30001]).unwrap();
        assert_eq!(
            patched_skill.buffs[0]
                .fields
                .iter()
                .find(|f| f.path == "common.mem_24")
                .unwrap()
                .value,
            "123"
        );
        let stages = table(&blobs, "stageinfo").unwrap();
        assert_eq!(replacements["stageinfo"].len(), 2);
        for key in [1017811, 1002224] {
            let original = stages.record_bytes(key).unwrap();
            let row = reader::patrol_stage(original).unwrap();
            let field = &row.fields[0];
            assert_eq!(field.value, "259200");
            let changed = &replacements["stageinfo"][&key];
            assert_eq!(
                reader::patrol_stage(changed).unwrap().fields[0].value,
                "129600"
            );
            assert_eq!(&original[..field.offset], &changed[..field.offset]);
            assert_eq!(&original[field.offset + 4..], &changed[field.offset + 4..]);
            for end in 0..field.offset + 44 {
                assert!(reader::patrol_stage(&original[..end]).is_err());
            }
            for n in [0, u32::MAX] {
                let mut invalid = original.to_vec();
                invalid[field.offset..field.offset + 4].copy_from_slice(&n.to_le_bytes());
                assert!(reader::patrol_stage(&invalid).is_err());
            }
        }
        for (name, rows) in &replacements {
            let t = table(&blobs, name).unwrap();
            let (b, h) = t.rebuild(rows).unwrap();
            let parsed = crate::tables::IndexedTable::parse(
                crate::tables::schema(name)
                    .or_else(|| schema(name))
                    .unwrap(),
                &b,
                &h,
            )
            .unwrap();
            for (key, bytes) in rows {
                assert_eq!(parsed.record_bytes(*key).unwrap(), bytes);
            }
            let before_body = &blobs[&format!("{name}.staticinfobody")];
            for (old, new) in t.records().iter().zip(parsed.records()) {
                assert_eq!(old.key, new.key);
                if !rows.contains_key(&old.key) {
                    assert_eq!(
                        &before_body[old.offset as usize..old.offset as usize + old.length],
                        &b[new.offset as usize..new.offset as usize + new.length]
                    );
                }
            }
        }
        for row in c.records.iter().filter(|r| r.table == "skill") {
            if let Some(bytes) = replacements.get("skill").and_then(|v| v.get(&row.key)) {
                let parsed = reader::skill(bytes).unwrap();
                for old in &row.fields {
                    let new = parsed.fields.iter().find(|f| f.name == old.name).unwrap();
                    if ["stamina", "spirit"].contains(&old.rule.as_str())
                        && old.value.parse::<i64>().unwrap() > 0
                    {
                        assert_eq!(old.value, new.value);
                    }
                }
            }
        }
        // Either user-facing option reaches the same single condition; no
        // other city/quest/bounty condition is changed as a side effect.
        for q in [
            AdvancedRequest {
                dragon_regions: true,
                ..Default::default()
            },
            AdvancedRequest {
                town_running: true,
                ..Default::default()
            },
        ] {
            let mut rows = BTreeMap::new();
            c.apply(&blobs, &q, &mut rows, &mut vec![]).unwrap();
            assert_eq!(rows["conditioninfo"], replacements["conditioninfo"]);
        }
        let detail = c
            .items
            .values()
            .find(|i| i.equip_type != 0 && !i.stats.is_empty())
            .unwrap();
        let mut edit = ItemEdit::default();
        let target_level = detail.enchant_levels.iter().max().unwrap() + 1;
        edit.enchant_copies.push(item_mods::EnchantCopy {
            source_level: detail.enchant_levels[0],
            target_level,
        });
        let mut stat = detail.stats[0].clone();
        stat.value = "123".into();
        edit.stats.push(stat);
        let mut inserted = edit.stats[0].clone();
        inserted.stat = c
            .statuses
            .iter()
            .find(|s| {
                !detail.stats.iter().any(|v| {
                    v.enchant_level == inserted.enchant_level
                        && v.list == inserted.list
                        && v.stat == s.key
                })
            })
            .unwrap()
            .key;
        inserted.value = "456".into();
        edit.stats.push(inserted);
        let mut copied_stat = edit.stats[0].clone();
        copied_stat.enchant_level = target_level;
        copied_stat.value = "789".into();
        edit.stats.push(copied_stat);
        let q = AdvancedRequest {
            items: BTreeMap::from([(detail.key, edit)]),
            ..Default::default()
        };
        let mut item_rows = BTreeMap::new();
        c.apply(&blobs, &q, &mut item_rows, &mut Vec::new())
            .unwrap();
        let parsed = item_mods::item_mod_info(&item_rows["iteminfo"][&detail.key]).unwrap();
        assert!(parsed.stats.len() > detail.stats.len());
        assert!(parsed.enchant_levels.contains(&target_level));
        assert!(
            parsed
                .stats
                .iter()
                .any(|s| s.enchant_level == target_level && s.value == "789")
        );
        let original = table(&blobs, "iteminfo").unwrap();
        let (body, header) = original.rebuild(&item_rows["iteminfo"]).unwrap();
        let table = table_from_item_pair(&body, &header);
        for r in original.records() {
            if r.key != detail.key {
                assert_eq!(original.record_bytes(r.key), table.record_bytes(r.key));
            }
        }
        // A formerly safe item becomes stateful through a new enchant row.
        let safe = c
            .items
            .values()
            .find(|i| {
                !i.stack_risk && i.can_copy_enchants && i.stack_size.parse::<u64>().unwrap() > 1
            })
            .unwrap();
        let edit = ItemEdit {
            enchant_copies: vec![item_mods::EnchantCopy {
                source_level: safe.enchant_levels[0],
                target_level: safe.enchant_levels[0] + 1,
            }],
            ..Default::default()
        };
        let mut risk = AdvancedRequest {
            items: BTreeMap::from([(safe.key, edit)]),
            stack_size: Some(999),
            ..Default::default()
        };
        assert!(
            c.apply(&blobs, &risk, &mut BTreeMap::new(), &mut vec![])
                .is_err()
        );
        risk.items.get_mut(&safe.key).unwrap().stack_size = Some(1);
        let mut safe_rows = BTreeMap::new();
        c.apply(&blobs, &risk, &mut safe_rows, &mut vec![]).unwrap();
        let new = item_mods::item_mod_info(&safe_rows["iteminfo"][&safe.key]).unwrap();
        assert_eq!(new.stack_size, "1");
        assert!(new.stack_risk);
        let q = AdvancedRequest {
            fields: BTreeMap::from([("skill/1/notAField".into(), "0".into())]),
            ..Default::default()
        };
        assert!(
            c.apply(&blobs, &q, &mut BTreeMap::new(), &mut Vec::new())
                .is_err()
        );
        println!(
            "advanced rows: {}, items: {}, changes: {}, tables: {:?}",
            c.records.len(),
            c.items.len(),
            changes.len(),
            replacements.keys().collect::<Vec<_>>()
        );
    }
    fn table_from_item_pair<'a>(body: &'a [u8], header: &[u8]) -> crate::tables::IndexedTable<'a> {
        crate::tables::IndexedTable::parse(crate::tables::schema("iteminfo").unwrap(), body, header)
            .unwrap()
    }
}
