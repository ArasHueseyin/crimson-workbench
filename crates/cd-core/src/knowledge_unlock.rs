//! Build-2976 journal-only unlocks, calculated from each selected save.
//! The recipe contains numeric keys and levels, never game text or save data.
use crate::{
    Error, Result,
    fingerprint::hash_bytes,
    mounts::{
        edit::{decode, field_mut, number, set_number, update_count},
        files::{SaveChoice, SaveEditBinding, choices, edit_save_values, read_pair},
    },
};
use crimson_format::save::{
    Body, DecodedField, FieldKind, FieldValue, ObjectBlock, ObjectLocatorWrapper, Save, ScalarValue,
};
use serde::{Deserialize, Serialize};
use std::{
    collections::{BTreeMap, BTreeSet},
    path::{Path, PathBuf},
};

const RECIPE: &str = include_str!("../schemas/journal-knowledge-2976.json");
fn bad(message: &str) -> Error {
    Error::Invalid(message.into())
}
fn targets() -> Result<Vec<(u32, u32)>> {
    let keys: Vec<(u32, u32)> = serde_json::from_str(RECIPE)?;
    if keys.len() != 3939
        || keys.iter().map(|k| k.0).collect::<BTreeSet<_>>().len() != keys.len()
        || keys
            .iter()
            .any(|(key, level)| *key == 0 || ![1, 2, 3, 5].contains(level))
    {
        return Err(bad("Invalid journal knowledge recipe"));
    }
    Ok(keys)
}
#[derive(Clone, Debug, Deserialize, Serialize, PartialEq, Eq)]
pub struct Summary {
    pub total: usize,
    pub new_entries: usize,
    pub upgraded_entries: usize,
    pub already_complete: usize,
}
#[derive(Clone, Debug, Deserialize, Serialize, PartialEq, Eq)]
pub struct Plan {
    pub save_sha256: String,
    pub lobby_sha256: String,
    pub plan_id: String,
    pub summary: Summary,
}
#[derive(Serialize)]
pub struct Preview {
    pub save_root: PathBuf,
    pub saves: Vec<SaveChoice>,
    pub selected_save: Option<String>,
    pub plan: Option<Plan>,
    pub game_running: bool,
}
#[derive(Clone, Debug, Deserialize, Serialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct Request {
    pub save_root: PathBuf,
    pub id: String,
    pub save: String,
    pub save_sha256: String,
    pub lobby_sha256: String,
    pub plan_id: String,
}
pub type Receipt = crate::save_action::Receipt<Request, Plan>;

