//! Reviewed map reveal, bound to an authenticated save pair and build 2976.
use crate::{
    Result,
    map_fog::{self, Plan},
    mounts::files::{SaveChoice, choices, read_pair},
};
use serde::{Deserialize, Serialize};
use std::path::{Path, PathBuf};

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
fn validate_game(game: &Path) -> Result<()> {
    if crate::fingerprint::known_for_game(game)?.exe_version != "1.0.0.2976" {
        return Err(crate::Error::Invalid(
            "Map reveal supports game build 1.0.0.2976".into(),
        ));
    }
    Ok(())
}
pub fn preview(game: &Path, root: &Path, selected: Option<&str>) -> Result<Preview> {
    validate_game(game)?;
    let plan = if let Some(id) = selected {
        let (save, lobby) = read_pair(root, id)?;
        Some(map_fog::plan(&save, &lobby)?)
    } else {
        None
    };
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
        return Err(crate::Error::Invalid("Invalid map reveal request".into()));
    }
    crate::mounts::files::edit_save_values(
        game,
        project,
        root,
        request,
        crate::mounts::files::SaveEditBinding {
            id: &request.id,
            save: &request.save,
            root: &request.save_root,
            save_sha256: &request.save_sha256,
            lobby_sha256: &request.lobby_sha256,
            history: "map-reveals",
            message: "World map fog revealed. Quest, knowledge and discovery records are preserved. Load this save in game.",
        },
        |save, lobby| {
            let (candidate, plan) = map_fog::build(save, lobby, &request.plan_id)?;
            if plan.before == plan.after {
                return Err(crate::Error::Invalid(
                    "This map is already fully revealed. Nothing changed.".into(),
                ));
            }
            Ok((candidate, plan))
        },
    )
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;
    #[test]
    #[cfg(windows)]
    #[ignore = "private historical save and EXE required; writes only to temporary copies"]
    fn private_map_transaction_backs_up_and_recovers_without_touching_other_data() {
        let fixture = PathBuf::from(std::env::var_os("CD_MAP_FOG_FIXTURE").expect("private save"));
        let exe =
            PathBuf::from(std::env::var_os("CD_MAP_FOG_EXE").expect("read-only original EXE"));
        let original = fs::read(&fixture).unwrap();
        let lobby = fs::read(fixture.with_file_name("lobby.save")).unwrap();
        let temp = tempfile::tempdir().unwrap();
        let game = temp.path().join("game");
        let project = temp.path().join("project");
        let root = temp.path().join("saves");
        let slot = root.join("123/slot0");
        fs::create_dir_all(game.join("bin64")).unwrap();
        fs::create_dir_all(&slot).unwrap();
        fs::create_dir(&project).unwrap();
        fs::copy(&exe, game.join("bin64/CrimsonDesert.exe")).unwrap();
        fs::write(slot.join("save.save"), &original).unwrap();
        fs::write(slot.join("lobby.save"), &lobby).unwrap();
        let preview = preview(&game, &root, Some("123/slot0")).unwrap();
        let plan = preview.plan.unwrap();
        let request = Request {
            save_root: root.canonicalize().unwrap(),
            id: "a".repeat(32),
            save: "123/slot0".into(),
            save_sha256: plan.save_sha256,
            lobby_sha256: plan.lobby_sha256,
            plan_id: plan.plan_id,
        };
        if preview.game_running {
            assert!(apply(&game, &project, &root, &request).is_err());
            assert_eq!(fs::read(slot.join("save.save")).unwrap(), original);
            assert!(!project.join(".local").exists());
            eprintln!(
                "Game running: refusal checked on temporary copies; commit/recovery branches deferred."
            );
            return;
        }
        let result = apply(&game, &project, &root, &request).unwrap();
        let candidate = fs::read(slot.join("save.save")).unwrap();
        assert_eq!(map_fog::inspect(&candidate).unwrap(), result.plan.after);
        assert_eq!(fs::read(slot.join("lobby.save")).unwrap(), lobby);
        assert_eq!(
            fs::read(result.backup.join("original.save")).unwrap(),
            original
        );
        assert_eq!(
            fs::read(result.backup.join("original.lobby")).unwrap(),
            lobby
        );
        assert_eq!(
            apply(&game, &project, &root, &request).unwrap().request,
            request
        );
        fs::remove_file(result.backup.join("receipt.json")).unwrap();
        assert_eq!(
            apply(&game, &project, &root, &request).unwrap().request,
            request
        );
        let mut stale = request.clone();
        stale.id = "b".repeat(32);
        assert!(apply(&game, &project, &root, &stale).is_err());
        let mut collision = request.clone();
        collision.plan_id = "c".repeat(64);
        assert!(apply(&game, &project, &root, &collision).is_err());
        fs::remove_file(result.backup.join("receipt.json")).unwrap();
        fs::write(slot.join("save.save"), &original).unwrap();
        assert!(apply(&game, &project, &root, &request).is_err());
        assert_eq!(fs::read(slot.join("save.save")).unwrap(), original);
        assert_eq!(fs::read(slot.join("lobby.save")).unwrap(), lobby);
        assert_eq!(fs::read(&fixture).unwrap(), original);
        assert_eq!(
            fs::read(fixture.with_file_name("lobby.save")).unwrap(),
            lobby
        );
    }
}
