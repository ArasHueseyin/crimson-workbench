//! Offline map-fog reveal for build 2976. No quest or discovery edits.
//! The caller binds the game build and save pair and commits with a backup.
use crate::{Error, Result, fingerprint::hash_bytes};
use crimson_format::save::{Body, DecodedField, FieldKind, FieldValue, ObjectBlock, Save};
use serde::{Deserialize, Serialize};
use std::ops::Range;

const BITMAP_BYTES: usize = 0x80000;
const TOTAL_CELLS: u32 = (BITMAP_BYTES * 8) as u32;

#[derive(Clone, Debug, Deserialize, Serialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct State {
    pub revealed_cells: u32,
    pub total_cells: u32,
}
#[derive(Clone, Debug, Deserialize, Serialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct Plan {
    pub save_sha256: String,
    pub lobby_sha256: String,
    pub plan_id: String,
    pub before: State,
    pub after: State,
}
fn bad(text: &str) -> Error {
    Error::Invalid(text.into())
}
fn checked_tree(object: &ObjectBlock) -> Result<()> {
    if !object.undecoded_ranges.is_empty()
        || object
            .fields
            .iter()
            .any(|f| f.present && f.kind == FieldKind::Unknown)
    {
        return Err(bad("Unknown save fields: no map reveal prepared"));
    }
    for f in &object.fields {
        match &f.value {
            FieldValue::Locator {
                child: Some(child), ..
            } => checked_tree(child)?,
            FieldValue::ObjectList { elements, .. } => {
                for child in elements {
                    checked_tree(child)?;
                }
            }
            _ => {}
        }
    }
    Ok(())
}
fn array<'a>(
    object: &'a ObjectBlock,
    name: &str,
    type_name: &str,
    width: usize,
) -> Result<(&'a DecodedField, &'a [u8], Range<usize>)> {
    let mut fields = object.fields.iter().filter(|f| f.name == name);
    let f = fields
        .next()
        .ok_or_else(|| bad("Map fog array is missing"))?;
    if fields.next().is_some()
        || !f.present
        || f.absent_marker
        || f.kind != FieldKind::DynamicArray
        || f.type_name != type_name
        || f.meta_kind != 3
        || f.meta_size as usize != width
        || f.meta_aux != 4096
    {
        return Err(bad("Unsupported map fog array layout"));
    }
    match &f.value {
        FieldValue::DynamicArray {
            count,
            bytes,
            header_variant,
            header_bytes,
            trailer_bytes,
        } if *count as usize <= BITMAP_BYTES
            && bytes.len() == *count as usize * width
            && *header_variant == "generic"
            && header_bytes.len() == 5
            && header_bytes[0] == 0
            && header_bytes[1..] == count.to_le_bytes()
            && trailer_bytes.is_empty()
            && f.end.checked_sub(f.start) == Some(5 + bytes.len()) =>
        {
            Ok((f, bytes, f.start + 5..f.end))
        }
        _ => Err(bad("Unsupported map fog array encoding")),
    }
}
/// Mirror the native bounded RLE index construction, not a guessed 64-run rule.
fn validate_rle(codes: &[u8], lengths: &[u8], jumps: &[u8]) -> Result<State> {
    if codes.is_empty()
        || codes.len() != lengths.len()
        || codes.len() > BITMAP_BYTES
        || lengths.contains(&0)
        || lengths.iter().map(|v| usize::from(*v)).sum::<usize>() != BITMAP_BYTES
    {
        return Err(bad("Invalid map fog run lengths"));
    }
    let count = (codes.len() / 64).min(1024);
    if jumps.len() != count * 4 || count == 0 {
        return Err(bad("Invalid map fog index size"));
    }
    let stride = codes.len() / count;
    let mut prefix = 0u32;
    let mut next = stride;
    let mut entry = 0;
    for (i, length) in lengths.iter().enumerate() {
        if i == next && entry < count - 1 {
            if jumps[entry * 4..entry * 4 + 4] != prefix.to_le_bytes() {
                return Err(bad("Invalid map fog index"));
            }
            entry += 1;
            next += stride;
        }
        prefix += u32::from(*length);
    }
    if entry != count - 1 || jumps[(count - 1) * 4..] != (BITMAP_BYTES as u32).to_le_bytes() {
        return Err(bad("Invalid final map fog index"));
    }
    Ok(State {
        revealed_cells: codes
            .iter()
            .zip(lengths)
            .map(|(code, length)| code.count_ones() * u32::from(*length))
            .sum(),
        total_cells: TOTAL_CELLS,
    })
}
fn decode(bytes: &[u8]) -> Result<(Save, State, Range<usize>)> {
    let save = Save::parse(bytes).map_err(|e| Error::Invalid(e.to_string()))?;
    if !save.hmac_ok {
        return Err(bad("Save authentication failed"));
    }
    let body = Body::parse(&save.body)?;
    let blocks = body.decode_blocks(&save.body);
    if blocks.len() != body.toc.entries.len()
        || body.toc.entries.len() != body.toc.toc_count as usize
    {
        return Err(bad("Save could not be read completely"));
    }
    for block in &blocks {
        checked_tree(block)?;
    }
    if body.write(&save.body, &blocks)? != save.body {
        return Err(bad("Save roundtrip is not byte-identical"));
    }
    let mut fog = blocks.iter().filter(|b| b.class_name == "FogSaveData");
    let fog_block = fog.next().ok_or_else(|| bad("Map fog block is missing"))?;
    if fog.next().is_some() || fog_block.fields.len() != 1 {
        return Err(bad("Map fog block is ambiguous"));
    }
    let locator = &fog_block.fields[0];
    if locator.name != "_fogRLESaveData"
        || locator.type_name != "ReflectObject"
        || !locator.present
        || locator.absent_marker
        || locator.kind != FieldKind::ObjectLocator
        || locator.meta_kind != 4
        || locator.meta_size != 8
        || locator.meta_aux != 8
    {
        return Err(bad("Unsupported map fog locator"));
    }
    let rle = match &locator.value {
        FieldValue::Locator {
            child: Some(child),
            child_type_name,
            child_reserved: 0,
            child_sentinel1: u32::MAX,
            child_sentinel2: u32::MAX,
            wrapper_prefix,
            ..
        } if child_type_name == "SimpleRunLengthEncodingSaveData"
            && wrapper_prefix.is_empty()
            && child.class_name == "SimpleRunLengthEncodingSaveData"
            && child.fields.len() == 3 =>
        {
            child
        }
        _ => return Err(bad("Unsupported map fog object")),
    };
    let (_, jumps, _) = array(rle, "_jumpingTable", "uint32", 4)?;
    let (_, codes, range) = array(rle, "_codeList", "uint8", 1)?;
    let (_, lengths, _) = array(rle, "_lengthList", "uint8", 1)?;
    if save.body.get(range.clone()) != Some(codes) {
        return Err(bad("Map fog payload location differs"));
    }
    let state = validate_rle(codes, lengths, jumps)?;
    Ok((save, state, range))
}
pub fn inspect(save: &[u8]) -> Result<State> {
    Ok(decode(save)?.1)
}
fn make_plan(save: &[u8], lobby: &[u8], before: State) -> Result<Plan> {
    let lobby_save = Save::parse(lobby).map_err(|e| Error::Invalid(e.to_string()))?;
    if !lobby_save.hmac_ok {
        return Err(bad("Lobby save authentication failed"));
    }
    let save_sha256 = hash_bytes(save);
    let lobby_sha256 = hash_bytes(lobby);
    let plan_id =
        hash_bytes(format!("map-fog-reveal-2976-v1:{save_sha256}:{lobby_sha256}").as_bytes());
    Ok(Plan {
        save_sha256,
        lobby_sha256,
        plan_id,
        before,
        after: State {
            revealed_cells: TOTAL_CELLS,
            total_cells: TOTAL_CELLS,
        },
    })
}
pub fn plan(save: &[u8], lobby: &[u8]) -> Result<Plan> {
    make_plan(save, lobby, inspect(save)?)
}
pub fn build(save: &[u8], lobby: &[u8], expected_plan_id: &str) -> Result<(Vec<u8>, Plan)> {
    let (mut decoded, before, range) = decode(save)?;
    let plan = make_plan(save, lobby, before)?;
    if plan.plan_id != expected_plan_id {
        return Err(bad("Map reveal preview is stale; review this save again"));
    }
    if plan.before == plan.after {
        return Ok((save.to_vec(), plan));
    }
    let original_body = decoded.body.clone();
    decoded.body[range.clone()].fill(255);
    let candidate = decoded
        .write_with_nonce(decoded.header.nonce())
        .map_err(|e| Error::Invalid(e.to_string()))?;
    let (reread, actual, actual_range) = decode(&candidate)?;
    if actual != plan.after || actual_range != range {
        return Err(bad("Map reveal verification failed"));
    }
    let mut inverse = reread.body;
    inverse[range.clone()].copy_from_slice(&original_body[range]);
    if inverse != original_body {
        return Err(bad("Map reveal changed unrelated save bytes"));
    }
    Ok((candidate, plan))
}

