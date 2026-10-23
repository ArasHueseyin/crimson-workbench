//! Read the language configured for this Steam installation, without changing it.
use crate::discovery::{STEAM_APP_ID, Vdf, lookup, parse_vdf, string};
use serde::Serialize;
use std::{fs::File, io::Read, path::Path};

#[derive(Debug, Clone, Serialize, PartialEq, Eq)]
pub struct ItemLanguage {
    pub language: String,
    pub source: &'static str,
}

/// Only inspect the manifest next to the selected installation. A different
/// Steam library, another game, or a global client language must not win.
pub fn resolve(game: &Path, preference: &str) -> ItemLanguage {
    if preference != "auto" {
        return ItemLanguage {
            language: preference.into(),
            source: "manual",
        };
    }
    if let Some(language) = steam_language(game) {
        return ItemLanguage {
            language: language.into(),
            source: "steam",
        };
    }
    ItemLanguage {
        language: "eng".into(),
        source: "fallback",
    }
}

fn steam_language(game: &Path) -> Option<&'static str> {
    let bundle = crate::discovery::mac_bundle(game);
    let game = match &bundle {
        Some(bundle) => bundle.parent()?,
        None => game,
    };
    let common = game.parent()?;
    let steamapps = common.parent()?;
    if !common.file_name()?.eq_ignore_ascii_case("common")
        || !steamapps.file_name()?.eq_ignore_ascii_case("steamapps")
    {
        return None;
    }
    let mut text = String::new();
    File::open(steamapps.join(format!("appmanifest_{STEAM_APP_ID}.acf")))
        .ok()?
        .take(128 * 1024 + 1)
        .read_to_string(&mut text)
        .ok()?;
    if text.len() > 128 * 1024 {
        return None;
    }
    let parsed = parse_vdf(text.trim_start_matches('\u{feff}')).ok()?;
    let Vdf::Object(app) = lookup(&parsed, "AppState")? else {
        return None;
    };
    if string(app, "appid")? != STEAM_APP_ID
        || !game
            .file_name()?
            .eq_ignore_ascii_case(string(app, "installdir")?)
    {
        return None;
    }
    // UserConfig is the requested language, MountedConfig the installed one.
    // Use the installed language while a Steam language download is pending.
    let section = lookup(app, "MountedConfig").or_else(|| lookup(app, "UserConfig"))?;
    let Vdf::Object(config) = section else {
        return None;
    };
    steam_code(string(config, "language")?)
}

fn steam_code(value: &str) -> Option<&'static str> {
    Some(match value.to_ascii_lowercase().as_str() {
        "arabic" => "ara",
        "english" => "eng",
        "french" => "fre",
        "german" => "ger",
        "italian" => "ita",
        "japanese" => "jpn",
        "koreana" => "kor",
        "polish" => "pol",
        "brazilian" => "por-br",
        "russian" => "rus",
        "spanish" => "spa-es",
        "latam" => "spa-mx",
        "turkish" => "tur",
        "schinese" => "zho-cn",
        "tchinese" => "zho-tw",
        _ => return None,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;
    #[test]
    fn follows_installed_game_language_on_every_open_without_writes() {
        let root = tempfile::tempdir().unwrap();
        let apps = root.path().join("steamapps");
        let game = apps.join("common/Crimson Desert");
        fs::create_dir_all(&game).unwrap();
        let bundle = game.join("CrimsonDesert_Steam.app");
        let packages = bundle.join("Contents/Resources/packages");
        fs::create_dir_all(&packages).unwrap();
        fs::create_dir_all(bundle.join("Contents/MacOS")).unwrap();
        fs::write(
            bundle.join("Contents/MacOS/CrimsonDesert_Steam"),
            b"fixture",
        )
        .unwrap();
        let manifest = apps.join(format!("appmanifest_{STEAM_APP_ID}.acf"));
        for (steam, code) in [
            ("german", "ger"),
            ("english", "eng"),
            ("brazilian", "por-br"),
        ] {
            let text = format!(
                r#""AppState" {{ "appid" "3321460" "installdir" "Crimson Desert" "UserConfig" {{ "language" "french" }} "MountedConfig" {{ "language" "{steam}" }} }}"#
            );
            fs::write(&manifest, &text).unwrap();
            assert_eq!(
                resolve(&game, "auto"),
                ItemLanguage {
                    language: code.into(),
                    source: "steam"
                }
            );
            assert_eq!(resolve(&game, "jpn").language, "jpn");
            assert_eq!(resolve(&packages, "auto"), resolve(&game, "auto"));
            assert_eq!(fs::read_to_string(&manifest).unwrap(), text);
        }
    }
    #[test]
    fn unrelated_missing_malformed_and_unknown_settings_fall_back_to_english() {
        let root = tempfile::tempdir().unwrap();
        let apps = root.path().join("steamapps");
        let game = apps.join("common/Crimson Desert");
        fs::create_dir_all(&game).unwrap();
        assert_eq!(resolve(&game, "auto").source, "fallback");
        let manifest = apps.join(format!("appmanifest_{STEAM_APP_ID}.acf"));
        for text in [
            "broken",
            r#""AppState" { "appid" "123" "installdir" "Crimson Desert" "UserConfig" { "language" "german" } }"#,
            r#""AppState" { "appid" "3321460" "installdir" "Other Game" "UserConfig" { "language" "german" } }"#,
            r#""AppState" { "appid" "3321460" "installdir" "Crimson Desert" "UserConfig" { "language" "unknown" } }"#,
        ] {
            fs::write(&manifest, text).unwrap();
            assert_eq!(
                resolve(&game, "auto"),
                ItemLanguage {
                    language: "eng".into(),
                    source: "fallback"
                }
            );
        }
        fs::write(&manifest, r#""AppState" { "appid" "3321460" "installdir" "Crimson Desert" "UserConfig" { "language" "german" } }"#).unwrap();
        assert_eq!(resolve(&game, "auto").language, "ger");
        assert_eq!(resolve(root.path(), "auto").source, "fallback");
        fs::write(&manifest, " ".repeat(128 * 1024 + 1)).unwrap();
        assert_eq!(resolve(&game, "auto").source, "fallback");
    }
    #[test]
    fn steam_codes_cover_every_supported_catalog_language() {
        for code in [
            "arabic",
            "english",
            "french",
            "german",
            "italian",
            "japanese",
            "koreana",
            "polish",
            "brazilian",
            "russian",
            "spanish",
            "latam",
            "turkish",
            "schinese",
            "tchinese",
        ] {
            assert!(
                crate::supported_languages()
                    .iter()
                    .any(|l| Some(l.language.as_str()) == steam_code(code))
            );
        }
    }
}
