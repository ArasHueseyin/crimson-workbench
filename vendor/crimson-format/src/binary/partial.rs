// Adapted from crimson-rs b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0.
// Copyright (c) 2026 Tommy Tran. MIT; see LICENSE and docs/FORMAT_PORT.md.
use std::io;
/// Partial-compression scheme used by Pearl Abyss PAZ archives when
/// `raw_compression == 1`. The on-disk payload is one of:
///
/// 1. **Identity** — when LZ4 yielded no gain, the engine stores the file
///    verbatim and sets `compressed_size == uncompressed_size`. The
///    bytes ARE the file; no decoder needed.
/// 2. **Header + LZ4(prefix dict)** — the first `PARTIAL_HEADER_BYTES`
///    (128) bytes are stored verbatim, then the remainder is one LZ4
///    block. The decoder uses those 128 bytes as a prefix dictionary
///    so back-references can reach into the header. Covers every file
///    under `0012/ui/texture/icon/` in 1.06 (every item icon).
/// 3. **DDS per-mip table** — for DDS textures the engine can encode
///    each mip level independently. The on-disk size of mip *i* is
///    stored as a u32 in the DDS reserved area at `0x20 + 4*i` (11
///    slots, mips 0..10). A non-zero slot smaller than that mip's raw
///    size means LZ4-compressed; equal means raw; `0` means "all
///    remaining mips are stored raw, sequentially". Covers the
///    worldmap SDF tiles and large diffuse textures the simpler rule
///    misses. Strategy + offsets reverse-engineered by NattKh in the
///    CrimsonForge modding tool — see `core/compression_engine.py`
///    `_decompress_type1_dds_per_mip_sizes`.
///
/// **Not yet handled**: the PAR-container layout used by `.pam` /
/// `.pamlod` / `.pac` mesh assets in 0009/0015 (per-section LZ4 blocks
/// indexed by an 8-slot table at offset 0x10). Those return an
/// `InvalidData` error here so the caller can distinguish them from
/// outright PAZ corruption.
const PARTIAL_HEADER_BYTES: usize = 128;

pub(crate) fn decompress_partial(
    decrypted: &[u8],
    uncompressed_size: usize,
) -> io::Result<Vec<u8>> {
    if decrypted.len() == uncompressed_size {
        // Identity case — the engine declined LZ4 because the file
        // doesn't compress (already-block-compressed BC formats, etc.).
        return Ok(decrypted.to_vec());
    }
    if decrypted.len() <= PARTIAL_HEADER_BYTES || uncompressed_size <= PARTIAL_HEADER_BYTES {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            format!(
                "partial PAZ entry too short for header({})+lz4: decrypted={} u_size={}",
                PARTIAL_HEADER_BYTES,
                decrypted.len(),
                uncompressed_size,
            ),
        ));
    }

    // Strategy 2 — header(128) + LZ4(rest) with the header as a prefix
    // dictionary. Cheap to try first because no DDS parsing is required.
    if let Some(out) = try_partial_header_lz4(decrypted, uncompressed_size) {
        return Ok(out);
    }
    // Strategy 3 — DDS-only per-mip layout.
    if let Some(out) = try_partial_dds_per_mip(decrypted, uncompressed_size) {
        return Ok(out);
    }
    Err(io::Error::new(
        io::ErrorKind::InvalidData,
        format!(
            "partial PAZ entry uses an unrecognised layout (decrypted={}, u_size={})",
            decrypted.len(),
            uncompressed_size
        ),
    ))
}

fn try_partial_header_lz4(decrypted: &[u8], uncompressed_size: usize) -> Option<Vec<u8>> {
    let dict = &decrypted[..PARTIAL_HEADER_BYTES];
    let body = lz4_flex::block::decompress_with_dict(
        &decrypted[PARTIAL_HEADER_BYTES..],
        uncompressed_size - PARTIAL_HEADER_BYTES,
        dict,
    )
    .ok()?;
    if body.len() + PARTIAL_HEADER_BYTES != uncompressed_size {
        return None;
    }
    let mut out = Vec::with_capacity(uncompressed_size);
    out.extend_from_slice(dict);
    out.extend_from_slice(&body);
    Some(out)
}

