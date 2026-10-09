use crate::{
    IndexIdentity, SearchPage, SearchQuery,
    config::LocalConfig,
    data::{Error, GameData, Result},
    discovery::{self, DiscoveryOptions, Installation},
    index::Index,
    paths::PathPolicy,
};
use serde::Serialize;
use std::{
    fs,
    path::{Path, PathBuf},
};

#[derive(Debug, Serialize)]
pub struct DiscoveryReport {
    pub installations: Vec<Installation>,
    pub save_directories: Vec<PathBuf>,
    pub configured_game: Option<PathBuf>,
}
pub fn discover_project(project: &Path, game_override: Option<&Path>) -> Result<DiscoveryReport> {
    let options = options(project, game_override)?;
    Ok(DiscoveryReport {
        installations: discovery::discover_with(&options)?,
        save_directories: discovery::locate_saves_with(&options),
        configured_game: options.explicit_game_dir,
    })
}
/// Select an explicit root, or the sole detected root. Multiple accounts inside
/// one root are fine; multiple platform roots require an explicit user choice.
pub fn project_save_root(project: &Path) -> Result<Option<PathBuf>> {
    let options = options(project, None)?;
    Ok(select_save_root(
        options.explicit_save_dir.clone(),
        discovery::locate_saves_with(&options),
    ))
}
fn select_save_root(explicit: Option<PathBuf>, discovered: Vec<PathBuf>) -> Option<PathBuf> {
    explicit.or_else(|| match discovered.as_slice() {
        [one] => Some(one.clone()),
        _ => None,
    })
}
#[cfg(test)]
mod setup_tests {
    use super::*;
    #[test]
    fn save_selection_never_silently_picks_another_platform() {
        let steam = PathBuf::from("steam");
        let epic = PathBuf::from("epic");
        assert_eq!(select_save_root(None, vec![]), None);
        assert_eq!(
            select_save_root(None, vec![steam.clone()]),
            Some(steam.clone())
        );
        assert_eq!(
            select_save_root(None, vec![steam.clone(), epic.clone()]),
            None
        );
        assert_eq!(
            select_save_root(Some(epic.clone()), vec![steam]),
            Some(epic)
        );
    }
}
fn options(project: &Path, game_override: Option<&Path>) -> Result<DiscoveryOptions> {
    let project = project.canonicalize()?;
    let config = LocalConfig::load(&project)?;
    let mut options = DiscoveryOptions::from_environment();
    options.explicit_game_dir = game_override
        .map(|p| {
            if p.is_absolute() {
                p.to_path_buf()
            } else {
                project.join(p)
            }
        })
        .or(config.game_dir);
    options.explicit_save_dir = config.save_dir;
    Ok(options)
}
/// Protect every discovered installation and save root, including future paths.
pub fn output_policy(project: &Path, game_override: Option<&Path>) -> Result<PathPolicy> {
    let options = options(project, game_override)?;
    let installations = discovery::discover_with(&options)?;
    let mut roots: Vec<_> = installations.into_iter().map(|i| i.path).collect();
    roots.extend(discovery::locate_saves_with(&options));
    roots.extend(options.explicit_game_dir);
    roots.extend(options.explicit_save_dir);
    if let Some(local) = options.local_app_data {
        for relative in [
            "Pearl Abyss/CD/save",
            "Pearl Abyss/CD_Epic/save",
            "Pearl Abyss/CD_GamePass/save",
            "CrimsonDesert/Saved/SaveGames",
        ] {
            roots.push(local.join(relative));
        }
    }
    Ok(PathPolicy::from_roots(roots, project.canonicalize()?)?)
}
pub fn select_game(report: &DiscoveryReport) -> Result<PathBuf> {
    if report.configured_game.is_some() {
        // Discovery puts the explicit selection first and normalizes Xbox
        // wrapper directories to their actual Content installation root.
        return report
            .installations
            .first()
            .map(|v| v.path.clone())
            .ok_or_else(|| Error::Invalid("Configured installation was not discovered".into()));
    }
    match report.installations.as_slice() {
        [one] => Ok(one.path.clone()),
        [] => Err(Error::Invalid(
            "No Crimson Desert installation found. Set CD_GAME_DIR in .env or use --game.".into(),
        )),
        _ => Err(Error::Invalid(
            "Multiple installations found. Select one with --game or CD_GAME_DIR.".into(),
        )),
    }
}
/// Generated game data belongs only in the project's gitignored output roots.
fn generated_path(policy: &PathPolicy, path: &Path, cache: bool) -> Result<PathBuf> {
    let resolved = if cache {
        policy.check_cache(path)?
    } else {
        policy.check_output(path)?
    };
    let first = resolved
        .strip_prefix(policy.output_root())
        .ok()
        .and_then(|p| p.components().next())
        .and_then(|c| c.as_os_str().to_str());
    if !matches!(first, Some(".local" | "exports")) {
        return Err(Error::Invalid(
            "Generated files must be inside .local/ or exports/ in the project".into(),
        ));
    }
    Ok(resolved)
}
pub fn write_generated(policy: &PathPolicy, path: &Path, bytes: &[u8]) -> Result<PathBuf> {
    let path = generated_path(policy, path, false)?;
    Ok(policy.write_new(&path, bytes)?)
}
pub struct Workspace {
    data: GameData,
    policy: PathPolicy,
}
#[derive(Debug, Serialize)]
pub struct IndexBuildInfo {
    pub path: PathBuf,
    pub rebuilt: bool,
    pub items: usize,
    pub language: String,
    pub fingerprint: String,
}
impl Workspace {
    pub fn open(project: &Path, game_override: Option<&Path>, language: &str) -> Result<Self> {
        let report = discover_project(project, game_override)?;
        let game = select_game(&report)?;
        let policy = output_policy(project, game_override)?;
        let data = GameData::open(&game, language)?;
        Ok(Self { data, policy })
    }
    pub fn data(&self) -> &GameData {
        &self.data
    }
    pub fn policy(&self) -> &PathPolicy {
        &self.policy
    }
    pub fn write_json(&self, path: &Path, value: &impl Serialize) -> Result<PathBuf> {
        let mut bytes = serde_json::to_vec_pretty(value)?;
        bytes.push(b'\n');
        write_generated(&self.policy, path, &bytes)
    }
    pub(crate) fn index(&self, cache: Option<&Path>) -> Result<(Index, IndexBuildInfo)> {
        let desired = cache.unwrap_or_else(|| Path::new(".local/index/items.sqlite"));
        let path = generated_path(&self.policy, desired, true)?;
        let parent = path
            .parent()
            .ok_or_else(|| Error::Invalid("Index needs a parent directory".into()))?;
        fs::create_dir_all(parent)?;
        let path = generated_path(&self.policy, &path, true)?;
        let mut index = Index::open(&path)?;
        let items = self.data.localized_items()?;
        let identity = IndexIdentity {
            fingerprint: self.data.fingerprint().digest.clone(),
            language: self.data.language().into(),
            schema_version: self.data.fingerprint().schema_id.clone().ok_or_else(|| {
                Error::Invalid("Cannot index a build without an admitted read schema".into())
            })?,
        };
        let rebuilt = index.rebuild_if_needed(&identity, &items)?;
        let info = IndexBuildInfo {
            path,
            rebuilt,
            items: items.len(),
            language: identity.language,
            fingerprint: identity.fingerprint,
        };
        Ok((index, info))
    }
    pub fn build_index(&self, cache: Option<&Path>) -> Result<IndexBuildInfo> {
        Ok(self.index(cache)?.1)
    }
    pub fn search(&self, query: SearchQuery, cache: Option<&Path>) -> Result<SearchPage> {
        Ok(self.index(cache)?.0.search(query)?)
    }
    pub fn indexed_item(
        &self,
        key: u32,
        cache: Option<&Path>,
    ) -> Result<Option<crate::IndexedItem>> {
        Ok(self.index(cache)?.0.get(key)?)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn explicit_selection_uses_normalized_content_root() {
        let report = DiscoveryReport {
            configured_game: Some(PathBuf::from("XboxGames/Crimson Desert")),
            installations: vec![Installation {
                path: PathBuf::from("XboxGames/Crimson Desert/Content"),
                platform: discovery::Platform::GamePass,
                build_id: None,
            }],
            save_directories: vec![],
        };
        assert_eq!(select_game(&report).unwrap(), report.installations[0].path);
    }
    #[test]
    fn generated_data_cannot_escape_ignored_roots_or_overwrite() {
        let dir = tempfile::tempdir().unwrap();
        let policy = PathPolicy::from_roots(vec![], dir.path().canonicalize().unwrap()).unwrap();
        for path in ["items.json", "src/items.json", ".local/../items.json"] {
            assert!(write_generated(&policy, Path::new(path), b"data").is_err());
            assert!(generated_path(&policy, Path::new(path), true).is_err());
        }
        let saved = write_generated(&policy, Path::new(".local/items.json"), b"one").unwrap();
        assert!(write_generated(&policy, &saved, b"two").is_err());
        assert_eq!(fs::read(saved).unwrap(), b"one");
        assert!(write_generated(&policy, Path::new("exports/items.json"), b"ok").is_ok());
    }
}
