//! Buff matrix inspection and fixed-width numeric edits for the pinned build.
//! Unknown semantics stay explicit; references and structural fields are read-only.
use super::{
    bad,
    reader::{self, Reader},
};
use crate::Result;
use serde::{Deserialize, Serialize};
use std::{collections::BTreeMap, sync::OnceLock};

mod buff_info;
mod suffix;
mod summon;
pub(super) use buff_info::inspect as inspect_buff_info;

#[derive(Deserialize)]
#[serde(untagged)]
enum Op {
    Bytes(usize),
    Variable(String),
}
#[derive(Deserialize)]
struct Layout {
    type_id: u8,
    name: String,
    ops: Vec<Op>,
    opaque_layout: bool,
}
fn layouts() -> &'static [Layout] {
    static LAYOUTS: OnceLock<Vec<Layout>> = OnceLock::new();
    LAYOUTS.get_or_init(|| {
        serde_json::from_str(include_str!(
            "../../../schemas/steam-25381195.skill-buffs.json"
        ))
        .expect("embedded buff layouts")
    })
}
#[derive(Serialize)]
pub struct Value {
    pub path: String,
    pub offset: usize,
    pub bytes: usize,
    pub kind: &'static str,
    pub value: String,
    /// Only confirmed fixed-width i64 values, never references, counts or type tags.
    pub editable: bool,
}
#[derive(Serialize)]
pub struct Buff {
    pub matrix_level: usize,
    pub index: usize,
    pub type_id: Option<u8>,
    pub name: String,
    pub opaque_layout: bool,
    pub fields: Vec<Value>,
}
#[derive(Serialize)]
pub struct SkillDetail {
    pub key: u32,
    pub name: String,
    pub byte_len: usize,
    pub matrix_start: usize,
    pub matrix_end: usize,
    pub level_counts: Vec<usize>,
    pub buffs: Vec<Buff>,
    /// Named header/suffix fields; existing scalar editing stays in Record.fields.
    pub record_fields: Vec<Value>,
    pub raw_hex: String,
}
fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|v| format!("{v:02x}")).collect()
}
struct Inspect<'a> {
    r: Reader<'a>,
    values: Vec<Value>,
}
impl Inspect<'_> {
    fn number(&mut self, path: impl Into<String>, width: usize, signed: bool) -> Result<u64> {
        let offset = self.r.at;
        let n = self.r.num(width)?;
        self.values.push(Value {
            path: path.into(),
            offset,
            bytes: width,
            kind: if signed {
                "signed integer"
            } else {
                "unsigned integer"
            },
            value: if signed {
                (n as i64).to_string()
            } else {
                n.to_string()
            },
            editable: signed && width == 8,
        });
        Ok(n)
    }
    fn text(&mut self, path: &str) -> Result<()> {
        let offset = self.r.at;
        let value = self.r.text()?;
        self.values.push(Value {
            path: path.into(),
            offset,
            bytes: self.r.at - offset,
            kind: "UTF-8 (u32 length)",
            value,
            editable: false,
        });
        Ok(())
    }
    fn array(&mut self, path: &str, width: usize) -> Result<()> {
        let count = self.number(format!("{path}.count"), 4, false)?;
        if count > 1000 {
            return Err(bad("buff array limit"));
        }
        for n in 0..count {
            self.number(format!("{path}[{n}]"), width, false)?;
        }
        Ok(())
    }
    fn raw(&mut self, path: &str, n: usize) -> Result<()> {
        let offset = self.r.at;
        let value = hex(self.r.take(n)?);
        self.values.push(Value {
            path: path.into(),
            offset,
            bytes: n,
            kind: "opaque hex",
            value,
            editable: false,
        });
        Ok(())
    }
    fn common(&mut self) -> Result<()> {
        for (memory, width) in [
            (12, 4),
            (16, 4),
            (20, 1),
            (21, 1),
            (24, 8),
            (32, 8),
            (40, 8),
        ] {
            self.number(format!("common.mem_{memory}"), width, width == 8)?;
        }
        self.text("common.mem_48")?;
        for (memory, width) in [
            (56, 4),
            (58, 1),
            (60, 4),
            (62, 4),
            (64, 4),
            (66, 4),
            (68, 1),
            (69, 1),
            (88, 4),
            (90, 4),
        ] {
            self.number(format!("common.mem_{memory}"), width, false)?;
        }
        self.array("common.mem_96", 4)?;
        for memory in [128, 72, 76, 80, 84] {
            self.number(format!("common.mem_{memory}"), 4, false)?;
        }
        self.array("common.mem_112", 4)?;
        self.number("common.mem_132", 1, false)?;
        self.number("common.mem_136", 4, false)?;
        Ok(())
    }
    fn tail(&mut self, layout: &Layout) -> Result<()> {
        for (n, op) in layout.ops.iter().enumerate() {
            let path = format!("payload.part_{n}");
            match op {
                Op::Bytes(width) if [1, 2, 4, 8].contains(width) && !layout.opaque_layout => {
                    self.number(path, *width, *width == 8)?;
                }
                Op::Bytes(28) if !layout.opaque_layout => {
                    // Current Graph reader 0x1424047b0: three i64 values + u32 curve tag.
                    for n in 0..3 {
                        self.number(format!("{path}.val{n}"), 8, true)?;
                    }
                    self.number(format!("{path}.curve"), 4, false)?;
                }
                Op::Bytes(width) => self.raw(&path, *width)?,
                Op::Variable(kind) => match kind.as_str() {
                    "additional_costs" => self.additional_costs()?,
                    "summon" => self.summon()?,
                    "sub_level" => {
                        // Serializer 0x141f315b0 writes a resolved reference, then
                        // four bytes at memory +0x94. Neither is an i64 scalar.
                        self.number("payload.subLevelReference", 4, false)?;
                        self.raw("payload.mem_148", 4)?;
                    }
                    "text" => self.text(&path)?,
                    "array4" => self.array(&path, 4)?,
                    "texts" => {
                        let count = self.number(format!("{path}.count"), 4, false)?;
                        if count > 1000 {
                            return Err(bad("buff string array limit"));
                        }
                        for i in 0..count {
                            self.text(&format!("{path}[{i}]"))?;
                        }
                    }
                    "immune" => {
                        let kind = self.number(format!("{path}.kind"), 1, false)?;
                        self.number(format!("{path}.flag"), 1, false)?;
                        let width = *[1, 4, 4, 4, 4, 8]
                            .get(kind as usize)
                            .ok_or_else(|| bad("unknown immune payload"))?;
                        self.array(&format!("{path}.entries"), width)?;
                    }
                    _ => return Err(bad("unsupported buff payload")),
                },
            }
        }
        Ok(())
    }
}
fn matrix(mut i: Inspect<'_>) -> Result<(Inspect<'_>, Vec<Buff>, Vec<usize>, usize)> {
    let levels = i.r.count()?;
    if levels > 100 {
        return Err(bad("skill matrix levels"));
    }
    let matrix_start = i.r.at;
    let mut buffs = Vec::new();
    let mut level_counts = Vec::new();
    for level in 0..levels {
        let count = i.r.count()?;
        if count > 100 {
            return Err(bad("skill matrix width"));
        }
        level_counts.push(count);
        for index in 0..count {
            let flag = i.number("null", 1, false)?;
            let mut buff = Buff {
                matrix_level: level,
                index,
                type_id: None,
                name: "null".into(),
                opaque_layout: false,
                fields: vec![],
            };
            if flag == 0 {
                let typ = i.number("type", 1, false)? as u8;
                let layout = layouts()
                    .iter()
                    .find(|v| v.type_id == typ)
                    .ok_or_else(|| bad("unknown skill buff variant"))?;
                i.common()?;
                i.tail(layout)?;
                buff.type_id = Some(typ);
                buff.name = layout.name.clone();
                buff.opaque_layout = layout.opaque_layout;
            } else if flag != 1 {
                return Err(bad("invalid null buff flag"));
            }
            buff.fields = std::mem::take(&mut i.values);
            buffs.push(buff);
        }
    }
    Ok((i, buffs, level_counts, matrix_start))
}

