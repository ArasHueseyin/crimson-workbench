//! Selected-save contribution edits, reviewed and bound to an exact save pair.
use crate::{
    Result,
    contribution_values::{self, Edit, Plan},
    mounts::files::{SaveChoice, choices, read_pair},
};
use serde::{Deserialize, Serialize};
use std::path::{Path, PathBuf};

#[derive(Serialize)]
pub struct Preview {
    pub save_root: PathBuf,
    pub saves: Vec<SaveChoice>,
    pub selected_save: Option<String>,
    pub balances: Vec<contribution_values::Balance>,
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
    pub edits: Vec<Edit>,
}
pub type Receipt = crate::save_action::Receipt<Request, Plan>;
fn validate_game(game: &Path) -> Result<()> {
    if crate::fingerprint::known_for_game(game)?.exe_version != "1.0.0.2976" {
        return Err(crate::Error::Invalid(
            "Contribution edits support game build 1.0.0.2976".into(),
        ));
    }
    Ok(())
}
pub fn preview(
    game: &Path,
    root: &Path,
    selected: Option<&str>,
    edits: &[Edit],
) -> Result<Preview> {
    validate_game(game)?;
    let mut result = Preview {
        save_root: root.canonicalize()?,
        saves: choices(root)?,
        selected_save: selected.map(str::to_owned),
        balances: vec![],
        plan: None,
        game_running: crate::apply::guard::game_running()?,
    };
    if let Some(id) = selected {
        let (save, lobby) = read_pair(root, id)?;
        result.balances = contribution_values::inspect(&save)?;
        if !edits.is_empty() {
            result.plan = Some(contribution_values::plan(&save, &lobby, edits)?);
        }
    } else if !edits.is_empty() {
        return Err(crate::Error::Invalid(
            "Choose a save before editing contribution balances".into(),
        ));
    }
    Ok(result)
}
pub fn apply(game: &Path, project: &Path, root: &Path, request: &Request) -> Result<Receipt> {
    validate_game(game)?;
    crate::mounts::files::edit_contributions(game, project, root, request)
}

