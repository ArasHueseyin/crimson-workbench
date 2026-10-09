//! Versioned record-index readers. Raw record bodies have NO inferred semantics.
use serde::Serialize;
use std::{collections::BTreeSet, io};

#[derive(Debug, Clone, Copy, Serialize)]
pub struct TableSchema {
    pub name: &'static str,
    pub count_bytes: usize,
    pub key_bytes: usize,
    pub interpretation: &'static str,
}

/// Measured against the exact header hashes in steam-25381195.observed.json.
pub const TABLE_SCHEMAS: &[TableSchema] = &[
    TableSchema {
        name: "characterinfo",
        count_bytes: 2,
        key_bytes: 4,
        interpretation: "raw-records",
    },
    TableSchema {
        name: "crafttoolgroupinfo",
        count_bytes: 2,
        key_bytes: 2,
        interpretation: "raw-records",
    },
    TableSchema {
        name: "crafttoolinfo",
        count_bytes: 2,
        key_bytes: 2,
        interpretation: "raw-records",
    },
    TableSchema {
        name: "dropsetinfo",
        count_bytes: 2,
        key_bytes: 4,
        interpretation: "raw-records",
    },
    TableSchema {
        name: "fieldinfo",
        count_bytes: 2,
        key_bytes: 4,
        interpretation: "raw-records",
    },
    TableSchema {
        name: "inventory",
        count_bytes: 2,
        key_bytes: 2,
        interpretation: "raw-records",
    },
    TableSchema {
        name: "iteminfo",
        count_bytes: 2,
        key_bytes: 4,
        interpretation: "typed-and-raw-fields",
    },
    TableSchema {
        name: "multichangeinfo",
        count_bytes: 2,
        key_bytes: 4,
        interpretation: "raw-records",
    },
    TableSchema {
        name: "questinfo",
        count_bytes: 4,
        key_bytes: 4,
        interpretation: "raw-records",
    },
    TableSchema {
        name: "regioninfo",
        count_bytes: 2,
        key_bytes: 2,
        interpretation: "raw-records",
    },
    TableSchema {
        name: "skill",
        count_bytes: 2,
        key_bytes: 4,
        interpretation: "raw-records",
    },
    TableSchema {
        name: "stageinfo",
        count_bytes: 4,
        key_bytes: 4,
        interpretation: "raw-records",
    },
    TableSchema {
        name: "storeinfo",
        count_bytes: 2,
        key_bytes: 2,
        interpretation: "raw-records",
    },
    TableSchema {
        name: "vehicleinfo",
        count_bytes: 2,
        key_bytes: 2,
        interpretation: "raw-records",
    },
];

pub fn schema(name: &str) -> Option<TableSchema> {
    TABLE_SCHEMAS.iter().find(|s| s.name == name).copied()
}

#[derive(Debug, Serialize)]
pub struct RecordInfo {
    pub key: u32,
    pub offset: u32,
    pub length: usize,
    pub interpretation: &'static str,
}

pub struct IndexedTable<'a> {
    schema: TableSchema,
    body: &'a [u8],
    records: Vec<RecordInfo>,
}

fn invalid(message: &str) -> io::Error {
    io::Error::new(io::ErrorKind::InvalidData, message)
}

fn number(data: &[u8], width: usize) -> io::Result<u32> {
    match width {
        2 if data.len() >= 2 => Ok(u16::from_le_bytes(data[..2].try_into().unwrap()).into()),
        4 if data.len() >= 4 => Ok(u32::from_le_bytes(data[..4].try_into().unwrap())),
        _ => Err(invalid("unsupported or truncated index integer")),
    }
}