pub(super) fn matrix_end(bytes: &[u8]) -> Result<usize> {
    let mut r = Reader::new(bytes);
    r.head(4)?;
    r.take(4)?;
    Ok(matrix(Inspect { r, values: vec![] })?.0.r.at)
}

pub fn inspect(bytes: &[u8]) -> Result<SkillDetail> {
    let mut i = Inspect {
        r: Reader::new(bytes),
        values: vec![],
    };
    let key = i.number("key", 4, false)? as u32;
    i.text("stringKey")?;
    let name = i.values.last().unwrap().value.clone();
    if i.number("isBlocked", 1, false)? > 1 {
        return Err(bad("invalid blocked flag"));
    }
    i.number("cooltime", 4, false)?;
    // Include the level count among the header fields but let matrix own its read.
    let count_at = i.r.at;
    i.number("buffLevelList.count", 4, false)?;
    i.r.at = count_at;
    let mut record_fields = std::mem::take(&mut i.values);
    let (mut i, buffs, level_counts, matrix_start) = matrix(i)?;
    let end = i.r.at;
    reader::skill_tail(bytes, end)?;
    i.suffix()?;
    i.r.end()?;
    record_fields.extend(i.values);
    // Read-only inspection must never create an alternative edit path.
    for f in &mut record_fields {
        f.editable = false;
    }
    Ok(SkillDetail {
        key,
        name,
        byte_len: bytes.len(),
        matrix_start,
        matrix_end: end,
        level_counts,
        buffs,
        record_fields,
        raw_hex: hex(bytes),
    })
}