#[cfg(test)]
mod tests {
    use super::*;
    fn fixture() -> (Vec<u8>, Vec<u8>, Vec<u8>) {
        let mut lengths = vec![255; BITMAP_BYTES / 255];
        lengths.push((BITMAP_BYTES % 255) as u8);
        let codes = vec![0; lengths.len()];
        let count = (lengths.len() / 64).min(1024);
        let stride = lengths.len() / count;
        let mut jumps = Vec::new();
        for i in 1..count {
            jumps.extend(
                lengths[..i * stride]
                    .iter()
                    .map(|v| u32::from(*v))
                    .sum::<u32>()
                    .to_le_bytes(),
            );
        }
        jumps.extend((BITMAP_BYTES as u32).to_le_bytes());
        (codes, lengths, jumps)
    }
    #[test]
    fn native_rle_boundaries_and_full_reveal_are_checked() {
        let (codes, lengths, jumps) = fixture();
        assert_eq!(
            validate_rle(&codes, &lengths, &jumps)
                .unwrap()
                .revealed_cells,
            0
        );
        assert_eq!(
            validate_rle(&vec![255; codes.len()], &lengths, &jumps)
                .unwrap()
                .revealed_cells,
            TOTAL_CELLS
        );
        for case in 0..5 {
            let (mut codes, mut lengths, mut jumps) = fixture();
            match case {
                0 => {
                    codes.pop();
                }
                1 => lengths[0] = 0,
                2 => lengths[0] = 254,
                3 => jumps[0] ^= 1,
                _ => {
                    jumps.pop();
                }
            }
            assert!(validate_rle(&codes, &lengths, &jumps).is_err());
        }
        assert!(inspect(b"not a save").is_err());
    }
    #[test]
    fn native_index_caps_at_1024_with_a_variable_run_stride() {
        let lengths = vec![1; BITMAP_BYTES];
        let codes = vec![1; BITMAP_BYTES];
        let mut jumps = Vec::new();
        for i in 1..1024u32 {
            jumps.extend((i * 512).to_le_bytes());
        }
        jumps.extend((BITMAP_BYTES as u32).to_le_bytes());
        assert_eq!(
            validate_rle(&codes, &lengths, &jumps)
                .unwrap()
                .revealed_cells,
            BITMAP_BYTES as u32
        );
        jumps[4] ^= 1;
        assert!(validate_rle(&codes, &lengths, &jumps).is_err());
    }
    #[test]
    #[ignore = "requires a private historical save; all transforms remain in memory"]
    fn private_fog_only_roundtrip_is_exact_and_bound_to_both_files() {
        let path = std::path::PathBuf::from(
            std::env::var_os("CD_MAP_FOG_FIXTURE").expect("private fixture"),
        );
        let source = std::fs::read(&path).unwrap();
        let lobby = std::fs::read(path.with_file_name("lobby.save")).unwrap();
        let review = plan(&source, &lobby).unwrap();
        assert!(review.before.revealed_cells > 0 && review.before.revealed_cells < TOTAL_CELLS);
        let (candidate, actual) = build(&source, &lobby, &review.plan_id).unwrap();
        assert_eq!(actual, review);
        assert_eq!(inspect(&candidate).unwrap(), review.after);
        let repeated = plan(&candidate, &lobby).unwrap();
        assert_eq!(
            build(&candidate, &lobby, &repeated.plan_id).unwrap().0,
            candidate
        );
        assert!(build(&candidate, &lobby, &review.plan_id).is_err());
        let decoded_lobby = Save::parse(&lobby).unwrap();
        let mut different_nonce = decoded_lobby.header.nonce();
        different_nonce[0] ^= 1;
        let other_lobby = decoded_lobby.write_with_nonce(different_nonce).unwrap();
        assert!(Save::parse(&other_lobby).unwrap().hmac_ok);
        assert!(build(&source, &other_lobby, &review.plan_id).is_err());
        let mut bad_lobby = lobby.clone();
        bad_lobby[0] ^= 1;
        assert!(plan(&source, &bad_lobby).is_err());
        assert_eq!(std::fs::read(&path).unwrap(), source);
        assert_eq!(
            std::fs::read(path.with_file_name("lobby.save")).unwrap(),
            lobby
        );
    }
}
