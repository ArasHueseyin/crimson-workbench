//! Versioned save recipe. No game assets or personal save contents are bundled.
use crate::{
    Error, Result,
    fingerprint::hash_bytes,
    mounts::{
        edit::{decode, field_mut, list, number, set_number, update_count},
        files::{SaveChoice, choices, read_pair},
    },
};
use crimson_format::save::{FieldValue, ObjectBlock};
use serde::{Deserialize, Serialize};
use std::{
    collections::BTreeMap,
    path::{Path, PathBuf},
};
fn bad(s: impl Into<String>) -> Error {
    Error::Invalid(s.into())
}

#[derive(Deserialize)]
struct Ability {
    key: u32,
    maximum: u32,
    costs: Vec<u32>,
    owner: u32,
}
fn abilities() -> Result<Vec<Ability>> {
    Ok(serde_json::from_str(include_str!(
        "../schemas/marketplace-skills-2976.json"
    ))?)
}
#[derive(Clone, Serialize, Deserialize)]
pub struct Summary {
    pub total: usize,
    pub new_abilities: usize,
    pub upgraded_abilities: usize,
    pub already_maxed: usize,
    pub gifted_points: [u64; 3],
    pub available_points: [u64; 3],
}
#[derive(Serialize)]
pub struct Preview {
    pub save_root: PathBuf,
    pub saves: Vec<SaveChoice>,
    pub selected_save: Option<String>,
    pub save_sha256: Option<String>,
    pub lobby_sha256: Option<String>,
    pub plan_id: Option<String>,
    pub summary: Option<Summary>,
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
#[derive(Clone, Serialize, Deserialize)]
pub struct Receipt {
    pub request: Request,
    pub summary: Summary,
    pub backup: PathBuf,
    pub message: String,
}

fn validate_game(game: &Path) -> Result<()> {
    if crate::fingerprint::known_for_game(game)?.exe_version != "1.0.0.2976" {
        return Err(bad("All skills supports game build 1.0.0.2976"));
    }
    Ok(())
}
pub fn preview(game: &Path, root: &Path, selected: Option<&str>) -> Result<Preview> {
    validate_game(game)?;
    let saves = choices(root)?;
    let mut result = Preview {
        save_root: root.canonicalize()?,
        saves,
        selected_save: selected.map(str::to_owned),
        save_sha256: None,
        lobby_sha256: None,
        plan_id: None,
        summary: None,
        game_running: crate::apply::guard::game_running()?,
    };
    if let Some(id) = selected {
        let (save, lobby) = read_pair(root, id)?;
        let (_, summary) = build(&save)?;
        result.plan_id = Some(plan_id(&save, &lobby));
        result.save_sha256 = Some(hash_bytes(&save));
        result.lobby_sha256 = Some(hash_bytes(&lobby));
        result.summary = Some(summary);
    }
    Ok(result)
}
fn plan_id(save: &[u8], lobby: &[u8]) -> String {
    hash_bytes(
        format!(
            "all-skills-2976-v1:{}:{}",
            hash_bytes(save),
            hash_bytes(lobby)
        )
        .as_bytes(),
    )
}
pub fn apply(game: &Path, project: &Path, root: &Path, request: &Request) -> Result<Receipt> {
    validate_game(game)?;
    crate::mounts::files::unlock_skills(game, project, root, request)
}

fn index(entries: &[ObjectBlock], key: &str) -> Result<BTreeMap<u64, usize>> {
    let mut result = BTreeMap::new();
    for (i, entry) in entries.iter().enumerate() {
        if result.insert(number(entry, key)?, i).is_some() {
            return Err(bad(format!("Duplicate save entry: {key}")));
        }
    }
    Ok(result)
}
fn append(block: &mut ObjectBlock, name: &str, object: ObjectBlock) -> Result<usize> {
    let f = field_mut(block, name)?;
    let FieldValue::ObjectList {
        count,
        elements,
        header_bytes,
        header_variant,
    } = &mut f.value
    else {
        return Err(bad("Skill save list missing"));
    };
    let index = elements.len();
    elements.push(object);
    *count = u32::try_from(elements.len()).map_err(|_| bad("Skill list too large"))?;
    update_count(header_bytes, header_variant, *count)?;
    Ok(index)
}
fn entry_mut<'a>(
    block: &'a mut ObjectBlock,
    name: &str,
    index: usize,
) -> Result<&'a mut ObjectBlock> {
    let FieldValue::ObjectList { elements, .. } = &mut field_mut(block, name)?.value else {
        return Err(bad("Skill list missing"));
    };
    elements
        .get_mut(index)
        .ok_or_else(|| bad("Skill save index missing"))
}
fn donor(entries: &[ObjectBlock], names: &[&str]) -> Result<ObjectBlock> {
    entries.iter().find(|e| e.fields.iter().filter(|f| f.present).all(|f| names.contains(&f.name.as_str())) && names.iter().all(|n| number(e,n).is_ok())).cloned().ok_or_else(|| bad("No verified skill entry template exists in this save. Learn one skill in game first."))
}

