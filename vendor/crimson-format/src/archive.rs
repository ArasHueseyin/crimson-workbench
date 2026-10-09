//! Safe archive IO and bounded PAMT/PAPGT readers, based on the formats in the
//! pinned crimson-rs sources. All filesystem opens are read-only.
use crate::{MAX_FILE_BYTES, checksum, invalid};
use serde::{Deserialize, Serialize};
use std::{
    collections::{HashMap, HashSet},
    fs::File,
    io::{self, Read, Seek, SeekFrom},
    path::{Path, PathBuf},
};
const MAX_META: usize = 16 * 1024 * 1024;
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct ArchiveGroup {
    pub name: String,
    pub is_optional: bool,
    pub language: u16,
    pub pamt_checksum: u32,
    pub present: bool,
}
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct ArchiveEntry {
    pub group: String,
    pub directory: String,
    pub name: String,
    pub path: String,
    pub chunk_id: u16,
    pub chunk_offset: u32,
    pub compressed_size: u32,
    pub uncompressed_size: u32,
    pub flags: u8,
}
#[derive(Debug)]
pub struct Archive {
    root: PathBuf,
    groups: Vec<ArchiveGroup>,
    registry_bytes: Vec<u8>,
}
impl Archive {
    pub fn open(root: impl AsRef<Path>) -> io::Result<Self> {
        let root = root.as_ref().canonicalize()?;
        if !root.is_dir() {
            return Err(invalid("archive root is not a directory"));
        }
        let path = contained(&root, &root.join("meta/0.papgt"))?;
        let bytes = read_bounded(&path, MAX_META)?;
        Self::with_registry(root, bytes)
    }
    /// Read original groups through a caller-verified registry snapshot. This
    /// performs no writes and confers no trust on the supplied registry.
    pub fn with_registry(root: impl AsRef<Path>, bytes: Vec<u8>) -> io::Result<Self> {
        let root = root.as_ref().canonicalize()?;
        if !root.is_dir() || bytes.len() > MAX_META {
            return Err(invalid("invalid archive root or registry size"));
        }
        let groups = registry(&bytes)?
            .into_iter()
            .map(|mut g| {
                g.present = root.join(&g.name).join("0.pamt").is_file();
                g
            })
            .collect();
        Ok(Self {
            root,
            groups,
            registry_bytes: bytes,
        })
    }
    pub fn root(&self) -> &Path {
        &self.root
    }
    pub fn groups(&self) -> &[ArchiveGroup] {
        &self.groups
    }
    pub fn serialize_registry(&self) -> io::Result<Vec<u8>> {
        serialize_registry(&self.registry_bytes)
    }
    pub fn serialize_group(&self, group: &str) -> io::Result<Vec<u8>> {
        Ok(self.metadata(group)?.serialized)
    }
    fn metadata(&self, group: &str) -> io::Result<Metadata> {
        let g = self
            .groups
            .iter()
            .find(|g| g.name == group)
            .ok_or_else(|| invalid("group is not registered"))?;
        let path = contained(&self.root, &self.root.join(group).join("0.pamt"))?;
        parse_pamt(group, &read_bounded(&path, MAX_META)?, g.pamt_checksum)
    }
    pub fn list_group(&self, group: &str) -> io::Result<Vec<ArchiveEntry>> {
        Ok(self.metadata(group)?.entries)
    }
    pub fn extract(&self, entry: &ArchiveEntry) -> io::Result<Vec<u8>> {
        // Do not trust caller-provided offsets, flags or paths.
        let meta = self.metadata(&entry.group)?;
        if !meta.entries.iter().any(|real| real == entry) {
            return Err(invalid(
                "archive entry differs from registered PAMT metadata",
            ));
        }
        let stored = entry.compressed_size as usize;
        let size = entry.uncompressed_size as usize;
        if stored > MAX_FILE_BYTES || size > MAX_FILE_BYTES {
            return Err(invalid("archive entry exceeds 128 MiB read limit"));
        }
        validate_flags(entry.flags)?;
        let path = contained(
            &self.root,
            &self
                .root
                .join(&entry.group)
                .join(format!("{}.paz", entry.chunk_id)),
        )?;
        let mut file = File::open(path)?;
        let end = u64::from(entry.chunk_offset) + u64::from(entry.compressed_size);
        if end > file.metadata()?.len() {
            return Err(invalid("entry extends beyond actual PAZ length"));
        }
        file.seek(SeekFrom::Start(u64::from(entry.chunk_offset)))?;
        let mut raw = vec![0; stored];
        file.read_exact(&mut raw)?;
        let decoded = match entry.flags >> 4 {
            0 => raw,
            3 => {
                crate::crypto::chacha20::decrypt_pack_entry(&raw, &meta.encrypt_info, &entry.name)?
            }
            _ => return Err(invalid("unsupported crypto")),
        };
        let output = match entry.flags & 15 {
            0 => decoded,
            1 => crate::binary::partial::decompress_partial(&decoded, size)?,
            2 => lz4_flex::block::decompress(&decoded, size).map_err(|e| invalid(e.to_string()))?,
            3 => {
                let decoder = flate2::read::ZlibDecoder::new(decoded.as_slice());
                let mut out = Vec::new();
                decoder.take(size as u64 + 1).read_to_end(&mut out)?;
                out
            }
            _ => return Err(invalid("unsupported compression")),
        };
        if output.len() != size {
            return Err(invalid("decompressed length differs from PAMT"));
        }
        Ok(output)
    }
}
pub(crate) fn read_bounded(path: &Path, limit: usize) -> io::Result<Vec<u8>> {
    let file = File::open(path)?;
    if file.metadata()?.len() > limit as u64 {
        return Err(invalid("file exceeds size limit"));
    }
    let mut bytes = Vec::new();
    file.take(limit as u64 + 1).read_to_end(&mut bytes)?;
    if bytes.len() > limit {
        return Err(invalid("file grew beyond size limit"));
    }
    Ok(bytes)
}
fn contained(root: &Path, path: &Path) -> io::Result<PathBuf> {
    let p = path.canonicalize()?;
    if !p.starts_with(root) {
        return Err(invalid("archive path escapes game directory"));
    }
    Ok(p)
}
fn component(s: &str) -> bool {
    !s.is_empty()
        && !matches!(s, "." | "..")
        && !s.bytes().any(|c| c < 32 || b"/\\:\0".contains(&c))
}
fn directory(s: &str) -> bool {
    s.is_empty() || s.split('/').all(component)
}
fn validate_flags(flags: u8) -> io::Result<()> {
    if !matches!(flags & 15, 0..=3) {
        return Err(invalid(format!(
            "unsupported compression flag {}",
            flags & 15
        )));
    }
    if !matches!(flags >> 4, 0 | 3) {
        return Err(invalid(format!("unsupported crypto flag {}", flags >> 4)));
    }
    Ok(())
}
struct Cursor<'a> {
    data: &'a [u8],
    pos: usize,
}
impl<'a> Cursor<'a> {
    fn new(data: &'a [u8]) -> Self {
        Self { data, pos: 0 }
    }
    fn take(&mut self, n: usize) -> io::Result<&'a [u8]> {
        let end = self
            .pos
            .checked_add(n)
            .ok_or_else(|| invalid("length overflow"))?;
        let b = self
            .data
            .get(self.pos..end)
            .ok_or_else(|| invalid("truncated archive metadata"))?;
        self.pos = end;
        Ok(b)
    }
    fn u8(&mut self) -> io::Result<u8> {
        Ok(self.take(1)?[0])
    }
    fn u16(&mut self) -> io::Result<u16> {
        Ok(u16::from_le_bytes(self.take(2)?.try_into().unwrap()))
    }
    fn u32(&mut self) -> io::Result<u32> {
        Ok(u32::from_le_bytes(self.take(4)?.try_into().unwrap()))
    }
    fn count(&mut self, stride: usize) -> io::Result<usize> {
        let n = self.u32()? as usize;
        if n > 1_000_000 || n > self.data.len().saturating_sub(self.pos) / stride {
            return Err(invalid("implausible archive record count"));
        }
        Ok(n)
    }
    fn buffer(&mut self) -> io::Result<&'a [u8]> {
        let n = self.u32()? as usize;
        self.take(n)
    }
    fn finish(&self) -> io::Result<()> {
        if self.pos != self.data.len() {
            Err(invalid("unparsed archive metadata bytes"))
        } else {
            Ok(())
        }
    }
}
pub(crate) fn registry(data: &[u8]) -> io::Result<Vec<ArchiveGroup>> {
    if data.len() < 12 {
        return Err(invalid("truncated PAPGT header"));
    }
    let mut r = Cursor::new(data);
    r.u32()?;
    let expected = r.u32()?;
    if checksum(&data[12..]) != expected {
        return Err(invalid("PAPGT checksum mismatch"));
    }
    let count = r.u8()? as usize;
    r.take(3)?;
    let mut rows = Vec::with_capacity(count);
    for _ in 0..count {
        let optional = r.u8()?;
        let language = r.u16()?;
        let zero = r.u8()?;
        if optional > 1 || zero != 0 {
            return Err(invalid("unknown PAPGT entry flags"));
        }
        let offset = r.u32()?;
        let crc = r.u32()?;
        rows.push((optional, language, offset, crc));
    }
    let names = r.buffer()?;
    r.finish()?;
    let mut seen = HashSet::new();
    rows.into_iter()
        .map(|(optional, language, offset, pamt_checksum)| {
            let rest = names
                .get(offset as usize..)
                .ok_or_else(|| invalid("PAPGT name offset out of range"))?;
            let end = rest
                .iter()
                .position(|b| *b == 0)
                .ok_or_else(|| invalid("PAPGT name not terminated"))?;
            let name = std::str::from_utf8(&rest[..end])
                .map_err(|e| invalid(e.to_string()))?
                .to_owned();
            if name.is_empty()
                || name.len() > 16
                || !name.bytes().all(|c| c.is_ascii_digit())
                || !seen.insert(name.clone())
            {
                return Err(invalid("invalid or duplicate numeric PAPGT group"));
            }
            Ok(ArchiveGroup {
                name,
                is_optional: optional != 0,
                language,
                pamt_checksum,
                present: false,
            })
        })
        .collect()
}
fn serialize_registry(data: &[u8]) -> io::Result<Vec<u8>> {
    registry(data)?;
    let mut r = Cursor::new(data);
    let unknown = r.u32()?;
    r.u32()?;
    let count = r.u8()?;
    let unknown1 = r.u8()?;
    let unknown2 = r.u16()?;
    let mut rows = Vec::with_capacity(count as usize);
    for _ in 0..count {
        rows.push((r.u8()?, r.u16()?, r.u8()?, r.u32()?, r.u32()?));
    }
    let names = r.buffer()?;
    r.finish()?;
    let mut out = Vec::with_capacity(data.len());
    out.extend(unknown.to_le_bytes());
    out.extend(0u32.to_le_bytes());
    out.push(count);
    out.push(unknown1);
    out.extend(unknown2.to_le_bytes());
    for (optional, language, zero, offset, crc) in rows {
        out.push(optional);
        out.extend(language.to_le_bytes());
        out.push(zero);
        out.extend(offset.to_le_bytes());
        out.extend(crc.to_le_bytes());
    }
    out.extend((names.len() as u32).to_le_bytes());
    out.extend(names);
    let crc = checksum(&out[12..]);
    out[4..8].copy_from_slice(&crc.to_le_bytes());
    Ok(out)
}
struct Metadata {
    encrypt_info: [u8; 3],
    entries: Vec<ArchiveEntry>,
    serialized: Vec<u8>,
}
fn parse_pamt(group: &str, data: &[u8], expected: u32) -> io::Result<Metadata> {
    if data.len() < 12 {
        return Err(invalid("truncated PAMT header"));
    }
    let mut r = Cursor::new(data);
    let stored = r.u32()?;
    if checksum(&data[12..]) != stored || stored != expected {
        return Err(invalid("PAMT checksum differs from header or registry"));
    }
    let count = r.u16()? as usize;
    if r.u16()? != 0 {
        return Err(invalid("unknown PAMT header field"));
    }
    let encrypt_unknown = r.u8()?;
    let encrypt_info: [u8; 3] = r.take(3)?.try_into().unwrap();
    if count > data.len().saturating_sub(12) / 12 {
        return Err(invalid("implausible PAZ chunk count"));
    }
    let mut chunks = HashMap::new();
    let mut chunk_rows = Vec::with_capacity(count);
    for _ in 0..count {
        let id = r.u32()?;
        let crc = r.u32()?;
        let size = r.u32()?;
        chunk_rows.push((id, crc, size));
        if chunks.insert(id, size).is_some() {
            return Err(invalid("duplicate PAZ chunk"));
        }
    }
    let dir_names = r.buffer()?;
    let file_names = r.buffer()?;
    let dirs_count = r.count(16)?;
    let mut dirs = Vec::with_capacity(dirs_count);
    for _ in 0..dirs_count {
        dirs.push((
            r.u32()?,
            r.u32()? as i32,
            r.u32()? as usize,
            r.u32()? as usize,
        ));
    }
    let files_count = r.count(20)?;
    let mut files = Vec::with_capacity(files_count);
    for _ in 0..files_count {
        files.push((
            r.u32()? as i32,
            r.u32()?,
            r.u32()?,
            r.u32()?,
            r.u16()?,
            r.u8()?,
            r.u8()?,
        ));
    }
    r.finish()?;
    let mut output = Vec::with_capacity(files_count);
    let mut paths = HashSet::new();
    let mut assigned = vec![false; files_count];
    let mut total_path_bytes = 0usize;
    for &(hash, name_offset, start, count) in &dirs {
        let path = trie_string(dir_names, name_offset)?;
        if !directory(&path) || checksum(path.as_bytes()) != hash {
            return Err(invalid("invalid PAMT directory or name checksum"));
        }
        let end = start
            .checked_add(count)
            .filter(|e| *e <= files.len())
            .ok_or_else(|| invalid("PAMT directory file range exceeds file count"))?;
        for (index, &(name_offset, offset, stored, size, chunk, flags, unknown)) in
            files[start..end].iter().enumerate()
        {
            if std::mem::replace(&mut assigned[start + index], true) {
                return Err(invalid("PAMT file belongs to multiple directories"));
            }
            if unknown != 0 {
                return Err(invalid("unknown PAMT file marker"));
            }
            // Unknown codecs remain visible in enumeration; extraction refuses them explicitly.
            let chunk_size = chunks
                .get(&(chunk as u32))
                .ok_or_else(|| invalid("unknown PAZ chunk"))?;
            if u64::from(offset) + u64::from(stored) > u64::from(*chunk_size) {
                return Err(invalid("entry exceeds recorded PAZ chunk size"));
            }
            let name = trie_string(file_names, name_offset)?;
            if !component(&name) {
                return Err(invalid("invalid PAMT filename"));
            }
            let full = if path.is_empty() {
                name.clone()
            } else {
                format!("{path}/{name}")
            };
            total_path_bytes = total_path_bytes
                .checked_add(full.len())
                .ok_or_else(|| invalid("path budget overflow"))?;
            if total_path_bytes > 16 * 1024 * 1024 {
                return Err(invalid("resolved archive paths exceed 16 MiB budget"));
            }
            if !paths.insert(full.clone()) {
                return Err(invalid("duplicate virtual archive path"));
            }
            output.push(ArchiveEntry {
                group: group.into(),
                directory: path.clone(),
                name,
                path: full,
                chunk_id: chunk,
                chunk_offset: offset,
                compressed_size: stored,
                uncompressed_size: size,
                flags,
            });
        }
    }
    if assigned.iter().any(|v| !*v) {
        return Err(invalid("orphan PAMT file record"));
    }
    let mut serialized = Vec::with_capacity(data.len());
    serialized.extend(stored.to_le_bytes());
    serialized.extend((chunk_rows.len() as u16).to_le_bytes());
    serialized.extend(0u16.to_le_bytes());
    serialized.push(encrypt_unknown);
    serialized.extend(encrypt_info);
    for (id, crc, size) in chunk_rows {
        serialized.extend(id.to_le_bytes());
        serialized.extend(crc.to_le_bytes());
        serialized.extend(size.to_le_bytes());
    }
    for trie in [dir_names, file_names] {
        serialized.extend((trie.len() as u32).to_le_bytes());
        serialized.extend(trie);
    }
    serialized.extend((dirs.len() as u32).to_le_bytes());
    for (hash, off, start, count) in dirs {
        serialized.extend(hash.to_le_bytes());
        serialized.extend(off.to_le_bytes());
        serialized.extend((start as u32).to_le_bytes());
        serialized.extend((count as u32).to_le_bytes());
    }
    serialized.extend((files.len() as u32).to_le_bytes());
    for (name, offset, stored, size, chunk, flags, unknown) in files {
        serialized.extend(name.to_le_bytes());
        serialized.extend(offset.to_le_bytes());
        serialized.extend(stored.to_le_bytes());
        serialized.extend(size.to_le_bytes());
        serialized.extend(chunk.to_le_bytes());
        serialized.push(flags);
        serialized.push(unknown);
    }
    let crc = checksum(&serialized[12..]);
    serialized[..4].copy_from_slice(&crc.to_le_bytes());
    Ok(Metadata {
        encrypt_info,
        entries: output,
        serialized,
    })
}
fn trie_string(data: &[u8], mut offset: i32) -> io::Result<String> {
    let mut seen = HashSet::new();
    let mut parts = Vec::new();
    let mut length = 0usize;
    while offset != -1 {
        if offset < 0 || !seen.insert(offset) || seen.len() > 256 {
            return Err(invalid("cyclic or excessively deep PAMT trie"));
        }
        let mut r = Cursor::new(
            data.get(offset as usize..)
                .ok_or_else(|| invalid("trie offset out of range"))?,
        );
        offset = r.u32()? as i32;
        let n = r.u8()? as usize;
        let part = r.take(n)?;
        length += n;
        if length > 4096 {
            return Err(invalid("virtual path exceeds limit"));
        }
        parts.push(part);
    }
    let bytes: Vec<u8> = parts.into_iter().rev().flatten().copied().collect();
    String::from_utf8(bytes).map_err(|e| invalid(e.to_string()))
}

