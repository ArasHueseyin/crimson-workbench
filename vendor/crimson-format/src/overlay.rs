//! Deterministic, memory-only table overlays. No filesystem authority.
//! Layout references and MIT crypto provenance: docs/research/APPLY.md.
use crate::{MAX_FILE_BYTES, checksum, invalid};
use std::{collections::BTreeMap, io};

pub struct Overlay {
    pub pamt: Vec<u8>,
    pub paz: Vec<u8>,
    pub checksum: u32,
}

/// Validate an entire table overlay without filesystem access.
pub fn validate_pair(pamt: &[u8], paz: &[u8]) -> io::Result<Vec<String>> {
    crate::archive::validate_overlay_pair(pamt, paz)
}

/// Inspect a checksummed registry, including optional groups absent on disk.
pub fn registry_groups(bytes: &[u8]) -> io::Result<Vec<crate::ArchiveGroup>> {
    crate::archive::registry(bytes)
}

fn number(out: &mut Vec<u8>, n: usize) -> io::Result<()> {
    out.extend(
        u32::try_from(n)
            .map_err(|_| invalid("overlay length overflow"))?
            .to_le_bytes(),
    );
    Ok(())
}
fn blob(out: &mut Vec<u8>, bytes: &[u8]) -> io::Result<()> {
    number(out, bytes.len())?;
    out.extend(bytes);
    Ok(())
}
fn trie(out: &mut Vec<u8>, name: &str) -> io::Result<u32> {
    let mut parent = u32::MAX;
    for part in name.as_bytes().chunks(255) {
        let at = u32::try_from(out.len()).map_err(|_| invalid("trie too large"))?;
        out.extend(parent.to_le_bytes());
        out.push(part.len() as u8);
        out.extend(part);
        parent = at;
    }
    Ok(parent)
}

/// Build one compressed/encrypted group using the source PAMT encryption header.
/// This narrow builder accepts only paired static-info tables in their real directory.
pub fn build(files: &BTreeMap<String, Vec<u8>>, encryption_header: [u8; 4]) -> io::Result<Overlay> {
    build_with_storage(files, encryption_header, false)
}

/// Store table bytes directly using the archive's native uncompressed mode.
pub fn build_uncompressed(
    files: &BTreeMap<String, Vec<u8>>,
    encryption_header: [u8; 4],
) -> io::Result<Overlay> {
    build_with_storage(files, encryption_header, true)
}

pub const LION_DIRECTORY: &str = "character/appearance/2_mon/cd_m0002_00_fourfeet/cd_m0002_00_lion";
pub const LION_FILE: &str = "cd_m0002_00_lion_0001_00000.app_xml";
pub const ROKADE_DIRECTORY: &str = "character/appearance/4_riding/cd_r0002_00_horse";
pub const ROKADE_FILE: &str = "cd_r0002_00_horse_workbench_rokade_130.app_xml";
pub const WOLF_DIRECTORY: &str = "character/appearance/2_mon/cd_m0002_00_fourfeet/cd_m0002_00_wolf";
pub const TIGER_DIRECTORY: &str =
    "character/appearance/2_mon/cd_m0002_00_fourfeet/cd_m0002_00_tiger";
pub const WOLF_FILES: [&str; 3] = [
    "cd_m0002_00_wolf_workbench_great_135.app_xml",
    "cd_m0002_00_wolf_workbench_windpath_135.app_xml",
    "cd_m0002_00_wolf_workbench_silverfang_135.app_xml",
];
pub const TIGER_FILES: [&str; 2] = [
    "cd_m0002_00_tiger_workbench_140.app_xml",
    "cd_m0002_00_tiger_workbench_bone_140.app_xml",
];
pub const TIGER_SKIN_DIRECTORIES: [&str; 4] = [
    "character/bin__/prefab/2_mon/cd_m0002_00_fourfeet/cd_m0002_00_dog/cd_m0002_00_tiger",
    "character/modelproperty/2_mon/cd_m0002_00_fourfeet/cd_m0002_00_dog/cd_m0002_00_tiger",
    "character/texture",
    "character/bin__",
];
const TIGER_SKIN_FILES: [&[&str]; 4] = [
    &[
        "wb_m0002_00_tiger_00_0001.prefab",
        "wb_m0002_00_tiger_00_0001_bone01.prefab",
    ],
    &["cd_m0002_00_tiger_00_0001.pac_xml"],
    &[
        "wb_m0002_00_tiger_nude_0001_00_01_01.dds",
        "wb_m0002_00_tiger_nude_0001_00_01_02.dds",
        "wb_m0002_00_tiger_head_0001_00_01_01.dds",
        "wb_m0002_00_tiger_head_0001_00_01_02.dds",
    ],
    &["partprefabtable.pappt"],
];
pub(crate) fn companion_resource(directory: &str, name: &str) -> bool {
    (directory == WOLF_DIRECTORY && WOLF_FILES.contains(&name))
        || (directory == TIGER_DIRECTORY && TIGER_FILES.contains(&name))
        || TIGER_SKIN_DIRECTORIES
            .iter()
            .position(|d| *d == directory)
            .is_some_and(|part| TIGER_SKIN_FILES[part].contains(&name))
}