/// Per-mip DDS layout: the DDS reserved area carries up to 11 u32 slots
/// giving each mip's on-disk byte length. A non-zero slot smaller than
/// the mip's raw size means that mip is LZ4-compressed; equal means
/// raw; zero means "the remaining mips are stored raw, sequentially".
fn try_partial_dds_per_mip(decrypted: &[u8], uncompressed_size: usize) -> Option<Vec<u8>> {
    let info = DdsInfo::parse(decrypted)?;
    if info.expected_total_size()? != uncompressed_size {
        return None;
    }
    if info.mip_count == 0 {
        return None;
    }
    let raw_mip_sizes: Vec<usize> = (0..info.mip_count)
        .map(|lvl| {
            let mw = (info.width >> lvl).max(1);
            let mh = (info.height >> lvl).max(1);
            info.mip_payload_size(mw, mh)
        })
        .collect::<Option<Vec<_>>>()?;

    // Up to 11 slots at offset 0x20.
    let max_explicit = info.mip_count.min(11);
    let mut reserved = [0u32; 11];
    for (i, slot) in reserved.iter_mut().enumerate().take(max_explicit) {
        let off = 0x20 + i * 4;
        *slot = u32::from_le_bytes(decrypted[off..off + 4].try_into().ok()?);
    }
    // Sanity: every explicit value must fit its expected raw size (LZ4
    // never produces larger output in this pipeline). Bail if not, so
    // we don't munge non-per-mip layouts.
    for (i, &value) in reserved.iter().enumerate().take(max_explicit) {
        if value == 0 {
            continue;
        }
        if value as usize > raw_mip_sizes[i] + 16 {
            return None;
        }
    }

    let body = &decrypted[info.data_offset..];
    let mut pos = 0usize;
    let mut out = Vec::with_capacity(uncompressed_size);
    out.extend_from_slice(&decrypted[..info.data_offset]);

    for lvl in 0..info.mip_count {
        let on_disk = if lvl < max_explicit {
            reserved[lvl] as usize
        } else {
            0
        };
        if on_disk == 0 {
            // Trailing raw mips — the body holds the remaining mip
            // levels stored sequentially without further compression.
            for r in raw_mip_sizes.iter().take(info.mip_count).skip(lvl) {
                if pos + r > body.len() {
                    return None;
                }
                out.extend_from_slice(&body[pos..pos + r]);
                pos += r;
            }
            break;
        }

        if pos + on_disk > body.len() {
            return None;
        }
        let chunk = &body[pos..pos + on_disk];
        pos += on_disk;
        let expected_raw = raw_mip_sizes[lvl];
        if on_disk == expected_raw {
            out.extend_from_slice(chunk);
        } else {
            let decoded = lz4_flex::block::decompress(chunk, expected_raw).ok()?;
            if decoded.len() != expected_raw {
                return None;
            }
            out.extend_from_slice(&decoded);
        }
    }

    if pos != body.len() {
        // Leftover body bytes mean we picked the wrong strategy.
        return None;
    }
    if out.len() != uncompressed_size {
        return None;
    }
    Some(out)
}

/// Minimal DDS header reader, just enough for the per-mip partial
/// decompressor: width, height, mip count, header length, and per-mip
/// raw byte size given (width, height). Covers every format observed
/// in Crimson Desert 1.06's PAZ archives (DXT1/3/5, DX10-wrapped
/// BC1..BC7, packed RGB(A), single-channel luminance, DX10 RGBA8 /
/// R8 / R16F / etc).
#[derive(Debug, Clone, Copy)]
struct DdsInfo {
    width: usize,
    height: usize,
    mip_count: usize,
    data_offset: usize,
    /// Distinguishes between block-compressed (BC*/DXT*), packed BPP,
    /// or DX10 DXGI codes.
    body: DdsBody,
}

#[derive(Debug, Clone, Copy)]
enum DdsBody {
    /// 8-byte 4×4 blocks: BC1 / DXT1 / BC4.
    Block8,
    /// 16-byte 4×4 blocks: BC2/3/5/6/7 / DXT3 / DXT5.
    Block16,
    /// Plain pixels at N bits each.
    PixelsBpp(usize),
}