impl<'a> IndexedTable<'a> {
    pub fn parse(schema: TableSchema, body: &'a [u8], header: &[u8]) -> io::Result<Self> {
        if ![2, 4].contains(&schema.count_bytes) || ![2, 4].contains(&schema.key_bytes) {
            return Err(invalid("unsupported index schema"));
        }
        let count = number(header, schema.count_bytes)? as usize;
        let expected = count
            .checked_mul(schema.key_bytes + 4)
            .and_then(|v| v.checked_add(schema.count_bytes))
            .ok_or_else(|| invalid("index size overflow"))?;
        if expected != header.len() {
            return Err(invalid(
                "header length does not match versioned count/key widths",
            ));
        }
        let mut records = Vec::with_capacity(count);
        let mut keys = BTreeSet::new();
        let mut offsets = BTreeSet::new();
        for i in 0..count {
            let at = schema.count_bytes + i * (schema.key_bytes + 4);
            let key = number(&header[at..], schema.key_bytes)?;
            let offset = number(&header[at + schema.key_bytes..], 4)?;
            if offset as usize >= body.len() {
                return Err(invalid("record offset outside body"));
            }
            if !keys.insert(key) {
                return Err(invalid("duplicate record key"));
            }
            if !offsets.insert(offset) {
                return Err(invalid("duplicate record offset"));
            }
            records.push(RecordInfo {
                key,
                offset,
                length: 0,
                interpretation: "raw bytes; field semantics unconfirmed",
            });
        }
        for r in &mut records {
            let end = offsets
                .range((
                    std::ops::Bound::Excluded(r.offset),
                    std::ops::Bound::Unbounded,
                ))
                .next()
                .map(|v| *v as usize)
                .unwrap_or(body.len());
            r.length = end - r.offset as usize;
        }
        Ok(Self {
            schema,
            body,
            records,
        })
    }
    pub fn records(&self) -> &[RecordInfo] {
        &self.records
    }
    pub fn record_bytes(&self, key: u32) -> Option<&'a [u8]> {
        self.records
            .iter()
            .find(|r| r.key == key)
            .map(|r| &self.body[r.offset as usize..r.offset as usize + r.length])
    }
    pub fn serialize_header(&self) -> io::Result<Vec<u8>> {
        let mut out = Vec::new();
        let n = u32::try_from(self.records.len()).map_err(|_| invalid("too many index records"))?;
        write_number(&mut out, n, self.schema.count_bytes)?;
        for record in &self.records {
            write_number(&mut out, record.key, self.schema.key_bytes)?;
            out.extend_from_slice(&record.offset.to_le_bytes());
        }
        Ok(out)
    }
    pub fn serialize_body(&self) -> Vec<u8> {
        let mut records: Vec<_> = self.records.iter().collect();
        records.sort_by_key(|r| r.offset);
        let prefix = records
            .first()
            .map(|r| r.offset as usize)
            .unwrap_or(self.body.len());
        let mut out = self.body[..prefix].to_vec();
        for r in records {
            out.extend_from_slice(&self.body[r.offset as usize..r.offset as usize + r.length]);
        }
        out
    }

    /// Rebuild from the original snapshot, retaining physical order, prefix and index key order.
    pub fn rebuild(
        &self,
        replacements: &std::collections::BTreeMap<u32, Vec<u8>>,
    ) -> io::Result<(Vec<u8>, Vec<u8>)> {
        if replacements
            .keys()
            .any(|k| !self.records.iter().any(|r| r.key == *k))
        {
            return Err(invalid("replacement key is absent from original table"));
        }
        let mut physical: Vec<_> = self.records.iter().collect();
        physical.sort_by_key(|r| r.offset);
        let prefix = physical
            .first()
            .map_or(self.body.len(), |r| r.offset as usize);
        let mut body = self.body[..prefix].to_vec();
        let mut offsets = std::collections::BTreeMap::new();
        for row in physical {
            offsets.insert(
                row.key,
                u32::try_from(body.len()).map_err(|_| invalid("body too large"))?,
            );
            let bytes = replacements
                .get(&row.key)
                .map(Vec::as_slice)
                .unwrap_or_else(|| self.record_bytes(row.key).unwrap());
            if number(bytes, self.schema.key_bytes)? != row.key {
                return Err(invalid("replacement key mismatch"));
            }
            body.extend(bytes);
            if body.len() > crimson_format::MAX_FILE_BYTES {
                return Err(invalid("body exceeds size limit"));
            }
        }
        let mut header = Vec::new();
        write_number(
            &mut header,
            self.records.len() as u32,
            self.schema.count_bytes,
        )?;
        for row in &self.records {
            write_number(&mut header, row.key, self.schema.key_bytes)?;
            header.extend(offsets[&row.key].to_le_bytes());
        }
        IndexedTable::parse(self.schema, &body, &header)?;
        Ok((body, header))
    }
}

