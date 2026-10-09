use super::*;
pub(super) struct Fixture {
    _dir: tempfile::TempDir,
    pub(super) root: PathBuf,
    pub(super) cache: PathBuf,
    pub(super) app: PathBuf,
}
impl Fixture {
    pub(super) fn new() -> Self {
        let dir = tempfile::tempdir().unwrap();
        let base = dir.path().canonicalize().unwrap();
        let root = base.join("steamapps/common/Game");
        let cache = base.join("depotcache");
        let app = base.join("steamapps/appmanifest_3321460.acf");
        fs::create_dir_all(&cache).unwrap();
        for p in ["bin64", "meta", "0000"] {
            fs::create_dir_all(root.join(p)).unwrap();
        }
        let mut registry = vec![0; 12];
        registry[8] = 2;
        for (offset, optional) in [(0u32, 0u8), (5, 1)] {
            registry.push(optional);
            registry.extend(0x7fffu16.to_le_bytes());
            registry.push(0);
            registry.extend(offset.to_le_bytes());
            registry.extend(0u32.to_le_bytes());
        }
        registry.extend(10u32.to_le_bytes());
        registry.extend(b"0000\0");
        registry.extend(b"0001\0");
        let crc = crimson_format::checksum(&registry[12..]);
        registry[4..8].copy_from_slice(&crc.to_le_bytes());
        fs::write(root.join("meta/0.papgt"), &registry).unwrap();
        fs::write(
            root.join("bin64/CrimsonDesert.exe"),
            b"own synthetic executable",
        )
        .unwrap();
        fs::write(root.join("0000/0.pamt"), b"own synthetic metadata").unwrap();
        fs::write(root.join("0000/0.paz"), b"own synthetic archive").unwrap();
        let paths: Vec<_> = [
            "meta/0.papgt",
            "bin64/CrimsonDesert.exe",
            "0000/0.pamt",
            "0000/0.paz",
        ]
        .into_iter()
        .map(|p| (p, fs::metadata(root.join(p)).unwrap().len()))
        .collect();
        let size: u64 = paths.iter().map(|(_, n)| n).sum();
        fs::write(
            cache.join("1_2.manifest"),
            depot::tests::fixture_hashes(
                &paths
                    .iter()
                    .map(|(p, n)| {
                        use sha1::Digest;
                        (
                            *p,
                            *n,
                            sha1::Sha1::digest(fs::read(root.join(p)).unwrap()).into(),
                        )
                    })
                    .collect::<Vec<_>>(),
                1,
                2,
            ),
        )
        .unwrap();
        fs::write(&app,format!(r#""AppState" {{ "appid" "3321460" "installdir" "Game" "buildid" "123" "TargetBuildID" "123" "InstalledDepots" {{ "1" {{ "manifest" "2" "size" "{size}" }} }} }}"#)).unwrap();
        Self {
            _dir: dir,
            root,
            cache,
            app,
        }
    }
    fn inspect(&self) -> Inventory {
        inspect_with(&self.root, &[]).unwrap()
    }
}
#[test]
fn matching_metadata_never_certifies_or_enables_writes() {
    let f = Fixture::new();
    let before = fs::read(f.root.join("meta/0.papgt")).unwrap();
    let r = f.inspect();
    assert!(r.directory_scan_complete && r.depot_comparison_available);
    assert_eq!(r.expected_files, 4);
    assert_eq!(r.actual_files, 4);
    assert!(!r.content_verified && !r.certified_vanilla && !r.can_apply);
    assert!(r.depots.iter().all(|d| !d.authenticated));
    assert!(
        r.issues.is_empty(),
        "{}",
        serde_json::to_string(&r.issues).unwrap()
    );
    assert_eq!(r.executables, vec!["bin64/CrimsonDesert.exe"]);
    assert!(r.files.iter().all(|f| f.state == "size_matches"));
    assert_eq!(r.groups.len(), 2);
    assert!(!r.groups[1].installed && r.groups[1].optional);
    assert_eq!(fs::read(f.root.join("meta/0.papgt")).unwrap(), before);
    assert!(!f.root.join(".workbench").exists());
    // Same-size content modifications remain explicitly unverified.
    let archive = f.root.join("0000/0.paz");
    let len = fs::metadata(&archive).unwrap().len() as usize;
    fs::write(archive, vec![b'X'; len]).unwrap();
    let r = f.inspect();
    assert!(r.files.iter().all(|f| f.state == "size_matches"));
    assert!(!r.content_verified && !r.can_apply);
}
#[test]
fn detects_missing_changed_extra_files_and_unregistered_groups() {
    let f = Fixture::new();
    fs::remove_file(f.root.join("bin64/CrimsonDesert.exe")).unwrap();
    fs::write(f.root.join("0000/0.paz"), b"changed size").unwrap();
    fs::create_dir(f.root.join("0042")).unwrap();
    fs::write(f.root.join("0042/0.pamt"), b"extra").unwrap();
    fs::write(f.root.join("bin64/Other.exe"), b"extra").unwrap();
    let r = f.inspect();
    for code in ["missing", "size_mismatch", "unregistered_group"] {
        assert!(r.issues.iter().any(|i| i.code == code), "{code}");
    }
    assert!(
        r.files
            .iter()
            .any(|f| f.path == "bin64/Other.exe" && f.state == "additional")
    );
    assert_eq!(r.executables.len(), 2);
    assert!(!r.can_apply);
}
#[test]
fn wrong_cache_identity_corruption_and_missing_cache_are_not_partial_success() {
    let f = Fixture::new();
    let p = f.cache.join("1_2.manifest");
    fs::write(&p, depot::tests::fixture(&[("x", 1)], 3, 4)).unwrap();
    for bytes in [None, Some(vec![0; 20])] {
        let r = f.inspect();
        assert!(!r.depot_comparison_available);
        assert_eq!(r.actual_files, 4);
        assert!(r.files.iter().all(|e| e.state == "uncompared"));
        assert_eq!(r.issues[0].code, "steam_metadata_unavailable");
        if let Some(bytes) = bytes {
            fs::write(&p, bytes).unwrap();
        } else {
            fs::remove_file(&p).unwrap();
        }
    }
    assert!(!f.inspect().depot_comparison_available);
}
#[test]
fn conflicting_caches_and_stale_app_metadata_refused() {
    let f = Fixture::new();
    let other = f._dir.path().join("other");
    fs::create_dir_all(other.join("depotcache")).unwrap();
    fs::write(other.join("depotcache/1_2.manifest"), b"different").unwrap();
    assert!(
        !inspect_with(&f.root, &[other])
            .unwrap()
            .depot_comparison_available
    );
    let original = fs::read_to_string(&f.app).unwrap();
    for changed in [
        original.replace("\"TargetBuildID\" \"123\"", "\"TargetBuildID\" \"124\""),
        original.replace("\"appid\" \"3321460\"", "\"appid\" \"99\""),
        original.replace("\"installdir\" \"Game\"", "\"installdir\" \"../Game\""),
        original.replace(
            "\"buildid\" \"123\"",
            "\"buildid\" \"123\" \"BuildID\" \"124\"",
        ),
    ] {
        fs::write(&f.app, changed).unwrap();
        assert!(!f.inspect().depot_comparison_available);
    }
}
#[test]
fn type_mismatch_and_registry_damage_are_visible() {
    let f = Fixture::new();
    fs::remove_file(f.root.join("0000/0.paz")).unwrap();
    fs::create_dir(f.root.join("0000/0.paz")).unwrap();
    fs::write(f.root.join("meta/0.papgt"), b"bad registry").unwrap();
    let r = f.inspect();
    assert!(r.issues.iter().any(|i| i.code == "type_mismatch"));
    assert!(r.issues.iter().any(|i| i.code == "registry_unavailable"));
    assert!(!r.can_apply);
}
#[test]
fn metadata_bounds_and_nesting_limits_fail_closed() {
    let f = Fixture::new();
    let p = f.cache.join("oversized");
    fs::write(&p, vec![0; 100]).unwrap();
    assert!(read_small(&p, 99).is_err());
    let mut path = f.root.clone();
    for _ in 0..34 {
        path = path.join("a");
        fs::create_dir(&path).unwrap();
    }
    let r = f.inspect();
    assert!(!r.directory_scan_complete);
    assert!(r.issues.iter().any(|i| i.code == "scan_incomplete"));
    assert!(!r.can_apply);
}

#[test]
fn linked_directory_is_reported_and_never_followed() {
    let f = Fixture::new();
    let outside = f._dir.path().join("outside");
    fs::create_dir(&outside).unwrap();
    fs::write(outside.join("sentinel.exe"), b"own outside fixture").unwrap();
    let link = f.root.join("redirect");
    #[cfg(windows)]
    {
        use std::os::windows::process::CommandExt;
        let output = std::process::Command::new("cmd.exe")
            .args(["/C", "mklink", "/J"])
            .arg(&link)
            .arg(&outside)
            .creation_flags(0x0800_0000)
            .output()
            .unwrap();
        assert!(
            output.status.success(),
            "{}",
            String::from_utf8_lossy(&output.stderr)
        );
    }
    #[cfg(not(windows))]
    std::os::unix::fs::symlink(&outside, &link).unwrap();
    let r = f.inspect();
    assert!(
        r.files
            .iter()
            .any(|e| e.path == "redirect" && e.state == "unsupported_entry")
    );
    assert!(!r.files.iter().any(|e| e.path.contains("sentinel")));
    assert!(read_small(&link.join("sentinel.exe"), 1024).is_err());
    assert_eq!(
        fs::read(outside.join("sentinel.exe")).unwrap(),
        b"own outside fixture"
    );
    // Remove only the link itself before the temporary fixture's recursive cleanup.
    #[cfg(windows)]
    fs::remove_dir(&link).unwrap();
    #[cfg(not(windows))]
    fs::remove_file(&link).unwrap();
}