fn entries(block: &ObjectBlock) -> Result<&[ObjectBlock]> {
    let f = block
        .fields
        .iter()
        .find(|f| f.name == "_list")
        .ok_or_else(|| bad("Knowledge list missing"))?;
    if f.meta_kind != 6 || f.meta_size != 0 || f.meta_aux != 4104 {
        return Err(bad("Unsupported knowledge list layout"));
    }
    match &f.value {
        FieldValue::ObjectList {
            count, elements, ..
        } if f.present && *count as usize == elements.len() => Ok(elements),
        FieldValue::None if !f.present => Ok(&[]),
        _ => Err(bad("Invalid knowledge list")),
    }
}
fn blank(body: &Body) -> Result<ObjectBlock> {
    let types: Vec<_> = body
        .schema
        .types
        .iter()
        .filter(|t| t.name == "KnowledgeElementSaveData")
        .collect();
    if types.len() != 1 {
        return Err(bad("Knowledge entry schema is ambiguous"));
    }
    let ty = types[0];
    let shape = [
        ("_key", "KnowledgeKey", 4),
        ("_level", "TLevel", 4),
        ("_learnedFieldTime", "uint64", 8),
        ("_isNewMark", "bool", 1),
    ];
    if ty.fields.len() != shape.len() || ty.index > u16::MAX as u32 {
        return Err(bad("Unsupported knowledge entry schema"));
    }
    let mut fields = Vec::new();
    for (i, (f, (name, kind, size))) in ty.fields.iter().zip(shape).enumerate() {
        if f.name != name
            || f.type_name != kind
            || f.meta_kind != 0
            || f.meta_size != size
            || f.meta_aux != 0
        {
            return Err(bad("Unsupported knowledge entry fields"));
        }
        fields.push(DecodedField {
            field_index: i as u32,
            name: f.name.clone(),
            type_name: f.type_name.clone(),
            meta_kind: 0,
            meta_size: size,
            meta_aux: 0,
            present: i < 3,
            absent_marker: false,
            kind: if i < 3 {
                FieldKind::FixedPrefix
            } else {
                FieldKind::Absent
            },
            value: match i {
                0 | 1 => FieldValue::Scalar(ScalarValue::U32(0)),
                2 => FieldValue::Scalar(ScalarValue::U64(0)),
                _ => FieldValue::None,
            },
            start: 0,
            end: 0,
            note: String::new(),
        });
    }
    Ok(ObjectBlock {
        class_index: ty.index,
        class_name: ty.name.clone(),
        data_offset: 0,
        data_size: 0,
        mask_byte_count: 1,
        mask_bytes: vec![7],
        reserved_u32: 0,
        fields,
        trailing_pad: vec![],
        undecoded_ranges: vec![],
        locator_wrapper: Some(ObjectLocatorWrapper {
            type_index: ty.index as u16,
            child_reserved: 0,
            sentinel1: u32::MAX,
            sentinel2: u32::MAX,
            payload_offset: 0,
        }),
    })
}
fn transform(bytes: &[u8]) -> Result<(Vec<u8>, Summary)> {
    let recipe = targets()?;
    let (mut save, body, mut blocks) = decode(bytes)?;
    if !save.hmac_ok {
        return Err(bad("Invalid save authentication"));
    }
    let indices: Vec<_> = blocks
        .iter()
        .enumerate()
        .filter(|(_, b)| b.class_name == "KnowledgeSaveData")
        .map(|(i, _)| i)
        .collect();
    if indices.len() != 1 {
        return Err(bad("Knowledge save data is ambiguous"));
    }
    let bi = indices[0];
    let original = blocks[bi].clone();
    let mut known = BTreeMap::new();
    let mut timestamp = 0;
    for (i, e) in entries(&original)?.iter().enumerate() {
        if e.class_name != "KnowledgeElementSaveData"
            || known.insert(number(e, "_key")?, i).is_some()
        {
            return Err(bad("Invalid or duplicate knowledge entry"));
        }
        timestamp = timestamp.max(number(e, "_learnedFieldTime")?);
    }
    let template = blank(&body)?;
    let mut summary = Summary {
        total: recipe.len(),
        new_entries: 0,
        upgraded_entries: 0,
        already_complete: 0,
    };
    let block = &mut blocks[bi];
    if !field_mut(block, "_list")?.present {
        let index = field_mut(block, "_list")?.field_index as usize;
        if index / 8 >= block.mask_bytes.len() {
            return Err(bad("Invalid knowledge presence mask"));
        }
        block.mask_bytes[index / 8] |= 1 << (index % 8);
        let f = field_mut(block, "_list")?;
        f.present = true;
        f.absent_marker = false;
        f.kind = FieldKind::ObjectList;
        f.value = FieldValue::ObjectList {
            count: 0,
            elements: vec![],
            header_variant: "zero4_count_u32",
            header_bytes: vec![0; 18],
        };
    }
    let FieldValue::ObjectList {
        count,
        elements,
        header_bytes,
        header_variant,
    } = &mut field_mut(block, "_list")?.value
    else {
        return Err(bad("Knowledge list missing"));
    };
    for (key, maximum) in recipe {
        if let Some(i) = known.get(&u64::from(key)) {
            if number(&elements[*i], "_level")? >= u64::from(maximum) {
                summary.already_complete += 1;
                continue;
            }
            set_number(&mut elements[*i], "_level", maximum.into())?;
            summary.upgraded_entries += 1;
        } else {
            let mut entry = template.clone();
            set_number(&mut entry, "_key", key.into())?;
            set_number(&mut entry, "_level", maximum.into())?;
            set_number(&mut entry, "_learnedFieldTime", timestamp)?;
            elements.push(entry);
            summary.new_entries += 1;
        }
    }
    if summary.already_complete == summary.total {
        return Ok((bytes.to_vec(), summary));
    }
    *count = u32::try_from(elements.len()).map_err(|_| bad("Knowledge list too large"))?;
    update_count(header_bytes, header_variant, *count)?;
    let candidate_body = body.write(&save.body, &blocks)?;
    blocks[bi] = original;
    if body.write(&save.body, &blocks)? != save.body {
        return Err(bad("Knowledge unlock whole-save inverse failed"));
    }
    save.body = candidate_body;
    let candidate = save
        .write_with_nonce(save.header.nonce())
        .map_err(|e| bad(&e.to_string()))?;
    let (after, _, actual) = decode(&candidate)?;
    if after.body != save.body {
        return Err(bad("Knowledge unlock encrypted readback failed"));
    }
    let levels: BTreeMap<_, _> = entries(&actual[bi])?
        .iter()
        .map(|e| Ok((number(e, "_key")?, number(e, "_level")?)))
        .collect::<Result<_>>()?;
    if targets()?.iter().any(|(key, max)| {
        levels
            .get(&u64::from(*key))
            .is_none_or(|n| *n < u64::from(*max))
    }) {
        return Err(bad("Knowledge unlock verification failed"));
    }
    Ok((candidate, summary))
}
fn identity(save: &[u8], lobby: &[u8]) -> String {
    hash_bytes(
        format!(
            "journal-2976-v1:{}:{}:{}",
            hash_bytes(RECIPE.as_bytes()),
            hash_bytes(save),
            hash_bytes(lobby)
        )
        .as_bytes(),
    )
}
pub fn plan(save: &[u8], lobby: &[u8]) -> Result<Plan> {
    if !Save::parse(lobby).map_err(|e| bad(&e.to_string()))?.hmac_ok {
        return Err(bad("Invalid lobby authentication"));
    }
    let (_, summary) = transform(save)?;
    Ok(Plan {
        save_sha256: hash_bytes(save),
        lobby_sha256: hash_bytes(lobby),
        plan_id: identity(save, lobby),
        summary,
    })
}
pub fn build(save: &[u8], lobby: &[u8], expected: &str) -> Result<(Vec<u8>, Plan)> {
    if identity(save, lobby) != expected {
        return Err(bad("Knowledge preview is stale. Review this save again."));
    }
    let review = plan(save, lobby)?;
    let (candidate, _) = transform(save)?;
    Ok((candidate, review))
}
fn validate_game(game: &Path) -> Result<()> {
    if crate::fingerprint::known_for_game(game)?.exe_version != "1.0.0.2976" {
        return Err(bad("Knowledge unlock supports game build 1.0.0.2976"));
    }
    Ok(())
}
pub fn preview(game: &Path, root: &Path, selected: Option<&str>) -> Result<Preview> {
    validate_game(game)?;
    let plan = selected
        .map(|id| {
            let (save, lobby) = read_pair(root, id)?;
            plan(&save, &lobby)
        })
        .transpose()?;
    Ok(Preview {
        save_root: root.canonicalize()?,
        saves: choices(root)?,
        selected_save: selected.map(str::to_owned),
        plan,
        game_running: crate::apply::guard::game_running()?,
    })
}
pub fn apply(game: &Path, project: &Path, root: &Path, request: &Request) -> Result<Receipt> {
    validate_game(game)?;
    if request.id.len() != 32
        || !request.id.bytes().all(|b| b.is_ascii_hexdigit())
        || [
            &request.save_sha256,
            &request.lobby_sha256,
            &request.plan_id,
        ]
        .iter()
        .any(|v| v.len() != 64 || !v.bytes().all(|b| b.is_ascii_hexdigit()))
    {
        return Err(bad("Invalid knowledge unlock request"));
    }
    edit_save_values(
        game,
        project,
        root,
        request,
        SaveEditBinding {
            id: &request.id,
            save: &request.save,
            root: &request.save_root,
            save_sha256: &request.save_sha256,
            lobby_sha256: &request.lobby_sha256,
            history: "knowledge-unlocks",
            message: "Journal knowledge unlocked in this save. Existing knowledge, skill points, inventory and unrelated save records are preserved. Load this save in game.",
        },
        |save, lobby| {
            let (candidate, plan) = build(save, lobby, &request.plan_id)?;
            if plan.summary.already_complete == plan.summary.total {
                return Err(bad(
                    "Journal knowledge is already complete. Nothing changed.",
                ));
            }
            Ok((candidate, plan))
        },
    )
}

#[cfg(test)]
#[path = "knowledge_unlock_tests.rs"]
mod tests;
