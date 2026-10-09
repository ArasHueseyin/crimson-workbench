use super::*;
use crimson_format::{checksum, overlay};
use std::fs;

struct Fixture {
    dir: tempfile::TempDir,
    baseline: Baseline,
    registry: Vec<u8>,
    files: BTreeMap<String, Vec<u8>>,
    // These Windows fixtures deliberately share the name probe.exe. A sibling
    // child exiting during Toolhelp -> OpenProcess makes the fail-closed guard
    // refuse an unrelated fixture too. Serialize their process lifetimes.
    _serial: std::sync::MutexGuard<'static, ()>,
}
impl Fixture {
    fn new() -> Self {
        static PROCESS_FIXTURES: std::sync::Mutex<()> = std::sync::Mutex::new(());
        let serial = PROCESS_FIXTURES.lock().unwrap();
        let dir = tempfile::tempdir().unwrap();
        fs::create_dir(dir.path().join("meta")).unwrap();
        fs::create_dir(dir.path().join("bin")).unwrap();
        fs::create_dir(dir.path().join("0008")).unwrap();
        let mut empty = vec![0; 16];
        empty[4..8].copy_from_slice(&checksum(&[0; 4]).to_le_bytes());
        let registry = overlay::register(&empty, "0008", 0x7fff, 123).unwrap();
        fs::write(dir.path().join("meta/0.papgt"), &registry).unwrap();
        fs::copy(
            std::env::current_exe().unwrap(),
            dir.path().join("bin/probe.exe"),
        )
        .unwrap();
        fs::write(dir.path().join("0008/0.paz"), b"synthetic original archive").unwrap();
        let baseline = Baseline::capture(
            dir.path(),
            &["bin/probe.exe", "0008/0.paz"],
            &["bin/probe.exe"],
        )
        .unwrap();
        let data = BTreeMap::from([
            ("example.staticinfobody".into(), vec![42; 256]),
            ("example.staticinfoheader".into(), vec![7; 32]),
        ]);
        let group = overlay::build(&data, [0x32, 2, 14, 97]).unwrap();
        let registered = overlay::register(&registry, "0041", 0x7fff, group.checksum).unwrap();
        let files = BTreeMap::from([
            ("0041/0.pamt".into(), group.pamt),
            ("0041/0.paz".into(), group.paz),
            ("meta/0.papgt".into(), registered),
        ]);
        Self {
            _serial: serial,
            dir,
            baseline,
            registry,
            files,
        }
    }
    fn root(&self) -> &Path {
        self.dir.path()
    }
}

#[test]
fn manifest_is_bound_to_marker_and_cannot_claim_vanilla_or_escape_paths() {
    let f = Fixture::new();
    assert_eq!(f.baseline.registry_sha256, hash_bytes(&f.registry));
    assert!(f.root().is_dir() && !f.files.is_empty());
    let bytes = serde_json::to_vec(&f.baseline).unwrap();
    assert!(Baseline::decode(&bytes, &hash_bytes(&bytes)).is_ok());
    assert!(Baseline::decode(&bytes, &"0".repeat(64)).is_err());
    for path in [
        "../outside",
        "C:/game.exe",
        "bin\\probe.exe",
        ".workbench/source",
        "meta/0.papgt",
        "bin/../probe.exe",
        "bin/probe.exe.",
    ] {
        let mut value = f.baseline.clone();
        value.sources.insert(
            path.into(),
            Source {
                bytes: 1,
                sha256: "0".repeat(64),
            },
        );
        assert!(value.validate().is_err(), "{path}");
    }
    let mut value = f.baseline.clone();
    value.kind = "certified-vanilla".into();
    assert!(value.validate().is_err());
    value = f.baseline.clone();
    value.sources.insert(
        "BIN/PROBE.EXE".into(),
        value.sources["bin/probe.exe"].clone(),
    );
    assert!(value.validate().is_err());
}

#[cfg(windows)]
fn run_copy(f: &Fixture) -> std::io::Result<std::process::Output> {
    use std::os::windows::process::CommandExt;
    std::process::Command::new(f.root().join("bin/probe.exe"))
        .arg("--list")
        .creation_flags(0x08000000)
        .output()
}