type NumericEdits = (Vec<u8>, Vec<(String, String, String)>);
/// Two pinned self-consumption skills use a VaryDataDefinedStat payload instead
/// of the suffix cost list. Enemy drains and regeneration are not action costs.
pub(super) fn self_cost_overrides(bytes: &[u8], percent: u32) -> Result<BTreeMap<String, String>> {
    let detail = inspect(bytes)?;
    let expected = match detail.key {
        40013 => "Active_UseResource_Mp",
        10300 => "Skill_ElementalReinforce_UseMp",
        _ => return Ok(BTreeMap::new()),
    };
    if detail.name != expected {
        return Err(bad("self-consumption skill identity changed"));
    }
    let mut edits = BTreeMap::new();
    for buff in &detail.buffs {
        if buff.type_id != Some(12)
            || !buff
                .fields
                .iter()
                .any(|f| f.path == "payload.part_0" && f.value == "1000027")
        {
            continue;
        }
        let field = buff
            .fields
            .iter()
            .find(|f| f.path == "payload.part_1")
            .ok_or_else(|| bad("missing self-consumption amount"))?;
        let before = field
            .value
            .parse::<i64>()
            .map_err(|_| bad("invalid consumption amount"))?;
        if before < 0 {
            edits.insert(
                format!("{}/{}/{}", buff.matrix_level, buff.index, field.path),
                super::scale(before, percent)?.to_string(),
            );
        }
    }
    Ok(edits)
}

pub(super) fn edit(bytes: &[u8], overrides: &BTreeMap<String, String>) -> Result<NumericEdits> {
    if overrides.len() > 1000 {
        return Err(bad("too many buff value edits"));
    }
    let detail = inspect(bytes)?;
    let mut remaining = overrides.clone();
    let mut output = bytes.to_vec();
    let mut changes = Vec::new();
    for buff in &detail.buffs {
        for field in &buff.fields {
            let id = format!("{}/{}/{}", buff.matrix_level, buff.index, field.path);
            let Some(value) = remaining.remove(&id) else {
                continue;
            };
            if !field.editable {
                return Err(bad(format!(
                    "{id}: structural/reference/opaque field is read-only"
                )));
            }
            let n = value
                .parse::<i64>()
                .map_err(|_| bad("buff value requires a decimal integer"))?;
            if n.unsigned_abs() > 1_000_000_000 {
                return Err(bad("buff input limit: -1000000000..1000000000"));
            }
            if n.to_string() == field.value {
                continue;
            }
            output[field.offset..field.offset + 8].copy_from_slice(&n.to_le_bytes());
            changes.push((format!("buffs/{id}"), field.value.clone(), n.to_string()));
        }
    }
    if !remaining.is_empty() {
        return Err(bad("unknown buff value override"));
    }
    inspect(&output)?;
    Ok((output, changes))
}

