//! Offline contribution-wallet edits for build 1.0.0.2976.
//!
//! This module transforms bytes only. The caller must verify the game build,
//! require a closed game, bind the selected save pair, and use the existing
//! backed-up save transaction. No items, slots, UIDs, experience or levels are
//! created. See `docs/research/CONTRIBUTION_VALUES.md` for native evidence.
use crate::{Error, Result};
use crimson_format::save::{
    Body, DecodedField, FieldKind, FieldValue, ObjectBlock, Save, ScalarValue,
};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::collections::{BTreeMap, BTreeSet};

pub const MAX_AMOUNT: u64 = 9_000_000_000_000_000_000;
const TYPES: [(u32, &str); 6] = [
    (1_000_658, "Graymane"),
    (1_001_753, "Demeniss"),
    (1_001_754, "Hernand"),
    (1_001_755, "Pailune"),
    (1_001_759, "Delesyia"),
    (1_003_749, "Tashkalp"),
];

#[derive(Clone, Debug, Deserialize, Serialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct Edit {
    pub key: u32,
    /// Exact decimal text; JSON numbers cannot represent this full range.
    pub target: String,
}

#[derive(Clone, Debug, Deserialize, Serialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct Balance {
    pub key: u32,
    pub name: String,
    /// None means no existing wallet entry. Creating one is unsupported.
    pub amount: Option<String>,
}

#[derive(Clone, Debug, Deserialize, Serialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct Change {
    pub key: u32,
    pub name: String,
    pub before: String,
    pub after: String,
}

#[derive(Clone, Debug, Deserialize, Serialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct Plan {
    pub save_sha256: String,
    pub lobby_sha256: String,
    pub plan_id: String,
    pub balances: Vec<Balance>,
    /// Includes selected unchanged entries so the review states exact targets.
    pub changes: Vec<Change>,
}

