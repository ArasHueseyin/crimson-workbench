//! Give Rokade a dedicated appearance instead of enlarging a shared horse model.
//! Recipes contain only identifiers and fingerprints; assets come from the game.
use super::super::{Change, bad};
use crate::{
    Result,
    fingerprint::hash_bytes,
    tables::{IndexedTable, TableSchema},
};
use crimson_format::{
    Archive,
    overlay::{self, Overlay},
};
use std::collections::BTreeMap;

const CHARACTER: u32 = 31378;
const APPEARANCE_AT: usize = 251;
const SOURCE_STRING: u32 = 3219883613;
// Stable private StringInfo identifier, absent from the pinned source table.
// Native lookup uses the serialized key, not a hash recomputed from the path.
const TARGET_STRING: u32 = 2357531225;
const ROW_HASH: &str = "2377d4ad08cf994dbc16f3dfd871eb2fde07270865cc7e6fee7300261817b5ae";
const SOURCE_FILE: &str = "cd_r0002_00_horse_01007.app_xml";
const APPEARANCE_HASH: &str = "cf8e9e98ad1ddf0811d2877eef122793e0ad6eb98ae8f1953b75b6cfd109a775";
const STRING_BODY_HASH: &str = "8f5da98d317963226d56f9b07622517d823482c2bf9da71b1e3c2204ad7420e4";
const STRING_HEADER_HASH: &str = "f5e9f43057c6b6f22fa7cf7975b522e15a06d3c103faa33d3b0998c5c13fdabc";
const STRING_SCHEMA: TableSchema = TableSchema {
    name: "stringinfo",
    count_bytes: 2,
    key_bytes: 4,
    interpretation: "typed string records",
};

fn append_string(body: &[u8], header: &[u8]) -> Result<(Vec<u8>, Vec<u8>)> {
    if body.len() != 1_893_299
        || header.len() != 254_498
        || hash_bytes(body) != STRING_BODY_HASH
        || hash_bytes(header) != STRING_HEADER_HASH
    {
        return Err(bad("Rokade string mapping changed; nothing written"));
    }
    let table = IndexedTable::parse(STRING_SCHEMA, body, header)?;
    let source = table
        .record_bytes(SOURCE_STRING)
        .ok_or_else(|| bad("Rokade appearance mapping absent"))?;
    let source_path = format!("{}/{SOURCE_FILE}", overlay::ROKADE_DIRECTORY);
    if source.get(13..) != Some(source_path.as_bytes())
        || source.get(4..9) != Some(&[0; 5])
        || table.record_bytes(TARGET_STRING).is_some()
        || table.serialize_body() != body
        || table.serialize_header()? != header
    {
        return Err(bad("Unexpected Rokade appearance mapping"));
    }
    let path = format!("{}/{}", overlay::ROKADE_DIRECTORY, overlay::ROKADE_FILE);
    let count = u16::try_from(table.records().len() + 1)
        .map_err(|_| bad("String mapping capacity exceeded"))?;
    let offset = u32::try_from(body.len()).map_err(|_| bad("String mapping too large"))?;
    let mut output = body.to_vec();
    output.extend(TARGET_STRING.to_le_bytes());
    output.extend([0; 5]);
    output.extend((path.len() as u32).to_le_bytes());
    output.extend(path.as_bytes());
    let mut index = header.to_vec();
    index[..2].copy_from_slice(&count.to_le_bytes());
    index.extend(TARGET_STRING.to_le_bytes());
    index.extend(offset.to_le_bytes());
    let rebuilt = IndexedTable::parse(STRING_SCHEMA, &output, &index)?;
    if rebuilt.records().len() != table.records().len() + 1
        || rebuilt.serialize_header()? != index
        || rebuilt.serialize_body() != output
        || rebuilt
            .record_bytes(TARGET_STRING)
            .and_then(|r| r.get(13..))
            != Some(path.as_bytes())
    {
        return Err(bad("Rokade string mapping roundtrip failed"));
    }
    Ok((output, index))
}

fn scale(original: &[u8]) -> Result<Vec<u8>> {
    if original.len() != 784 || hash_bytes(original) != APPEARANCE_HASH {
        return Err(bad("Rokade appearance changed; nothing written"));
    }
    let text = std::str::from_utf8(original).map_err(|_| bad("Rokade appearance is not UTF-8"))?;
    if text.matches("CharacterScale=").count() != 1
        || text.matches("CharacterScale=\"1.13\"").count() != 1
    {
        return Err(bad("Unexpected Rokade character scale"));
    }
    Ok(text
        .replacen("CharacterScale=\"1.13\"", "CharacterScale=\"1.469\"", 1)
        .into_bytes())
}