#[cfg(test)]
mod tests {
    use super::*;
    fn fixture() -> Vec<u8> {
        let mut b = 7u32.to_le_bytes().to_vec();
        b.extend(1u32.to_le_bytes());
        b.push(b'S');
        b.push(0);
        b.extend(10u32.to_le_bytes());
        b.extend(2u32.to_le_bytes());
        b.extend(1u32.to_le_bytes());
        b.push(1); // one null entry
        b.extend(0u32.to_le_bytes()); // empty second level
        b.extend(7u32.to_le_bytes());
        b.extend([0; 4]); // suffix identity
        b.extend(1u32.to_le_bytes());
        b.extend([0; 9]);
        b.extend([0; 56]); // two graphs
        b.extend([0; 8]); // two arrays
        b.extend([0; 8]);
        b.extend([0; 12]); // three cost arrays
        b.extend([0; 8]);
        b.extend([0; 5]);
        b.extend([0; 2]);
        b.extend([0; 4]);
        b.extend(1u32.to_le_bytes());
        b.extend([0; 4]);
        b.extend([0; 4]);
        b.extend([0; 4]);
        b.extend([0; 4]);
        b.extend([0; 4]);
        b
    }
    fn populated(typ: u8, tail: &[u8]) -> Vec<u8> {
        let mut b = fixture();
        let at = inspect(&b).unwrap().buffs[0].fields[0].offset;
        let mut buff = vec![0, typ];
        buff.extend([0; 102]); // empty common strings and arrays
        buff.extend(tail);
        b.splice(at..at + 1, buff);
        b
    }
    #[test]
    fn self_consumption_scales_only_owned_costs_and_explicit_buff_values_win() {
        let mut tail = 1000027u32.to_le_bytes().to_vec();
        tail.extend((-12000i64).to_le_bytes());
        tail.extend((-2147483648000i64).to_le_bytes());
        tail.extend(2147483647000i64.to_le_bytes());
        let b = populated(12, &tail);
        assert!(self_cost_overrides(&b, 0).unwrap().is_empty());
        let mut named = 40013u32.to_le_bytes().to_vec();
        let name = "Active_UseResource_Mp";
        named.extend((name.len() as u32).to_le_bytes());
        named.extend(name.as_bytes());
        named.extend(&b[9..]);
        let path = "0/0/payload.part_1";
        let auto = self_cost_overrides(&named, 50).unwrap();
        assert_eq!(auto[path], "-6000");
        let (half, _) = edit(&named, &auto).unwrap();
        let amount = inspect(&named).unwrap().buffs[0]
            .fields
            .iter()
            .find(|f| f.path == "payload.part_1")
            .unwrap()
            .offset;
        assert_eq!(&half[..amount], &named[..amount]);
        assert_eq!(&half[amount + 8..], &named[amount + 8..]);
        let mut merged = self_cost_overrides(&named, 0).unwrap();
        merged.insert(path.into(), "-123".into());
        let (_, diff) = edit(&named, &merged).unwrap();
        assert_eq!(
            diff,
            vec![(format!("buffs/{path}"), "-12000".into(), "-123".into())]
        );
        named[amount..amount + 8].copy_from_slice(&12000i64.to_le_bytes());
        assert!(self_cost_overrides(&named, 0).unwrap().is_empty());
    }
    #[test]
    fn buff_edits_preserve_structure_and_reject_refs_opaque_values_and_bad_numbers() {
        let b = populated(17, &[]);
        let path = "0/0/common.mem_24".to_string();
        let (changed, diff) = edit(&b, &BTreeMap::from([(path.clone(), "-123".into())])).unwrap();
        let d = inspect(&changed).unwrap();
        let f = d.buffs[0]
            .fields
            .iter()
            .find(|f| f.path == "common.mem_24")
            .unwrap();
        assert_eq!(f.value, "-123");
        assert_eq!(diff.len(), 1);
        assert_eq!(&changed[..f.offset], &b[..f.offset]);
        assert_eq!(&changed[f.offset + 8..], &b[f.offset + 8..]);
        assert!(
            edit(&changed, &BTreeMap::from([(path.clone(), "-123".into())]))
                .unwrap()
                .1
                .is_empty()
        );
        for value in ["", "1.5", "1e3", "1000000001", "-9223372036854775808"] {
            assert!(edit(&b, &BTreeMap::from([(path.clone(), value.into())])).is_err());
        }
        for path in [
            "0/0/type",
            "0/0/common.mem_12",
            "0/0/common.mem_96.count",
            "1/0/common.mem_24",
            "0/0/missing",
        ] {
            assert!(edit(&b, &BTreeMap::from([(path.into(), "0".into())])).is_err());
        }
        let opaque = populated(74, &[0; 8]);
        assert!(
            edit(
                &opaque,
                &BTreeMap::from([("0/0/payload.part_0".into(), "1".into())])
            )
            .is_err()
        );
    }
    #[test]
    fn summon_named_fields_preserve_bytes_and_reject_unsupported_shapes() {
        let b = populated(10, &[0; 181]);
        let d = inspect(&b).unwrap();
        let buff = &d.buffs[0];
        assert!(!buff.opaque_layout);
        let payload: Vec<_> = buff
            .fields
            .iter()
            .filter(|v| v.path.starts_with("payload."))
            .collect();
        assert_eq!(payload.iter().map(|v| v.bytes).sum::<usize>(), 181);
        assert!(payload.iter().all(|v| !v.editable));
        for name in [
            "characterKey",
            "spawnPercent",
            "deadLimitTime",
            "terrainRegionAutoSpawnData.spawnableCheckInterval",
        ] {
            let path = format!("payload.summon.{name}");
            assert!(payload.iter().any(|v| v.path == path));
            assert!(edit(&b, &BTreeMap::from([(format!("0/0/{path}"), "123".into())])).is_err());
        }
        let start = payload[0].offset;
        for (offset, width, value) in [(0, 4, 1001u32), (4, 1, 1), (4, 1, 2), (28, 4, u32::MAX)] {
            let mut invalid = b.clone();
            invalid[start + offset..start + offset + width]
                .copy_from_slice(&value.to_le_bytes()[..width]);
            assert!(inspect(&invalid).is_err());
        }
        // Nonempty select lists and UTF-8 strings change all following offsets.
        let mut varied = b.clone();
        varied[start + 28..start + 32].copy_from_slice(&2u32.to_le_bytes());
        varied.splice(start + 32..start + 32, "ä".as_bytes().iter().copied());
        varied[start..start + 4].copy_from_slice(&1u32.to_le_bytes());
        varied.splice(start + 4..start + 4, [7, 0, 9, 0]);
        let d = inspect(&varied).unwrap();
        assert!(
            d.buffs[0]
                .fields
                .iter()
                .any(|v| v.path.ends_with("selectDataList[0].regionInfo") && v.value == "9")
        );
        assert!(
            d.buffs[0]
                .fields
                .iter()
                .any(|v| v.path.ends_with("appearanceName") && v.value == "ä")
        );
        for end in 0..181 {
            let mut i = Inspect {
                r: Reader::new(&b[start..start + end]),
                values: vec![],
            };
            assert!(i.summon().is_err());
        }
        let (changed, _) = edit(
            &varied,
            &BTreeMap::from([("0/0/common.mem_24".into(), "123".into())]),
        )
        .unwrap();
        assert_eq!(&changed[start..], &varied[start..]);
    }
    #[test]
    fn sub_level_reference_and_unknown_word_are_separate_read_only_fields() {
        let b = populated(74, &[1, 2, 3, 4, 255, 255, 255, 255]);
        let d = inspect(&b).unwrap();
        let buff = &d.buffs[0];
        assert!(!buff.opaque_layout);
        let tail = &buff.fields[buff.fields.len() - 2..];
        assert_eq!(tail[0].path, "payload.subLevelReference");
        assert_eq!(tail[0].value, "67305985");
        assert_eq!(tail[1].path, "payload.mem_148");
        assert_eq!(tail[1].value, "ffffffff");
        assert!(tail.iter().all(|v| v.bytes == 4 && !v.editable));
        for field in tail {
            assert!(
                edit(
                    &b,
                    &BTreeMap::from([(format!("0/0/{}", field.path), "0".into())])
                )
                .is_err()
            );
        }
    }
    #[test]
    fn suffix_is_structural_even_when_group_key_differs_or_strings_contain_keys() {
        let mut b = populated(17, &[]);
        let start = matrix_end(&b).unwrap();
        // A group reference need not equal the record key. Searching for key 7
        // would reject this well-formed record before finding the actual suffix.
        b[start..start + 4].copy_from_slice(&900u32.to_le_bytes());
        let detail = inspect(&b).unwrap();
        assert_eq!(reader::skill(&b).unwrap().key, 7);
        let group = detail
            .record_fields
            .iter()
            .find(|v| v.path == "skillGroupKey")
            .unwrap();
        assert_eq!(
            (group.value.as_str(), group.kind, group.editable),
            ("900", "reference", false)
        );
        let text = detail
            .record_fields
            .iter()
            .find(|v| v.path == "devSkillDesc")
            .unwrap();
        let mut repeated_key = 7u32.to_le_bytes().to_vec();
        repeated_key.extend("ä\nDescription".as_bytes());
        b[text.offset..text.offset + 4].copy_from_slice(&(repeated_key.len() as u32).to_le_bytes());
        b.splice(text.offset + 4..text.offset + 4, repeated_key);
        let detail = inspect(&b).unwrap();
        assert_eq!(detail.matrix_end, start);
        assert!(detail.record_fields.iter().all(|v| !v.editable));
        assert!(
            reader::skill(&b)
                .unwrap()
                .description
                .ends_with("Description")
        );
        let (changed, _) = edit(
            &b,
            &BTreeMap::from([("0/0/common.mem_24".into(), "7".into())]),
        )
        .unwrap();
        assert_eq!(&changed[start..], &b[start..]);
        // Bad matrix data must not be rescued by a valid-looking suffix later.
        b[detail.buffs[0].fields[1].offset] = 255;
        assert!(reader::skill(&b).is_err());
        assert!(inspect(&b).is_err());
    }