fn write_number(out: &mut Vec<u8>, value: u32, width: usize) -> io::Result<()> {
    match width {
        2 => out.extend_from_slice(
            &u16::try_from(value)
                .map_err(|_| invalid("key/count exceeds u16"))?
                .to_le_bytes(),
        ),
        4 => out.extend_from_slice(&value.to_le_bytes()),
        _ => return Err(invalid("unknown integer width")),
    }
    Ok(())
}

pub fn hex(bytes: &[u8]) -> String {
    const DIGITS: &[u8; 16] = b"0123456789abcdef";
    let mut out = String::with_capacity(bytes.len() * 2);
    for b in bytes {
        out.push(DIGITS[(b >> 4) as usize] as char);
        out.push(DIGITS[(b & 15) as usize] as char);
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn rebuilds_index_and_raw_body_with_nonphysical_key_order() {
        let s = schema("inventory").unwrap();
        let header = [2, 0, 7, 0, 5, 0, 0, 0, 9, 0, 2, 0, 0, 0];
        let body = [0xaa, 0xbb, 9, 0, 0xff, 7, 0, 0x80, 0x81];
        let table = IndexedTable::parse(s, &body, &header).unwrap();
        assert_eq!(table.record_bytes(9).unwrap(), &[9, 0, 0xff]);
        assert_eq!(table.serialize_header().unwrap(), header);
        assert_eq!(table.serialize_body(), body);
        let (grown, new_header) = table
            .rebuild(&std::collections::BTreeMap::from([(
                9,
                vec![9, 0, 1, 2, 3, 4],
            )]))
            .unwrap();
        assert_eq!(&grown[..2], &[0xaa, 0xbb]);
        assert_eq!(&new_header[4..8], &8u32.to_le_bytes());
        let rebuilt = IndexedTable::parse(s, &grown, &new_header).unwrap();
        assert_eq!(rebuilt.records()[0].key, 7);
        assert_eq!(rebuilt.record_bytes(7), table.record_bytes(7));
        assert!(
            table
                .rebuild(&std::collections::BTreeMap::from([(9, vec![7, 0])]))
                .is_err()
        );
    }
    #[test]
    fn bad_indices_are_not_resynchronized() {
        let s = schema("inventory").unwrap();
        assert!(IndexedTable::parse(s, &[0; 5], &[1, 0]).is_err());
        assert!(IndexedTable::parse(s, &[0; 5], &[1, 0, 7, 0, 9, 0, 0, 0]).is_err());
        assert!(
            IndexedTable::parse(s, &[0; 5], &[2, 0, 7, 0, 0, 0, 0, 0, 7, 0, 2, 0, 0, 0]).is_err()
        );
        assert!(
            IndexedTable::parse(s, &[0; 5], &[2, 0, 7, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0]).is_err()
        );
    }
    #[test]
    fn quest_stage_count_is_u32_not_u16() {
        assert_eq!(schema("questinfo").unwrap().count_bytes, 4);
        assert_eq!(schema("stageinfo").unwrap().count_bytes, 4);
        let t = IndexedTable::parse(
            schema("questinfo").unwrap(),
            &[1],
            &[1, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0],
        )
        .unwrap();
        assert_eq!(t.records()[0].key, 7);
    }
}
