//! First-run configuration, confined to the application's writable workspace.
use crate::service::{AppError, Result};
use cd_core::{
    config::{LocalConfig, Preferences},
    discovery::DiscoveryOptions,
    paths::PathPolicy,
};
use std::{
    fs,
    path::{Path, PathBuf},
};

fn problem(message: impl Into<String>) -> AppError {
    AppError {
        code: "settings_error",
        message: message.into(),
    }
}

pub fn ensure_workspace(project: &Path) -> Result<()> {
    let config = LocalConfig::load(project).map_err(|e| {
        problem(format!(
            "Lokale Konfiguration konnte nicht gelesen werden: {e}"
        ))
    })?;
    let mut options = DiscoveryOptions::from_environment();
    // A stale explicit game path must not prevent the first-run UI from opening.
    options.explicit_game_dir = None;
    let mut roots: Vec<_> = cd_core::discovery::discover_with(&options)
        .map_err(|e| problem(e.to_string()))?
        .into_iter()
        .map(|i| i.path)
        .collect();
    roots.extend(cd_core::discovery::locate_saves_with(&options));
    roots.extend(config.game_dir);
    roots.extend(config.save_dir);
    PathPolicy::from_roots(roots, project.to_path_buf()).map_err(|e| {
        problem(format!(
            "Der Arbeitsordner muss außerhalb der Spiel- und Saveordner liegen: {e}"
        ))
    })?;
    fs::create_dir_all(project).map_err(|e| {
        problem(format!(
            "Arbeitsordner {} konnte nicht angelegt werden: {e}",
            project.display()
        ))
    })?;
    Ok(())
}

fn directory(path: Option<PathBuf>, label: &str) -> Result<Option<PathBuf>> {
    path.map(|path| {
        if !path.is_absolute() {
            return Err(problem(format!(
                "{label}: Bitte einen vollständigen absoluten Ordnerpfad eingeben."
            )));
        }
        let resolved = path
            .canonicalize()
            .map_err(|e| problem(format!("{label} ist nicht erreichbar: {e}")))?;
        if !resolved.is_dir() {
            return Err(problem(format!("{label} muss ein Ordner sein.")));
        }
        Ok(resolved)
    })
    .transpose()
}

pub fn validate(project: &Path, mut preferences: Preferences) -> Result<Preferences> {
    if preferences.version != 1
        || !cd_core::supported_languages()
            .iter()
            .any(|l| l.language == preferences.language)
    {
        return Err(problem("Unbekannte Einstellungsversion oder Itemsprache."));
    }
    preferences.game_dir = directory(preferences.game_dir, "Installationsordner")?;
    preferences.save_dir = directory(preferences.save_dir, "Spielstandordner")?;
    if let Some(game) = &preferences.game_dir {
        let found = cd_core::discovery::discover_with(&DiscoveryOptions { explicit_game_dir: Some(game.clone()), ..Default::default() })
            .map_err(|_| problem("Kein Crimson-Desert-Installationsordner: bin64/CrimsonDesert.exe und meta/0.papgt fehlen. Bei Game Pass ist auch der übergeordnete Installationsordner zulässig."))?;
        preferences.game_dir = found.first().map(|i| i.path.clone());
    }
    let roots = preferences
        .game_dir
        .iter()
        .chain(preferences.save_dir.iter())
        .cloned()
        .collect();
    PathPolicy::from_roots(roots, project.to_path_buf()).map_err(|e| {
        problem(format!(
            "Spiel- und Saveordner müssen vom Workbench-Arbeitsordner getrennt sein: {e}"
        ))
    })?;
    Ok(preferences)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn validates_roots_without_reading_or_changing_game_or_save_contents() {
        let temp = tempfile::tempdir().unwrap();
        let project = temp.path().join("workbench");
        let game = temp.path().join("game");
        let saves = temp.path().join("saves");
        fs::create_dir_all(&project).unwrap();
        fs::create_dir_all(game.join("bin64")).unwrap();
        fs::create_dir_all(game.join("meta")).unwrap();
        fs::create_dir_all(&saves).unwrap();
        fs::write(game.join("bin64/CrimsonDesert.exe"), b"synthetic").unwrap();
        fs::write(game.join("meta/0.papgt"), b"synthetic").unwrap();
        let preferences = Preferences {
            game_dir: Some(game.clone()),
            save_dir: Some(saves.clone()),
            language: "eng".into(),
            ..Default::default()
        };
        let validated = validate(&project, preferences.clone()).unwrap();
        assert_eq!(validated.save_dir, Some(saves.canonicalize().unwrap()));
        assert!(validate(&game, preferences.clone()).is_err());
        assert!(
            validate(
                &project,
                Preferences {
                    game_dir: Some(temp.path().join("missing")),
                    ..preferences.clone()
                }
            )
            .is_err()
        );
        assert!(
            validate(
                &project,
                Preferences {
                    save_dir: Some(PathBuf::from("relative")),
                    ..preferences.clone()
                }
            )
            .is_err()
        );
        assert!(
            validate(
                &project,
                Preferences {
                    language: "invalid".into(),
                    ..preferences
                }
            )
            .is_err()
        );
        assert_eq!(fs::read_dir(&project).unwrap().count(), 0);
        assert_eq!(fs::read_dir(&saves).unwrap().count(), 0);
        assert_eq!(
            fs::read(game.join("bin64/CrimsonDesert.exe")).unwrap(),
            b"synthetic"
        );
    }
}