    #[test]
    fn suffix_lists_keep_reference_widths_and_resource_values_separate() {
        let mut b = fixture();
        let d = inspect(&b).unwrap();
        // Insert from right to left, retaining original offsets.
        for (path, data) in [
            ("skillGroupKeyList.count", vec![0x34, 0x12]),
            (
                "useResourceItemList.count",
                [123u32.to_le_bytes().as_slice(), &42u64.to_le_bytes()].concat(),
            ),
            (
                "useResourceStatList.count",
                [
                    vec![3],
                    1000026u32.to_le_bytes().to_vec(),
                    vec![1],
                    (-10000i64).to_le_bytes().to_vec(),
                    111u32.to_le_bytes().to_vec(),
                    222u32.to_le_bytes().to_vec(),
                ]
                .concat(),
            ),
            (
                "usableCharacterInfoList.count",
                987u32.to_le_bytes().to_vec(),
            ),
        ] {
            let at = d
                .record_fields
                .iter()
                .find(|v| v.path == path)
                .unwrap()
                .offset;
            b[at..at + 4].copy_from_slice(&1u32.to_le_bytes());
            b.splice(at + 4..at + 4, data);
        }
        let d = inspect(&b).unwrap();
        for (path, value, kind, width) in [
            ("skillGroupKeyList[0]", "4660", "reference", 2),
            ("useResourceItemList[0].itemInfo", "123", "reference", 4),
            (
                "useResourceItemList[0].useItemCount",
                "2a00000000000000",
                "opaque hex",
                8,
            ),
            (
                "useResourceStatList[0].varyStatAmount",
                "-10000",
                "signed integer",
                8,
            ),
            (
                "useResourceStatList[0].increaseStatusInfo",
                "111",
                "reference",
                4,
            ),
            (
                "useResourceStatList[0].decreaseStatusInfo",
                "222",
                "reference",
                4,
            ),
        ] {
            let f = d.record_fields.iter().find(|v| v.path == path).unwrap();
            assert_eq!(
                (f.value.as_str(), f.kind, f.bytes, f.editable),
                (value, kind, width, false)
            );
        }
        for path in [
            "useResourceStatList.count",
            "useResourceItemList.count",
            "skillGroupKeyList.count",
        ] {
            let at = d
                .record_fields
                .iter()
                .find(|v| v.path == path)
                .unwrap()
                .offset;
            let mut bad = b.clone();
            bad[at..at + 4].copy_from_slice(&100001u32.to_le_bytes());
            assert!(inspect(&bad).is_err());
        }
        for end in 0..b.len() {
            assert!(inspect(&b[..end]).is_err());
        }
    }

