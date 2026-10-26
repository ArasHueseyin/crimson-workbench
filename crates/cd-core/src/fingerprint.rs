//! A known read schema is not a certification that an installation is vanilla.
use crate::discovery::read_exe_version;
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::{
    collections::BTreeMap,
    fs::File,
    io::{self, Read},
    path::Path,
    sync::OnceLock,
};

pub const SCHEMA_ID: &str = "steam-25381195-gamedata-2.3-v2";
const OBSERVATION: &str = include_str!("../../../docs/builds/steam-25381195.observed.json");
const UPDATE_OBSERVATION: &str = include_str!("../../../docs/builds/steam-25455892.observed.json");
const CURRENT_OBSERVATION: &str = include_str!("../../../docs/builds/steam-25477059.observed.json");

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct FileHash {
    pub path: String,
    pub bytes: u64,
    pub sha256: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TableHash {
    pub group: String,
    pub directory: String,
    pub name: String,
    pub size: u64,
    pub sha256: String,
}

#[derive(Clone, Debug, Deserialize)]
pub(crate) struct KnownBuild {
    pub steam_buildid: String,
    pub exe_version: String,
    pub files: Vec<FileHash>,
    /// Observed files in optional groups, as marked by the original PAPGT.
    #[serde(default)]
    pub optional_files: Vec<String>,
    pub tables: Vec<TableHash>,
}

impl KnownBuild {
    pub(crate) fn is_optional_file(&self, path: &str) -> bool {
        self.optional_files.iter().any(|optional| optional == path)
    }

    pub(crate) fn schema_id(&self) -> &'static str {
        match self.exe_version.as_str() {
            "1.0.0.2944" => SCHEMA_ID,
            "1.0.0.2949" => "steam-25455892-gamedata-2.3-v2",
            "1.0.0.2976" => "steam-25477059-gamedata-2.3-v2",
            _ => unreachable!("only compiled build observations are admitted"),
        }
    }
}
pub(crate) fn known_builds() -> &'static [KnownBuild] {
    static BUILDS: OnceLock<Vec<KnownBuild>> = OnceLock::new();
    BUILDS.get_or_init(|| {
        [OBSERVATION, UPDATE_OBSERVATION, CURRENT_OBSERVATION]
            .iter()
            .map(|source| {
                serde_json::from_str(source.trim_start_matches('\u{feff}'))
                    .expect("checked-in build metadata must be valid")
            })
            .collect()
    })
}
pub(crate) fn known_for_version(version: &str) -> Option<&'static KnownBuild> {
    known_builds()
        .iter()
        .find(|build| build.exe_version == version)
}
pub(crate) fn known_for_game(game: &Path) -> crate::Result<&'static KnownBuild> {
    let version = read_exe_version(&game.join("bin64/CrimsonDesert.exe"))?;
    version
        .as_deref()
        .and_then(known_for_version)
        .ok_or_else(|| {
            crate::Error::UnsupportedBuild(format!(
                "EXE version {version:?}; supported: 1.0.0.2944, 1.0.0.2949, 1.0.0.2976"
            ))
        })
}
// Original observation retained for historic fixtures and unknown-build diagnostics.
#[cfg(test)]
pub(crate) fn known() -> KnownBuild {
    known_builds()[0].clone()
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Fingerprint {
    pub format_version: u32,
    pub exe_version: Option<String>,
    pub schema_id: Option<String>,
    pub metadata_matches: bool,
    pub read_schema_supported: bool,
    /// Always false in Phase 1: hashes do not prove provenance.
    pub certified_vanilla: bool,
    pub digest: String,
    pub files: Vec<FileHash>,
    pub tables: Vec<TableHash>,
    pub diagnostics: Vec<String>,
}

impl Fingerprint {
    /// Diagnostic only: no table is semantically parsed during this operation.
    pub fn inspect(game: &Path) -> io::Result<Self> {
        Self::inspect_registry(game, None)
    }
    pub(crate) fn inspect_registry(game: &Path, original: Option<&[u8]>) -> io::Result<Self> {
        let root = game.canonicalize()?;
        let mut diagnostics = Vec::new();
        let exe_version = match read_exe_version(&root.join("bin64/CrimsonDesert.exe")) {
            Ok(v) => v,
            Err(e) => {
                diagnostics.push(format!("EXE version: {e}"));
                None
            }
        };
        let selected = exe_version.as_deref().and_then(known_for_version);
        let expected = selected.unwrap_or(&known_builds()[0]);
        if selected.is_none() {
            diagnostics.push(format!(
                "Unknown EXE version {:?}; supported versions: 1.0.0.2944, 1.0.0.2949, 1.0.0.2976",
                exe_version
            ));
        }
        let mut files = inspect_files(&root, expected, original, &mut diagnostics);
        files.sort_by(|a, b| a.path.cmp(&b.path));
        let digest = hash_bytes(&serde_json::to_vec(&(exe_version.clone(), &files))?);
        let supported = diagnostics.is_empty();
        Ok(Self {
            format_version: 1,
            exe_version,
            schema_id: None,
            metadata_matches: supported,
            read_schema_supported: false,
            certified_vanilla: false,
            digest,
            files,
            tables: Vec::new(),
            diagnostics,
        })
    }

    pub(crate) fn complete(&mut self, mut tables: Vec<TableHash>) -> io::Result<()> {
        tables.sort_by(|a, b| {
            (&a.group, &a.directory, &a.name).cmp(&(&b.group, &b.directory, &b.name))
        });
        self.digest = hash_bytes(&serde_json::to_vec(&(
            self.exe_version.clone(),
            &self.files,
            &tables,
        ))?);
        self.tables = tables;
        self.read_schema_supported = self.metadata_matches
            && self.diagnostics.is_empty()
            && self
                .exe_version
                .as_deref()
                .and_then(known_for_version)
                .is_some();
        self.schema_id = if self.read_schema_supported {
            self.exe_version
                .as_deref()
                .and_then(known_for_version)
                .map(|b| b.schema_id().into())
        } else {
            None
        };
        Ok(())
    }
}

fn inspect_files(
    root: &Path,
    expected: &KnownBuild,
    original: Option<&[u8]>,
    diagnostics: &mut Vec<String>,
) -> Vec<FileHash> {
    let mut files = Vec::new();
    for reference in &expected.files {
        let path = root.join(&reference.path);
        let digest = if reference.path == "meta/0.papgt" {
            original
                .map(|b| Ok((b.len() as u64, hash_bytes(b))))
                .unwrap_or_else(|| hash_file(&path))
        } else {
            hash_file(&path)
        };
        match digest {
            Ok((bytes, sha256)) => {
                if bytes != reference.bytes || sha256 != reference.sha256 {
                    diagnostics.push(format!("Unknown or changed file: {}", reference.path));
                }
                files.push(FileHash {
                    path: reference.path.clone(),
                    bytes,
                    sha256,
                });
            }
            Err(e)
                if e.kind() == io::ErrorKind::NotFound
                    && expected.is_optional_file(&reference.path)
                    // Only an absent optional group is valid. An installed group
                    // with missing metadata, a broken link or an IO error is not.
                    && path.parent().is_some_and(|group| {
                        std::fs::symlink_metadata(group)
                            .is_err_and(|e| e.kind() == io::ErrorKind::NotFound)
                    }) => {}
            Err(e) => diagnostics.push(format!("Cannot read {}: {e}", reference.path)),
        }
    }
    files
}

pub fn hash_bytes(bytes: &[u8]) -> String {
    format!("{:x}", Sha256::digest(bytes))
}

pub fn hash_file(path: &Path) -> io::Result<(u64, String)> {
    let mut file = File::open(path)?;
    let mut hasher = Sha256::new();
    let mut count = 0u64;
    let mut buffer = [0u8; 128 * 1024];
    loop {
        let n = file.read(&mut buffer)?;
        if n == 0 {
            break;
        }
        hasher.update(&buffer[..n]);
        count += n as u64;
    }
    Ok((count, format!("{:x}", hasher.finalize())))
}

#[derive(Debug, Serialize)]
pub struct HashDifference {
    pub path: String,
    pub before: Option<String>,
    pub after: Option<String>,
}

/// Compare diagnostic manifests even when a newer build has no supported schema.
pub fn diff(before: &Fingerprint, after: &Fingerprint) -> Vec<HashDifference> {
    fn entries(f: &Fingerprint) -> BTreeMap<String, String> {
        let mut out: BTreeMap<_, _> = f
            .files
            .iter()
            .map(|v| (v.path.clone(), v.sha256.clone()))
            .collect();
        for v in &f.tables {
            out.insert(
                format!("{}/{}/{}", v.group, v.directory, v.name),
                v.sha256.clone(),
            );
        }
        out.insert(
            "@exe_version".into(),
            f.exe_version.clone().unwrap_or_default(),
        );
        out
    }
    let left = entries(before);
    let right = entries(after);
    let mut keys: Vec<_> = left.keys().chain(right.keys()).cloned().collect();
    keys.sort();
    keys.dedup();
    keys.into_iter()
        .filter_map(|path| {
            let a = left.get(&path).cloned();
            let b = right.get(&path).cloned();
            (a != b).then_some(HashDifference {
                path,
                before: a,
                after: b,
            })
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;

    fn metadata_fixture(root: &Path) -> KnownBuild {
        let mut files = Vec::new();
        for (path, bytes) in [
            (
                "bin64/CrimsonDesert.exe",
                b"synthetic executable".as_slice(),
            ),
            ("meta/0.papgt", b"synthetic registry".as_slice()),
            ("0008/0.pamt", b"required tables".as_slice()),
            ("0037/0.pamt", b"optional language".as_slice()),
        ] {
            files.push(FileHash {
                path: path.into(),
                bytes: bytes.len() as u64,
                sha256: hash_bytes(bytes),
            });
            if path != "0037/0.pamt" {
                let file = root.join(path);
                fs::create_dir_all(file.parent().unwrap()).unwrap();
                fs::write(file, bytes).unwrap();
            }
        }
        KnownBuild {
            steam_buildid: "25477059".into(),
            exe_version: "1.0.0.2976".into(),
            files,
            optional_files: vec!["0037/0.pamt".into()],
            tables: vec![],
        }
    }

    #[test]
    fn absent_optional_language_group_does_not_block_other_language_installations() {
        let temp = tempfile::tempdir().unwrap();
        let expected = metadata_fixture(temp.path());
        fs::create_dir(temp.path().join("0039")).unwrap();
        fs::write(temp.path().join("0039/0.pamt"), b"other language").unwrap();
        let mut diagnostics = Vec::new();
        let files = inspect_files(temp.path(), &expected, None, &mut diagnostics);
        assert!(diagnostics.is_empty(), "{diagnostics:?}");
        assert_eq!(files, expected.files[..3]);
        assert!(!temp.path().join("0037").exists());
        assert_eq!(
            fs::read(temp.path().join("0039/0.pamt")).unwrap(),
            b"other language"
        );

        // Without the optional designation this is the reported v0.10.0 failure.
        let mut previous = expected;
        previous.optional_files.clear();
        inspect_files(temp.path(), &previous, None, &mut diagnostics);
        assert_eq!(diagnostics.len(), 1);
        assert!(diagnostics[0].starts_with("Cannot read 0037/0.pamt:"));
    }

    #[test]
    fn installed_optional_metadata_still_requires_its_exact_hash_and_size() {
        let temp = tempfile::tempdir().unwrap();
        let expected = metadata_fixture(temp.path());
        fs::create_dir(temp.path().join("0037")).unwrap();
        let path = temp.path().join("0037/0.pamt");
        fs::write(&path, b"optional language").unwrap();
        let mut diagnostics = Vec::new();
        assert_eq!(
            inspect_files(temp.path(), &expected, None, &mut diagnostics),
            expected.files
        );
        assert!(diagnostics.is_empty());
        for changed in [b"Optional language".as_slice(), b"truncated".as_slice()] {
            fs::write(&path, changed).unwrap();
            diagnostics.clear();
            inspect_files(temp.path(), &expected, None, &mut diagnostics);
            assert_eq!(diagnostics, ["Unknown or changed file: 0037/0.pamt"]);
        }
    }

    #[test]
    fn optional_group_with_missing_or_unreadable_metadata_is_rejected() {
        let temp = tempfile::tempdir().unwrap();
        let expected = metadata_fixture(temp.path());
        fs::create_dir(temp.path().join("0037")).unwrap();
        fs::write(temp.path().join("0037/0.paz"), b"partial installation").unwrap();
        for directory_instead_of_file in [false, true] {
            if directory_instead_of_file {
                fs::create_dir(temp.path().join("0037/0.pamt")).unwrap();
            }
            let mut diagnostics = Vec::new();
            inspect_files(temp.path(), &expected, None, &mut diagnostics);
            assert_eq!(diagnostics.len(), 1);
            assert!(diagnostics[0].starts_with("Cannot read 0037/0.pamt:"));
        }
    }

    #[test]
    fn missing_or_changed_required_files_remain_rejected_without_optional_group() {
        for path in ["bin64/CrimsonDesert.exe", "meta/0.papgt", "0008/0.pamt"] {
            let temp = tempfile::tempdir().unwrap();
            let expected = metadata_fixture(temp.path());
            fs::remove_file(temp.path().join(path)).unwrap();
            let mut diagnostics = Vec::new();
            inspect_files(temp.path(), &expected, None, &mut diagnostics);
            assert_eq!(diagnostics.len(), 1);
            assert!(diagnostics[0].starts_with(&format!("Cannot read {path}:")));
            fs::write(temp.path().join(path), b"modified").unwrap();
            diagnostics.clear();
            inspect_files(temp.path(), &expected, None, &mut diagnostics);
            assert_eq!(diagnostics, [format!("Unknown or changed file: {path}")]);
        }
    }

    #[test]
    fn original_registry_is_still_verified_for_installations_with_overlays() {
        let temp = tempfile::tempdir().unwrap();
        let expected = metadata_fixture(temp.path());
        fs::write(
            temp.path().join("meta/0.papgt"),
            b"managed overlay registry",
        )
        .unwrap();
        let mut diagnostics = Vec::new();
        inspect_files(
            temp.path(),
            &expected,
            Some(b"synthetic registry"),
            &mut diagnostics,
        );
        assert!(diagnostics.is_empty());
        inspect_files(
            temp.path(),
            &expected,
            Some(b"wrong original"),
            &mut diagnostics,
        );
        assert_eq!(diagnostics, ["Unknown or changed file: meta/0.papgt"]);
    }

    #[cfg(unix)]
    #[test]
    fn broken_optional_group_link_is_not_treated_as_an_absent_group() {
        let temp = tempfile::tempdir().unwrap();
        let expected = metadata_fixture(temp.path());
        std::os::unix::fs::symlink("missing", temp.path().join("0037")).unwrap();
        let mut diagnostics = Vec::new();
        inspect_files(temp.path(), &expected, None, &mut diagnostics);
        assert_eq!(diagnostics.len(), 1);
        assert!(diagnostics[0].starts_with("Cannot read 0037/0.pamt:"));
    }

    #[test]
    fn manifest_contains_actual_table_pairs() {
        for b in known_builds() {
            for optional in &b.optional_files {
                assert!(optional.ends_with("/0.pamt"));
                assert!(b.files.iter().any(|file| &file.path == optional));
                let group = optional.strip_suffix("/0.pamt").unwrap();
                assert!(!b.tables.iter().any(|table| table.group == group));
                assert!(
                    !crate::supported_languages()
                        .iter()
                        .any(|locale| locale.group == group)
                );
            }
            assert_eq!(b.tables.len(), 28);
            for body in b
                .tables
                .iter()
                .filter(|t| t.name.ends_with(".staticinfobody"))
            {
                let header = body.name.replace(".staticinfobody", ".staticinfoheader");
                assert!(b.tables.iter().any(|t| t.name == header));
            }
        }
    }
    #[test]
    fn build_selection_retains_old_and_new_identities_without_fallback_admission() {
        let old = known_for_version("1.0.0.2944").unwrap();
        let new = known_for_version("1.0.0.2949").unwrap();
        assert_ne!(old.schema_id(), new.schema_id());
        assert!(known_for_version("1.0.0.2950").is_none());
        for name in [
            "iteminfo.staticinfobody",
            "skill.staticinfobody",
            "inventory.staticinfobody",
        ] {
            let a = old.tables.iter().find(|r| r.name == name).unwrap();
            let b = new.tables.iter().find(|r| r.name == name).unwrap();
            assert_eq!(a.sha256, b.sha256);
        }
        for name in ["questinfo.staticinfobody", "stageinfo.staticinfobody"] {
            let a = old.tables.iter().find(|r| r.name == name).unwrap();
            let b = new.tables.iter().find(|r| r.name == name).unwrap();
            assert_ne!(a.sha256, b.sha256);
        }
        assert_eq!(
            old.files
                .iter()
                .find(|f| f.path == "bin64/CrimsonDesert.exe")
                .unwrap()
                .sha256,
            "6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7"
        );
        assert_eq!(
            new.files
                .iter()
                .find(|f| f.path == "bin64/CrimsonDesert.exe")
                .unwrap()
                .sha256,
            "a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a"
        );
    }
    #[test]
    fn modified_installation_is_diagnostic_only() {
        let temp = tempfile::tempdir().unwrap();
        std::fs::create_dir(temp.path().join("bin64")).unwrap();
        std::fs::write(
            temp.path().join("bin64/CrimsonDesert.exe"),
            b"not a known executable",
        )
        .unwrap();
        let probe = Fingerprint::inspect(temp.path()).unwrap();
        assert!(!probe.read_schema_supported);
        assert!(!probe.certified_vanilla);
        assert!(probe.schema_id.is_none());
        assert!(!probe.diagnostics.is_empty());
    }
    #[test]
    fn differences_include_added_removed_and_changed() {
        let mut a = Fingerprint {
            format_version: 1,
            exe_version: Some("1".into()),
            schema_id: None,
            metadata_matches: false,
            read_schema_supported: false,
            certified_vanilla: false,
            digest: String::new(),
            files: vec![
                FileHash {
                    path: "a".into(),
                    bytes: 1,
                    sha256: "old".into(),
                },
                FileHash {
                    path: "removed".into(),
                    bytes: 1,
                    sha256: "gone".into(),
                },
            ],
            tables: vec![],
            diagnostics: vec![],
        };
        let mut b = a.clone();
        b.files.remove(1);
        b.files[0].sha256 = "new".into();
        b.files.push(FileHash {
            path: "added".into(),
            bytes: 1,
            sha256: "new".into(),
        });
        assert_eq!(diff(&a, &b).len(), 3);
        a.exe_version = Some("2".into());
        assert_eq!(diff(&a, &b).len(), 4);
        assert!(diff(&a, &a).is_empty());
    }
}