#[cfg(windows)]
#[test]
fn held_guard_covers_apply_reapply_restore_backup_and_parent_directories() {
    let f = Fixture::new();
    let mut session = Session::open(f.root(), &f.baseline, || Ok(())).unwrap();
    for round in 0..3 {
        assert!(run_copy(&f).is_err());
        assert!(fs::write(f.root().join("0008/0.paz"), b"update").is_err());
        assert!(fs::write(f.root().join(".workbench/baseline.papgt"), b"damage").is_err());
        assert!(fs::rename(f.root().join("bin"), f.root().join("moved-bin")).is_err());
        assert!(fs::rename(f.root().join("meta"), f.root().join("moved-meta")).is_err());
        assert!(fs::rename(f.root().join(".workbench"), f.root().join("moved-state")).is_err());
        if round < 2 {
            let plan = session.plan(&f.files).unwrap();
            session.apply(&plan, None).unwrap();
        } else {
            session.restore(None).unwrap();
        }
    }
    assert_eq!(fs::read(f.root().join("meta/0.papgt")).unwrap(), f.registry);
    assert_eq!(
        fs::read(f.root().join("0008/0.paz")).unwrap(),
        b"synthetic original archive"
    );
    drop(session);
    assert!(run_copy(&f).unwrap().status.success());
    assert!(fs::write(f.root().join("0008/0.paz"), b"now writable").is_ok());
}

#[cfg(windows)]
#[test]
fn update_before_recovery_preserves_new_sources_registry_and_old_backup() {
    let f = Fixture::new();
    let mut session = Session::open(f.root(), &f.baseline, || Ok(())).unwrap();
    let plan = session.plan(&f.files).unwrap();
    assert!(session.apply(&plan, Some(3)).is_err());
    drop(session);
    fs::write(f.root().join("0008/0.paz"), b"synthetic update").unwrap();
    let registry_before = fs::read(f.root().join("meta/0.papgt")).unwrap();
    let backup_before = fs::read(f.root().join(".workbench/baseline.papgt")).unwrap();
    let failure = Session::open(f.root(), &f.baseline, || Ok(()));
    assert!(
        matches!(failure, Err(Error::Invalid(ref s)) if s.starts_with("protected source mismatch:"))
    );
    assert_eq!(
        fs::read(f.root().join("0008/0.paz")).unwrap(),
        b"synthetic update"
    );
    assert_eq!(
        fs::read(f.root().join("meta/0.papgt")).unwrap(),
        registry_before
    );
    assert_eq!(
        fs::read(f.root().join(".workbench/baseline.papgt")).unwrap(),
        backup_before
    );
    fs::write(f.root().join("0008/0.paz"), b"synthetic original archive").unwrap();
    let mut session = Session::open(f.root(), &f.baseline, || Ok(())).unwrap();
    assert!(!session.recover(None).unwrap().is_empty());
    session.restore(None).unwrap();
    assert_eq!(fs::read(f.root().join("meta/0.papgt")).unwrap(), f.registry);
}

#[cfg(windows)]
#[test]
fn update_after_commit_never_restores_the_old_registry_over_new_files() {
    let f = Fixture::new();
    let mut session = Session::open(f.root(), &f.baseline, || Ok(())).unwrap();
    let plan = session.plan(&f.files).unwrap();
    let group = plan.group_names().next().unwrap().to_owned();
    session.apply(&plan, None).unwrap();
    drop(session);
    let backup = fs::read(f.root().join(".workbench/baseline.papgt")).unwrap();
    fs::write(f.root().join("0008/0.paz"), b"new game build simulation").unwrap();
    let updated_registry = overlay::register(&f.registry, "0090", 0x7fff, 456).unwrap();
    fs::write(f.root().join("meta/0.papgt"), &updated_registry).unwrap();
    assert!(Session::open(f.root(), &f.baseline, || Ok(())).is_err());
    assert_eq!(
        fs::read(f.root().join("meta/0.papgt")).unwrap(),
        updated_registry
    );
    assert_eq!(
        fs::read(f.root().join(".workbench/baseline.papgt")).unwrap(),
        backup
    );
    assert!(f.root().join(group).join("0.paz").exists());
}