/// Dedicated tiger prefabs/textures, material variations and prefab-name mapping.
/// Callers must preserve the original material indices and append new ones;
/// the name mapping must retain all original entries. Original geometry,
/// prefab and texture destinations are never accepted.
pub fn build_tiger_skin(
    files: &BTreeMap<String, Vec<u8>>,
    part: usize,
    encryption_header: [u8; 4],
) -> io::Result<Overlay> {
    let directory = TIGER_SKIN_DIRECTORIES
        .get(part)
        .ok_or_else(|| invalid("unknown tiger skin resource kind"))?;
    build_in_directory(files, encryption_header, true, directory)
}

/// Dedicated appearances for the selected mount definitions. Original shared
/// appearances are never replaced. No arbitrary directory or filename allowed.
pub fn build_companion_appearances(
    files: &BTreeMap<String, Vec<u8>>,
    encryption_header: [u8; 4],
    wolves: bool,
) -> io::Result<Overlay> {
    let directory = if wolves {
        WOLF_DIRECTORY
    } else {
        TIGER_DIRECTORY
    };
    build_in_directory(files, encryption_header, true, directory)
}

pub const REGION_TIMER_DIRECTORY: &str = "sequencer/binary__/stageseq/system";
pub const REGION_TIMER_FILES: [&str; 4] = [
    "cd_seq_quest_getout_region_0100.pastage",
    "cd_seq_quest_getout_region_0200.pastage",
    "cd_seq_quest_getout_region_0300.pastage",
    "cd_seq_quest_getout_region_0400.pastage",
];

/// Only the four reviewed restricted-region state machines, in native raw mode.
pub fn build_region_timers(
    files: &BTreeMap<String, Vec<u8>>,
    encryption_header: [u8; 4],
) -> io::Result<Overlay> {
    if files.len() != REGION_TIMER_FILES.len()
        || REGION_TIMER_FILES
            .iter()
            .any(|name| !files.contains_key(*name))
    {
        return Err(invalid("all four verified region timer files are required"));
    }
    build_in_directory(files, encryption_header, true, REGION_TIMER_DIRECTORY)
}

/// The one verified appearance recipe. No arbitrary archive paths are accepted.
pub fn build_lion(bytes: Vec<u8>, encryption_header: [u8; 4]) -> io::Result<Overlay> {
    build_in_directory(
        &BTreeMap::from([(LION_FILE.into(), bytes)]),
        encryption_header,
        true,
        LION_DIRECTORY,
    )
}

/// Dedicated Rokade appearance; the source shared horse file is never replaced.
pub fn build_rokade(bytes: Vec<u8>, encryption_header: [u8; 4]) -> io::Result<Overlay> {
    build_in_directory(
        &BTreeMap::from([(ROKADE_FILE.into(), bytes)]),
        encryption_header,
        true,
        ROKADE_DIRECTORY,
    )
}

fn build_with_storage(
    files: &BTreeMap<String, Vec<u8>>,
    encryption_header: [u8; 4],
    raw: bool,
) -> io::Result<Overlay> {
    build_in_directory(
        files,
        encryption_header,
        raw,
        "gamedata/binarystaticinfo__/bin",
    )
}

