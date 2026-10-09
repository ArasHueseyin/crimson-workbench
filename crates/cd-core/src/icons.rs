//! Icon mapping based on the pinned MIT crimson-rs stringinfo format research.
//! Original parser with a complete typed roundtrip; no game asset is bundled.
use crate::{
    Error, Result,
    fingerprint::{TableHash, hash_bytes},
    tables::{IndexedTable, TableSchema},
};
use base64::Engine;
use crimson_format::{Archive, ArchiveEntry};
use serde::Serialize;
use std::{collections::BTreeMap, io::Cursor, path::Path, sync::Mutex};

const STRING_SCHEMA: TableSchema = TableSchema {
    name: "stringinfo",
    count_bytes: 2,
    key_bytes: 4,
    interpretation: "typed string records",
};
#[derive(Clone, Debug, Serialize)]
pub struct IconResult {
    pub data_url: Option<String>,
    pub source: Option<String>,
    pub reason: Option<String>,
    /// Present only for an illustrative portrait of a different variant.
    pub caption: Option<String>,
}
impl IconResult {
    pub fn unavailable(reason: impl Into<String>) -> Self {
        Self {
            data_url: None,
            source: None,
            reason: Some(reason.into()),
            caption: None,
        }
    }
}
pub(crate) struct IconCatalog {
    archive: Archive,
    strings: BTreeMap<u32, String>,
    entries: BTreeMap<String, ArchiveEntry>,
    cache: Mutex<BTreeMap<u32, IconResult>>,
    portraits: BTreeMap<String, (u8, Option<ArchiveEntry>)>,
    portrait_cache: Mutex<BTreeMap<String, IconResult>>,
}
impl IconCatalog {
    // Call only after GameData has verified the installation's metadata.
    pub fn open(game: &Path) -> Result<Self> {
        let archive = Archive::open(game)?;
        let group = archive.list_group("0008")?;
        let references: Vec<TableHash> =
            serde_json::from_str(include_str!("../schemas/steam-25381195.icons.json"))?;
        let mut blobs = vec![];
        for r in &references {
            let entry = group
                .iter()
                .find(|e| e.name == r.name && e.directory == r.directory)
                .ok_or_else(|| Error::UnsupportedBuild(format!("Missing {}", r.name)))?;
            let bytes = archive.extract(entry)?;
            if bytes.len() as u64 != r.size || hash_bytes(&bytes) != r.sha256 {
                return Err(Error::UnsupportedBuild(format!(
                    "Icon mapping hash differs: {}",
                    r.name
                )));
            }
            blobs.push(bytes);
        }
        let strings = parse_strings(&blobs[0], &blobs[1])?;
        let mut entries = BTreeMap::new();
        let mut portraits = BTreeMap::new();
        for entry in archive.list_group("0012")? {
            if let Some(key) = portrait_key(&entry.directory, &entry.name) {
                let priority = portrait_priority(&entry.name);
                portraits
                    .entry(key)
                    .and_modify(|(rank, found)| {
                        if priority < *rank {
                            *rank = priority;
                            *found = Some(entry.clone());
                        } else if priority == *rank {
                            *found = None;
                        }
                    })
                    .or_insert_with(|| (priority, Some(entry.clone())));
            }
            if entry.directory == "ui/texture/icon"
                && entry.name.ends_with(".dds")
                && entries.insert(entry.name.to_lowercase(), entry).is_some()
            {
                return Err(Error::Invalid("Ambiguous icon filename".into()));
            }
        }
        Ok(Self {
            archive,
            strings,
            entries,
            cache: Mutex::new(BTreeMap::new()),
            portraits,
            portrait_cache: Mutex::new(BTreeMap::new()),
        })
    }
    pub fn icon(&self, hash: u32) -> IconResult {
        if let Ok(cache) = self.cache.lock()
            && let Some(icon) = cache.get(&hash)
        {
            return icon.clone();
        }
        let icon = self
            .read_icon(hash)
            .unwrap_or_else(|e| IconResult::unavailable(e.to_string()));
        if let Ok(mut cache) = self.cache.lock() {
            if cache.len() >= 128 {
                cache.clear();
            }
            cache.insert(hash, icon.clone());
        }
        icon
    }
    fn read_icon(&self, hash: u32) -> Result<IconResult> {
        let Some(name) = self.strings.get(&hash) else {
            return Ok(IconResult::unavailable("Iconreferenz nicht aufgelöst"));
        };
        let normalized = name
            .trim_end_matches('\0')
            .replace('\\', "/")
            .to_lowercase();
        let filename = normalized
            .strip_prefix("ui/texture/icon/")
            .unwrap_or(&normalized);
        // Current StringInfo stores icon stems, while PAMT names carry .dds.
        let filename = if filename.ends_with(".dds") {
            filename.to_owned()
        } else {
            format!("{filename}.dds")
        };
        let Some(entry) = self.entries.get(&filename) else {
            return Ok(IconResult::unavailable(
                "Keine passende Icontextur im geprüften Archiv",
            ));
        };
        self.read_texture(entry)
    }
    pub fn mount_icon(&self, internal: &str) -> IconResult {
        let key = internal.to_ascii_lowercase();
        let exact = self.portraits.get(&key);
        let illustrative = if exact.is_none() {
            portrait_example(&key)
        } else {
            None
        };
        let entry = exact.or_else(|| illustrative.and_then(|k| self.portraits.get(k)));
        let mut icon = match entry.and_then(|(_, entry)| entry.as_ref()) {
            Some(entry) => self.read_portrait(entry),
            None => {
                IconResult::unavailable("Kein eindeutig zugeordnetes Tierporträt im Spielarchiv.")
            }
        };
        if icon.data_url.is_some() && illustrative.is_some() {
            icon.caption = Some("Beispielbild derselben Tierart aus dem Spielarchiv. Fell, Farbe und Ausstattung dieser Variante können abweichen.".into());
        }
        icon
    }
    fn read_portrait(&self, entry: &ArchiveEntry) -> IconResult {
        // Example variants share a texture. Decode it once per session; keep
        // captions outside this cache so they cannot leak to an exact match.
        if let Ok(cache) = self.portrait_cache.lock()
            && let Some(icon) = cache.get(&entry.path)
        {
            return icon.clone();
        }
        let icon = self
            .read_texture(entry)
            .unwrap_or_else(|e| IconResult::unavailable(e.to_string()));
        if let Ok(mut cache) = self.portrait_cache.lock() {
            if cache.len() >= 128 {
                cache.clear();
            }
            cache.insert(entry.path.clone(), icon.clone());
        }
        icon
    }
    fn read_texture(&self, entry: &ArchiveEntry) -> Result<IconResult> {
        if entry.uncompressed_size > 2 * 1024 * 1024 {
            return Err(Error::Invalid("Icon exceeds 2 MiB".into()));
        }
        let bytes = self.archive.extract(entry)?;
        let png = decode_icon(&bytes)?;
        Ok(IconResult {
            data_url: Some(format!(
                "data:image/png;base64,{}",
                base64::engine::general_purpose::STANDARD.encode(png)
            )),
            source: Some(entry.path.clone()),
            reason: None,
            caption: None,
        })
    }
}
// Both UI naming schemes use the full character stem. Prefer the dedicated
// mercenary portrait over the general character portrait; same-rank duplicates
// remain ambiguous. Model stems are indexed only for explicit illustrative use.
fn portrait_key(directory: &str, name: &str) -> Option<String> {
    if ![
        "ui/texture/image/horseimage",
        "ui/texture/image/portraitimage",
        "ui/texture/image/petthumbnail",
    ]
    .contains(&directory)
    {
        return None;
    }
    let name = name.to_ascii_lowercase();
    let stem = name
        .strip_prefix("cd_mercenary_portrait_")
        .or_else(|| name.strip_prefix("cd_portraitimage_"))?
        .strip_suffix(".dds")?;
    let stem = stem
        .strip_prefix("horseimage_")
        .or_else(|| stem.strip_prefix("donkeyimage_"))
        .or_else(|| stem.strip_prefix("petimage_"))
        .unwrap_or(stem);
    let stem = stem.strip_prefix("domestic_").unwrap_or(stem);
    let stem = stem
        .strip_prefix("animal_riding_")
        .map(|suffix| format!("riding_{suffix}"))
        .unwrap_or_else(|| stem.to_owned());
    (stem.starts_with("animal_")
        || stem.starts_with("riding_")
        || stem.starts_with("cd_r0002_00_horse_"))
    .then_some(stem)
}
fn portrait_priority(name: &str) -> u8 {
    let name = name.to_ascii_lowercase();
    if name.starts_with("cd_portraitimage_") {
        2
    } else if name.starts_with("cd_mercenary_portrait_domestic_animal_") {
        1
    } else {
        0
    }
}
// A small explicit species map, NOT a vehicle-family lookup: the latter would
// show e.g. a cow for a rhino. These are always labelled as example variants.
fn portrait_example(internal: &str) -> Option<&'static str> {
    if internal.starts_with("riding_horse_")
        || (internal.starts_with("animal_") && internal.split('_').any(|s| s == "horse"))
    {
        return Some("riding_horse_dabrera_2002");
    }
    if internal.starts_with("animal_donkey_") || internal.starts_with("riding_donkey_") {
        return Some("cd_r0002_00_horse_06001");
    }
    [
        ("animal_davrella_", "riding_horse_dabrera_2002"),
        ("animal_lumif_", "riding_horse_lumiph_2000"),
        ("animal_stefero_", "riding_horse_strapero_2001"),
        ("animal_stefano_", "riding_horse_strapero_2001"),
        ("animal_ayut_", "riding_horse_ayut_2008"),
        ("animal_pukret_", "riding_horse_pukre_2005"),
        ("animal_phuket_", "riding_horse_pukre_2005"),
        ("riding_camel_", "riding_camel_1"),
        ("animal_camel_", "riding_camel_1"),
        ("riding_cucubird_", "riding_cucubird_1"),
        ("riding_deer_", "riding_deer_1"),
        ("riding_alpineibex_", "riding_alpineibex_1"),
        ("riding_warthog_", "riding_warthog_1"),
        ("riding_wolf_", "riding_wolf_1"),
        ("animal_trained_wolf_", "animal_wolf_wild_30019"),
        ("animal_black_wolf_", "animal_wolf_wild_30019"),
    ]
    .into_iter()
    .find(|(prefix, _)| internal.starts_with(prefix))
    .map(|(_, key)| key)
}
fn parse_strings(body: &[u8], header: &[u8]) -> Result<BTreeMap<u32, String>> {
    let table = IndexedTable::parse(STRING_SCHEMA, body, header)?;
    let mut serialized = Vec::with_capacity(body.len());
    let mut output = BTreeMap::new();
    // Header order need not equal physical record order.
    let mut records: Vec<_> = table.records().iter().collect();
    records.sort_by_key(|r| r.offset);
    for record in records {
        let start = record.offset as usize;
        let raw = &body[start..start + record.length];
        if raw.len() < 13 {
            return Err(Error::Invalid("Truncated string record".into()));
        }
        let key = u32::from_le_bytes(raw[0..4].try_into().unwrap());
        let reserved = u32::from_le_bytes(raw[4..8].try_into().unwrap());
        let flag = raw[8];
        let length = u32::from_le_bytes(raw[9..13].try_into().unwrap()) as usize;
        if key != record.key || length != raw.len() - 13 {
            return Err(Error::Invalid("String index/length mismatch".into()));
        }
        let value = &raw[13..];
        serialized.extend_from_slice(&key.to_le_bytes());
        serialized.extend_from_slice(&reserved.to_le_bytes());
        serialized.push(flag);
        serialized.extend_from_slice(&(length as u32).to_le_bytes());
        serialized.extend_from_slice(value);
        // Non-UTF8 remains preserved in the roundtrip but cannot form an asset path.
        if let Ok(text) = std::str::from_utf8(value) {
            output.insert(key, text.into());
        }
    }
    if serialized != body || table.serialize_header()? != header {
        return Err(Error::Invalid("Stringinfo roundtrip failed".into()));
    }
    Ok(output)
}
fn decode_icon(bytes: &[u8]) -> Result<Vec<u8>> {
    if bytes.len() < 128 || bytes.len() > 2 * 1024 * 1024 || &bytes[..4] != b"DDS " {
        return Err(Error::Invalid("Invalid DDS icon".into()));
    }
    let number = |offset| u32::from_le_bytes(bytes[offset..offset + 4].try_into().unwrap());
    if number(4) != 124
        || !(1..=512).contains(&number(12))
        || !(1..=512).contains(&number(16))
        || number(24) > 1
        || number(28) > 10
        || number(112) != 0
    {
        return Err(Error::Invalid("Unsupported DDS dimensions/layers".into()));
    }
    let image = image::load_from_memory_with_format(bytes, image::ImageFormat::Dds)
        .map_err(|e| Error::Invalid(format!("DDS: {e}")))?;
    let mut png = Cursor::new(Vec::new());
    image
        .write_to(&mut png, image::ImageFormat::Png)
        .map_err(|e| Error::Invalid(format!("PNG: {e}")))?;
    Ok(png.into_inner())
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn mount_portrait_requires_an_exact_character_stem_and_known_directory() {
        assert_eq!(
            portrait_key(
                "ui/texture/image/portraitimage",
                "cd_mercenary_portrait_Animal_Lion_Circus_32349.dds"
            )
            .as_deref(),
            Some("animal_lion_circus_32349")
        );
        assert_eq!(
            portrait_key(
                "ui/texture/image/horseimage",
                "cd_mercenary_portrait_horseimage_animal_lumif_wild_32506.dds"
            )
            .as_deref(),
            Some("animal_lumif_wild_32506")
        );
        assert!(portrait_key("ui/texture/icon", "cd_mercenary_portrait_animal_bear.dds").is_none());
        assert!(portrait_key("ui/texture/image/portraitimage", "bear.dds").is_none());
        assert_eq!(
            portrait_key(
                "ui/texture/image/portraitimage",
                "cd_portraitimage_riding_camel_1.dds"
            )
            .as_deref(),
            Some("riding_camel_1")
        );
        assert_eq!(
            portrait_key(
                "ui/texture/image/portraitimage",
                "cd_mercenary_portrait_domestic_animal_riding_cucubird_1.dds"
            )
            .as_deref(),
            Some("riding_cucubird_1")
        );
        assert_eq!(
            portrait_key(
                "ui/texture/image/portraitimage",
                "cd_mercenary_portrait_domestic_animal_milkcow_domestic_30028.dds"
            )
            .as_deref(),
            Some("animal_milkcow_domestic_30028")
        );
        assert_eq!(
            portrait_key(
                "ui/texture/image/horseimage",
                "cd_mercenary_portrait_donkeyimage_cd_r0002_00_horse_06001.dds"
            )
            .as_deref(),
            Some("cd_r0002_00_horse_06001")
        );
        assert!(
            portrait_priority("cd_mercenary_portrait_riding_camel_1.dds")
                < portrait_priority("cd_portraitimage_riding_camel_1.dds")
        );
    }
    #[test]
    fn example_portraits_are_species_bounded() {
        assert_eq!(
            portrait_example("riding_horse_tiuta_unique_2050_kliff"),
            Some("riding_horse_dabrera_2002")
        );
        assert_eq!(
            portrait_example("animal_black_horse_wild_31378"),
            Some("riding_horse_dabrera_2002")
        );
        assert_eq!(
            portrait_example("animal_donkey_domestic_1"),
            Some("cd_r0002_00_horse_06001")
        );
        assert!(portrait_example("animal_bull_wild_32217").is_none());
        assert!(portrait_example("animal_giant_bull_32895").is_none());
        assert!(portrait_example("animal_baby_camel_wild_32387").is_none());
        assert_eq!(
            portrait_example("animal_davrella_domestic_saddle_31499"),
            Some("riding_horse_dabrera_2002")
        );
        // A deer uses the horse vehicle family in this build; never turn a
        // family match into a horse image.
        assert_eq!(portrait_example("riding_deer_boss"), Some("riding_deer_1"));
    }
    #[test]
    fn typed_string_roundtrip_preserves_reserved_bytes_and_rejects_false_lengths() {
        let mut body = vec![];
        body.extend_from_slice(&7u32.to_le_bytes());
        body.extend_from_slice(&123u32.to_le_bytes());
        body.push(5);
        body.extend_from_slice(&3u32.to_le_bytes());
        body.extend_from_slice(b"abc");
        let mut header = vec![1, 0];
        header.extend_from_slice(&7u32.to_le_bytes());
        header.extend_from_slice(&0u32.to_le_bytes());
        assert_eq!(parse_strings(&body, &header).unwrap()[&7], "abc");
        body[9] = 4;
        assert!(parse_strings(&body, &header).is_err());
    }
    #[test]
    fn oversized_dds_is_rejected_before_pixel_allocation() {
        let mut bytes = vec![0; 128];
        bytes[..4].copy_from_slice(b"DDS ");
        bytes[4..8].copy_from_slice(&124u32.to_le_bytes());
        bytes[12..16].copy_from_slice(&u32::MAX.to_le_bytes());
        assert!(decode_icon(&bytes).is_err());
    }
}
