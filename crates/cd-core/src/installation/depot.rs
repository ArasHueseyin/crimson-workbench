//! Own bounded reader of Steam's public manifest wire format; no Steam code or
//! authentication keys. A parsed cached manifest is NOT a trusted baseline.
use super::{bad, relative};
use std::{collections::BTreeSet, io};

pub(super) struct Depot {
    pub id: u32,
    pub manifest: u64,
    pub total_bytes: u64,
    pub signature_bytes: usize,
    pub entries: Vec<Entry>,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub(super) struct Entry {
    pub path: String,
    pub bytes: u64,
    pub directory: bool,
    pub sha1: Option<String>,
}

pub(super) fn parse(data: &[u8]) -> io::Result<Depot> {
    if data.len() > 16 * 1024 * 1024 {
        return Err(bad("depot manifest exceeds 16 MiB"));
    }
    let mut input = data;
    let payload = section(&mut input, 0x71f6_17d0)?;
    let metadata = section(&mut input, 0x1f48_12be)?;
    let signature = section(&mut input, 0x1b81_b817)?;
    if take(&mut input, 4)? != 0x32c4_15abu32.to_le_bytes() || !input.is_empty() {
        return Err(bad("invalid depot end marker or trailing data"));
    }
    let meta = fields(metadata)?;
    let id = u32::try_from(number(&meta, 1)?).map_err(|_| bad("depot ID overflow"))?;
    let manifest = number(&meta, 2)?;
    if id == 0 || manifest == 0 || number(&meta, 4)? != 0 {
        return Err(bad("invalid depot identity or encrypted filenames"));
    }
    let total_bytes = number(&meta, 5)?;
    if number(&meta, 9)? != crc32(payload) as u64 {
        return Err(bad("depot payload CRC mismatch"));
    }
    let sig = fields(signature)?;
    let signature_bytes = optional(&sig, 1)?
        .map(bytes)
        .transpose()?
        .map_or(0, <[u8]>::len);
    let mut entries = Vec::new();
    let mut names = BTreeSet::new();
    let mut total = 0u64;
    for (field, value) in fields(payload)? {
        if field != 1 {
            continue;
        }
        if entries.len() >= 16_384 {
            return Err(bad("too many depot entries"));
        }
        let mapping = fields(bytes(value)?)?;
        let path = relative(
            std::str::from_utf8(bytes(required(&mapping, 1)?)?)
                .map_err(|_| bad("invalid depot filename UTF-8"))?,
        )?;
        if !names.insert(path.to_lowercase()) {
            return Err(bad("duplicate depot path"));
        }
        let size = number(&mapping, 2)?;
        let flags = number(&mapping, 3)?;
        if flags > u32::MAX as u64 {
            return Err(bad("depot flags overflow"));
        }
        if optional(&mapping, 7)?
            .map(bytes)
            .transpose()?
            .is_some_and(|v| !v.is_empty())
        {
            return Err(bad("linked depot entries are unsupported"));
        }
        let directory = flags & 64 != 0;
        let sha1 = if directory {
            if size != 0 {
                return Err(bad("nonempty depot directory"));
            }
            None
        } else {
            let hash = bytes(required(&mapping, 5)?)?;
            if hash.len() != 20 {
                return Err(bad("invalid depot content SHA-1"));
            }
            total = total
                .checked_add(size)
                .ok_or_else(|| bad("depot size overflow"))?;
            Some(hash.iter().map(|b| format!("{b:02x}")).collect())
        };
        entries.push(Entry {
            path,
            bytes: size,
            directory,
            sha1,
        });
    }
    if entries.is_empty() || total != total_bytes {
        return Err(bad("empty depot or total size mismatch"));
    }
    Ok(Depot {
        id,
        manifest,
        total_bytes,
        signature_bytes,
        entries,
    })
}
fn take<'a>(input: &mut &'a [u8], len: usize) -> io::Result<&'a [u8]> {
    if input.len() < len {
        return Err(bad("truncated depot manifest"));
    }
    let (out, rest) = input.split_at(len);
    *input = rest;
    Ok(out)
}
fn section<'a>(input: &mut &'a [u8], magic: u32) -> io::Result<&'a [u8]> {
    if take(input, 4)? != magic.to_le_bytes() {
        return Err(bad("unexpected depot section"));
    }
    let len = u32::from_le_bytes(take(input, 4)?.try_into().unwrap()) as usize;
    take(input, len)
}
#[derive(Clone, Copy)]
enum Value<'a> {
    Number(u64),
    Bytes(&'a [u8]),
    Fixed,
}
fn varint(input: &mut &[u8]) -> io::Result<u64> {
    let mut n = 0u64;
    for shift in (0..70).step_by(7) {
        let b = take(input, 1)?[0];
        if shift == 63 && b > 1 {
            return Err(bad("protobuf varint overflow"));
        }
        n |= ((b & 127) as u64) << shift;
        if b < 128 {
            return Ok(n);
        }
    }
    Err(bad("unterminated protobuf varint"))
}
fn fields(mut input: &[u8]) -> io::Result<Vec<(u32, Value<'_>)>> {
    let mut result = Vec::new();
    while !input.is_empty() {
        if result.len() >= 32_768 {
            return Err(bad("too many protobuf fields"));
        }
        let key = varint(&mut input)?;
        if key >> 3 == 0 || key >> 3 > 0x1fff_ffff {
            return Err(bad("invalid protobuf field"));
        }
        let value = match key & 7 {
            0 => Value::Number(varint(&mut input)?),
            1 => {
                take(&mut input, 8)?;
                Value::Fixed
            }
            2 => {
                let len = usize::try_from(varint(&mut input)?)
                    .map_err(|_| bad("field length overflow"))?;
                Value::Bytes(take(&mut input, len)?)
            }
            5 => {
                take(&mut input, 4)?;
                Value::Fixed
            }
            _ => return Err(bad("unsupported protobuf wire type")),
        };
        result.push(((key >> 3) as u32, value));
    }
    Ok(result)
}
fn optional<'a>(fields: &[(u32, Value<'a>)], key: u32) -> io::Result<Option<Value<'a>>> {
    let mut found = fields.iter().filter(|(k, _)| *k == key).map(|(_, v)| *v);
    let value = found.next();
    if found.next().is_some() {
        return Err(bad("duplicate singular protobuf field"));
    }
    Ok(value)
}
fn required<'a>(fields: &[(u32, Value<'a>)], key: u32) -> io::Result<Value<'a>> {
    optional(fields, key)?.ok_or_else(|| bad("missing protobuf field"))
}
fn number(fields: &[(u32, Value<'_>)], key: u32) -> io::Result<u64> {
    match required(fields, key)? {
        Value::Number(v) => Ok(v),
        _ => Err(bad("expected protobuf number")),
    }
}
fn bytes(value: Value<'_>) -> io::Result<&[u8]> {
    match value {
        Value::Bytes(v) => Ok(v),
        _ => Err(bad("expected protobuf bytes")),
    }
}
// CRC covers the little-endian payload LENGTH followed by payload, not magic.
fn crc32(payload: &[u8]) -> u32 {
    let table: [u32; 256] = std::array::from_fn(|i| {
        let mut n = i as u32;
        for _ in 0..8 {
            n = (n >> 1) ^ if n & 1 != 0 { 0xedb8_8320 } else { 0 };
        }
        n
    });
    let mut crc = u32::MAX;
    for b in (payload.len() as u32).to_le_bytes().iter().chain(payload) {
        crc = (crc >> 8) ^ table[((crc as u8) ^ b) as usize];
    }
    !crc
}

#[cfg(test)]
pub(super) mod tests {
    use super::*;
    fn vi(mut n: u64) -> Vec<u8> {
        let mut v = Vec::new();
        while n >= 128 {
            v.push(n as u8 | 128);
            n >>= 7;
        }
        v.push(n as u8);
        v
    }
    fn num(k: u64, n: u64) -> Vec<u8> {
        [vi(k << 3), vi(n)].concat()
    }
    fn blob(k: u64, v: &[u8]) -> Vec<u8> {
        [vi(k << 3 | 2), vi(v.len() as u64), v.to_vec()].concat()
    }
    pub fn fixture(paths: &[(&str, u64)], id: u32, gid: u64) -> Vec<u8> {
        fixture_hashes(
            &paths
                .iter()
                .map(|(p, n)| (*p, *n, [7; 20]))
                .collect::<Vec<_>>(),
            id,
            gid,
        )
    }
    pub fn fixture_hashes(paths: &[(&str, u64, [u8; 20])], id: u32, gid: u64) -> Vec<u8> {
        let payload: Vec<u8> = paths
            .iter()
            .flat_map(|(p, n, hash)| {
                blob(
                    1,
                    &[blob(1, p.as_bytes()), num(2, *n), num(3, 0), blob(5, hash)].concat(),
                )
            })
            .collect();
        wrap(
            &payload,
            &[
                num(1, id as u64),
                num(2, gid),
                num(4, 0),
                num(5, paths.iter().map(|(_, n, _)| n).sum()),
                num(9, crc32(&payload) as u64),
            ]
            .concat(),
            &[],
        )
    }
    fn wrap(p: &[u8], m: &[u8], s: &[u8]) -> Vec<u8> {
        let mut out = Vec::new();
        for (magic, v) in [(0x71f6_17d0u32, p), (0x1f48_12be, m), (0x1b81_b817, s)] {
            out.extend(magic.to_le_bytes());
            out.extend((v.len() as u32).to_le_bytes());
            out.extend(v);
        }
        out.extend(0x32c4_15abu32.to_le_bytes());
        out
    }
    #[test]
    fn valid_manifest_and_every_truncation() {
        let v = fixture(&[("bin64\\probe.exe", 12), ("0000/0.paz", 5)], 1, 2);
        let d = parse(&v).unwrap();
        assert_eq!(d.total_bytes, 17);
        assert_eq!(d.entries[0].path, "bin64/probe.exe");
        assert_eq!(d.signature_bytes, 0);
        for i in 0..v.len() {
            assert!(parse(&v[..i]).is_err(), "prefix {i}");
        }
        let mut extra = v.clone();
        extra.push(0);
        assert!(parse(&extra).is_err());
        let mut crc = v;
        crc[20] ^= 1;
        assert!(parse(&crc).is_err());
    }
    #[test]
    fn untrusted_paths_duplicates_and_wire_errors() {
        for path in [
            "../a",
            "a/../b",
            "/a",
            "C:/x",
            "a:x",
            "a./x",
            "a/CON.txt",
            "a//x",
            "a\0x",
            "a/<x>",
        ] {
            assert!(parse(&fixture(&[(path, 1)], 1, 2)).is_err(), "{path}");
        }
        assert!(parse(&fixture(&[("a.exe", 1), ("A.exe", 1)], 1, 2)).is_err());
        assert!(fields(&[8, 255, 255, 255, 255, 255, 255, 255, 255, 255, 2]).is_err());
        assert!(fields(&[10, 255, 255, 255, 255, 127]).is_err());
        assert!(fields(&[0]).is_err());
        assert!(fields(&[11]).is_err());
        assert!(number(&fields(&[8, 1, 8, 2]).unwrap(), 1).is_err());
    }
    #[test]
    fn metadata_and_signature_do_not_grant_trust() {
        let payload = blob(
            1,
            &[blob(1, b"a"), num(2, 1), num(3, 0), blob(5, &[0; 20])].concat(),
        );
        let meta = [
            num(1, 1),
            num(2, 2),
            num(4, 0),
            num(5, 1),
            num(9, crc32(&payload) as u64),
        ]
        .concat();
        assert_eq!(
            parse(&wrap(&payload, &meta, &blob(1, b"unverified")))
                .unwrap()
                .signature_bytes,
            10
        );
        for (k, n) in [(1, 0), (2, 0), (4, 1), (5, 2), (9, 0)] {
            let mut altered = Vec::new();
            for (key, val) in [(1, 1), (2, 2), (4, 0), (5, 1), (9, crc32(&payload) as u64)] {
                altered.extend(num(key, if key == k { n } else { val }));
            }
            assert!(parse(&wrap(&payload, &altered, &[])).is_err());
        }
    }
}
