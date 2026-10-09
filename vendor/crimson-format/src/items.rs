//! Current 2.03 item schema and explicitly opaque indexed-table support.
use crate::{
    MAX_FILE_BYTES,
    binary::{BinaryRead, BinaryReadTracked, BinaryWrite, LocalizableString},
    invalid,
    item_info::ItemInfo,
};
use serde::Serialize;
use serde_json::{Value, json};
use std::{
    collections::{HashMap, HashSet},
    io,
};
#[derive(Debug, Clone, Serialize)]
pub struct LocalizableText {
    pub category: u8,
    pub index: u64,
    pub default: String,
}
impl From<&LocalizableString<'_>> for LocalizableText {
    fn from(s: &LocalizableString<'_>) -> Self {
        Self {
            category: s.category,
            index: s.index,
            default: s.default.data.to_string(),
        }
    }
}
#[derive(Debug, Clone, Serialize)]
pub struct ItemRecord {
    pub key: u32,
    pub string_key: String,
    pub item_type: u8,
    pub item_tier: u8,
    pub category_info: u16,
    pub max_stack_count: u64,
    pub name: LocalizableText,
    pub description: LocalizableText,
    pub description2: LocalizableText,
    pub icon_path: Option<u32>,
    pub inventory_info_list: [u16; 10],
    /// Status hashes, with no invented gameplay names or units.
    pub stat_keys: Vec<u32>,
    /// Explicit knowledge rewards; page conditions are not rewards.
    pub knowledge_keys: Vec<u32>,
    pub offset: usize,
    pub length: usize,
}
#[derive(Debug, Clone, Serialize)]
pub struct RawField {
    pub path: String,
    pub start: usize,
    pub end: usize,
    pub type_name: String,
    pub raw_hex: String,
    pub value: Value,
    /// "unknown" or "upstream_named": named fields are not a gameplay guarantee.
    pub interpretation: String,
}
#[derive(Debug, Clone, Serialize)]
pub struct RawRecord {
    pub key: u64,
    pub offset: usize,
    pub length: usize,
}
/// An indexed opaque body. This validates boundaries, NOT an unknown table schema.
#[derive(Debug)]
pub struct RawTable<'a> {
    body: &'a [u8],
    rows: Vec<RawRecord>,
    ordered_index: Vec<(u64, u32)>,
    key_width: usize,
    count_width: usize,
}
impl<'a> RawTable<'a> {
    pub fn parse(
        body: &'a [u8],
        header: &[u8],
        key_width: usize,
        count_width: usize,
    ) -> io::Result<Self> {
        if body.len() > MAX_FILE_BYTES
            || !matches!(key_width, 2 | 4 | 8)
            || !matches!(count_width, 2 | 4)
        {
            return Err(invalid("invalid table size or index widths"));
        }
        let count = number(
            header
                .get(..count_width)
                .ok_or_else(|| invalid("truncated table index"))?,
        )? as usize;
        let stride = key_width + 4;
        let required = count
            .checked_mul(stride)
            .and_then(|n| n.checked_add(count_width))
            .ok_or_else(|| invalid("table index length overflow"))?;
        if required != header.len() || count > 1_000_000 {
            return Err(invalid("table index count/length mismatch"));
        }
        if count == 0 {
            if !body.is_empty() {
                return Err(invalid("nonempty body without index entries"));
            }
            return Ok(Self {
                body,
                rows: vec![],
                ordered_index: vec![],
                key_width,
                count_width,
            });
        }
        let mut keys = HashSet::new();
        let mut offsets = HashSet::new();
        let mut ordered_index = Vec::with_capacity(count);
        for i in 0..count {
            let p = count_width + i * stride;
            let key = number(&header[p..p + key_width])?;
            let offset = number(&header[p + key_width..p + stride])? as u32;
            if !keys.insert(key) || !offsets.insert(offset) || offset as usize >= body.len() {
                return Err(invalid("duplicate key/offset or out-of-bounds table index"));
            }
            ordered_index.push((key, offset));
        }
        let mut sorted = ordered_index.clone();
        sorted.sort_unstable_by_key(|r| r.1);
        if sorted[0].1 != 0 {
            return Err(invalid("table body has unindexed leading bytes"));
        }
        let rows = sorted
            .iter()
            .enumerate()
            .map(|(i, &(key, offset))| RawRecord {
                key,
                offset: offset as usize,
                length: sorted.get(i + 1).map_or(body.len(), |r| r.1 as usize) - offset as usize,
            })
            .collect();
        Ok(Self {
            body,
            rows,
            ordered_index,
            key_width,
            count_width,
        })
    }
    pub fn records(&self) -> &[RawRecord] {
        &self.rows
    }
    pub fn record_bytes(&self, key: u64) -> io::Result<&'a [u8]> {
        let r = self
            .rows
            .iter()
            .find(|r| r.key == key)
            .ok_or_else(|| invalid("table key not found"))?;
        Ok(&self.body[r.offset..r.offset + r.length])
    }
    pub fn serialize_body(&self) -> Vec<u8> {
        self.body.to_vec()
    }
    pub fn serialize_header(&self) -> io::Result<Vec<u8>> {
        let mut output = Vec::new();
        let count = self.ordered_index.len() as u64;
        output.extend_from_slice(&count.to_le_bytes()[..self.count_width]);
        for &(key, offset) in &self.ordered_index {
            output.extend_from_slice(&key.to_le_bytes()[..self.key_width]);
            output.extend_from_slice(&offset.to_le_bytes());
        }
        Ok(output)
    }
}
fn number(bytes: &[u8]) -> io::Result<u64> {
    let mut value = [0; 8];
    if bytes.len() > 8 {
        return Err(invalid("integer too wide"));
    }
    value[..bytes.len()].copy_from_slice(bytes);
    Ok(u64::from_le_bytes(value))
}
#[derive(Debug)]
pub struct ItemTable<'a> {
    body: &'a [u8],
    index: RawTable<'a>,
    entries: Vec<ItemRecord>,
    by_key: HashMap<u32, usize>,
}
impl<'a> ItemTable<'a> {
    /// Parse only the current 2.03 schema; the caller must first gate the build.
    /// Every header key/offset, complete row consumption, and typed row roundtrip
    /// is checked. No anchor scans, skipped rows or trailing-byte tolerance.
    pub fn parse(body: &'a [u8], header: &'a [u8]) -> io::Result<Self> {
        let index = RawTable::parse(body, header, 4, 2)?;
        let mut entries = Vec::with_capacity(index.rows.len());
        let mut by_key = HashMap::new();
        for row in &index.rows {
            if row.length > 4 * 1024 * 1024 {
                return Err(invalid("item record exceeds 4 MiB limit"));
            }
            let bytes = &body[row.offset..row.offset + row.length];
            let mut offset = 0;
            let item = ItemInfo::read_from(bytes, &mut offset)
                .map_err(|e| invalid(format!("item {} at {}: {e}", row.key, row.offset)))?;
            if offset != bytes.len() || u64::from(item.key.0) != row.key {
                return Err(invalid(format!(
                    "item {} does not match index bounds/key",
                    row.key
                )));
            }
            let mut output = Vec::with_capacity(bytes.len());
            item.write_to(&mut output)?;
            if output != bytes {
                return Err(invalid(format!(
                    "item {} fails typed byte-identical roundtrip",
                    row.key
                )));
            }
            let mut stat_keys = Vec::new();
            let mut add_stats = |stats: &crate::item_info::structs::EnchantStatData| {
                stat_keys.extend(stats.max_stat_list.items.iter().map(|s| s.stat.0));
                stat_keys.extend(stats.regen_stat_list.items.iter().map(|s| s.stat.0));
                stat_keys.extend(stats.stat_list_static.items.iter().map(|s| s.stat.0));
                stat_keys.extend(stats.stat_list_static_level.items.iter().map(|s| s.stat.0));
            };
            add_stats(&item.sharpness_data.stat_data);
            for enchant in &item.enchant_data_list.items {
                add_stats(&enchant.enchant_stat_data);
            }
            stat_keys.sort_unstable();
            stat_keys.dedup();
            let mut knowledge_keys = vec![item.knowledge_info.0];
            for inspect in &item.inspect_data_list.items {
                if inspect.reward_own_knowledge != 0 {
                    knowledge_keys.push(inspect.reward_knowledge_info.0);
                }
            }
            knowledge_keys.retain(|&k| k != 0 && k != u32::MAX);
            knowledge_keys.sort_unstable();
            knowledge_keys.dedup();
            let record = ItemRecord {
                key: item.key.0,
                string_key: item.string_key.data.to_string(),
                item_type: item.item_type,
                item_tier: item.item_tier,
                category_info: item.category_info.0,
                max_stack_count: item.max_stack_count,
                name: (&item.item_name).into(),
                description: (&item.item_desc).into(),
                description2: (&item.item_desc2).into(),
                icon_path: item.item_icon_list.items.first().map(|i| i.icon_path.0),
                inventory_info_list: item.inventory_info_list,
                stat_keys,
                knowledge_keys,
                offset: row.offset,
                length: row.length,
            };
            by_key.insert(record.key, entries.len());
            entries.push(record);
        }
        Ok(Self {
            body,
            index,
            entries,
            by_key,
        })
    }
    pub fn entries(&self) -> &[ItemRecord] {
        &self.entries
    }
    pub fn serialize_body(&self) -> io::Result<Vec<u8>> {
        let mut output = Vec::with_capacity(self.body.len());
        for r in &self.entries {
            let mut offset = 0;
            ItemInfo::read_from(&self.body[r.offset..r.offset + r.length], &mut offset)?
                .write_to(&mut output)?;
        }
        Ok(output)
    }
    pub fn serialize_header(&self) -> io::Result<Vec<u8>> {
        self.index.serialize_header()
    }
    pub fn fields(&self, key: u32) -> io::Result<Vec<RawField>> {
        let record = self
            .by_key
            .get(&key)
            .map(|i| &self.entries[*i])
            .ok_or_else(|| invalid("item key not found"))?;
        let bytes = &self.body[record.offset..record.offset + record.length];
        let mut offset = 0;
        let mut ranges = Vec::new();
        ItemInfo::read_tracked(bytes, &mut offset, &mut String::new(), &mut ranges)?;
        if offset != bytes.len() {
            return Err(invalid("tracked item parser did not consume whole record"));
        }
        let mut output = Vec::with_capacity(ranges.len());
        let mut end = 0;
        for r in ranges {
            if r.start != end || r.end < r.start || r.end > bytes.len() {
                return Err(invalid("tracked fields overlap or leave unreported bytes"));
            }
            end = r.end;
            let data = &bytes[r.start..r.end];
            let unknown = r
                .path
                .split('.')
                .any(|p| p.starts_with("unk") || p.starts_with("unknown"))
                || r.path.contains("is_equip_quick_slot_visible");
            output.push(RawField {
                path: r.path,
                start: record.offset + r.start,
                end: record.offset + r.end,
                type_name: r.ty.into(),
                raw_hex: hex(data),
                value: field_value(r.ty, data),
                interpretation: if unknown { "unknown" } else { "upstream_named" }.into(),
            });
        }
        if end != bytes.len() {
            return Err(invalid("tracked item fields leave a trailing gap"));
        }
        Ok(output)
    }
}
pub(crate) fn hex(data: &[u8]) -> String {
    use std::fmt::Write;
    let mut s = String::with_capacity(data.len() * 2);
    for b in data {
        write!(&mut s, "{b:02x}").unwrap();
    }
    s
}
fn field_value(ty: &str, data: &[u8]) -> Value {
    match (ty, data.len()) {
        (name, 4) if name.ends_with("Key") => json!(u32::from_le_bytes(data.try_into().unwrap())),
        (name, 2) if name.ends_with("Key") => json!(u16::from_le_bytes(data.try_into().unwrap())),
        ("u8" | "COptional.tag", 1) => json!(data[0]),
        ("i8", 1) => json!(data[0] as i8),
        ("u16", 2) => json!(u16::from_le_bytes(data.try_into().unwrap())),
        (
            "u32"
            | "CString.len"
            | "CArray.count"
            | "EnchantDataList.count"
            | "EnchantDataList.sep",
            4,
        ) => json!(u32::from_le_bytes(data.try_into().unwrap())),
        ("i32", 4) => json!(i32::from_le_bytes(data.try_into().unwrap())),
        ("u64", 8) => {
            let v = u64::from_le_bytes(data.try_into().unwrap());
            if v > 9_007_199_254_740_991 {
                json!(v.to_string())
            } else {
                json!(v)
            }
        }
        ("i64", 8) => {
            let v = i64::from_le_bytes(data.try_into().unwrap());
            if v.unsigned_abs() > 9_007_199_254_740_991 {
                json!(v.to_string())
            } else {
                json!(v)
            }
        }
        ("f32", 4) => {
            let v = f32::from_le_bytes(data.try_into().unwrap());
            if v.is_finite() {
                json!(v)
            } else {
                json!(format!("{v}"))
            }
        }
        ("CString", _) => json!(String::from_utf8_lossy(data)),
        _ => json!(hex(data)),
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn opaque_index_preserves_order_and_rejects_invalid_bounds() {
        let body = [1, 2, 3, 4];
        let mut header = 2u16.to_le_bytes().to_vec();
        for (k, o) in [(9u32, 2u32), (7, 0)] {
            header.extend(k.to_le_bytes());
            header.extend(o.to_le_bytes());
        }
        let t = RawTable::parse(&body, &header, 4, 2).unwrap();
        assert_eq!(t.records()[0].key, 7);
        assert_eq!(t.record_bytes(9).unwrap(), [3, 4]);
        assert_eq!(t.serialize_header().unwrap(), header);
        header[6..10].copy_from_slice(&0u32.to_le_bytes());
        assert!(RawTable::parse(&body, &header, 4, 2).is_err());
    }
    #[test]
    fn invalid_item_is_not_accepted_as_opaque_roundtrip() {
        let mut h = 1u16.to_le_bytes().to_vec();
        h.extend(1u32.to_le_bytes());
        h.extend(0u32.to_le_bytes());
        assert!(ItemTable::parse(&[1, 0, 0, 0], &h).is_err());
    }
    #[test]
    fn integer_export_keeps_precision_and_float_bits() {
        assert_eq!(
            field_value("u64", &u64::MAX.to_le_bytes()),
            json!(u64::MAX.to_string())
        );
        assert_eq!(hex(&f32::NAN.to_le_bytes()), "0000c07f");
    }
}
