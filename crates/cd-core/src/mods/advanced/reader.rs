//! Readers for the hash-pinned September build. Uninterpreted suffixes stay opaque.
use super::{Field, Record, bad};
use crate::Result;

pub struct Reader<'a> {
    pub bytes: &'a [u8],
    pub at: usize,
}
impl<'a> Reader<'a> {
    pub fn new(bytes: &'a [u8]) -> Self {
        Self { bytes, at: 0 }
    }
    pub fn take(&mut self, n: usize) -> Result<&'a [u8]> {
        let end = self
            .at
            .checked_add(n)
            .ok_or_else(|| bad("field overflow"))?;
        let value = self
            .bytes
            .get(self.at..end)
            .ok_or_else(|| bad("truncated advanced record"))?;
        self.at = end;
        Ok(value)
    }
    pub fn num(&mut self, n: usize) -> Result<u64> {
        let mut b = [0; 8];
        if n > 8 {
            return Err(bad("integer width"));
        }
        b[..n].copy_from_slice(self.take(n)?);
        Ok(u64::from_le_bytes(b))
    }
    pub fn count(&mut self) -> Result<usize> {
        let n = self.num(4)? as usize;
        if n > 100_000 {
            return Err(bad("array limit"));
        }
        Ok(n)
    }
    pub fn array(&mut self, width: usize) -> Result<()> {
        let n = self.count()?;
        self.take(n * width)?;
        Ok(())
    }
    pub fn text(&mut self) -> Result<String> {
        let n = self.count()?;
        Ok(std::str::from_utf8(self.take(n)?)
            .map_err(|_| bad("invalid field UTF8"))?
            .to_owned())
    }
    pub fn loc(&mut self) -> Result<()> {
        self.take(9)?;
        self.text()?;
        Ok(())
    }
    pub fn head(&mut self, width: usize) -> Result<(u32, String)> {
        let key = self.num(width)? as u32;
        let name = self.text()?;
        if self.num(1)? > 1 {
            return Err(bad("invalid blocked flag"));
        }
        Ok((key, name))
    }
    pub fn end(&self) -> Result<()> {
        if self.at != self.bytes.len() {
            Err(bad("unexpected advanced record suffix"))
        } else {
            Ok(())
        }
    }
    fn field(
        &mut self,
        name: impl Into<String>,
        width: usize,
        signed: bool,
        min: i64,
        max: i64,
        rule: &str,
    ) -> Result<Field> {
        let offset = self.at;
        let value = self.num(width)? as i64;
        Ok(Field {
            name: name.into(),
            value: value.to_string(),
            min: min.to_string(),
            max: max.to_string(),
            rule: rule.into(),
            offset,
            width,
            signed,
        })
    }
}
pub fn mount(bytes: &[u8]) -> Result<Record> {
    let mut r = Reader::new(bytes);
    let (key, name) = r.head(4)?;
    r.loc()?;
    r.loc()?;
    r.take(8)?;
    r.text()?;
    r.take(10)?;
    let vehicle = r.num(2)?;
    let cooldown = r.field(
        "callMercenaryCoolTime",
        8,
        false,
        0,
        604800,
        "mount_cooldown",
    )?;
    let duration = r.field(
        "callMercenarySpawnDuration",
        8,
        false,
        0,
        604800,
        "mount_duration",
    )?;
    let fields = if vehicle != 0 && name.starts_with("Riding_") {
        vec![cooldown, duration]
    } else {
        vec![]
    };
    Ok(Record::new("characterinfo", key, name, "mount", fields))
}
/// Only the two hash-pinned patrol stages selected by Catalog. Never quest resets
/// inferred from names, sentinel timers, or unparsed sequencer variants.
pub fn patrol_stage(bytes: &[u8]) -> Result<Record> {
    let mut r = Reader::new(bytes);
    let (key, name) = r.head(4)?;
    for _ in 0..3 {
        r.loc()?;
    }
    // SequencerDesc, current reader 0x142348900.
    r.text()?;
    r.take(4)?;
    r.text()?;
    r.text()?;
    r.take(12 + 4 + 9 + 4)?;
    if r.num(1)? != 0 {
        return Err(bad("unsupported patrol sequencer condition"));
    }
    r.text()?;
    r.text()?;
    for _ in 0..r.count()? {
        r.text()?;
        r.text()?;
    }
    if r.count()? != 0 || r.count()? != 0 {
        return Err(bad("unsupported patrol sequencer bindings"));
    }
    r.array(2)?;
    r.array(2)?;
    for _ in 0..4 {
        r.array(4)?;
    }
    // StageInfo reader 0x14151abc0: direct spawn references, then resetSecond.
    let spawn = r.num(4)?;
    let node = r.num(4)?;
    if spawn == 0 && node == 0 {
        return Err(bad("patrol has no direct spawn reference"));
    }
    r.array(4)?;
    r.take(24)?;
    r.array(4)?;
    r.take(2 + 12)?;
    for _ in 0..4 {
        r.array(4)?;
    }
    r.array(1)?;
    r.array(1)?;
    r.take(12)?;
    r.array(4)?;
    r.array(4)?;
    r.text()?;
    for _ in 0..r.count()? {
        r.text()?;
    }
    r.array(4)?;
    r.take(8)?;
    r.text()?;
    r.text()?;
    r.take(6)?;
    let reset = r.field("resetSecond", 4, false, 1, 31_536_000, "patrol_reset")?;
    let value = reset
        .value
        .parse::<u64>()
        .map_err(|_| bad("patrol reset"))?;
    if value == 0 || value == u32::MAX as u64 {
        return Err(bad("patrol reset sentinel"));
    }
    // Remaining stage data has no edits and is preserved byte for byte.
    r.take(40)?;
    Ok(Record::new("stageinfo", key, name, "patrol", vec![reset]))
}
pub fn region(bytes: &[u8]) -> Result<Record> {
    let mut r = Reader::new(bytes);
    let (key, name) = r.head(2)?;
    r.loc()?;
    r.take(4)?;
    r.array(8)?;
    r.take(2)?;
    r.array(2)?;
    r.take(11)?;
    let run = r.field("limitVehicleRun", 1, false, 0, 1, "town_run")?;
    let town = r.num(1)?;
    r.take(3)?;
    let offset = r.at;
    let n = r.count()?;
    let forbidden = r.take(n)?;
    r.take(4)?;
    r.array(12)?;
    r.array(4)?;
    r.end()?;
    let mut row = Record::new(
        "regioninfo",
        key,
        name,
        "world",
        if town == 1 { vec![run] } else { vec![] },
    );
    if forbidden.contains(&79) {
        row.dragon_forbidden = Some((offset, forbidden.to_vec()));
    }
    Ok(row)
}
pub fn inventory(bytes: &[u8]) -> Result<Record> {
    let mut r = Reader::new(bytes);
    let (key, name) = r.head(2)?;
    if key == 2 && name == "Character" {
        // The hash-pinned character prefix contains a variable condition tree.
        // It stays opaque; only the complete trailing structure is interpreted.
        r.at = 9272;
    } else {
        // InventoryInfo 0x1415087c0, InventoryMoveData 0x141508490.
        r.array(3)?; // item type selectors: category u16 + subtype u8
        r.array(3)?;
        for _ in 0..r.count()? {
            r.take(1 + 2 + 2 + 4)?; // type, from/to inventories, money item
            for _ in 0..3 {
                r.loc()?;
            }
            r.array(17)?; // two item refs, cost ref, consume flag, condition ref
            if r.num(1)? != 0 {
                return Err(bad("unsupported inventory move condition tree"));
            }
            r.loc()?;
        }
    }
    // 0x1401ef7bb initializes the inventory code path's ceiling to 1460;
    // 0x142407b1f rejects indices at/above it. Use that conservative ceiling,
    // not the u16 format limit. This is not a proof of every engine/save limit.
    let mut fields = vec![
        r.field("defaultSlotCount", 2, false, 1, 1460, "slots")?,
        r.field("maxSlotCount", 2, false, 1, 1460, "slots")?,
    ];
    r.loc()?;
    r.loc()?;
    r.take(4 + 1 + 8)?;
    for _ in 0..3 {
        if r.num(1)? > 1 {
            return Err(bad("inventory flag"));
        }
    }
    r.array(12)?;
    r.end()?;
    // Player inventory plus verified general/Kuku/housing storage records.
    // Currency, quest, recovery, vehicle and other specialized containers keep
    // their original counts; their independent gameplay limits are not proven.
    if !matches!(
        (key, name.as_str()),
        (2, "Character")
            | (8, "CampWareHouse")
            | (9, "WareHouse")
            | (13, "Kuku")
            | (15, "Housing_Dresser")
            | (16, "Housing_Refrigerator")
            | (17, "Housing_Symbol")
            | (18, "Housing_Collecting")
            | (19, "Housing_GatheredMaterials")
    ) {
        fields.clear();
    }
    Ok(Record::new("inventory", key, name, "inventory", fields))
}
/// Current EXE 0x1414ffd10 and array reader 0x14153b680. This controls
/// faction reoccupation, not ordinary NPC/quest respawns. Only delayTime is
/// editable; quest conditions, probabilities, close timers and node links stay intact.
pub fn reoccupation(bytes: &[u8]) -> Result<Record> {
    let mut r = Reader::new(bytes);
    let (key, name) = r.head(2)?;
    let quests = r.count()?;
    if quests == 0 {
        return Err(bad("reoccupation has no quest"));
    }
    r.take(quests * 20)?; // quest:u32, condition:u32, rate:u64, closeTime:u32
    let nodes = r.count()?;
    if nodes == 0 {
        return Err(bad("reoccupation has no faction node"));
    }
    r.take(nodes * 4)?;
    let delay = r.field("delayTime", 4, false, 1, 31_536_000, "reoccupation_delay")?;
    if delay.value == "0" || delay.value == u32::MAX.to_string() {
        return Err(bad("reoccupation delay sentinel"));
    }
    r.take(4)?; // protectCombatPower:f32, untouched
    r.end()?;
    let mut row = Record::new(
        "factionreblockadinginfo",
        key,
        name,
        "reoccupation",
        vec![delay],
    );
    row.description = "Wartezeit der Gebietswiederbesetzung. Andere Bedingungen und Quest-Zeitgeber bleiben erhalten. Spielzeiteinheit und Wirkung sind noch nicht abgenommen.".into();
    Ok(row)
}
pub fn equip(bytes: &[u8]) -> Result<Record> {
    let mut r = Reader::new(bytes);
    let (key, name) = r.head(4)?;
    r.take(12 + 8 + 2)?;
    let field = r.field("decreaseEndurancePercent", 8, false, 0, 100000000, "wear")?;
    r.take(8 + 4 + 8)?;
    r.array(4)?;
    r.loc()?;
    r.take(1)?;
    r.end()?;
    Ok(Record::new(
        "equiptypeinfo",
        key,
        name,
        "durability",
        vec![field],
    ))
}
pub fn field(bytes: &[u8]) -> Result<Record> {
    let mut r = Reader::new(bytes);
    let (key, name) = r.head(4)?;
    // Current executable serializes 15 scalar bytes at the end (old upstream layout is obsolete).
    let start = bytes
        .len()
        .checked_sub(15)
        .filter(|p| *p >= r.at)
        .ok_or_else(|| bad("field tail missing"))?;
    if bytes[start + 1..start + 12].iter().any(|v| *v > 1) {
        return Err(bad("field tail flag"));
    }
    r.at = start + 5;
    let value = r.field("alwaysCallVehicle_dev", 1, false, 0, 1, "call_vehicle")?;
    let fields = if key == 1 && name == "MainField" {
        vec![value]
    } else {
        vec![]
    };
    Ok(Record::new("fieldinfo", key, name, "world", fields))
}
pub(super) fn skill_tail(bytes: &[u8], at: usize) -> Result<(Vec<Field>, String, String)> {
    let mut r = Reader { bytes, at };
    let mut fields = Vec::new();
    r.take(8)?;
    fields.push(r.field("learnLevel", 4, false, 0, 10000, "numeric")?);
    r.take(1 + 8)?;
    for graph in ["needUpgradeItemCountGraph", "needUpgradeExperienceGraph"] {
        for part in ["val0", "val1", "val2"] {
            fields.push(r.field(
                format!("{graph}.{part}"),
                8,
                true,
                -1000000000,
                1000000000,
                "numeric",
            )?);
        }
        r.take(4)?;
    }
    r.array(4)?;
    r.array(4)?;
    r.take(8)?;
    for list in ["resources", "items", "driver_resources"] {
        for index in 0..r.count()? {
            if list == "items" {
                r.take(12)?;
                continue;
            }
            let kind = r.num(1)?;
            let stat = r.num(4)?;
            r.take(1)?;
            let rule = match (kind, stat) {
                (3, 1000026) => "stamina",
                (3, 1000027) => "spirit",
                _ => "numeric",
            };
            fields.push(r.field(
                format!("{list}[{index}].stat[{stat}]"),
                8,
                true,
                -1000000000,
                1000000000,
                rule,
            )?);
            r.take(8)?;
        }
    }
    fields.push(r.field("battery", 8, true, -1000000000, 1000000000, "numeric")?);
    for _ in 0..5 {
        if r.num(1)? > 1 {
            return Err(bad("skill tail boolean"));
        }
    }
    r.take(2)?;
    r.array(4)?;
    fields.push(r.field("maxLevel", 4, false, 0, 10000, "numeric")?);
    r.array(2)?;
    r.take(4)?;
    let display = r.text()?;
    let description = r.text()?;
    r.take(4)?;
    r.end()?;
    Ok((fields, display, description))
}
pub fn skill(bytes: &[u8]) -> Result<Record> {
    let mut r = Reader::new(bytes);
    let (key, name) = r.head(4)?;
    let cooldown = r.field("cooltime", 4, false, 0, 604800, "skill_cooldown")?;
    // The following key is skillGroupKey, not a second copy of this record's
    // identity. Walk the pinned matrix layout; never search for matching bytes.
    let end = super::skill_detail::matrix_end(bytes)?;
    let (mut fields, display, description) = skill_tail(bytes, end)?;
    fields.insert(0, cooldown);
    let mut row = Record::new("skill", key, name, "skill", fields);
    row.display = display;
    row.description = description;
    Ok(row)
}
pub fn buff_info(bytes: &[u8]) -> Result<Record> {
    let detail = super::skill_detail::inspect_buff_info(bytes)?;
    if detail.fields.iter().map(|f| f.bytes).sum::<usize>() != bytes.len() {
        return Err(bad("incomplete buff info coverage"));
    }
    Ok(detail.row)
}
fn spawn_targets(r: &mut Reader<'_>) -> Result<Vec<Field>> {
    let mut fields = Vec::new();
    for i in 0..r.count()? {
        for j in 0..r.count()? {
            r.take(1)?;
            for k in 0..r.count()? {
                let character = r.num(4)?;
                r.take(6)?;
                let sub = r.num(4)?;
                r.take(6)?;
                for (suffix, reference) in [("count", character), ("subCount", sub)] {
                    let field = r.field(
                        format!("spawn[{i}].group[{j}].character[{k}:{reference}].{suffix}"),
                        1,
                        false,
                        0,
                        255,
                        "spawn_count",
                    )?;
                    if reference != 0 || field.value != "0" {
                        fields.push(field);
                    }
                }
            }
            r.take(28)?;
            r.array(4)?;
            r.take(28 + 16 + 3 + 8)?;
        }
        r.array(2)?;
        r.array(2)?;
        r.array(4)?;
        r.array(4)?;
        r.take(24 + 7)?;
        fields.push(r.field(
            format!("spawn[{i}].spawnLimitCount"),
            2,
            false,
            0,
            65535,
            "spawn_limit",
        )?);
    }
    Ok(fields)
}
pub fn pool(bytes: &[u8]) -> Result<Record> {
    let mut r = Reader::new(bytes);
    let (key, name) = r.head(4)?;
    let fields = spawn_targets(&mut r)?;
    r.array(4)?;
    r.text()?;
    r.take(1 + 4 + 16 + 12 + 5)?;
    r.end()?;
    Ok(Record::new(
        "spawningpoolautospawninfo",
        key,
        name,
        "spawn",
        fields,
    ))
}
pub fn terrain(bytes: &[u8]) -> Result<Record> {
    let mut r = Reader::new(bytes);
    let (key, name) = r.head(4)?;
    r.array(1)?;
    r.text()?;
    r.text()?;
    r.array(2)?;
    r.array(2)?;
    r.array(4)?;
    r.array(4)?;
    let fields = spawn_targets(&mut r)?;
    // Road/bitmap/scheduling suffix is deliberately preserved without editable semantics.
    if bytes.len() - r.at < 20 {
        return Err(bad("terrain spawn suffix missing"));
    }
    Ok(Record::new(
        "terrainregionautospawninfo",
        key,
        name,
        "spawn",
        fields,
    ))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn inventory_moves_are_structural_and_special_containers_are_read_only() {
        let mut bytes = 13u16.to_le_bytes().to_vec();
        bytes.extend(4u32.to_le_bytes());
        bytes.extend(b"Kuku");
        bytes.push(0);
        bytes.extend(1u32.to_le_bytes());
        bytes.extend([7, 0, 2]);
        bytes.extend(0u32.to_le_bytes());
        bytes.extend(1u32.to_le_bytes()); // move
        bytes.extend([0; 9]);
        for _ in 0..3 {
            bytes.extend([0; 13]);
        }
        bytes.extend(1u32.to_le_bytes()); // nested item move
        bytes.extend([0; 17]);
        let condition = bytes.len();
        bytes.push(0);
        bytes.extend([0; 13]);
        let slots = bytes.len();
        bytes.extend(10u16.to_le_bytes());
        bytes.extend(1000u16.to_le_bytes());
        bytes.extend([0; 26 + 4 + 1 + 8 + 3]);
        bytes.extend(1u32.to_le_bytes());
        bytes.extend([0; 12]);
        let row = inventory(&bytes).unwrap();
        assert_eq!(row.fields[0].offset, slots);
        assert_eq!(row.fields[0].value, "10");
        assert_eq!(row.fields[1].value, "1000");
        for end in 0..bytes.len() {
            assert!(inventory(&bytes[..end]).is_err());
        }
        let mut bad = bytes.clone();
        bad[condition] = 1;
        assert!(inventory(&bad).is_err());
        let mut special = bytes.clone();
        special[..2].copy_from_slice(&14u16.to_le_bytes());
        assert!(inventory(&special).unwrap().fields.is_empty());
        bytes.push(0);
        assert!(inventory(&bytes).is_err());
    }

    #[test]
    fn reoccupation_reader_requires_complete_structure_and_preserves_other_timers() {
        let mut bytes = 7u16.to_le_bytes().to_vec();
        bytes.extend_from_slice(&4u32.to_le_bytes());
        bytes.extend_from_slice(b"Fort");
        bytes.push(0);
        bytes.extend_from_slice(&1u32.to_le_bytes());
        bytes.extend_from_slice(&101u32.to_le_bytes()); // quest
        bytes.extend_from_slice(&202u32.to_le_bytes()); // condition
        bytes.extend_from_slice(&10000u64.to_le_bytes()); // probability
        bytes.extend_from_slice(&259200u32.to_le_bytes()); // quest close timer
        bytes.extend_from_slice(&2u32.to_le_bytes());
        bytes.extend_from_slice(&303u32.to_le_bytes());
        bytes.extend_from_slice(&404u32.to_le_bytes());
        bytes.extend_from_slice(&86400u32.to_le_bytes());
        bytes.extend_from_slice(&(-1f32).to_le_bytes());
        let row = reoccupation(&bytes).unwrap();
        assert_eq!(row.key, 7);
        assert_eq!(row.fields.len(), 1);
        assert_eq!(row.fields[0].name, "delayTime");
        assert_eq!(row.fields[0].value, "86400");
        assert_eq!(row.fields[0].offset, bytes.len() - 8);
        for end in 0..bytes.len() {
            assert!(reoccupation(&bytes[..end]).is_err());
        }
        for (offset, value) in [(11, 0), (11, u32::MAX), (35, 0), (47, 0), (47, u32::MAX)] {
            let mut bad = bytes.clone();
            bad[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
            assert!(reoccupation(&bad).is_err());
        }
        bytes.push(0);
        assert!(reoccupation(&bytes).is_err());
    }
}
