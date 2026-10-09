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

fn build_with_storage(
    files: &BTreeMap<String, Vec<u8>>,
    encryption_header: [u8; 4],
    raw: bool,
) -> io::Result<Overlay> {
    if files.is_empty() || files.len() > 64 {
        return Err(invalid("overlay needs 1..64 files"));
    }
    let directory = "gamedata/binarystaticinfo__/bin";
    let mut paz = Vec::new();
    let mut names = Vec::new();
    let mut records = Vec::new();
    let mut total = 0usize;
    for (name, bytes) in files {
        let (stem, ext) = name
            .split_once('.')
            .ok_or_else(|| invalid("table filename required"))?;
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

#[cfg(test)]
mod tests {
    use super::*;
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
