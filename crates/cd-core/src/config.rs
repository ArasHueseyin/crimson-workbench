//! Local configuration without changing process environment or writing source data.
use serde::{Deserialize, Serialize};
use std::{
    collections::BTreeMap,
    fs, io,
    path::{Path, PathBuf},
};

/// Per-workspace user preferences. These contain paths, never game/save bytes.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct Preferences {
    pub version: u32,
    pub game_dir: Option<PathBuf>,
    pub save_dir: Option<PathBuf>,
    pub language: String,
}
impl Default for Preferences {
    fn default() -> Self {
        Self {
            version: 1,
            game_dir: None,
            save_dir: None,
            language: "ger".into(),
        }
    }
}
impl Preferences {
    pub fn load(project: &Path) -> io::Result<Self> {
        let path = project.join("settings.json");
        let metadata = match fs::symlink_metadata(&path) {
            Ok(value) => value,
            Err(e) if e.kind() == io::ErrorKind::NotFound => return Ok(Self::default()),
            Err(e) => return Err(e),
        };
        ordinary_settings_file(&path, &metadata)?;
        if metadata.len() > 32 * 1024 {
            return Err(io::Error::new(
                io::ErrorKind::InvalidData,
                "Einstellungsdatei ist zu groß.",
            ));
        }
        let preferences: Self = serde_json::from_slice(&fs::read(path)?)?;
        if preferences.version != 1
            || !crate::supported_languages()
                .iter()
                .any(|l| l.language == preferences.language)
        {
            return Err(io::Error::new(
                io::ErrorKind::InvalidData,
                "Unbekannte Einstellungsversion oder Sprache.",
            ));
        }
        for value in [&preferences.game_dir, &preferences.save_dir]
            .into_iter()
            .flatten()
        {
            if !value.is_absolute() {
                return Err(io::Error::new(
                    io::ErrorKind::InvalidData,
                    "Gespeicherte Pfade müssen absolut sein.",
                ));
            }
        }
        Ok(preferences)
    }

    /// Replace only the application-owned settings file, atomically on the same volume.
    pub fn store(&self, project: &Path) -> io::Result<()> {
        use std::io::Write;
        use std::sync::atomic::{AtomicU64, Ordering};
        static NEXT: AtomicU64 = AtomicU64::new(0);
        let destination = project.join("settings.json");
        match fs::symlink_metadata(&destination) {
            Ok(meta) => ordinary_settings_file(&destination, &meta)?,
            Err(e) if e.kind() == io::ErrorKind::NotFound => {}
            Err(e) => return Err(e),
        }
        let temporary = project.join(format!(
            ".settings-{}-{}.tmp",
            std::process::id(),
            NEXT.fetch_add(1, Ordering::Relaxed)
        ));
        let mut file = fs::OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(&temporary)?;
        let result = (|| {
            file.write_all(&serde_json::to_vec_pretty(self)?)?;
            file.write_all(b"\n")?;
            file.sync_all()?;
            drop(file);
            #[cfg(windows)]
            {
                use std::os::windows::ffi::OsStrExt;
                use windows_sys::Win32::Storage::FileSystem::{
                    MOVEFILE_REPLACE_EXISTING, MOVEFILE_WRITE_THROUGH, MoveFileExW,
                };
                let from: Vec<u16> = temporary.as_os_str().encode_wide().chain(Some(0)).collect();
                let to: Vec<u16> = destination
                    .as_os_str()
                    .encode_wide()
                    .chain(Some(0))
                    .collect();
                if unsafe {
                    MoveFileExW(
                        from.as_ptr(),
                        to.as_ptr(),
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH,
                    )
                } == 0
                {
                    return Err(io::Error::last_os_error());
                }
            }
            #[cfg(not(windows))]
            {
                fs::rename(&temporary, &destination)?;
                fs::File::open(project)?.sync_all()?;
            }
            Ok(())
        })();
        if result.is_err() {
            let _ = fs::remove_file(&temporary);
        }
        result
    }
}
fn ordinary_settings_file(path: &Path, metadata: &fs::Metadata) -> io::Result<()> {
    #[cfg(windows)]
    {
        use std::os::windows::fs::MetadataExt;
        if metadata.file_attributes() & 0x400 != 0 {
            return Err(io::Error::new(
                io::ErrorKind::PermissionDenied,
                "Verknüpfte Einstellungsdatei wird nicht verwendet.",
            ));
        }
    }
    if !metadata.is_file()
        || metadata.file_type().is_symlink()
        || crate::paths::link_count(path, metadata)? != 1
    {
        return Err(io::Error::new(
            io::ErrorKind::PermissionDenied,
            "Einstellungsdatei muss eine eigene reguläre Datei sein.",
        ));
    }
    Ok(())
}

#[derive(Debug, Default, Clone)]
pub struct LocalConfig {
    pub game_dir: Option<PathBuf>,
    pub save_dir: Option<PathBuf>,
}