    #[test]
    fn graph_payload_exposes_three_signed_values_but_never_curve_tag() {
        let mut b = Vec::new();
        for n in [-1i64, 2, 3] {
            b.extend(n.to_le_bytes());
        }
        b.extend(7u32.to_le_bytes());
        let mut i = Inspect {
            r: Reader::new(&b),
            values: vec![],
        };
        i.tail(&Layout {
            type_id: 0,
            name: String::new(),
            ops: vec![Op::Bytes(28)],
            opaque_layout: false,
        })
        .unwrap();
        i.r.end().unwrap();
        assert_eq!(
            i.values
                .iter()
                .map(|v| (v.value.as_str(), v.editable))
                .collect::<Vec<_>>(),
            vec![("-1", true), ("2", true), ("3", true), ("7", false)]
        );
    }
    #[test]
    fn matrix_nulls_empty_levels_and_exact_end_are_preserved() {
        let b = fixture();
        let d = inspect(&b).unwrap();
        assert_eq!(d.key, 7);
        assert_eq!(d.level_counts, vec![1, 0]);
        assert_eq!(d.buffs.len(), 1);
        assert!(d.buffs[0].type_id.is_none());
        assert_eq!(d.raw_hex, hex(&b));
        for end in 0..b.len() {
            assert!(inspect(&b[..end]).is_err());
        }
        let mut invalid = b.clone();
        invalid[d.buffs[0].fields[0].offset] = 2;
        assert!(inspect(&invalid).is_err());
        let mut invalid = b.clone();
        invalid[d.matrix_start..d.matrix_start + 4].copy_from_slice(&101u32.to_le_bytes());
        assert!(inspect(&invalid).is_err());
        let mut invalid = b;
        invalid.insert(d.matrix_end, 0);
        assert!(inspect(&invalid).is_err());
    }
    #[test]
    fn common_values_keep_signed_numbers_and_bound_variable_lengths() {
        let mut b = vec![0; 34];
        b[10..18].copy_from_slice(&(-5i64).to_le_bytes());
        b.extend(1u32.to_le_bytes());
        b.push(b'X');
        b.extend([0; 64]);
        let mut i = Inspect {
            r: Reader::new(&b),
            values: vec![],
        };
        i.common().unwrap();
        i.r.end().unwrap();
        assert!(
            i.values
                .iter()
                .any(|f| f.path == "common.mem_24" && f.value == "-5")
        );
        for end in 0..b.len() {
            let mut i = Inspect {
                r: Reader::new(&b[..end]),
                values: vec![],
            };
            assert!(i.common().is_err());
        }
        b[34..38].copy_from_slice(&u32::MAX.to_le_bytes());
        let mut i = Inspect {
            r: Reader::new(&b),
            values: vec![],
        };
        assert!(i.common().is_err());
    }
}