pub(crate) fn validate_overlay_pair(pamt: &[u8], paz: &[u8]) -> io::Result<Vec<String>> {
    if pamt.len() < 24 || pamt.len() > MAX_META || paz.len() > MAX_FILE_BYTES {
        return Err(invalid("overlay length outside bounds"));
    }
    let number = |at: usize| u32::from_le_bytes(pamt[at..at + 4].try_into().unwrap());
    if pamt[4..8] != [1, 0, 0, 0]
        || number(12) != 0
        || number(16) != checksum(paz)
        || number(20) as usize != paz.len()
    {
        return Err(invalid("overlay chunk checksum, count or length mismatch"));
    }
    let meta = parse_pamt("0000", pamt, number(0))?;
    let names: HashSet<_> = meta.entries.iter().map(|e| e.name.as_str()).collect();
    if meta.entries.is_empty() || meta.entries.len() > 64 {
        return Err(invalid("overlay file count outside bounds"));
    }
    let mut spans = Vec::new();
    let mut total = 0usize;
    for e in &meta.entries {
        let (stem, ext) = e
            .name
            .split_once('.')
            .ok_or_else(|| invalid("overlay table name required"))?;
        let partner = match ext {
            "staticinfobody" => "staticinfoheader",
            "staticinfoheader" => "staticinfobody",
            _ => return Err(invalid("non-table overlay entry")),
        };
        if stem.is_empty()
            || !stem
                .bytes()
                .all(|b| b.is_ascii_lowercase() || b.is_ascii_digit())
            || !names.contains(format!("{stem}.{partner}").as_str())
            || e.directory != "gamedata/binarystaticinfo__/bin"
            || !matches!(e.flags, 0 | 0x32)
            || e.chunk_id != 0
            || e.chunk_offset % 16 != 0
            || e.uncompressed_size as usize > MAX_FILE_BYTES
        {
            return Err(invalid("overlay entry outside supported table schema"));
        }
        let start = e.chunk_offset as usize;
        let end = start + e.compressed_size as usize;
        spans.push((start, end));
        total = total
            .checked_add(e.uncompressed_size as usize)
            .ok_or_else(|| invalid("overlay size overflow"))?;
        if e.uncompressed_size == 0 || total > MAX_FILE_BYTES {
            return Err(invalid("decoded overlay exceeds size limit"));
        }
        let cipher = paz
            .get(start..end)
            .ok_or_else(|| invalid("overlay entry out of bounds"))?;
        let decoded = if e.flags == 0 {
            if e.compressed_size != e.uncompressed_size {
                return Err(invalid("raw overlay lengths differ"));
            }
            cipher.to_vec()
        } else {
            let compressed =
                crate::crypto::chacha20::decrypt_pack_entry(cipher, &meta.encrypt_info, &e.name)?;
            lz4_flex::block::decompress(&compressed, e.uncompressed_size as usize)
                .map_err(|e| invalid(e.to_string()))?
        };
        if decoded.len() != e.uncompressed_size as usize {
            return Err(invalid("overlay decoded length differs from metadata"));
        }
    }
    spans.sort_unstable();
    if spans.windows(2).any(|pair| pair[0].1 > pair[1].0) {
        return Err(invalid("overlapping overlay entries"));
    }
    Ok(meta.entries.into_iter().map(|e| e.path).collect())
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn refuses_trie_cycles_and_bad_paths() {
        assert!(trie_string(&[0, 0, 0, 0, 0], 0).is_err());
        assert!(trie_string(&[], i32::MIN).is_err());
        for p in ["../x", "/x", "x//y", "x/../y", "C:/x", "x\\y"] {
            assert!(!directory(p), "{p}");
        }
    }
    #[test]
    fn refuses_unknown_codecs() {
        for flag in [4, 15, 0x10, 0x20, 0x40, 0xff] {
            assert!(validate_flags(flag).is_err());
        }
        for flag in [0, 1, 2, 3, 0x30, 0x32] {
            assert!(validate_flags(flag).is_ok());
        }
    }
    #[test]
    fn rejects_truncated_and_wrong_checksum() {
        assert!(registry(&[]).is_err());
        assert!(parse_pamt("0008", &[0; 12], 0).is_err());
        let mut bytes = vec![0; 16];
        bytes[8] = 1;
        assert!(registry(&bytes).is_err());
    }
    fn fixture() -> (tempfile::TempDir, Vec<u8>) {
        use std::fs;
        let tmp = tempfile::tempdir().unwrap();
        fs::create_dir(tmp.path().join("meta")).unwrap();
        fs::create_dir(tmp.path().join("0008")).unwrap();
        let plain = b"synthetic encrypted archive payload, repeated repeated repeated".to_vec();
        let enc = [11, 22, 33];
        let compressed = lz4_flex::block::compress(&plain);
        let cipher =
            crate::crypto::chacha20::decrypt_pack_entry(&compressed, &enc, "test.bin").unwrap();
        let mut pamt = vec![0; 4];
        pamt.extend(1u16.to_le_bytes());
        pamt.extend(0u16.to_le_bytes());
        pamt.push(0);
        pamt.extend(enc);
        pamt.extend(0u32.to_le_bytes());
        pamt.extend(checksum(&cipher).to_le_bytes());
        pamt.extend((cipher.len() as u32).to_le_bytes());
        for name in ["gamedata/test", "test.bin"] {
            let mut trie = (-1i32).to_le_bytes().to_vec();
            trie.push(name.len() as u8);
            trie.extend(name.as_bytes());
            pamt.extend((trie.len() as u32).to_le_bytes());
            pamt.extend(trie);
        }
        pamt.extend(1u32.to_le_bytes());
        pamt.extend(checksum(b"gamedata/test").to_le_bytes());
        pamt.extend(0u32.to_le_bytes());
        pamt.extend(0u32.to_le_bytes());
        pamt.extend(1u32.to_le_bytes());
        pamt.extend(1u32.to_le_bytes());
        pamt.extend(0u32.to_le_bytes());
        pamt.extend(0u32.to_le_bytes());
        pamt.extend((cipher.len() as u32).to_le_bytes());
        pamt.extend((plain.len() as u32).to_le_bytes());
        pamt.extend(0u16.to_le_bytes());
        pamt.extend([0x32, 0]);
        let pamt_crc = checksum(&pamt[12..]);
        pamt[..4].copy_from_slice(&pamt_crc.to_le_bytes());
        let mut papgt = vec![0; 12];
        papgt[8] = 1;
        papgt.push(0);
        papgt.extend(0x7fffu16.to_le_bytes());
        papgt.push(0);
        papgt.extend(0u32.to_le_bytes());
        papgt.extend(pamt_crc.to_le_bytes());
        papgt.extend(5u32.to_le_bytes());
        papgt.extend(b"0008\0");
        let crc = checksum(&papgt[12..]);
        papgt[4..8].copy_from_slice(&crc.to_le_bytes());
        fs::write(tmp.path().join("meta/0.papgt"), papgt).unwrap();
        fs::write(tmp.path().join("0008/0.pamt"), pamt).unwrap();
        fs::write(tmp.path().join("0008/0.paz"), cipher).unwrap();
        (tmp, plain)
    }
    #[test]
    fn native_archive_decrypts_and_roundtrips_real_structures() {
        let (tmp, plain) = fixture();
        let a = Archive::open(tmp.path()).unwrap();
        let entries = a.list_group("0008").unwrap();
        assert_eq!(entries.len(), 1);
        assert_eq!(a.extract(&entries[0]).unwrap(), plain);
        assert_eq!(
            a.serialize_registry().unwrap(),
            std::fs::read(tmp.path().join("meta/0.papgt")).unwrap()
        );
        assert_eq!(
            a.serialize_group("0008").unwrap(),
            std::fs::read(tmp.path().join("0008/0.pamt")).unwrap()
        );
    }
    #[test]
    fn rejects_forged_entry_and_truncated_actual_chunk() {
        let (tmp, _) = fixture();
        let a = Archive::open(tmp.path()).unwrap();
        let entries = a.list_group("0008").unwrap();
        let mut forged = entries[0].clone();
        forged.chunk_offset += 1;
        assert!(a.extract(&forged).is_err());
        std::fs::write(tmp.path().join("0008/0.paz"), [0]).unwrap();
        assert!(a.extract(&entries[0]).is_err());
    }
    #[test]
    fn registry_checksum_does_not_accept_a_modified_pamt() {
        let (tmp, _) = fixture();
        let a = Archive::open(tmp.path()).unwrap();
        let path = tmp.path().join("0008/0.pamt");
        let mut bytes = std::fs::read(&path).unwrap();
        bytes[8] ^= 1; // header-only change still parsed; body mutation must fail
        bytes[20] ^= 1;
        std::fs::write(path, bytes).unwrap();
        assert!(a.list_group("0008").is_err());
    }
}