#[cfg(windows)]
#[test]
fn process_failure_before_or_after_acquisition_creates_no_backup_and_releases_handles() {
    let f = Fixture::new();
    for fail_on in [1, 2] {
        let mut calls = 0;
        let result = Session::open(f.root(), &f.baseline, || {
            calls += 1;
            if calls == fail_on {
                Err(bad("process enumeration failed or process started"))
            } else {
                Ok(())
            }
        });
        assert!(result.is_err());
        assert!(!f.root().join(".workbench").exists());
        assert!(run_copy(&f).unwrap().status.success());
    }
    fs::hard_link(f.root().join("0008/0.paz"), f.root().join("foreign-link")).unwrap();
    assert!(Session::open(f.root(), &f.baseline, || Ok(())).is_err());
    assert!(!f.root().join(".workbench").exists());
}

#[cfg(windows)]
#[test]
fn guard_remains_held_after_interruption_and_after_callback_refusal() {
    use std::cell::Cell;
    let f = Fixture::new();
    let allow = Cell::new(true);
    let mut session = Session::open(f.root(), &f.baseline, || {
        if allow.get() {
            Ok(())
        } else {
            Err(bad("process no longer reliably stopped"))
        }
    })
    .unwrap();
    let plan = session.plan(&f.files).unwrap();
    assert!(session.apply(&plan, Some(3)).is_err());
    assert!(run_copy(&f).is_err());
    allow.set(false);
    assert!(session.recover(None).is_err());
    assert!(session.restore(None).is_err());
    assert!(run_copy(&f).is_err());
    allow.set(true);
    session.recover(None).unwrap();
    session.restore(None).unwrap();
    drop(session);
    assert!(run_copy(&f).unwrap().status.success());
}

#[cfg(windows)]
#[test]
fn fixture_child_wait() {
    let Ok(path) = std::env::var("CW_PROTECTION_FIXTURE_READY") else {
        return;
    };
    let ready = std::path::PathBuf::from(path);
    let exe = std::env::current_exe().unwrap();
    assert_eq!(ready.file_name().unwrap(), "child.ready");
    assert_eq!(
        ready.parent().unwrap().canonicalize().unwrap(),
        exe.parent()
            .unwrap()
            .parent()
            .unwrap()
            .canonicalize()
            .unwrap()
    );
    fs::write(&ready, b"ready").unwrap();
    for _ in 0..3000 {
        if ready.with_extension("release").exists() {
            return;
        }
        std::thread::sleep(std::time::Duration::from_millis(10));
    }
}

#[cfg(windows)]
#[test]
fn already_running_copy_is_refused_before_state_creation() {
    use std::os::windows::process::CommandExt;
    struct Child(std::process::Child);
    impl Drop for Child {
        fn drop(&mut self) {
            let _ = self.0.kill();
            let _ = self.0.wait();
        }
    }
    let f = Fixture::new();
    let ready = f.root().join("child.ready");
    let mut child = Child(
        std::process::Command::new(f.root().join("bin/probe.exe"))
            .args(["--exact", "apply::protected::tests::fixture_child_wait"])
            .env("CW_PROTECTION_FIXTURE_READY", &ready)
            .creation_flags(0x08000000)
            .stdout(std::process::Stdio::null())
            .stderr(std::process::Stdio::null())
            .spawn()
            .unwrap(),
    );
    let start = std::time::Instant::now();
    while !ready.exists() {
        assert!(start.elapsed() < std::time::Duration::from_secs(15));
        assert!(child.0.try_wait().unwrap().is_none());
        std::thread::sleep(std::time::Duration::from_millis(10));
    }
    assert!(Session::open(f.root(), &f.baseline, || Ok(())).is_err());
    assert!(!f.root().join(".workbench").exists());
    fs::write(ready.with_extension("release"), b"exit").unwrap();
    assert!(child.0.wait().unwrap().success());
    assert!(Session::open(f.root(), &f.baseline, || Ok(())).is_ok());
}
