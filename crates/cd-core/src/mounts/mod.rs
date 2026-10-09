//! Mount registration using same-family clones or a blank current-schema entry.
//! This module never spawns actors or writes a running game's save.
mod base;
mod edit;
pub(crate) mod files;
use crate::{Error, GameData, Result, fingerprint::hash_bytes};
pub use files::{Receipt, SaveChoice};
use serde::{Deserialize, Serialize};
use std::{
    collections::BTreeMap,
    path::{Path, PathBuf},
};
fn bad(message: impl Into<String>) -> Error {
    Error::Invalid(message.into())
}

#[derive(Clone, Debug, Serialize)]
pub struct Mount {
    pub key: u32,
    pub internal: String,
    pub name: String,
    pub description: String,
    pub family: String,
    pub vehicle: u16,
    pub owned: usize,
    pub supported: bool,
    pub reason: String,
    pub registration: String,
}
fn family(vehicle: u16) -> Option<&'static str> {
    Some(match vehicle {
        16960 => "Pferde",
        16962 => "Wildschweine",
        16966 => "Wölfe",
        16968 => "Löwen",
        16969 => "Tiger",
        16978 => "Kamele",
        16979 => "Bären",
        16980 => "Rentiere",
        16982 => "Kukuvögel",
        16983 => "Elefanten",
        16985 => "Leguane",
        16986 => "Dinosaurier",
        16993 => "Steinböcke",
        16994 => "Rinder",
        _ => return None,
    })
}
// These offsets are the existing build-pinned characterinfo reader's prefix.
// The base table was hash-checked by GameData before this function is called.
pub fn catalog(data: &GameData) -> Result<Vec<Mount>> {
    let (body, head, loc) = data.mount_inputs()?;
    let names: BTreeMap<_, _> = loc
        .entries()
        .iter()
        .map(|e| (e.string_key.as_str(), e.string_value.as_str()))
        .collect();
    let table = crate::tables::IndexedTable::parse(
        crate::tables::schema("characterinfo").unwrap(),
        &body,
        &head,
    )?;
    let mut out = Vec::new();
    for record in table.records() {
        let bytes = table
            .record_bytes(record.key)
            .ok_or_else(|| bad("Mount record missing"))?;
        let mut r = crate::mods::advanced::reader::Reader::new(bytes);
        let key = r.num(4)? as u32;
        let internal = r.text()?;
        let blocked = r.num(1)?;
        r.num(1)?;
        let name_key = r.num(8)?.to_string();
        let fallback = r.text()?;
        r.loc()?;
        r.take(8)?;
        r.text()?;
        r.take(10)?;
        let vehicle = r.num(2)? as u16;
        let Some(family) = family(vehicle) else {
            continue;
        };
        if blocked != 0 || !(internal.starts_with("Riding_") || internal.starts_with("Animal_")) {
            continue;
        }
        if ["Sequencer", "Sequence", "Dummy", "Tutorial"]
            .iter()
            .any(|s| internal.contains(s))
        {
            continue;
        }
        let name = names
            .get(name_key.as_str())
            .filter(|s| !s.is_empty())
            .copied()
            .unwrap_or(if fallback.is_empty() {
                &internal
            } else {
                &fallback
            })
            .to_owned();
        out.push(Mount{key,internal,name,description:format!("Reittier der Gruppe {family}. Wird dauerhaft im Stall des ausgewählten Spielstands registriert."),
            family:family.into(),vehicle,owned:0,supported:false,reason:"Spielstand auswählen.".into(),registration:"unavailable".into()});
    }
    out.sort_by(|a, b| a.name.cmp(&b.name).then(a.key.cmp(&b.key)));
    Ok(out)
}
#[derive(Serialize)]
pub struct Snapshot {
    pub mounts: Vec<Mount>,
    pub saves: Vec<SaveChoice>,
    pub selected_save: Option<String>,
    pub save_sha256: String,
    pub lobby_sha256: String,
    pub game_running: bool,
    pub message: String,
}
#[derive(Clone, Debug, Deserialize, Serialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct Request {
    pub id: String,
    pub save: String,
    pub save_sha256: String,
    pub lobby_sha256: String,
    pub key: u32,
}
impl Request {
    fn validate(&self) -> Result<()> {
        let hex = |s: &str, n: usize| {
            s.len() == n
                && s.bytes()
                    .all(|b| b.is_ascii_digit() || (b'a'..=b'f').contains(&b))
        };
        if !hex(&self.id, 32)
            || !hex(&self.save_sha256, 64)
            || !hex(&self.lobby_sha256, 64)
            || self.key == 0
        {
            return Err(bad("Ungültige Reittier-Anfrage."));
        }
        Ok(())
    }
}
pub fn save_root() -> Result<PathBuf> {
    Ok(
        PathBuf::from(std::env::var_os("LOCALAPPDATA").ok_or_else(|| bad("LOCALAPPDATA fehlt."))?)
            .join("Pearl Abyss/CD/save"),
    )
}
pub fn snapshot(mut mounts: Vec<Mount>, root: &Path, selected: Option<&str>) -> Result<Snapshot> {
    let saves = files::choices(root)?;
    let choice = match selected {
        Some(id) => Some(
            saves
                .iter()
                .find(|s| s.id == id)
                .ok_or_else(|| bad("Spielstand nicht gefunden."))?,
        ),
        None => saves.first(),
    };
    let running = crate::apply::guard::game_running()?;
    let mut out = Snapshot {
        mounts: Vec::new(),
        saves: Vec::new(),
        selected_save: choice.map(|c| c.id.clone()),
        save_sha256: String::new(),
        lobby_sha256: String::new(),
        game_running: running,
        message: if running {
            "Zum Hinzufügen speichern und das Spiel vollständig schließen."
        } else {
            "Ein Reittier auswählen und im Stall registrieren. Danach diesen Spielstand laden."
        }
        .into(),
    };
    if let Some(choice) = choice {
        let (save, lobby) = files::read_pair(root, &choice.id)?;
        out.save_sha256 = hash_bytes(&save);
        out.lobby_sha256 = hash_bytes(&lobby);
        edit::annotate(&save, &mut mounts)?;
    } else {
        out.message = "Keine vollständigen Spielstände gefunden.".into();
    }
    out.mounts = mounts;
    out.saves = saves;
    Ok(out)
}
pub fn register(
    game: &Path,
    project: &Path,
    mounts: &[Mount],
    root: &Path,
    request: &Request,
) -> Result<Receipt> {
    request.validate()?;
    let mount = mounts
        .iter()
        .find(|m| m.key == request.key)
        .ok_or_else(|| bad("Kein auswählbares Reittier mit dieser ID."))?;
    files::register(game, project, root, request, mount, mounts)
}