pub(crate) fn rokade(
    archive: &Archive,
    blobs: &BTreeMap<String, Vec<u8>>,
    replacements: &mut BTreeMap<&'static str, BTreeMap<u32, Vec<u8>>>,
    changes: &mut Vec<Change>,
) -> Result<(BTreeMap<String, Vec<u8>>, Overlay)> {
    let table = super::super::table(blobs, "characterinfo")?;
    let original = table
        .record_bytes(CHARACTER)
        .ok_or_else(|| bad("Rokade character absent"))?;
    if hash_bytes(original) != ROW_HASH
        || original.get(APPEARANCE_AT..APPEARANCE_AT + 4)
            != Some(SOURCE_STRING.to_le_bytes().as_slice())
    {
        return Err(bad("Rokade character schema changed; nothing written"));
    }
    let entries = archive.list_group("0008")?;
    let read = |name: &str| -> Result<Vec<u8>> {
        let entry = entries
            .iter()
            .find(|e| e.name == name && e.directory == "gamedata/binarystaticinfo__/bin")
            .ok_or_else(|| bad("Original Rokade string table absent"))?;
        Ok(archive.extract(entry)?)
    };
    let (body, header) = append_string(
        &read("stringinfo.staticinfobody")?,
        &read("stringinfo.staticinfoheader")?,
    )?;
    let appearance = archive
        .list_group("0009")?
        .into_iter()
        .find(|e| e.directory == overlay::ROKADE_DIRECTORY && e.name == SOURCE_FILE)
        .ok_or_else(|| bad("Original Rokade appearance absent"))?;
    let bytes = scale(&archive.extract(&appearance)?)?;
    let source = archive.serialize_group("0009")?;
    let pack = overlay::build_rokade(bytes, source[8..12].try_into().unwrap())?;

    // Merge with the reviewed speed/stamina edits to this same character row.
    let row = replacements
        .entry("characterinfo")
        .or_default()
        .entry(CHARACTER)
        .or_insert_with(|| original.to_vec());
    if row.get(APPEARANCE_AT..APPEARANCE_AT + 4) != Some(SOURCE_STRING.to_le_bytes().as_slice()) {
        return Err(bad("Another setting conflicts with Rokade's appearance"));
    }
    row[APPEARANCE_AT..APPEARANCE_AT + 4].copy_from_slice(&TARGET_STRING.to_le_bytes());
    changes.push(Change {
        module: "Marketplace",
        table: "Rokade appearance",
        key: CHARACTER,
        name: "Rokade".into(),
        field: "Character scale (+30%, dedicated appearance)".into(),
        before: "1.13".into(),
        after: "1.469".into(),
    });
    Ok((
        BTreeMap::from([
            ("stringinfo.staticinfobody".into(), body),
            ("stringinfo.staticinfoheader".into(), header),
        ]),
        pack,
    ))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn changed_or_truncated_appearance_and_mapping_are_rejected() {
        assert!(scale(&[]).is_err());
        assert!(scale(&vec![0; 784]).is_err());
        assert!(append_string(&[], &[]).is_err());
        assert!(append_string(&vec![0; 1_893_299], &vec![0; 254_498]).is_err());
    }
    #[test]
    #[ignore = "requires private original fixtures; read only"]
    fn dedicated_appearance_preserves_the_original_mapping_and_surrounding_xml() {
        let root = std::path::PathBuf::from(
            std::env::var_os("CD_ROKADE_FIXTURE").expect("private fixtures required"),
        );
        let source = std::fs::read(root.join(SOURCE_FILE)).unwrap();
        let result = scale(&source).unwrap();
        let inverse = String::from_utf8(result.clone()).unwrap().replacen(
            "CharacterScale=\"1.469\"",
            "CharacterScale=\"1.13\"",
            1,
        );
        assert_eq!(inverse.as_bytes(), source);
        assert!(scale(&result).is_err());
        let body = std::fs::read(root.join("stringinfo.staticinfobody")).unwrap();
        let header = std::fs::read(root.join("stringinfo.staticinfoheader")).unwrap();
        let (after, index) = append_string(&body, &header).unwrap();
        assert_eq!(
            &after[..body.len()],
            body,
            "every original string stays byte-identical"
        );
        assert_eq!(&index[2..header.len()], &header[2..]);
        let table = IndexedTable::parse(STRING_SCHEMA, &body, &header).unwrap();
        let rebuilt = IndexedTable::parse(STRING_SCHEMA, &after, &index).unwrap();
        for (row, new_row) in table.records().iter().zip(rebuilt.records()) {
            assert_eq!(
                (new_row.key, new_row.offset, new_row.length),
                (row.key, row.offset, row.length)
            );
        }
        assert!(
            append_string(&after, &index).is_err(),
            "do not adopt an already modified table"
        );
        let mut unknown = source;
        unknown[0] ^= 1;
        assert!(scale(&unknown).is_err());
    }
}