fn build_in_directory(
    files: &BTreeMap<String, Vec<u8>>,
    encryption_header: [u8; 4],
    raw: bool,
    directory: &str,
) -> io::Result<Overlay> {
    if files.is_empty() || files.len() > 64 {
        return Err(invalid("overlay needs 1..64 files"));
    }
    let mut paz = Vec::new();
    let mut names = Vec::new();
    let mut records = Vec::new();
    let mut total = 0usize;
    for (name, bytes) in files {
        let (stem, ext) = name
            .split_once('.')
            .ok_or_else(|| invalid("table filename required"))?;
        if directory == LION_DIRECTORY {
            if name != LION_FILE || files.len() != 1 || !raw {
                return Err(invalid("only the verified lion appearance is allowed"));
            }
        } else if directory == ROKADE_DIRECTORY {
            if name != ROKADE_FILE || files.len() != 1 || !raw {
                return Err(invalid("only the dedicated Rokade appearance is allowed"));
            }
        } else if directory == REGION_TIMER_DIRECTORY {
            if !raw || files.len() != 4 || !REGION_TIMER_FILES.contains(&name.as_str()) {
                return Err(invalid(
                    "only verified restricted-region scripts are allowed",
                ));
            }
        } else if directory == WOLF_DIRECTORY || directory == TIGER_DIRECTORY {
            let allowed: &[&str] = if directory == WOLF_DIRECTORY {
                &WOLF_FILES
            } else {
                &TIGER_FILES
            };
            if !raw || !allowed.contains(&name.as_str()) {
                return Err(invalid(
                    "only dedicated companion mount appearances are allowed",
                ));
            }
        } else if let Some(part) = TIGER_SKIN_DIRECTORIES.iter().position(|d| *d == directory) {
            if !raw || !TIGER_SKIN_FILES[part].contains(&name.as_str()) {
                return Err(invalid("only dedicated tiger skin resources are allowed"));
            }
        } else {
            if stem.is_empty()
                || !stem
                    .bytes()
                    .all(|b| b.is_ascii_lowercase() || b.is_ascii_digit())
                || !matches!(ext, "staticinfobody" | "staticinfoheader")
            {
                return Err(invalid("only table body/header filenames allowed"));
            }
            let partner = if ext == "staticinfobody" {
                "staticinfoheader"
            } else {
                "staticinfobody"
            };
            if !files.contains_key(&format!("{stem}.{partner}")) {
                return Err(invalid("table body/header must be supplied together"));
            }
        }
        total = total
            .checked_add(bytes.len())
            .ok_or_else(|| invalid("overlay size overflow"))?;
        if bytes.is_empty() || total > MAX_FILE_BYTES {
            return Err(invalid("overlay exceeds 128 MiB"));
        }
        let stored = if raw {
            bytes.clone()
        } else {
            let compressed = lz4_flex::block::compress(bytes);
            crate::crypto::chacha20::decrypt_pack_entry(
                &compressed,
                &encryption_header[1..].try_into().unwrap(),
                name,
            )?
        };
        let name_offset = trie(&mut names, name)?;
        records.extend(name_offset.to_le_bytes());
        number(&mut records, paz.len())?;
        number(&mut records, stored.len())?;
        number(&mut records, bytes.len())?;
        records.extend([0, 0, if raw { 0 } else { 0x32 }, 0]);
        paz.extend(stored);
        paz.resize(paz.len().next_multiple_of(16), 0);
    }
    if paz.len() > MAX_FILE_BYTES {
        return Err(invalid("encoded overlay exceeds 128 MiB"));
    }
    let mut dirs = Vec::new();
    let dir_offset = if raw {
        let mut parent = u32::MAX;
        for (index, segment) in directory.split('/').enumerate() {
            let name = if index == 0 {
                segment.to_owned()
            } else {
                format!("/{segment}")
            };
            let at = u32::try_from(dirs.len()).map_err(|_| invalid("trie too large"))?;
            dirs.extend(parent.to_le_bytes());
            dirs.push(name.len() as u8);
            dirs.extend(name.as_bytes());
            parent = at;
        }
        parent
    } else {
        trie(&mut dirs, directory)?
    };
    let mut pamt = vec![0; 4];
    pamt.extend([1, 0, 0, 0]);
    pamt.extend(encryption_header);
    pamt.extend(0u32.to_le_bytes());
    pamt.extend(checksum(&paz).to_le_bytes());
    number(&mut pamt, paz.len())?;
    blob(&mut pamt, &dirs)?;
    blob(&mut pamt, &names)?;
    pamt.extend(1u32.to_le_bytes());
    pamt.extend(checksum(directory.as_bytes()).to_le_bytes());
    pamt.extend(dir_offset.to_le_bytes());
    pamt.extend(0u32.to_le_bytes());
    number(&mut pamt, files.len())?;
    number(&mut pamt, files.len())?;
    pamt.extend(records);
    let checksum = checksum(&pamt[12..]);
    pamt[..4].copy_from_slice(&checksum.to_le_bytes());
    Ok(Overlay {
        pamt,
        paz,
        checksum,
    })
}