impl LocalConfig {
    pub fn load(project: &Path) -> io::Result<Self> {
        // Bootstrap reports damaged preferences and lets the user replace them.
        // Never let their contents redirect game/save access after a parse failure.
        let preferences = Preferences::load(project).unwrap_or_default();
        let vars = match fs::read_to_string(project.join(".env")) {
            Ok(text) => parse_env(&text)?,
            Err(e) if e.kind() == io::ErrorKind::NotFound => BTreeMap::new(),
            Err(e) => return Err(e),
        };
        let get = |key: &str| -> Option<PathBuf> {
            std::env::var_os(key)
                .filter(|v| !v.is_empty())
                .map(PathBuf::from)
                .or_else(|| vars.get(key).filter(|v| !v.is_empty()).map(PathBuf::from))
                .map(|p| if p.is_absolute() { p } else { project.join(p) })
        };
        Ok(Self {
            game_dir: preferences.game_dir.or_else(|| get("CD_GAME_DIR")),
            save_dir: preferences.save_dir.or_else(|| get("CD_SAVE_DIR")),
        })
    }
}

fn parse_env(text: &str) -> io::Result<BTreeMap<String, String>> {
    let mut out = BTreeMap::new();
    for (line_no, line) in text.trim_start_matches('\u{feff}').lines().enumerate() {
        let line = line.trim();
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        let line = line.strip_prefix("export ").unwrap_or(line);
        let (key, value) = line.split_once('=').ok_or_else(|| {
            io::Error::new(
                io::ErrorKind::InvalidData,
                format!(".env line {}: expected KEY=VALUE", line_no + 1),
            )
        })?;
        let value = value.trim();
        let value = if value.starts_with(['\'', '"']) {
            let quote = value.as_bytes()[0];
            if value.len() < 2 || value.as_bytes().last() != Some(&quote) {
                return Err(io::Error::new(
                    io::ErrorKind::InvalidData,
                    format!(".env line {}: unmatched quote", line_no + 1),
                ));
            }
            &value[1..value.len() - 1]
        } else {
            value
        };
        out.insert(key.trim().to_owned(), value.to_owned());
    }
    Ok(out)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn windows_paths_and_bom_are_literal() {
        let env = parse_env(
            "\u{feff}# local\nCD_GAME_DIR=\"C:\\Steam Games\\CD\"\nCD_SAVE_DIR='D:\\save#1'\n",
        )
        .unwrap();
        assert_eq!(env["CD_GAME_DIR"], "C:\\Steam Games\\CD");
        assert_eq!(env["CD_SAVE_DIR"], "D:\\save#1");
        assert!(parse_env("CD_GAME_DIR=\"unfinished").is_err());
    }
    #[test]
    fn preferences_roundtrip_replace_and_recover_without_touching_sources() {
        let project = tempfile::tempdir().unwrap();
        let game = project.path().join("synthetic-game");
        let save = project.path().join("synthetic-saves");
        fs::create_dir(&game).unwrap();
        fs::create_dir(&save).unwrap();
        fs::write(game.join("unchanged"), b"game").unwrap();
        fs::write(save.join("unchanged"), b"save").unwrap();
        let preferences = Preferences {
            version: 1,
            game_dir: Some(game.clone()),
            save_dir: Some(save.clone()),
            language: "eng".into(),
        };
        preferences.store(project.path()).unwrap();
        assert_eq!(Preferences::load(project.path()).unwrap(), preferences);
        let next = Preferences {
            language: "ger".into(),
            ..preferences
        };
        next.store(project.path()).unwrap();
        assert_eq!(Preferences::load(project.path()).unwrap(), next);
        fs::write(
            project.path().join("settings.json"),
            b"interrupted old JSON",
        )
        .unwrap();
        assert!(Preferences::load(project.path()).is_err());
        next.store(project.path()).unwrap();
        assert_eq!(Preferences::load(project.path()).unwrap(), next);
        assert_eq!(fs::read(game.join("unchanged")).unwrap(), b"game");
        assert_eq!(fs::read(save.join("unchanged")).unwrap(), b"save");
        assert!(
            !fs::read_dir(project.path()).unwrap().any(|e| e
                .unwrap()
                .file_name()
                .to_string_lossy()
                .ends_with(".tmp"))
        );
    }
    #[test]
    fn linked_settings_are_never_read_or_overwritten() {
        let project = tempfile::tempdir().unwrap();
        let other = project.path().join("private-data");
        fs::write(&other, b"do not touch").unwrap();
        fs::hard_link(&other, project.path().join("settings.json")).unwrap();
        assert!(Preferences::load(project.path()).is_err());
        assert!(Preferences::default().store(project.path()).is_err());
        assert_eq!(fs::read(&other).unwrap(), b"do not touch");
    }
    #[test]
    fn unknown_settings_and_relative_paths_are_rejected() {
        let project = tempfile::tempdir().unwrap();
        for raw in [
            r#"{"version":2,"game_dir":null,"save_dir":null,"language":"ger"}"#,
            r#"{"version":1,"game_dir":"relative","save_dir":null,"language":"ger"}"#,
            r#"{"version":1,"game_dir":null,"save_dir":null,"language":"xx"}"#,
        ] {
            fs::write(project.path().join("settings.json"), raw).unwrap();
            assert!(Preferences::load(project.path()).is_err());
        }
    }
}