pub(crate) fn build(bytes: &[u8]) -> Result<(Vec<u8>, Summary)> {
    let (mut save, body, mut blocks) = decode(bytes)?;
    let original_body = save.body.clone();
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
    let mut known = index(list(&original, "_list")?, "_key")?;
    let mut learned = index(list(&original, "_skillLearnSaveDataList")?, "_knowledgeKey")?;
    let points = index(
        list(&original, "_skillPointSaveDataList")?,
        "_skillPointOwnerType",
    )?;
    if points.keys().copied().collect::<Vec<_>>() != vec![0, 1, 2] {
        return Err(bad("All three character point ledgers are required"));
    }
    let mut spent = [0u64; 3];
    for entry in list(&original, "_skillLearnSaveDataList")? {
        let owner = number(entry, "_skillPointOwnerType")?;
        if owner > 2 {
            return Err(bad("Unknown skill point owner"));
        }
        spent[owner as usize] = spent[owner as usize]
            .checked_add(number(entry, "_usedArtifactCount")?)
            .ok_or_else(|| bad("Skill ledger overflow"))?;
    }
    let mut summary = Summary {
        total: 140,
        new_abilities: 0,
        upgraded_abilities: 0,
        already_maxed: 0,
        gifted_points: [0; 3],
        available_points: [0; 3],
    };
    for (owner, i) in &points {
        let entry = &list(&original, "_skillPointSaveDataList")?[*i];
        let current = number(entry, "_currentSkillPoint")?;
        let earned = number(entry, "_elapsedSkillPoint")?;
        if current.checked_add(spent[*owner as usize]) != Some(earned) {
            return Err(bad(
                "Skill point accounting is inconsistent. Nothing changed.",
            ));
        }
        summary.available_points[*owner as usize] = current;
    }
    let targets = abilities()?;
    if targets.len() != 140 {
        return Err(bad("Invalid skill recipe"));
    }
    let timestamp = list(&original, "_list")?
        .iter()
        .map(|o| number(o, "_learnedFieldTime"))
        .collect::<Result<Vec<_>>>()?
        .into_iter()
        .max()
        .unwrap_or(0);
    let block = &mut blocks[bi];
    for ability in targets {
        if ability.owner > 2 || ability.maximum as usize != ability.costs.len() {
            return Err(bad("Invalid skill recipe levels"));
        }
        let key = ability.key as u64;
        let before = known
            .get(&key)
            .map(|i| number(&list(&original, "_list")?[*i], "_level"))
            .transpose()?
            .unwrap_or(0);
        if before > ability.maximum as u64 {
            return Err(bad("Saved skill exceeds verified maximum"));
        }
        if before == ability.maximum as u64 {
            summary.already_maxed += 1;
            continue;
        }
        let extra = ability.costs[before as usize..]
            .iter()
            .map(|n| *n as u64)
            .sum::<u64>();
        let ledger = learned.get(&key).copied();
        let (owner, used) = match ledger {
            Some(i) => {
                let o = &list(&original, "_skillLearnSaveDataList")?[i];
                (
                    number(o, "_skillPointOwnerType")?,
                    number(o, "_usedArtifactCount")?,
                )
            }
            None => (ability.owner as u64, 0),
        };
        if owner > 2
            || used
                != ability.costs[..before as usize]
                    .iter()
                    .map(|n| *n as u64)
                    .sum::<u64>()
        {
            return Err(bad(
                "Partially learned skill has an unsupported cost history",
            ));
        }
        let i = match known.get(&key) {
            Some(i) => {
                summary.upgraded_abilities += 1;
                *i
            }
            None => {
                summary.new_abilities += 1;
                let mut object = donor(
                    list(&original, "_list")?,
                    &["_key", "_level", "_learnedFieldTime"],
                )?;
                set_number(&mut object, "_key", key)?;
                set_number(&mut object, "_learnedFieldTime", timestamp)?;
                let i = append(block, "_list", object)?;
                known.insert(key, i);
                i
            }
        };
        set_number(
            entry_mut(block, "_list", i)?,
            "_level",
            ability.maximum as u64,
        )?;
        if extra > 0 {
            let i = match ledger {
                Some(i) => i,
                None => {
                    let mut object = donor(
                        list(&original, "_skillLearnSaveDataList")?,
                        &[
                            "_knowledgeKey",
                            "_usedArtifactCount",
                            "_skillPointOwnerType",
                        ],
                    )?;
                    set_number(&mut object, "_knowledgeKey", key)?;
                    set_number(&mut object, "_skillPointOwnerType", owner)?;
                    let i = append(block, "_skillLearnSaveDataList", object)?;
                    learned.insert(key, i);
                    i
                }
            };
            set_number(
                entry_mut(block, "_skillLearnSaveDataList", i)?,
                "_usedArtifactCount",
                used + extra,
            )?;
            summary.gifted_points[owner as usize] = summary.gifted_points[owner as usize]
                .checked_add(extra)
                .ok_or_else(|| bad("Skill gift overflow"))?;
        }
    }
    for (owner, i) in &points {
        let entry = entry_mut(block, "_skillPointSaveDataList", *i)?;
        let earned = number(entry, "_elapsedSkillPoint")?
            .checked_add(summary.gifted_points[*owner as usize])
            .ok_or_else(|| bad("Skill point overflow"))?;
        if earned > 65535 {
            return Err(bad("Skill point limit exceeded"));
        }
        set_number(entry, "_elapsedSkillPoint", earned)?;
    }
    let candidate = body.write(&save.body, &blocks)?;
    // Restoring the sole edited block must recreate the entire original body.
    // This checks that all unrelated payloads and absolute pointers are retained.
    blocks[bi] = original;
    if body.write(&save.body, &blocks)? != original_body {
        return Err(bad("Whole-save inverse failed"));
    }
    save.body = candidate;
    let after = save
        .write_with_nonce(save.header.nonce())
        .map_err(|e| bad(e.to_string()))?;
    let (readback, _, _) = decode(&after)?;
    if readback.body != save.body {
        return Err(bad("Skill save encrypted readback failed"));
    }
    Ok((after, summary))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn recipe_is_bounded_unique_and_covers_all_three_characters() {
        let targets = abilities().unwrap();
        assert_eq!(targets.len(), 140);
        let keys: std::collections::BTreeSet<_> = targets.iter().map(|a| a.key).collect();
        assert_eq!(keys.len(), 140);
        for a in targets {
            assert!(a.owner <= 2 && a.maximum >= 1 && a.maximum <= 18);
            assert_eq!(a.maximum as usize, a.costs.len());
            assert!(a.costs.iter().all(|c| *c <= 5));
        }
        assert!(build(b"not a save").is_err());
    }
    #[test]
    #[ignore = "requires private pre-unlock save fixture; never writes it"]
    fn private_save_preserves_points_and_is_idempotent() {
        let source =
            std::fs::read(std::env::var_os("CD_SKILL_FIXTURE").expect("private fixture required"))
                .unwrap();
        let (after, summary) = build(&source).unwrap();
        assert_eq!(summary.total, 140);
        assert!(summary.new_abilities + summary.upgraded_abilities > 0);
        let (again, summary2) = build(&after).unwrap();
        assert_eq!(summary2.already_maxed, 140);
        assert_eq!(summary2.gifted_points, [0; 3]);
        assert_eq!(after, again);
        assert_eq!(summary.available_points, summary2.available_points);
    }
}