fn bad(message: impl Into<String>) -> Error {
    Error::Invalid(message.into())
}
fn hash(bytes: &[u8]) -> String {
    format!("{:x}", Sha256::digest(bytes))
}
fn name(key: u32) -> Result<&'static str> {
    TYPES
        .iter()
        .find(|(k, _)| *k == key)
        .map(|(_, n)| *n)
        .ok_or_else(|| bad("Unknown contribution type"))
}
fn amount(text: &str) -> Result<u64> {
    if text.is_empty()
        || text.len() > 19
        || text.starts_with('0')
        || !text.bytes().all(|c| c.is_ascii_digit())
    {
        return Err(bad(
            "Enter a whole contribution amount from 1 to 9000000000000000000",
        ));
    }
    let value = text
        .parse::<u64>()
        .map_err(|_| bad("Contribution amount is out of range"))?;
    if !(1..=MAX_AMOUNT).contains(&value) {
        return Err(bad("Contribution amount is out of range"));
    }
    Ok(value)
}
fn normalized(edits: &[Edit]) -> Result<BTreeMap<u32, u64>> {
    if edits.is_empty() || edits.len() > TYPES.len() {
        return Err(bad("Select between one and six contribution balances"));
    }
    let mut result = BTreeMap::new();
    for edit in edits {
        name(edit.key)?;
        if result.insert(edit.key, amount(&edit.target)?).is_some() {
            return Err(bad("Duplicate contribution target"));
        }
    }
    Ok(result)
}
fn field<'a>(object: &'a ObjectBlock, key: &str) -> Result<&'a DecodedField> {
    let mut matches = object.fields.iter().filter(|f| f.name == key);
    let first = matches
        .next()
        .ok_or_else(|| bad(format!("Save field missing: {key}")))?;
    if matches.next().is_some() {
        return Err(bad(format!("Duplicate save field: {key}")));
    }
    Ok(first)
}
fn scalar<'a>(
    object: &'a ObjectBlock,
    key: &str,
    type_name: &str,
    width: usize,
) -> Result<&'a DecodedField> {
    let f = field(object, key)?;
    if !f.present
        || f.kind != FieldKind::FixedPrefix
        || f.type_name != type_name
        || f.meta_kind != 0
        || f.meta_size as usize != width
        || f.meta_aux != 0
        || f.end.checked_sub(f.start) != Some(width)
        || f.absent_marker
    {
        return Err(bad(format!("Unsupported contribution save field: {key}")));
    }
    Ok(f)
}
fn unsigned(f: &DecodedField) -> Result<u64> {
    match &f.value {
        FieldValue::Scalar(ScalarValue::U16(v)) => Ok(u64::from(*v)),
        FieldValue::Scalar(ScalarValue::U32(v)) => Ok(u64::from(*v)),
        FieldValue::Scalar(ScalarValue::U64(v)) => Ok(*v),
        _ => Err(bad(format!("Invalid contribution scalar: {}", f.name))),
    }
}
fn list<'a>(object: &'a ObjectBlock, key: &str) -> Result<&'a [ObjectBlock]> {
    let f = field(object, key)?;
    match &f.value {
        FieldValue::ObjectList {
            count, elements, ..
        } if f.present && f.kind == FieldKind::ObjectList && *count as usize == elements.len() => {
            Ok(elements)
        }
        _ => Err(bad(format!("Invalid contribution list: {key}"))),
    }
}
fn checked_tree(object: &ObjectBlock) -> Result<()> {
    if !object.undecoded_ranges.is_empty()
        || object
            .fields
            .iter()
            .any(|f| f.present && f.kind == FieldKind::Unknown)
    {
        return Err(bad("Unknown save fields: no contribution changes prepared"));
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

struct Located {
    value: u64,
    start: usize,
    end: usize,
}
fn decode(bytes: &[u8]) -> Result<(Save, BTreeMap<u32, Located>)> {
    let save = Save::parse(bytes).map_err(|e| bad(e.to_string()))?;
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
    let inventories: Vec<_> = blocks
        .iter()
        .filter(|b| b.class_name == "InventorySaveData")
        .collect();
    if inventories.len() != 1 {
        return Err(bad("Inventory save block is missing or ambiguous"));
    }
    let containers = list(inventories[0], "_inventorylist")?;
    let mut wallet = None;
    let mut container_keys = BTreeSet::new();
    for container in containers {
        if container.class_name != "InventoryElementSaveData" {
            return Err(bad("Unknown inventory container"));
        }
        let key = unsigned(scalar(container, "_inventoryKey", "InventoryKey", 2)?)?;
        if !container_keys.insert(key) {
            return Err(bad("Duplicate inventory container"));
        }
        if key == 1 {
            wallet = Some(container);
        }
    }
    let wallet = wallet.ok_or_else(|| bad("Money inventory is missing"))?;
    let mut balances = BTreeMap::new();
    let mut slots = BTreeSet::new();
    let mut uids = BTreeSet::new();
    for item in list(wallet, "_itemList")? {
        if item.class_name != "ItemSaveData" {
            return Err(bad("Unknown money entry"));
        }
        let key = unsigned(scalar(item, "_itemKey", "ItemKey", 4)?)?;
        let key = u32::try_from(key).map_err(|_| bad("Invalid money item key"))?;
        let slot = unsigned(scalar(item, "_slotNo", "TItemSlotNo", 2)?)?;
        let uid = unsigned(scalar(item, "_itemNo", "ItemNo", 8)?)?;
        if !slots.insert(slot) || !uids.insert(uid) {
            return Err(bad("Ambiguous money inventory slots or IDs"));
        }
        if !TYPES.iter().any(|(k, _)| *k == key) {
            continue;
        }
        if unsigned(scalar(item, "_saveVersion", "uint32", 4)?)? != 1
            || unsigned(scalar(item, "_transferredItemKey", "ItemKey", 4)?)? != u64::from(key)
            || uid == 0
            || uid == u64::MAX
            || slot == u64::from(u16::MAX)
        {
            return Err(bad("Unsupported contribution wallet entry"));
        }
        let stack = scalar(item, "_stackCount", "TStackCount", 8)?;
        let value = unsigned(stack)?;
        if value > MAX_AMOUNT
            || value == 0
            || stack.end > save.body.len()
            || save.body[stack.start..stack.end] != value.to_le_bytes()
        {
            return Err(bad("Contribution balance is outside the verified range"));
        }
        if balances
            .insert(
                key,
                Located {
                    value,
                    start: stack.start,
                    end: stack.end,
                },
            )
            .is_some()
        {
            return Err(bad("Duplicate contribution wallet entry"));
        }
    }
    Ok((save, balances))
}
fn descriptions(located: &BTreeMap<u32, Located>) -> Vec<Balance> {
    TYPES
        .iter()
        .map(|(key, name)| Balance {
            key: *key,
            name: (*name).to_owned(),
            amount: located.get(key).map(|v| v.value.to_string()),
        })
        .collect()
}

pub fn inspect(save: &[u8]) -> Result<Vec<Balance>> {
    let (_, located) = decode(save)?;
    Ok(descriptions(&located))
}
fn make_plan(
    save: &[u8],
    lobby: &[u8],
    edits: &BTreeMap<u32, u64>,
    located: &BTreeMap<u32, Located>,
) -> Result<Plan> {
    let lobby_save = Save::parse(lobby).map_err(|e| bad(e.to_string()))?;
    if !lobby_save.hmac_ok {
        return Err(bad("Lobby save authentication failed"));
    }
    let save_sha256 = hash(save);
    let lobby_sha256 = hash(lobby);
    let changes = edits
        .iter()
        .map(|(key, after)| {
            let before = located.get(key).ok_or_else(|| {
                bad(format!(
                    "{} has no existing contribution wallet entry",
                    name(*key).unwrap_or("This faction")
                ))
            })?;
            if *after < before.value {
                return Err(bad(
                    "Contribution targets must keep or increase the current balance",
                ));
            }
            Ok(Change {
                key: *key,
                name: name(*key)?.to_owned(),
                before: before.value.to_string(),
                after: after.to_string(),
            })
        })
        .collect::<Result<Vec<_>>>()?;
    let exact_edits: Vec<_> = edits.iter().map(|(k, v)| (*k, v.to_string())).collect();
    let plan_id = hash(
        format!(
            "contribution-wallet-2976-v1:{save_sha256}:{lobby_sha256}:{}",
            serde_json::to_string(&exact_edits)?
        )
        .as_bytes(),
    );
    Ok(Plan {
        save_sha256,
        lobby_sha256,
        plan_id,
        balances: descriptions(located),
        changes,
    })
}
pub fn plan(save: &[u8], lobby: &[u8], edits: &[Edit]) -> Result<Plan> {
    let edits = normalized(edits)?;
    let (_, located) = decode(save)?;
    make_plan(save, lobby, &edits, &located)
}

/// Rebuild from a freshly read save pair and the previously reviewed plan ID.
/// The returned save is private candidate bytes, not a filesystem write.
pub fn build(
    save: &[u8],
    lobby: &[u8],
    edits: &[Edit],
    expected_plan_id: &str,
) -> Result<(Vec<u8>, Plan)> {
    let edits = normalized(edits)?;
    let (mut decoded, located) = decode(save)?;
    let review = make_plan(save, lobby, &edits, &located)?;
    if review.plan_id != expected_plan_id {
        return Err(bad(
            "Contribution preview is stale; review the save and amounts again",
        ));
    }
    if edits
        .iter()
        .all(|(key, value)| located[key].value == *value)
    {
        return Ok((save.to_vec(), review));
    }
    let original_body = decoded.body.clone();
    for (key, value) in &edits {
        let target = &located[key];
        decoded.body[target.start..target.end].copy_from_slice(&value.to_le_bytes());
    }
    let candidate = decoded
        .write_with_nonce(decoded.header.nonce())
        .map_err(|e| bad(e.to_string()))?;
    let (reread, actual) = decode(&candidate)?;
    let mut inverse = reread.body;
    for (key, value) in &edits {
        let after = &actual[key];
        let before = &located[key];
        if after.value != *value || after.start != before.start || after.end != before.end {
            return Err(bad("Contribution candidate verification failed"));
        }
        inverse[after.start..after.end].copy_from_slice(&before.value.to_le_bytes());
    }
    if inverse != original_body {
        return Err(bad("Contribution candidate changed unrelated save bytes"));
    }
    Ok((candidate, review))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn decimal_targets_are_exact_bounded_and_unique() {
        for v in ["1", "100", "9007199254740993", "9000000000000000000"] {
            assert_eq!(amount(v).unwrap().to_string(), v);
        }
        for v in [
            "",
            "0",
            "00",
            "01",
            "-1",
            "+1",
            " 1",
            "1 ",
            "1.0",
            "1e3",
            "９",
            "9000000000000000001",
            "18446744073709551615",
        ] {
            assert!(amount(v).is_err(), "{v}");
        }
        let one = Edit {
            key: 1_000_658,
            target: "9007199254740993".into(),
        };
        assert!(normalized(&[]).is_err());
        assert!(
            normalized(&[Edit {
                key: 1,
                target: "1".into()
            }])
            .is_err()
        );
        assert!(normalized(&[one.clone(), one.clone()]).is_err());
        assert_eq!(
            serde_json::to_string(&one).unwrap(),
            r#"{"key":1000658,"target":"9007199254740993"}"#
        );
        assert!(
            serde_json::from_str::<Edit>(r#"{"key":1000658,"target":9007199254740993}"#).is_err()
        );
        assert!(inspect(b"not a save").is_err());
    }

    #[test]
    #[ignore = "requires an existing private historical save fixture; no live save access"]
    fn private_wallet_set_is_exact_reversible_and_bound_to_both_files() {
        let path = std::path::PathBuf::from(
            std::env::var_os("CD_CONTRIBUTION_FIXTURE").expect("private fixture required"),
        );
        let source = std::fs::read(&path).unwrap();
        let lobby = std::fs::read(path.with_file_name("lobby.save")).unwrap();
        let (original, located) = decode(&source).unwrap();
        assert_eq!(located.len(), 6);
        let selected: Vec<_> = TYPES
            .iter()
            .map(|(key, _)| Edit {
                key: *key,
                target: MAX_AMOUNT.to_string(),
            })
            .collect();
        let review = plan(&source, &lobby, &selected).unwrap();
        let (candidate, receipt) = build(&source, &lobby, &selected, &review.plan_id).unwrap();
        assert_eq!(receipt, review);
        assert!(
            inspect(&candidate)
                .unwrap()
                .iter()
                .all(|b| b.amount.as_deref() == Some("9000000000000000000"))
        );
        let inverse_edits: Vec<_> = located
            .iter()
            .map(|(key, v)| Edit {
                key: *key,
                target: v.value.to_string(),
            })
            .collect();
        assert!(plan(&candidate, &lobby, &inverse_edits).is_err());
        let mut inverse = Save::parse(&candidate).unwrap();
        for previous in located.values() {
            inverse.body[previous.start..previous.end]
                .copy_from_slice(&previous.value.to_le_bytes());
        }
        assert_eq!(inverse.body, original.body);
        let second = plan(&candidate, &lobby, &selected).unwrap();
        assert_eq!(
            build(&candidate, &lobby, &selected, &second.plan_id)
                .unwrap()
                .0,
            candidate
        );
        let single = vec![Edit {
            key: 1_000_658,
            target: "9007199254740993".into(),
        }];
        let single_plan = plan(&source, &lobby, &single).unwrap();
        let (single_candidate, _) = build(&source, &lobby, &single, &single_plan.plan_id).unwrap();
        let after = inspect(&single_candidate).unwrap();
        for before in inspect(&source).unwrap() {
            let actual = after.iter().find(|b| b.key == before.key).unwrap();
            assert_eq!(
                actual.amount,
                if before.key == 1_000_658 {
                    Some("9007199254740993".into())
                } else {
                    before.amount
                }
            );
        }
        assert!(build(&source, &lobby, &single, &review.plan_id).is_err());
        assert!(build(&candidate, &lobby, &selected, &review.plan_id).is_err());
        let mut reordered = selected.clone();
        reordered.reverse();
        assert_eq!(
            plan(&source, &lobby, &reordered).unwrap().plan_id,
            review.plan_id
        );
        let mut other_lobby = Save::parse(&lobby).unwrap();
        other_lobby.header.set_nonce([7; 16]);
        let other_lobby = other_lobby
            .write_with_nonce(other_lobby.header.nonce())
            .unwrap();
        assert!(build(&source, &other_lobby, &selected, &review.plan_id).is_err());
        let mut corrupt = source.clone();
        corrupt[0x2a] ^= 1;
        assert!(inspect(&corrupt).is_err());
        let stack = &located[&1_000_658];
        for invalid in [0, MAX_AMOUNT + 1, u64::MAX] {
            let mut altered = original.clone();
            altered.body[stack.start..stack.end].copy_from_slice(&invalid.to_le_bytes());
            assert!(inspect(&altered.write_with_nonce(altered.header.nonce()).unwrap()).is_err());
        }
        // Simulate an absent earned wallet type without adding or removing any object.
        let body = Body::parse(&original.body).unwrap();
        let blocks = body.decode_blocks(&original.body);
        let inv = blocks
            .iter()
            .find(|b| b.class_name == "InventorySaveData")
            .unwrap();
        let wallet = list(inv, "_inventorylist")
            .unwrap()
            .iter()
            .find(|b| unsigned(field(b, "_inventoryKey").unwrap()).unwrap() == 1)
            .unwrap();
        let rows = list(wallet, "_itemList").unwrap();
        let row = rows
            .iter()
            .find(|b| unsigned(field(b, "_itemKey").unwrap()).unwrap() == 1_000_658)
            .unwrap();
        let mut missing = original.clone();
        for key in ["_itemKey", "_transferredItemKey"] {
            let f = field(row, key).unwrap();
            missing.body[f.start..f.end].copy_from_slice(&999_999u32.to_le_bytes());
        }
        let missing = missing.write_with_nonce(missing.header.nonce()).unwrap();
        assert!(
            inspect(&missing)
                .unwrap()
                .iter()
                .find(|b| b.key == 1_000_658)
                .unwrap()
                .amount
                .is_none()
        );
        assert!(plan(&missing, &lobby, &single).is_err());
        let another = rows
            .iter()
            .find(|b| unsigned(field(b, "_itemKey").unwrap()).unwrap() == 1_001_754)
            .unwrap();
        let mut duplicate = original.clone();
        for key in ["_itemKey", "_transferredItemKey"] {
            let f = field(another, key).unwrap();
            duplicate.body[f.start..f.end].copy_from_slice(&1_000_658u32.to_le_bytes());
        }
        assert!(
            inspect(
                &duplicate
                    .write_with_nonce(duplicate.header.nonce())
                    .unwrap()
            )
            .is_err()
        );
    }
}
