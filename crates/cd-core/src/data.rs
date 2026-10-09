use crate::{
    IndexedItem,
    fingerprint::{self, Fingerprint, TableHash},
    tables::{self, IndexedTable, TABLE_SCHEMAS},
};
use crimson_format::{Archive, ArchiveEntry, ItemRecord, ItemTable, LocalizableText, Paloc};
use serde::{Deserialize, Serialize};
use serde_json::{Value, json};
use std::{
    collections::BTreeMap,
    io::{self, Read},
    path::Path,
};

#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("{0}")]
    Io(#[from] io::Error),
    #[error("JSON: {0}")]
    Json(#[from] serde_json::Error),
    #[error(transparent)]
    Index(#[from] crate::IndexError),
    #[error("Unknown or modified build; no table was interpreted: {0}")]
    UnsupportedBuild(String),
    #[error("{0}")]
    Invalid(String),
}
pub type Result<T> = std::result::Result<T, Error>;
type ModInputs = (BTreeMap<String, Vec<u8>>, Vec<u8>, [u8; 4]);
#[derive(Serialize)]
pub struct AbyssStoneDetail {
    #[serde(flatten)]
    pub values: crimson_format::item_mods::ItemModInfo,
    pub name: String,
    pub description: String,
}

#[derive(Debug, Clone, Deserialize, Serialize)]
pub struct Language {
    pub language: String,
    pub group: String,
    pub directory: String,
    pub name: String,
    pub size: u64,
    pub sha256: String,
}

pub fn supported_languages() -> Vec<Language> {
    serde_json::from_str(
        include_str!("../schemas/steam-25381195.locales.json").trim_start_matches('\u{feff}'),
    )
    .expect("checked-in locale fingerprints must be valid")
}

fn language_code(value: &str) -> String {
    match value.to_lowercase().as_str() {
        "de" | "de-de" | "deutsch" => "ger".into(),
        "en" | "en-us" | "en-gb" | "english" => "eng".into(),
        other => other.into(),
    }
}

#[derive(Debug, Serialize)]
pub struct TableStatus {
    pub name: String,
    pub interpretation: String,
    pub count_bytes: usize,
    pub key_bytes: usize,
    pub records: usize,
    pub body_bytes: usize,
    pub header_bytes: usize,
}

#[derive(Debug, Serialize)]
pub struct RoundtripCheck {
    pub target: String,
    pub interpretation: String,
    pub bytes: usize,
    pub sha256: String,
    pub byte_identical: bool,
}
#[derive(Debug, Serialize)]
pub struct RoundtripReport {
    pub all_passed: bool,
    pub checks: Vec<RoundtripCheck>,
    pub tracked_items: usize,
}

/// Immutable, fully fingerprint-checked snapshot. No filesystem mutation API.
pub struct GameData {
    archive: Archive,
    fingerprint: Fingerprint,
    blobs: BTreeMap<String, Vec<u8>>,
    items: Vec<ItemRecord>,
    language: String,
    localization: Paloc,
    localized: BTreeMap<String, String>,
}

impl GameData {
    pub fn open(game: &Path, language: &str) -> Result<Self> {
        let original = match crate::apply::original_registry(game) {
            Ok(original) => original,
            Err(ownership_error) => {
                // Manual overlays can be outside the file transaction journal.
                // A checked original registry still permits a BASE catalog read.
                // The write path keeps using original_registry and rejects them.
                let known = fingerprint::known_for_game(game)?;
                let expected = known
                    .files
                    .iter()
                    .find(|f| f.path == "meta/0.papgt")
                    .ok_or_else(|| Error::Invalid("Known registry missing".into()))?;
                let backup = game.join(".workbench/baseline.papgt");
                let bytes = match std::fs::read(backup) {
                    Ok(bytes) => bytes,
                    Err(_) => return Err(ownership_error),
                };
                if bytes.len() as u64 != expected.bytes
                    || fingerprint::hash_bytes(&bytes) != expected.sha256
                {
                    return Err(ownership_error);
                }
                Some(bytes)
            }
        };
        let mut fingerprint = Fingerprint::inspect_registry(game, original.as_deref())?;
        if !fingerprint.metadata_matches {
            return Err(Error::UnsupportedBuild(fingerprint.diagnostics.join("; ")));
        }
        // Bind tables to the version whose full metadata was just verified.
        // Reading the EXE version again could select a different build if a
        // game update lands between fingerprinting and table extraction.
        let expected = fingerprint
            .exe_version
            .as_deref()
            .and_then(fingerprint::known_for_version)
            .ok_or_else(|| Error::UnsupportedBuild("Verified build identity missing".into()))?;
        let archive = match original {
            Some(bytes) => Archive::with_registry(game, bytes)?,
            None => Archive::open(game)?,
        };
        let table_entries = archive.list_group("0008")?;
        let mut blobs = BTreeMap::new();
        let mut table_hashes = Vec::new();
        // Compare ALL input hashes before any semantic table parser is invoked.
        for reference in &expected.tables {
            let entry = find_entry(&table_entries, &reference.directory, &reference.name)?;
            let bytes = archive.extract(entry)?;
            verify_hash(&bytes, reference.size, &reference.sha256, &reference.name)?;
            table_hashes.push(reference.clone());
            blobs.insert(reference.name.clone(), bytes);
        }
        let code = language_code(language);
        let reference = supported_languages().into_iter().find(|v|v.language==code)
            .ok_or_else(|| Error::Invalid(format!("Language {code:?} has no verified item localization for this build; use the languages command")))?;
        let locale_entries = archive.list_group(&reference.group)?;
        let locale_bytes = archive.extract(find_entry(
            &locale_entries,
            &reference.directory,
            &reference.name,
        )?)?;
        verify_hash(
            &locale_bytes,
            reference.size,
            &reference.sha256,
            "selected item.paloc",
        )?;
        table_hashes.push(TableHash {
            group: reference.group,
            directory: reference.directory,
            name: reference.name,
            size: reference.size,
            sha256: reference.sha256,
        });
        fingerprint.complete(table_hashes)?;
        for schema in TABLE_SCHEMAS {
            IndexedTable::parse(
                *schema,
                &blobs[&body_name(schema.name)],
                &blobs[&header_name(schema.name)],
            )?;
        }
        let items = ItemTable::parse(
            &blobs[&body_name("iteminfo")],
            &blobs[&header_name("iteminfo")],
        )?
        .entries()
        .to_vec();
        let localization = Paloc::parse(&locale_bytes)?;
        let mut localized = BTreeMap::new();
        for entry in localization.entries() {
            if let Some(existing) =
                localized.insert(entry.string_key.clone(), entry.string_value.clone())
                && existing != entry.string_value
            {
                return Err(Error::Invalid(format!(
                    "Conflicting PALOC key {}",
                    entry.string_key
                )));
            }
        }
        Ok(Self {
            archive,
            fingerprint,
            blobs,
            items,
            language: code,
            localization,
            localized,
        })
    }
    pub fn fingerprint(&self) -> &Fingerprint {
        &self.fingerprint
    }
    pub fn items(&self) -> &[ItemRecord] {
        &self.items
    }
    pub fn language(&self) -> &str {
        &self.language
    }

    /// Socket-stone base values from the verified immutable ItemInfo snapshot.
    /// This deliberately avoids opening the full mod editor or any save files.
    pub fn abyss_stone_details(&self) -> Result<Vec<AbyssStoneDetail>> {
        let body = &self.blobs[&body_name("iteminfo")];
        self.items
            .iter()
            .filter(|i| i.item_type == 74 && i.category_info == 2501)
            .map(|i| {
                let values =
                    crimson_format::item_mods::item_mod_info(&body[i.offset..i.offset + i.length])?;
                let summary = self.item_summary(i);
                Ok(AbyssStoneDetail {
                    values,
                    name: summary["name"].as_str().unwrap_or(&i.string_key).into(),
                    description: summary["description"].as_str().unwrap_or_default().into(),
                })
            })
            .collect()
    }

    pub(crate) fn mod_inputs(&self) -> Result<ModInputs> {
        let mut files: BTreeMap<String, Vec<u8>> = self
            .blobs
            .iter()
            .filter(|(n, _)| {
                [
                    "storeinfo.",
                    "dropsetinfo.",
                    "iteminfo.",
                    "characterinfo.",
                    "regioninfo.",
                    "inventory.",
                    "skill.",
                    "fieldinfo.",
                    "stageinfo.",
                ]
                .iter()
                .any(|prefix| n.starts_with(prefix))
            })
            .map(|(n, b)| (n.clone(), b.clone()))
            .collect();
        let references: Vec<TableHash> =
            serde_json::from_str(include_str!("../schemas/steam-25381195.advanced.json"))?;
        let entries = self.archive.list_group("0008")?;
        for r in references {
            let bytes = self
                .archive
                .extract(find_entry(&entries, &r.directory, &r.name)?)?;
            verify_hash(&bytes, r.size, &r.sha256, &r.name)?;
            files.insert(r.name, bytes);
        }
        let pamt = self.archive.serialize_group("0008")?;
        Ok((
            files,
            self.archive.serialize_registry()?,
            pamt[8..12].try_into().unwrap(),
        ))
    }
    pub fn game_root(&self) -> &Path {
        self.archive.root()
    }
    pub(crate) fn mount_inputs(&self) -> Result<(Vec<u8>, Vec<u8>, Paloc)> {
        let reference = supported_languages()
            .into_iter()
            .find(|v| v.language == self.language)
            .ok_or_else(|| Error::Invalid("Mount language unavailable".into()))?;
        let entries = self.archive.list_group(&reference.group)?;
        let locale = self.archive.extract(find_entry(
            &entries,
            &reference.directory,
            "character.paloc",
        )?)?;
        Ok((
            self.blobs["characterinfo.staticinfobody"].clone(),
            self.blobs["characterinfo.staticinfoheader"].clone(),
            Paloc::parse(&locale)?,
        ))
    }

    pub(crate) fn crafting_blobs(&self) -> Result<BTreeMap<String, Vec<u8>>> {
        let references: Vec<TableHash> =
            serde_json::from_str(include_str!("../schemas/steam-25381195.crafting.json"))?;
        let entries = self.archive.list_group("0008")?;
        let mut blobs = BTreeMap::new();
        for r in references {
            let bytes = self
                .archive
                .extract(find_entry(&entries, &r.directory, &r.name)?)?;
            verify_hash(&bytes, r.size, &r.sha256, &r.name)?;
            blobs.insert(r.name, bytes);
        }
        for name in ["multichangeinfo", "dropsetinfo", "crafttoolinfo"] {
            for filename in [body_name(name), header_name(name)] {
                blobs.insert(filename.clone(), self.blobs[&filename].clone());
            }
        }
        Ok(blobs)
    }

    pub fn tables(&self) -> Vec<TableStatus> {
        TABLE_SCHEMAS
            .iter()
            .map(|schema| {
                let body = &self.blobs[&body_name(schema.name)];
                let header = &self.blobs[&header_name(schema.name)];
                let parsed =
                    IndexedTable::parse(*schema, body, header).expect("validated immutable table");
                TableStatus {
                    name: schema.name.into(),
                    interpretation: schema.interpretation.into(),
                    count_bytes: schema.count_bytes,
                    key_bytes: schema.key_bytes,
                    records: parsed.records().len(),
                    body_bytes: body.len(),
                    header_bytes: header.len(),
                }
            })
            .collect()
    }
    fn resolve(&self, text: &LocalizableText) -> Option<String> {
        if text.index != 0
            && let Some(value) = self
                .localized
                .get(&text.index.to_string())
                .filter(|v| !v.is_empty())
        {
            return Some(value.clone());
        }
        if !text.default.is_empty() && text.default != text.index.to_string() {
            Some(text.default.clone())
        } else {
            None
        }
    }
    /// Read-only display groups, independently bound to the observed 2976 table.
    pub fn item_groups(&self) -> Result<Vec<crate::item_groups::ItemGroup>> {
        let entries = self.archive.list_group("0008")?;
        let mut blobs = Vec::new();
        for (name, size, hash) in [
            (
                "itemgroupinfo.staticinfobody",
                374216,
                "719b9f91c0bffa1b676563739e0378a72b659a870e936093accc8a9cb046e45c",
            ),
            (
                "itemgroupinfo.staticinfoheader",
                9614,
                "f1d8ccfed1cb20f7079a3f752206f8585d9e787fad7a53a5f5fbdd482505f4d5",
            ),
        ] {
            let entry = find_entry(&entries, "gamedata/binarystaticinfo__/bin", name)?;
            let bytes = self.archive.extract(entry)?;
            verify_hash(&bytes, size, hash, name)?;
            blobs.push(bytes);
        }
        crate::item_groups::parse(&blobs[0], &blobs[1], |index, default| {
            self.resolve(&LocalizableText {
                category: 0,
                index,
                default: default.into(),
            })
            .unwrap_or_default()
        })
    }
    fn item_summary(&self, item: &ItemRecord) -> Value {
        let name = self.resolve(&item.name);
        let description = [
            self.resolve(&item.description),
            self.resolve(&item.description2),
        ]
        .into_iter()
        .flatten()
        .filter(|s| !s.is_empty())
        .collect::<Vec<_>>()
        .join("\n");
        json!({"record":item,"name":name.as_deref().unwrap_or(&item.string_key),"description":description,
            "language":self.language,"name_resolved":name.is_some(),"category_label":null,
            "interpretation":"Named fields follow the pinned schema; unknown fields remain raw. Category numbers are not invented labels."})
    }
    pub fn localized_items(&self) -> Result<Vec<IndexedItem>> {
        self.items
            .iter()
            .map(|item| {
                let summary = self.item_summary(item);
                Ok(IndexedItem {
                    key: item.key,
                    internal_key: item.string_key.clone(),
                    name: summary["name"].as_str().unwrap_or(&item.string_key).into(),
                    description: summary["description"].as_str().unwrap_or_default().into(),
                    item_type: item.item_type.into(),
                    category: Some(item.category_info.to_string()),
                    max_stack_count: item.max_stack_count,
                    detail_json: serde_json::to_string(&summary)?,
                })
            })
            .collect()
    }
    pub fn item_detail(&self, key: u32, fields: bool) -> Result<Value> {
        let item = self
            .items
            .iter()
            .find(|v| v.key == key)
            .ok_or_else(|| Error::Invalid(format!("Unknown item key {key}")))?;
        let mut summary = self.item_summary(item);
        if fields {
            let table = ItemTable::parse(
                &self.blobs[&body_name("iteminfo")],
                &self.blobs[&header_name("iteminfo")],
            )?;
            summary["fields"] = serde_json::to_value(table.fields(key)?)?;
        }
        Ok(summary)
    }
    /// Other tables expose validated record boundaries and exact raw bytes only.
    pub fn dump_table(&self, name: &str, key: Option<u32>, fields: bool) -> Result<Value> {
        let schema = tables::schema(name)
            .ok_or_else(|| Error::Invalid(format!("No verified schema for table {name:?}")))?;
        if name == "iteminfo" {
            if let Some(key) = key {
                return self.item_detail(key, fields);
            }
            if fields {
                return Err(Error::Invalid(
                    "Full field export requires --item to bound output; use one record at a time"
                        .into(),
                ));
            }
            return Ok(
                json!({"schema":self.fingerprint.schema_id,"table":name,"interpretation":schema.interpretation,
                "items":self.items.iter().map(|i|self.item_summary(i)).collect::<Vec<_>>() }),
            );
        }
        let body = &self.blobs[&body_name(name)];
        let header = &self.blobs[&header_name(name)];
        let table = IndexedTable::parse(schema, body, header)?;
        if let Some(key) = key {
            let record = table
                .records()
                .iter()
                .find(|r| r.key == key)
                .ok_or_else(|| Error::Invalid(format!("No record {key} in {name}")))?;
            return Ok(
                json!({"table":name,"record":record,"type":"raw bytes","raw_hex":tables::hex(table.record_bytes(key).unwrap())}),
            );
        }
        Ok(
            json!({"schema":self.fingerprint.schema_id,"table":name,"interpretation":"raw record index; body field semantics unknown",
            "body_sha256":fingerprint::hash_bytes(body),"header_sha256":fingerprint::hash_bytes(header),"records":table.records()}),
        )
    }
    pub fn roundtrip(&self, all_fields: bool) -> Result<RoundtripReport> {
        let mut checks = Vec::new();
        for schema in TABLE_SCHEMAS {
            let body = &self.blobs[&body_name(schema.name)];
            let header = &self.blobs[&header_name(schema.name)];
            let table = IndexedTable::parse(*schema, body, header)?;
            checks.push(check(
                body_name(schema.name),
                "opaque record boundaries, not semantic fields",
                body,
                &table.serialize_body(),
            ));
            checks.push(check(
                header_name(schema.name),
                "parsed count/key/offset index",
                header,
                &table.serialize_header()?,
            ));
        }
        let body = &self.blobs[&body_name("iteminfo")];
        let header = &self.blobs[&header_name("iteminfo")];
        let items = ItemTable::parse(body, header)?;
        checks.push(check(
            "iteminfo.typed_body".into(),
            "full typed parse/serialize",
            body,
            &items.serialize_body()?,
        ));
        checks.push(check(
            "iteminfo.typed_header".into(),
            "indexed typed record boundaries",
            header,
            &items.serialize_header()?,
        ));
        let mut tracked_items = 0;
        if all_fields {
            for item in items.entries() {
                let fields = items.fields(item.key)?;
                if fields.is_empty() {
                    return Err(Error::Invalid("Empty field map".into()));
                }
                tracked_items += 1;
            }
        }
        checks.push(check(
            "item.paloc.container".into(),
            "immutable original envelope plus checked decoded payload",
            self.localization.original_bytes(),
            &self.localization.serialize()?,
        ));
        checks.push(check(
            "item.paloc.payload".into(),
            "parsed entry list",
            self.localization.decoded_payload(),
            &self.localization.serialize_payload()?,
        ));
        let registry = self.read_verified_metadata("meta/0.papgt")?;
        checks.push(check(
            "meta/0.papgt".into(),
            "parsed registry records and name buffer",
            &registry,
            &self.archive.serialize_registry()?,
        ));
        let pamt = self.read_verified_metadata("0008/0.pamt")?;
        checks.push(check(
            "0008/0.pamt".into(),
            "parsed metadata and preserved trie buffers",
            &pamt,
            &self.archive.serialize_group("0008")?,
        ));
        let all_passed = !checks.is_empty() && checks.iter().all(|c| c.byte_identical);
        Ok(RoundtripReport {
            all_passed,
            checks,
            tracked_items,
        })
    }
    fn read_verified_metadata(&self, relative: &str) -> Result<Vec<u8>> {
        let reference = self
            .fingerprint
            .files
            .iter()
            .find(|v| v.path == relative)
            .ok_or_else(|| Error::Invalid(format!("No verified metadata for {relative}")))?;
        let mut bytes = Vec::new();
        std::fs::File::open(self.archive.root().join(relative))?
            .take(reference.bytes + 1)
            .read_to_end(&mut bytes)?;
        verify_hash(&bytes, reference.bytes, &reference.sha256, relative)?;
        Ok(bytes)
    }
}
fn check(
    target: String,
    interpretation: &str,
    original: &[u8],
    serialized: &[u8],
) -> RoundtripCheck {
    RoundtripCheck {
        target,
        interpretation: interpretation.into(),
        bytes: original.len(),
        sha256: fingerprint::hash_bytes(original),
        byte_identical: original == serialized,
    }
}
fn find_entry<'a>(
    entries: &'a [ArchiveEntry],
    directory: &str,
    name: &str,
) -> Result<&'a ArchiveEntry> {
    entries
        .iter()
        .find(|e| e.directory == directory && e.name == name)
        .ok_or_else(|| Error::UnsupportedBuild(format!("Missing archived file {directory}/{name}")))
}
fn verify_hash(bytes: &[u8], size: u64, sha: &str, label: &str) -> Result<()> {
    if bytes.len() as u64 != size || fingerprint::hash_bytes(bytes) != sha {
        return Err(Error::UnsupportedBuild(format!(
            "Content hash differs for {label}"
        )));
    }
    Ok(())
}
fn body_name(name: &str) -> String {
    format!("{name}.staticinfobody")
}
fn header_name(name: &str) -> String {
    format!("{name}.staticinfoheader")
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn hashes_are_checked_before_semantic_parsing() {
        assert!(matches!(
            verify_hash(b"modified", 3, &fingerprint::hash_bytes(b"old"), "iteminfo"),
            Err(Error::UnsupportedBuild(_))
        ));
    }
    #[test]
    fn language_aliases_are_explicit() {
        assert_eq!(language_code("de-DE"), "ger");
        assert_eq!(language_code("EN"), "eng");
        assert_eq!(language_code("wat"), "wat");
    }
}
