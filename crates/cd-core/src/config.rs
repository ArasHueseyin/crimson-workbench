//! Local configuration without changing process environment or writing source data.
use std::{
    collections::BTreeMap,
    fs, io,
    path::{Path, PathBuf},
};

#[derive(Debug, Default, Clone)]
pub struct LocalConfig {
    pub game_dir: Option<PathBuf>,
    pub save_dir: Option<PathBuf>,
}

impl LocalConfig {
    pub fn load(project: &Path) -> io::Result<Self> {
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
            game_dir: get("CD_GAME_DIR"),
            save_dir: get("CD_SAVE_DIR"),
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
}
