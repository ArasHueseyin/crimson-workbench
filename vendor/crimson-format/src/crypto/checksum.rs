// Adapted from crimson-rs b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0.
// Copyright (c) 2026 Tommy Tran. MIT; see LICENSE and docs/FORMAT_PORT.md.
//! Jenkins "hashlittle2" hash with custom constant 0xDEBA1DCD.
//!
//! This is used by the Crimson Desert file formats for checksum validation
//! of .papgt and .pamt files, as well as directory name verification.

#[inline]
fn rot(x: u32, k: u32) -> u32 {
    x.rotate_left(k)
}

#[inline]
fn mix(mut a: u32, mut b: u32, mut c: u32) -> (u32, u32, u32) {
    a = (a.wrapping_sub(c)) ^ rot(c, 4);
    c = c.wrapping_add(b);
    b = (b.wrapping_sub(a)) ^ rot(a, 6);
    a = a.wrapping_add(c);
    c = (c.wrapping_sub(b)) ^ rot(b, 8);
    b = b.wrapping_add(a);
    a = (a.wrapping_sub(c)) ^ rot(c, 16);
    c = c.wrapping_add(b);
    b = (b.wrapping_sub(a)) ^ rot(a, 19);
    a = a.wrapping_add(c);
    c = (c.wrapping_sub(b)) ^ rot(b, 4);
    b = b.wrapping_add(a);
    (a, b, c)
}

#[inline]
fn finalize(mut a: u32, mut b: u32, mut c: u32) -> (u32, u32, u32) {
    c = (c ^ b).wrapping_sub(rot(b, 14));
    a = (a ^ c).wrapping_sub(rot(c, 11));
    b = (b ^ a).wrapping_sub(rot(a, 25));
    c = (c ^ b).wrapping_sub(rot(b, 16));
    a = (a ^ c).wrapping_sub(rot(c, 4));
    b = (b ^ a).wrapping_sub(rot(a, 14));
    c = (c ^ b).wrapping_sub(rot(b, 24));
    (a, b, c)
}

pub fn calculate_checksum(data: &[u8]) -> u32 {
    let length = data.len() as u32;
    let init = length.wrapping_add(0xDEBA1DCD);
    let mut a = init;
    let mut b = init;
    let mut c = init;

    let mut offset = 0usize;
    let mut remaining = data.len();

    // Process 12-byte blocks
    while remaining > 12 {
        let w0 = u32::from_le_bytes(data[offset..offset + 4].try_into().unwrap());
        let w1 = u32::from_le_bytes(data[offset + 4..offset + 8].try_into().unwrap());
        let w2 = u32::from_le_bytes(data[offset + 8..offset + 12].try_into().unwrap());
        a = a.wrapping_add(w0);
        b = b.wrapping_add(w1);
        c = c.wrapping_add(w2);
        (a, b, c) = mix(a, b, c);
        offset += 12;
        remaining -= 12;
    }

    if remaining == 0 {
        return c;
    }

    // Process tail: pad with zeros to 12 bytes
    let mut tail = [0u8; 12];
    tail[..remaining].copy_from_slice(&data[offset..offset + remaining]);
    let w0 = u32::from_le_bytes(tail[0..4].try_into().unwrap());
    let w1 = u32::from_le_bytes(tail[4..8].try_into().unwrap());
    let w2 = u32::from_le_bytes(tail[8..12].try_into().unwrap());
    a = a.wrapping_add(w0);
    b = b.wrapping_add(w1);
    c = c.wrapping_add(w2);

    let (_, _, c) = finalize(a, b, c);
    c
}