/// Prepend a group; retain all original header bits, records and string-table bytes.
pub fn register(
    original: &[u8],
    name: &str,
    language: u16,
    pamt_checksum: u32,
) -> io::Result<Vec<u8>> {
    let groups = crate::archive::registry(original)?;
    if groups.len() >= 255
        || name.len() != 4
        || !name.bytes().all(|b| b.is_ascii_digit())
        || groups.iter().any(|g| g.name == name)
    {
        return Err(invalid("registry capacity or group collision"));
    }
    let end = 12 + groups.len() * 12;
    let old_names = &original[end + 4..];
    let mut out = original[..12].to_vec();
    out[8] += 1;
    out.push(0);
    out.extend(language.to_le_bytes());
    out.push(0);
    number(&mut out, old_names.len())?;
    out.extend(pamt_checksum.to_le_bytes());
    out.extend(&original[12..end]);
    number(&mut out, old_names.len() + name.len() + 1)?;
    out.extend(old_names);
    out.extend(name.as_bytes());
    out.push(0);
    let crc = checksum(&out[12..]);
    out[4..8].copy_from_slice(&crc.to_le_bytes());
    crate::archive::registry(&out)?;
    Ok(out)
}

/// Remove exactly one checked registration while preserving every other group's
/// order, flags, language and checksum, including registrations added later.
pub fn unregister(
    original: &[u8],
    name: &str,
    language: u16,
    pamt_checksum: u32,
) -> io::Result<Vec<u8>> {
    let groups = crate::archive::registry(original)?;
    let index = groups
        .iter()
        .position(|g| g.name == name)
        .ok_or_else(|| invalid("owned registration missing"))?;
    let owned = &groups[index];
    if owned.is_optional || owned.language != language || owned.pamt_checksum != pamt_checksum {
        return Err(invalid("owned registration changed"));
    }
    let end = 12 + groups.len() * 12;
    let names = &original[end + 4..];
    let record = 12 + index * 12;
    let start = u32::from_le_bytes(original[record + 4..record + 8].try_into().unwrap()) as usize;
    let stop = start + name.len() + 1;
    if names.get(start..stop) != Some([name.as_bytes(), &[0]].concat().as_slice()) {
        return Err(invalid("owned name changed"));
    }
    let mut out = original[..12].to_vec();
    out[8] -= 1;
    for (i, entry) in original[12..end].chunks_exact(12).enumerate() {
        if i == index {
            continue;
        }
        let mut entry = entry.to_vec();
        let offset = u32::from_le_bytes(entry[4..8].try_into().unwrap()) as usize;
        if (start..stop).contains(&offset) {
            return Err(invalid("overlapping registry name"));
        }
        if offset >= stop {
            entry[4..8].copy_from_slice(&((offset - (stop - start)) as u32).to_le_bytes());
        }
        out.extend(entry);
    }
    number(&mut out, names.len() - (stop - start))?;
    out.extend(&names[..start]);
    out.extend(&names[stop..]);
    let crc = checksum(&out[12..]);
    out[4..8].copy_from_slice(&crc.to_le_bytes());
    crate::archive::registry(&out)?;
    Ok(out)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn independent_registration_removal_preserves_earlier_and_later_mods() {
        let mut empty = vec![0; 16];
        empty[0] = 7;
        empty[11] = 42;
        empty[4..8].copy_from_slice(&checksum(&[0; 4]).to_le_bytes());
        let before = register(&empty, "0044", 0x7fff, 123).unwrap();
        let installed = register(&before, "9000", 0x1234, 456).unwrap();
        assert_eq!(unregister(&installed, "9000", 0x1234, 456).unwrap(), before);
        let later = register(&installed, "0046", 0x7fff, 789).unwrap();
        let removed = unregister(&later, "9000", 0x1234, 456).unwrap();
        assert_eq!(removed, register(&before, "0046", 0x7fff, 789).unwrap());
        assert!(unregister(&later, "9000", 0x7fff, 456).is_err());
        assert!(unregister(&later, "9000", 0x1234, 999).is_err());
        assert!(unregister(&later, "9999", 0x1234, 456).is_err());
        let mut changed_flags = installed;
        changed_flags[12] = 1;
        let crc = checksum(&changed_flags[12..]);
        changed_flags[4..8].copy_from_slice(&crc.to_le_bytes());
        assert!(unregister(&changed_flags, "9000", 0x1234, 456).is_err());
    }
    #[test]
    fn rokade_overlay_accepts_only_the_dedicated_path() {
        let bytes = b"<Appearance/>".to_vec();
        let pack = build_rokade(bytes.clone(), [0; 4]).unwrap();
        assert_eq!(
            validate_pair(&pack.pamt, &pack.paz).unwrap(),
            [format!("{ROKADE_DIRECTORY}/{ROKADE_FILE}")]
        );
        let shared = BTreeMap::from([("cd_r0002_00_horse_01007.app_xml".into(), bytes)]);
        assert!(build_in_directory(&shared, [0; 4], true, ROKADE_DIRECTORY).is_err());
    }
    #[test]
    fn companion_overlays_are_limited_to_the_curated_resource_paths() {
        for (wolves, names) in [
            (true, WOLF_FILES.as_slice()),
            (false, TIGER_FILES.as_slice()),
        ] {
            let files = names
                .iter()
                .map(|name| ((*name).to_owned(), b"<Appearance/>".to_vec()))
                .collect();
            let pack = build_companion_appearances(&files, [0; 4], wolves).unwrap();
            assert_eq!(
                validate_pair(&pack.pamt, &pack.paz).unwrap().len(),
                names.len()
            );
            let shared = BTreeMap::from([("shared.app_xml".into(), vec![1])]);
            assert!(build_companion_appearances(&shared, [0; 4], wolves).is_err());
        }
        for (part, allowed) in TIGER_SKIN_FILES.iter().enumerate() {
            let files = allowed
                .iter()
                .map(|name| ((*name).to_owned(), vec![1; 12]))
                .collect();
            let pack = build_tiger_skin(&files, part, [0; 4]).unwrap();
            assert_eq!(
                validate_pair(&pack.pamt, &pack.paz).unwrap().len(),
                allowed.len()
            );
            let arbitrary = BTreeMap::from([("other.dds".into(), vec![1])]);
            assert!(build_tiger_skin(&arbitrary, part, [0; 4]).is_err());
        }
        let unrelated_mapping = BTreeMap::from([("other.pappt".into(), vec![1])]);
        assert!(build_tiger_skin(&unrelated_mapping, 3, [0; 4]).is_err());
        assert!(build_tiger_skin(&BTreeMap::new(), 4, [0; 4]).is_err());
    }
    #[test]
    fn region_overlay_accepts_only_the_complete_curated_set() {
        let files: BTreeMap<String, Vec<u8>> = REGION_TIMER_FILES
            .iter()
            .map(|name| ((*name).into(), vec![7; 80]))
            .collect();
        let pack = build_region_timers(&files, [0; 4]).unwrap();
        let paths = validate_pair(&pack.pamt, &pack.paz).unwrap();
        assert_eq!(paths.len(), 4);
        assert!(paths.iter().all(|p| p.starts_with(REGION_TIMER_DIRECTORY)));
        let mut incomplete = files.clone();
        incomplete.remove(REGION_TIMER_FILES[0]);
        assert!(build_region_timers(&incomplete, [0; 4]).is_err());
        let mut unknown = files;
        let value = unknown.remove(REGION_TIMER_FILES[0]).unwrap();
        unknown.insert("unreviewed.pastage".into(), value);
        assert!(build_region_timers(&unknown, [0; 4]).is_err());
    }
    #[test]
    fn raw_table_pair_reopens_and_rejects_mismatched_lengths() {
        let files = BTreeMap::from([
            (
                "inventory.staticinfobody".into(),
                (0..=255).cycle().take(8195).collect(),
            ),
            ("inventory.staticinfoheader".into(), vec![7; 128]),
        ]);
        let pack = build_uncompressed(&files, [0x32, 2, 14, 97]).unwrap();
        assert_eq!(validate_pair(&pack.pamt, &pack.paz).unwrap().len(), 2);
        let mut empty = vec![0; 16];
        empty[4..8].copy_from_slice(&checksum(&[0; 4]).to_le_bytes());
        let registry = register(&empty, "0042", 0x3fff, pack.checksum).unwrap();
        let tmp = tempfile::tempdir().unwrap();
        std::fs::create_dir(tmp.path().join("meta")).unwrap();
        std::fs::create_dir(tmp.path().join("0042")).unwrap();
        for (p, b) in [
            ("meta/0.papgt", &registry),
            ("0042/0.pamt", &pack.pamt),
            ("0042/0.paz", &pack.paz),
        ] {
            std::fs::write(tmp.path().join(p), b).unwrap();
        }
        let archive = crate::Archive::open(tmp.path()).unwrap();
        for entry in archive.list_group("0042").unwrap() {
            assert_eq!(entry.flags, 0);
            assert_eq!(archive.extract(&entry).unwrap(), files[&entry.name]);
        }
        let mut broken = pack.pamt.clone();
        let at = broken.len() - 40 + 12;
        broken[at..at + 4].copy_from_slice(&8194u32.to_le_bytes());
        let crc = checksum(&broken[12..]);
        broken[..4].copy_from_slice(&crc.to_le_bytes());
        assert!(
            validate_pair(&broken, &pack.paz)
                .unwrap_err()
                .to_string()
                .contains("raw overlay lengths differ")
        );
    }
    #[test]
    fn encrypted_pair_reopens_with_real_archive_reader() {
        let files = BTreeMap::from([
            ("test.staticinfobody".into(), vec![0x45; 15000]),
            ("test.staticinfoheader".into(), vec![0x81; 100]),
        ]);
        let pack = build(&files, [0x32, 2, 14, 97]).unwrap();
        assert_eq!(pack.paz, build(&files, [0x32, 2, 14, 97]).unwrap().paz);
        let mut reg = vec![0; 16];
        reg[0] = 7;
        reg[11] = 42;
        reg[4..8].copy_from_slice(&checksum(&[0; 4]).to_le_bytes());
        let registered = register(&reg, "0041", 0x7fff, pack.checksum).unwrap();
        assert_eq!(registered[0], 7);
        assert_eq!(registered[11], 42);
        assert!(register(&registered, "0041", 0x7fff, 1).is_err());
        let tmp = tempfile::tempdir().unwrap();
        std::fs::create_dir(tmp.path().join("meta")).unwrap();
        std::fs::create_dir(tmp.path().join("0041")).unwrap();
        for (name, bytes) in [
            ("meta/0.papgt", registered),
            ("0041/0.pamt", pack.pamt),
            ("0041/0.paz", pack.paz),
        ] {
            std::fs::write(tmp.path().join(name), bytes).unwrap();
        }
        let archive = crate::Archive::open(tmp.path()).unwrap();
        for entry in archive.list_group("0041").unwrap() {
            assert_eq!(archive.extract(&entry).unwrap(), files[&entry.name]);
        }
    }
    #[test]
    fn validates_pair_bytes_and_rejects_incorrect_decoded_length() {
        let files = BTreeMap::from([
            ("test.staticinfobody".into(), vec![0x45; 512]),
            ("test.staticinfoheader".into(), vec![0x81; 42]),
        ]);
        let mut pack = build(&files, [0x32, 2, 14, 97]).unwrap();
        assert_eq!(validate_pair(&pack.pamt, &pack.paz).unwrap().len(), 2);
        let size_offset = pack.pamt.len() - 40 + 12;
        pack.pamt[size_offset..size_offset + 4].copy_from_slice(&513u32.to_le_bytes());
        let crc = checksum(&pack.pamt[12..]);
        pack.pamt[..4].copy_from_slice(&crc.to_le_bytes());
        let error = validate_pair(&pack.pamt, &pack.paz).unwrap_err();
        assert!(error.to_string().contains("decoded length"), "{error}");
    }
    #[test]
    fn rejects_paths_and_unpaired_tables() {
        for name in ["../x.staticinfobody", "x.staticinfobody", "x.txt"] {
            assert!(build(&BTreeMap::from([(name.into(), vec![1])]), [0; 4]).is_err());
        }
    }
}