impl DdsInfo {
    fn parse(data: &[u8]) -> Option<Self> {
        if data.len() < 128 || &data[..4] != b"DDS " {
            return None;
        }
        // The standard header is 124 bytes after the 4-byte magic;
        // dwSize at offset 4 should be 124.
        let flags = u32::from_le_bytes(data[8..12].try_into().ok()?);
        let height = u32::from_le_bytes(data[12..16].try_into().ok()?) as usize;
        let width = u32::from_le_bytes(data[16..20].try_into().ok()?) as usize;
        let mip_count = if flags & 0x00020000 != 0 {
            u32::from_le_bytes(data[28..32].try_into().ok()?) as usize
        } else {
            1
        };
        if width == 0
            || height == 0
            || width > 32768
            || height > 32768
            || mip_count == 0
            || mip_count > 16
        {
            return None;
        }
        // Pixel format at offset 0x4C.
        let pf_flags = u32::from_le_bytes(data[80..84].try_into().ok()?);
        let fourcc = &data[84..88];
        let bpp = u32::from_le_bytes(data[88..92].try_into().ok()?) as usize;

        const DDPF_ALPHAPIXELS: u32 = 0x1;
        const DDPF_FOURCC: u32 = 0x4;
        const DDPF_RGB: u32 = 0x40;
        const DDPF_LUMINANCE: u32 = 0x20000;
        let _ = DDPF_ALPHAPIXELS;

        let mut data_offset = 128usize;
        let body = if pf_flags & DDPF_FOURCC != 0 {
            match fourcc {
                b"DXT1" => DdsBody::Block8,
                b"DXT3" | b"DXT5" | b"BC5U" | b"ATI2" => DdsBody::Block16,
                b"BC4U" | b"ATI1" => DdsBody::Block8,
                b"DX10" => {
                    if data.len() < 148 {
                        return None;
                    }
                    data_offset = 148;
                    let dxgi = u32::from_le_bytes(data[128..132].try_into().ok()?);
                    dxgi_body(dxgi)?
                }
                _ => return None,
            }
        } else if pf_flags & (DDPF_RGB | DDPF_LUMINANCE) != 0 {
            DdsBody::PixelsBpp(bpp)
        } else {
            return None;
        };
        Some(DdsInfo {
            width,
            height,
            mip_count,
            data_offset,
            body,
        })
    }

    fn mip_payload_size(self, width: usize, height: usize) -> Option<usize> {
        match self.body {
            DdsBody::Block8 => {
                let bw = width.div_ceil(4).max(1);
                let bh = height.div_ceil(4).max(1);
                bw.checked_mul(bh)?.checked_mul(8)
            }
            DdsBody::Block16 => {
                let bw = width.div_ceil(4).max(1);
                let bh = height.div_ceil(4).max(1);
                bw.checked_mul(bh)?.checked_mul(16)
            }
            DdsBody::PixelsBpp(bpp) => {
                if bpp == 0 || bpp % 8 != 0 {
                    return None;
                }
                width.checked_mul(height)?.checked_mul(bpp / 8)
            }
        }
    }

    fn expected_total_size(self) -> Option<usize> {
        let mut total = self.data_offset;
        let (mut w, mut h) = (self.width.max(1), self.height.max(1));
        let mips = self.mip_count.max(1);
        for _ in 0..mips {
            total = total.checked_add(self.mip_payload_size(w, h)?)?;
            w = (w / 2).max(1);
            h = (h / 2).max(1);
        }
        Some(total)
    }
}

/// DX10 DXGI_FORMAT → DdsBody. Codes per
/// https://learn.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format
/// trimmed to what Pearl Abyss actually ships in 1.06.
fn dxgi_body(dxgi: u32) -> Option<DdsBody> {
    Some(match dxgi {
        // RGBA8 / BGRA8 32-bit
        28..=31 | 87..=91 => DdsBody::PixelsBpp(32),
        // R10G10B10A2
        24 | 25 => DdsBody::PixelsBpp(32),
        // R16G16B16A16_FLOAT
        10 => DdsBody::PixelsBpp(64),
        // R32G32B32A32_FLOAT
        2 => DdsBody::PixelsBpp(128),
        // R16_FLOAT
        54 | 55 => DdsBody::PixelsBpp(16),
        // R32_FLOAT
        41 | 43 => DdsBody::PixelsBpp(32),
        // R8_UNORM / R8_UINT
        61 | 62 => DdsBody::PixelsBpp(8),
        // Block-compressed
        70..=72 => DdsBody::Block8,  // BC1
        73..=78 => DdsBody::Block16, // BC2 + BC3
        79..=81 => DdsBody::Block8,  // BC4
        82..=84 => DdsBody::Block16, // BC5
        94..=96 => DdsBody::Block16, // BC6H
        97..=99 => DdsBody::Block16, // BC7
        _ => return None,
    })
}

#[cfg(test)]
mod port_tests {
    use super::*;
    #[test]
    fn malicious_dds_dimensions_and_mips_are_rejected() {
        let mut data = [0u8; 128];
        data[..4].copy_from_slice(b"DDS ");
        data[8..12].copy_from_slice(&0x20000u32.to_le_bytes());
        data[12..16].copy_from_slice(&u32::MAX.to_le_bytes());
        data[16..20].copy_from_slice(&u32::MAX.to_le_bytes());
        data[28..32].copy_from_slice(&u32::MAX.to_le_bytes());
        assert!(DdsInfo::parse(&data).is_none());
        assert!(decompress_partial(&data, 512).is_err());
    }
}
