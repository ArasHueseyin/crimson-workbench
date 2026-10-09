//! Read only the player's inventory and equipment, never world or shop items.
use crate::{
    Error, GameData, Result,
    fingerprint::hash_bytes,
    mounts::{
        files::{SaveChoice, choices, read_pair},
        save_root,
    },
};
use crimson_format::save::{Body, FieldValue, ObjectBlock, Save, ScalarValue};
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;

fn bad(s: impl Into<String>) -> Error {
    Error::Invalid(s.into())
}
fn number(o: &ObjectBlock, name: &str) -> Result<u64> {
    let f = o
        .fields
        .iter()
        .find(|f| f.name == name)
        .ok_or_else(|| bad(format!("Item-Feld fehlt: {name}")))?;
    if !f.present {
        return Ok(0);
    }
    match &f.value {
        FieldValue::Scalar(ScalarValue::U8(n)) => Ok(*n as u64),
        FieldValue::Scalar(ScalarValue::U16(n)) => Ok(*n as u64),
        FieldValue::Scalar(ScalarValue::U32(n)) => Ok(*n as u64),
        FieldValue::Scalar(ScalarValue::U64(n)) => Ok(*n),
        FieldValue::Scalar(ScalarValue::I32(n)) if *n >= 0 => Ok(*n as u64),
        FieldValue::Scalar(ScalarValue::I64(n)) if *n >= 0 => Ok(*n as u64),
        _ => Err(bad(format!("Unbekanntes Item-Feld: {name}"))),
    }
}
#[derive(Clone, Debug, PartialEq, Serialize)]
pub struct Candidate {
    pub uid: String,
    pub item_key: u32,
    pub name: String,
    pub baseline: u8,
    pub location: String,
    pub eligible: bool,
    pub reason: Option<String>,
    pub extended: bool,
}
#[derive(Serialize)]
pub struct Candidates {
    pub saves: Vec<SaveChoice>,
    pub selected_save: Option<String>,
    pub save_revision: Option<String>,
    pub items: Vec<Candidate>,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
pub struct AddRequest {
    pub revision: String,
    pub save: String,
    pub save_revision: String,
    pub uid: String,
}
fn shape(o: &ObjectBlock) -> Result<u8> {
    if !o.undecoded_ranges.is_empty()
        || number(o, "_maxSocketCount")? != 5
        || number(o, "_stackCount")? != 1
    {
        return Err(bad(
            "Nur einzeln gespeicherte Ausrüstung mit vollständig gelesenem Fünf-Sockel-Format ist unterstützt.",
        ));
    }
    let base = number(o, "_validSocketCount")?;
    if base > 5 {
        return Err(bad(
            "Vorhandene Sockelanzahl liegt außerhalb des geprüften Formats.",
        ));
    }
    let f = o
        .fields
        .iter()
        .find(|f| f.name == "_socketSaveDataList" && f.present)
        .ok_or_else(|| bad("Sockelliste fehlt."))?;
    let FieldValue::ObjectList {
        count, elements, ..
    } = &f.value
    else {
        return Err(bad("Unbekannte Sockelliste."));
    };
    if *count != 5 || elements.len() != 5 {
        return Err(bad("Sockelliste hat nicht fünf Einträge."));
    }
    for (i, e) in elements.iter().enumerate() {
        if e.class_name != "ItemSocketSaveData" || !e.undecoded_ranges.is_empty() {
            return Err(bad("Unbekanntes Sockelformat."));
        }
        if i >= base as usize && number(e, "_itemKey")? != 0 {
            return Err(bad("Ein gesperrter Basissockel enthält einen Stein."));
        }
    }
    Ok(base as u8)
}
pub(crate) fn parse(data: &GameData, raw: &[u8]) -> Result<Vec<Candidate>> {
    let s = Save::parse(raw).map_err(|e| bad(e.to_string()))?;
    let body = Body::parse(&s.body)?;
    let blocks = body.decode_blocks(&s.body);
    let gear = super::extra_sockets::gear_keys();
    let names = data.localized_items()?;
    let mut found = BTreeMap::<String, Candidate>::new();
    fn walk(
        o: &ObjectBlock,
        location: &str,
        gear: &[u32],
        names: &[crate::IndexedItem],
        found: &mut BTreeMap<String, Candidate>,
    ) -> Result<()> {
        if o.class_name == "ItemSaveData" {
            let uid = number(o, "_itemNo")?;
            let key =
                u32::try_from(number(o, "_itemKey")?).map_err(|_| bad("Ungültiger Item-Key."))?;
            if uid == 0 || uid == u64::MAX || !gear.contains(&key) {
                return Ok(());
            }
            let result = shape(o);
            let candidate = Candidate {
                uid: uid.to_string(),
                item_key: key,
                name: names
                    .iter()
                    .find(|n| n.key == key)
                    .map(|n| n.name.clone())
                    .unwrap_or_else(|| key.to_string()),
                baseline: result.as_ref().copied().unwrap_or(0),
                location: location.into(),
                eligible: result.is_ok(),
                reason: result.err().map(|e| e.to_string()),
                extended: false,
            };
            if let Some(old) = found.get(&candidate.uid) {
                if old.item_key != candidate.item_key
                    || old.baseline != candidate.baseline
                    || old.eligible != candidate.eligible
                {
                    return Err(bad(
                        "Widersprüchliche doppelte Gegenstandsinstanz im Spielstand.",
                    ));
                }
            } else {
                found.insert(candidate.uid.clone(), candidate);
            }
            return Ok(());
        }
        for f in &o.fields {
            match &f.value {
                FieldValue::Locator {
                    child: Some(child), ..
                } => walk(child, location, gear, names, found)?,
                FieldValue::ObjectList {
                    count, elements, ..
                } => {
                    if *count as usize != elements.len() {
                        return Err(bad("Unvollständig gelesene Inventarliste."));
                    }
                    for e in elements {
                        walk(e, location, gear, names, found)?;
                    }
                }
                _ => {}
            }
        }
        Ok(())
    }
    for b in blocks.iter().filter(|b| {
        matches!(
            b.class_name.as_str(),
            "InventorySaveData" | "EquipmentSaveData" | "MercenaryClanSaveData"
        )
    }) {
        let location = match b.class_name.as_str() {
            "EquipmentSaveData" => "Ausgerüstet",
            "MercenaryClanSaveData" => "Charakter / Begleiter",
            _ => "Inventar / Lager",
        };
        walk(b, location, &gear, &names, &mut found)?;
    }
    let mut items: Vec<_> = found.into_values().collect();
    items.sort_by(|a, b| a.name.cmp(&b.name).then(a.uid.cmp(&b.uid)));
    Ok(items)
}
pub fn candidates(data: &GameData, selected: Option<&str>) -> Result<Candidates> {
    let saves = choices(&save_root()?)?;
    let chosen = match selected {
        Some(id) => Some(
            saves
                .iter()
                .find(|s| s.id == id)
                .ok_or_else(|| bad("Spielstand nicht gefunden."))?,
        ),
        None => saves.first(),
    };
    let Some(chosen) = chosen else {
        return Ok(Candidates {
            saves,
            selected_save: None,
            save_revision: None,
            items: vec![],
        });
    };
    let id = chosen.id.clone();
    let (raw, _) = read_pair(&save_root()?, &id)?;
    let revision = hash_bytes(&raw);
    let mut items = parse(data, &raw)?;
    let existing = super::extra_sockets::snapshot(data)?;
    for item in &mut items {
        if let Some(r) = existing.records.iter().find(|r| r.uid == item.uid) {
            item.extended = true;
            item.eligible = false;
            item.reason = Some(
                if r.item_key == item.item_key && r.baseline == item.baseline {
                    "Bereits um zehn Sockel erweitert.".into()
                } else {
                    "Vorhandene Erweiterung stimmt nicht mit diesem Spielstand überein.".into()
                },
            );
        }
    }
    Ok(Candidates {
        saves,
        selected_save: Some(id),
        save_revision: Some(revision),
        items,
    })
}