pub(crate) fn validate_request(request: &Request) -> Result<()> {
    if request.id.len() != 32
        || !request.id.bytes().all(|b| b.is_ascii_hexdigit())
        || [
            &request.save_sha256,
            &request.lobby_sha256,
            &request.plan_id,
        ]
        .iter()
        .any(|v| v.len() != 64 || !v.bytes().all(|b| b.is_ascii_hexdigit()))
        || request.edits.is_empty()
        || request.edits.len() > 6
    {
        return Err(crate::Error::Invalid(
            "Invalid contribution edit request".into(),
        ));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn malformed_save_review_envelopes_are_refused() {
        let original = Request {
            save_root: PathBuf::from("fixture"),
            id: "a".repeat(32),
            save: "123/slot0".into(),
            save_sha256: "b".repeat(64),
            lobby_sha256: "c".repeat(64),
            plan_id: "d".repeat(64),
            edits: vec![Edit {
                key: 1000658,
                target: "100".into(),
            }],
        };
        validate_request(&original).unwrap();
        for case in 0..5 {
            let mut invalid = original.clone();
            match case {
                0 => invalid.id = "../escape".into(),
                1 => invalid.save_sha256.clear(),
                2 => invalid.lobby_sha256 = "x".repeat(64),
                3 => invalid.plan_id = "a".repeat(65),
                _ => invalid.edits.clear(),
            }
            assert!(validate_request(&invalid).is_err());
        }
    }
    #[test]
    #[cfg(windows)]
    #[ignore = "requires private save fixture and original EXE; all writes confined to owned temporary copies"]
    fn private_pair_transaction_backs_up_retries_recovers_and_refuses_stale_requests() {
        use crate::fingerprint::hash_bytes;
        use std::fs;
        let fixture = PathBuf::from(
            std::env::var_os("CD_CONTRIBUTION_FIXTURE").expect("private historical save"),
        );
        let exe = PathBuf::from(
            std::env::var_os("CD_CONTRIBUTION_EXE").expect("read-only original executable"),
        );
        let source = fs::read(&fixture).unwrap();
        let lobby = fs::read(fixture.with_file_name("lobby.save")).unwrap();
        let exe_hash = crate::fingerprint::hash_file(&exe).unwrap().1;
        assert_eq!(
            exe_hash,
            "57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7"
        );
        let temp = tempfile::tempdir().unwrap();
        let game = temp.path().join("fixture-game");
        let root = temp.path().join("fixture-saves");
        let project = temp.path().join("fixture-project");
        let slot = root.join("123/slot0");
        fs::create_dir_all(game.join("bin64")).unwrap();
        fs::create_dir_all(&slot).unwrap();
        fs::create_dir(&project).unwrap();
        fs::copy(&exe, game.join("bin64/CrimsonDesert.exe")).unwrap();
        fs::write(slot.join("save.save"), &source).unwrap();
        fs::write(slot.join("lobby.save"), &lobby).unwrap();
        let edits = vec![Edit {
            key: 1000658,
            target: "9007199254740993".into(),
        }];
        let review = preview(&game, &root, Some("123/slot0"), &edits).unwrap();
        let plan = review.plan.unwrap();
        let request = Request {
            save_root: root.canonicalize().unwrap(),
            id: "a".repeat(32),
            save: "123/slot0".into(),
            save_sha256: plan.save_sha256,
            lobby_sha256: plan.lobby_sha256,
            plan_id: plan.plan_id,
            edits,
        };
        if review.game_running {
            assert!(apply(&game, &project, &root, &request).is_err());
            assert_eq!(fs::read(slot.join("save.save")).unwrap(), source);
            assert!(!project.join(".local").exists());
            panic!(
                "Running-game refusal verified on temporary copies; full transaction test requires the game to be closed later. Do not close it for this test."
            );
        }
        let result = apply(&game, &project, &root, &request).unwrap();
        assert_eq!(
            fs::read(result.backup.join("original.save")).unwrap(),
            source
        );
        assert_eq!(
            fs::read(result.backup.join("original.lobby")).unwrap(),
            lobby
        );
        assert_eq!(fs::read(slot.join("lobby.save")).unwrap(), lobby);
        let candidate = fs::read(slot.join("save.save")).unwrap();
        assert_eq!(
            hash_bytes(&candidate),
            hash_bytes(&fs::read(result.backup.join("candidate.save")).unwrap())
        );
        let balances = contribution_values::inspect(&candidate).unwrap();
        assert_eq!(
            balances
                .iter()
                .find(|b| b.key == 1000658)
                .unwrap()
                .amount
                .as_deref(),
            Some("9007199254740993")
        );
        assert_eq!(
            apply(&game, &project, &root, &request).unwrap().request,
            request
        );
        assert_eq!(fs::read(slot.join("save.save")).unwrap(), candidate);
        let mut conflicting = request.clone();
        conflicting.edits[0].target = "9007199254740994".into();
        assert!(apply(&game, &project, &root, &conflicting).is_err());
        let mut stale = request.clone();
        stale.id = "b".repeat(32);
        assert!(apply(&game, &project, &root, &stale).is_err());
        assert!(
            !project
                .join(".local/contribution-edits")
                .join(&stale.id)
                .exists()
        );
        // Only owned fixture receipts are removed to model interrupted publication.
        fs::remove_file(result.backup.join("receipt.json")).unwrap();
        assert_eq!(
            apply(&game, &project, &root, &request).unwrap().request,
            request
        );
        assert_eq!(fs::read(slot.join("save.save")).unwrap(), candidate);
        fs::remove_file(result.backup.join("receipt.json")).unwrap();
        fs::write(slot.join("save.save"), &source).unwrap();
        assert!(
            apply(&game, &project, &root, &request)
                .unwrap_err()
                .to_string()
                .contains("rolled back")
        );
        assert_eq!(fs::read(slot.join("save.save")).unwrap(), source);
        assert_eq!(fs::read(slot.join("lobby.save")).unwrap(), lobby);
        assert_eq!(fs::read(&fixture).unwrap(), source);
        assert_eq!(crate::fingerprint::hash_file(&exe).unwrap().1, exe_hash);
    }
}
