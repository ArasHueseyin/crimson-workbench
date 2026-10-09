//! PALOC list reader with lossless preservation of the original LZ4 container.
use crate::{MAX_FILE_BYTES, invalid};
use serde::Serialize;
use std::io;
#[derive(Debug, Clone, Serialize)]
pub struct PalocEntry {
    pub unk_id: u64,
    pub string_key: String,
    pub string_value: String,
}
#[derive(Debug)]
pub struct Paloc {
    original: Vec<u8>,
    payload: Vec<u8>,
    entries: Vec<PalocEntry>,
    raw: Vec<(u64, Vec<u8>, Vec<u8>)>,
    wrapped: bool,
}
impl Paloc {
    pub fn parse(data: &[u8]) -> io::Result<Self> {
        if data.len() > MAX_FILE_BYTES {
            return Err(invalid("PALOC exceeds 128 MiB limit"));
        }
        let wrapped = data.starts_with(b"paloc");
        let payload = if wrapped {
            if data.len() < 512 {
                return Err(invalid("truncated PALOC container"));
            }
            let reserved = u32::from_le_bytes(data[5..9].try_into().unwrap());
            let stored = u32::from_le_bytes(data[9..13].try_into().unwrap()) as usize;
            let decoded = u32::from_le_bytes(data[13..17].try_into().unwrap()) as usize;
            if reserved != 0 || data[17..512].iter().any(|b| *b != 0) {
                return Err(invalid("unknown PALOC header fields"));
            }
            if stored != data.len() - 512
                || decoded > MAX_FILE_BYTES
                || decoded > stored.saturating_mul(255).saturating_add(16)
            {
                return Err(invalid(
                    "PALOC compressed/decoded size mismatch or limit exceeded",
                ));
            }
            let output = lz4_flex::block::decompress(&data[512..], decoded)
                .map_err(|e| invalid(e.to_string()))?;
            if output.len() != decoded {
                return Err(invalid("PALOC decoded length mismatch"));
            }
            output
        } else {
            data.to_vec()
        };
        if payload.len() < 4 {
            return Err(invalid("truncated PALOC entry count"));
        }
        let end = payload.len() - 4;
        let count = u32::from_le_bytes(payload[end..].try_into().unwrap()) as usize;
        if count > end / 16 || count > 250_000 {
            return Err(invalid("implausible PALOC entry count"));
        }
        let mut p = 0;
        let mut entries = Vec::with_capacity(count);
        let mut raw = Vec::with_capacity(count);
        for _ in 0..count {
            let id = u64::from_le_bytes(take(&payload[..end], &mut p, 8)?.try_into().unwrap());
            let key = string(&payload[..end], &mut p)?;
            let value = string(&payload[..end], &mut p)?;
            entries.push(PalocEntry {
                unk_id: id,
                string_key: String::from_utf8_lossy(key).into_owned(),
                string_value: String::from_utf8_lossy(value).into_owned(),
            });
            raw.push((id, key.to_vec(), value.to_vec()));
        }
        if p != end {
            return Err(invalid("PALOC unconsumed entry bytes"));
        }
        let result = Self {
            original: data.to_vec(),
            payload,
            entries,
            raw,
            wrapped,
        };
        if result.serialize_payload()? != result.payload {
            return Err(invalid("PALOC payload roundtrip failed"));
        }
        Ok(result)
    }
    pub fn entries(&self) -> &[PalocEntry] {
        &self.entries
    }
    pub fn is_wrapped(&self) -> bool {
        self.wrapped
    }
    pub fn original_bytes(&self) -> &[u8] {
        &self.original
    }
    pub fn decoded_payload(&self) -> &[u8] {
        &self.payload
    }
    pub fn serialize_payload(&self) -> io::Result<Vec<u8>> {
        let mut output = Vec::with_capacity(self.payload.len());
        for (id, key, value) in &self.raw {
            output.extend(id.to_le_bytes());
            for text in [key, value] {
                let n =
                    u32::try_from(text.len()).map_err(|_| invalid("PALOC string exceeds u32"))?;
                output.extend(n.to_le_bytes());
                output.extend(text);
            }
        }
        output.extend((self.raw.len() as u32).to_le_bytes());
        Ok(output)
    }
    /// No edit surface exists in phase 1. Preserve the original compressed block
    /// rather than assuming recompression reproduces the engine's bytes.
    pub fn serialize(&self) -> io::Result<Vec<u8>> {
        if self.serialize_payload()? != self.payload {
            return Err(invalid("PALOC payload differs from immutable original"));
        }
        Ok(self.original.clone())
    }
}
fn take<'a>(data: &'a [u8], p: &mut usize, n: usize) -> io::Result<&'a [u8]> {
    let end = p
        .checked_add(n)
        .ok_or_else(|| invalid("PALOC offset overflow"))?;
    let out = data
        .get(*p..end)
        .ok_or_else(|| invalid("truncated PALOC string/entry"))?;
    *p = end;
    Ok(out)
}
fn string<'a>(data: &'a [u8], p: &mut usize) -> io::Result<&'a [u8]> {
    let n = u32::from_le_bytes(take(data, p, 4)?.try_into().unwrap()) as usize;
    take(data, p, n)
}
#[cfg(test)]
mod tests {
    use super::*;
    fn bare() -> Vec<u8> {
        let mut data = 1u64.to_le_bytes().to_vec();
        for s in [b"key".as_slice(), b"value"] {
            data.extend((s.len() as u32).to_le_bytes());
            data.extend(s);
        }
        data.extend(1u32.to_le_bytes());
        data
    }
    #[test]
    fn payload_and_original_container_roundtrip() {
        let data = bare();
        let block = lz4_flex::block::compress(&data);
        let mut wrapped = b"paloc".to_vec();
        wrapped.extend(0u32.to_le_bytes());
        wrapped.extend((block.len() as u32).to_le_bytes());
        wrapped.extend((data.len() as u32).to_le_bytes());
        wrapped.resize(512, 0);
        wrapped.extend(block);
        for src in [&data, &wrapped] {
            let p = Paloc::parse(src).unwrap();
            assert_eq!(p.entries()[0].string_value, "value");
            assert_eq!(p.serialize().unwrap(), *src);
            assert_eq!(p.serialize_payload().unwrap(), data);
        }
    }
    #[test]
    fn unknown_container_fields_and_huge_counts_fail() {
        let mut d = bare();
        let len = d.len();
        d[len - 4..].copy_from_slice(&u32::MAX.to_le_bytes());
        assert!(Paloc::parse(&d).is_err());
        let mut wrapped = vec![0; 512];
        wrapped[..5].copy_from_slice(b"paloc");
        wrapped[20] = 1;
        assert!(Paloc::parse(&wrapped).is_err());
        assert!(Paloc::parse(b"paloc").is_err());
    }
    #[test]
    fn invalid_utf8_stays_lossless() {
        let mut d = 1u64.to_le_bytes().to_vec();
        d.extend(1u32.to_le_bytes());
        d.push(255);
        d.extend(0u32.to_le_bytes());
        d.extend(1u32.to_le_bytes());
        let p = Paloc::parse(&d).unwrap();
        assert_eq!(p.serialize_payload().unwrap(), d);
    }
}
