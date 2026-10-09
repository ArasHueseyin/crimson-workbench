//! Saved document knowledge status. Never changes knowledge, inventory or saves.
use crate::{
    Error, Result,
    mounts::files::{SaveChoice, choices, read_pair},
};
use crimson_format::{
    ItemRecord,
    save::{Body, FieldValue, ObjectBlock, Save, ScalarValue},
};
use serde::Serialize;
use std::{collections::BTreeMap, path::Path};
fn bad(s: impl Into<String>) -> Error {
    Error::Invalid(s.into())
}
#[derive(Serialize)]
pub struct ItemStatus {
    pub key: u32,
    pub total: usize,
    pub learned: usize,
    pub unknown: usize,
    pub state: &'static str,
}
#[derive(Serialize)]
pub struct Snapshot {
    pub saves: Vec<SaveChoice>,
    pub selected_save: Option<String>,
    pub modified: Option<u64>,
    pub items: Vec<ItemStatus>,
    pub message: String,
}
fn scalar(o: &ObjectBlock, name: &str) -> Result<u32> {
    match o
        .fields
        .iter()
        .find(|f| f.name == name && f.present)
        .map(|f| &f.value)
    {
        Some(FieldValue::Scalar(ScalarValue::U32(n))) => Ok(*n),
        _ => Err(bad(format!("Unbekanntes Wissensfeld {name}"))),
    }
}
fn learned(save_bytes: &[u8]) -> Result<BTreeMap<u32, Option<bool>>> {
    let save = Save::parse(save_bytes).map_err(|e| bad(e.to_string()))?;
    let body = Body::parse(&save.body)?;
    let blocks = body.decode_blocks(&save.body);
    let matches: Vec<_> = blocks
        .iter()
        .filter(|b| b.class_name == "KnowledgeSaveData")
        .collect();
    if matches.len() != 1 {
        return Err(bad("Wissensdaten nicht eindeutig gelesen."));
    }
    let block = matches[0];
    if !block.undecoded_ranges.is_empty() {
        return Err(bad("Wissensformat nicht vollständig gelesen."));
    }
    let f = block
        .fields
        .iter()
        .find(|f| f.name == "_list" && f.present)
        .ok_or_else(|| bad("Wissensliste fehlt."))?;
    let FieldValue::ObjectList {
        count, elements, ..
    } = &f.value
    else {
        return Err(bad("Wissensliste nicht lesbar."));
    };
    if *count as usize != elements.len() {
        return Err(bad("Wissensanzahl stimmt nicht."));
    }
    let mut known = BTreeMap::new();
    for element in elements {
        if element.class_name != "KnowledgeElementSaveData" || !element.undecoded_ranges.is_empty()
        {
            return Err(bad("Unbestätigter Wissenseintrag."));
        }
        let key = scalar(element, "_key")?;
        let level = scalar(element, "_level")?;
        let value = match level {
            0 => Some(false),
            1..=5 => Some(true),
            _ => None,
        };
        if known.insert(key, value).is_some() {
            return Err(bad("Doppelte Wissens-ID: Status nicht eindeutig."));
        }
    }
    Ok(known)
}
fn status(key: u32, rewards: &[u32], known: &BTreeMap<u32, Option<bool>>) -> ItemStatus {
    let mut yes = 0;
    let mut unknown = 0;
    for k in rewards {
        match known.get(k) {
            Some(Some(true)) => yes += 1,
            Some(None) => unknown += 1,
            _ => {}
        }
    }
    let total = rewards.len();
    let state = if total == 0 || unknown > 0 {
        "unknown"
    } else if yes == total {
        "learned"
    } else if yes == 0 {
        "missing"
    } else {
        "partial"
    };
    ItemStatus {
        key,
        total,
        learned: yes,
        unknown,
        state,
    }
}
pub fn snapshot(items: &[ItemRecord], root: &Path, selected: Option<&str>) -> Result<Snapshot> {
    let saves = choices(root)?;
    let choice = match selected {
        Some(id) => Some(
            saves
                .iter()
                .find(|s| s.id == id)
                .ok_or_else(|| bad("Spielstand nicht gefunden."))?,
        ),
        None => saves.first(),
    };
    let mut snapshot=Snapshot{saves:Vec::new(),selected_save:choice.map(|c|c.id.clone()),modified:choice.map(|c|c.modified),items:Vec::new(),message:"Wissen aus dem gespeicherten Spielstand. Nach dem Lesen im Spiel speichern und hier neu einlesen.".into()};
    if let Some(c) = choice {
        let (save, _) = read_pair(root, &c.id)?;
        let known = learned(&save)?;
        snapshot.items = items
            .iter()
            .filter(|i| !i.knowledge_keys.is_empty())
            .map(|i| status(i.key, &i.knowledge_keys, &known))
            .collect();
    } else {
        snapshot.message =
            "Kein gespeicherter Spielstand gefunden. Wissensstatus unbekannt.".into();
    }
    snapshot.saves = saves;
    Ok(snapshot)
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn multi_reward_documents_distinguish_partial_unknown_and_zero_level() {
        let known = BTreeMap::from([(10, Some(true)), (11, Some(false)), (12, None)]);
        assert_eq!(status(1, &[10], &known).state, "learned");
        assert_eq!(status(1, &[10, 11], &known).state, "partial");
        assert_eq!(status(1, &[11, 13], &known).state, "missing");
        assert_eq!(status(1, &[10, 12], &known).state, "unknown");
        assert_eq!(status(1, &[], &known).state, "unknown");
    }
    #[test]
    #[ignore = "requires private encrypted save fixture; read only"]
    fn real_saved_knowledge_matches_typed_rewards() {
        let path = std::path::PathBuf::from(std::env::var_os("CD_MOUNT_FIXTURE").unwrap());
        let game = std::path::PathBuf::from(std::env::var_os("CD_MOUNT_GAME").unwrap());
        let data = crate::GameData::open(&game, "ger").unwrap();
        let known = learned(&std::fs::read(path).unwrap()).unwrap();
        assert!(known.len() > 1000);
        let rewards: Vec<_> = data
            .items()
            .iter()
            .filter(|i| !i.knowledge_keys.is_empty())
            .collect();
        assert!(!rewards.is_empty());
        let statuses: Vec<_> = rewards
            .iter()
            .map(|i| status(i.key, &i.knowledge_keys, &known))
            .collect();
        assert!(statuses.iter().any(|s| s.state == "learned"));
        assert!(statuses.iter().any(|s| s.state == "missing"));
        println!(
            "Read-only knowledge: {} saved entries, {} items with typed knowledge rewards; learned and missing cases present.",
            known.len(),
            rewards.len()
        );
    }
}
