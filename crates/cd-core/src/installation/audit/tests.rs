use super::*;
use crate::installation::tests::Fixture;
fn run(f: &Fixture) -> Report {
    verify_with(&f.root, &[], &AtomicBool::new(false), |_| {}, || Ok(())).unwrap()
}
#[test]
fn full_content_comparison_and_exact_progress_without_trust() {
    let f = Fixture::new();
    let cancel = AtomicBool::new(false);
    let mut progress = Vec::new();
    let r = verify_with(&f.root, &[], &cancel, |p| progress.push(p), || Ok(())).unwrap();
    assert_eq!(r.files.len(), 4);
    assert!(r.all_files_match_cache && r.metadata_stable);
    assert!(!r.certified_vanilla && !r.can_apply);
    let end = progress.last().unwrap();
    assert_eq!(end.phase, "complete");
    assert_eq!(end.files_done, 4);
    assert_eq!(end.bytes_done, end.total_bytes);
    assert!(
        r.files
            .iter()
            .all(|f| f.sha256.len() == 64 && f.sha1.len() == 40)
    );
    assert!(!f.root.join(".workbench").exists());
}
#[test]
fn same_size_change_matches_inventory_but_fails_content() {
    let f = Fixture::new();
    let p = f.root.join("0000/0.paz");
    let size = fs::metadata(&p).unwrap().len();
    fs::write(p, vec![b'X'; size as usize]).unwrap();
    let r = run(&f);
    assert!(!r.all_files_match_cache);
    assert_eq!(r.files.iter().filter(|f| !f.matches_cache).count(), 1);
    assert!(!r.can_apply);
}
#[test]
fn streamed_sha_vectors_and_multi_block_file() {
    let f = Fixture::new();
    let p = f.root.join("0000/0.paz");
    fs::write(&p, b"abc").unwrap();
    rebuild_manifest(&f);
    let r = run(&f);
    let row = r.files.iter().find(|f| f.path == "0000/0.paz").unwrap();
    assert_eq!(row.sha1, "a9993e364706816aba3e25717850c26c9cd0d89d");
    assert_eq!(
        row.sha256,
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
    );
    fs::write(&p, vec![b'a'; 2 * 1024 * 1024 + 17]).unwrap();
    rebuild_manifest(&f);
    assert!(run(&f).all_files_match_cache);
}
#[test]
fn cancellation_never_returns_complete_report() {
    let f = Fixture::new();
    let cancel = AtomicBool::new(true);
    let mut called = false;
    assert!(verify_with(&f.root, &[], &cancel, |_| called = true, || Ok(())).is_err());
    assert!(!called);
    cancel.store(false, Ordering::Release);
    let mut completed = false;
    let err = verify_with(
        &f.root,
        &[],
        &cancel,
        |p| {
            if p.files_done == 1 {
                cancel.store(true, Ordering::Release);
            }
            completed |= p.phase == "complete";
        },
        || Ok(()),
    )
    .err()
    .unwrap();
    assert!(err.to_string().contains("abgebrochen"));
    assert!(!completed);
}
#[test]
fn running_unknown_and_newly_started_game_refuse_hashing_or_completion() {
    let f = Fixture::new();
    let cancel = AtomicBool::new(false);
    for message in ["running", "unknown"] {
        let mut called = false;
        let e = verify_with(
            &f.root,
            &[],
            &cancel,
            |_| called = true,
            || Err(Error::Invalid(message.into())),
        )
        .err()
        .unwrap();
        assert_eq!(e.to_string(), message);
        assert!(!called);
    }
    let mut checks = 0;
    let mut complete = false;
    assert!(
        verify_with(
            &f.root,
            &[],
            &cancel,
            |p| complete |= p.phase == "complete",
            || {
                checks += 1;
                if checks == 3 {
                    Err(Error::Invalid("started".into()))
                } else {
                    Ok(())
                }
            }
        )
        .is_err()
    );
    assert!(!complete);
}
#[test]
fn update_and_already_hashed_source_change_invalidate_whole_run() {
    for change_manifest in [false, true] {
        let f = Fixture::new();
        let mut changed = false;
        let e = verify_with(
            &f.root,
            &[],
            &AtomicBool::new(false),
            |p| {
                if p.phase == "validating" && !changed {
                    changed = true;
                    if change_manifest {
                        let s = fs::read_to_string(&f.app).unwrap().replace("123", "124");
                        fs::write(&f.app, s).unwrap();
                    } else {
                        let path = f.root.join("0000/0.paz");
                        let modified = fs::metadata(&path).unwrap().modified().unwrap();
                        fs::write(&path, b"changed after hashing").unwrap();
                        // The replacement has the same size. Give the metadata
                        // comparison a deterministic timestamp change, even on
                        // a fast runner where both writes share one clock tick.
                        File::options()
                            .write(true)
                            .open(path)
                            .unwrap()
                            .set_times(
                                fs::FileTimes::new()
                                    .set_modified(modified + Duration::from_secs(2)),
                            )
                            .unwrap();
                    }
                }
            },
            || Ok(()),
        )
        .err()
        .unwrap();
        assert!(changed);
        assert!(e.to_string().contains("geändert") || e.to_string().contains("abweichend"));
    }
}
#[test]
fn missing_extra_and_hardlinked_sources_do_not_produce_full_success() {
    let f = Fixture::new();
    fs::write(f.root.join("foreign.dll"), b"extra").unwrap();
    assert!(verify_with(&f.root, &[], &AtomicBool::new(false), |_| {}, || Ok(())).is_err());
    fs::remove_file(f.root.join("foreign.dll")).unwrap();
    let alias = f.root.parent().unwrap().join("alias");
    fs::hard_link(f.root.join("0000/0.paz"), &alias).unwrap();
    assert!(verify_with(&f.root, &[], &AtomicBool::new(false), |_| {}, || Ok(())).is_err());
    fs::remove_file(alias).unwrap();
    fs::remove_file(f.root.join("0000/0.paz")).unwrap();
    assert!(verify_with(&f.root, &[], &AtomicBool::new(false), |_| {}, || Ok(())).is_err());
}
#[test]
fn zero_length_and_growth_between_inventory_and_read() {
    let f = Fixture::new();
    fs::write(f.root.join("0000/0.paz"), []).unwrap();
    rebuild_manifest(&f);
    assert!(run(&f).all_files_match_cache);
    let mut changed = false;
    let result = verify_with(
        &f.root,
        &[],
        &AtomicBool::new(false),
        |p| {
            if p.current_path.as_deref() == Some("0000/0.paz") && !changed {
                changed = true;
                fs::write(f.root.join("0000/0.paz"), b"grown").unwrap();
            }
        },
        || Ok(()),
    );
    assert!(result.is_err());
    assert!(changed);
}

#[test]
fn public_process_gate_precedes_any_content_read() {
    let f = Fixture::new();
    let status = crate::apply::status(&f.root).game_running;
    let mut content = false;
    let result = verify(&f.root, &AtomicBool::new(false), |p| {
        content |= p.phase == "hashing"
    });
    if status != Some(false) {
        assert!(result.is_err());
        assert!(!content);
    } else {
        // If a real game starts between the two checks, refusal is also correct.
        if let Ok(report) = result {
            assert!(report.all_files_match_cache);
        }
    }
}

#[test]
fn content_handle_allows_writes_but_observes_metadata_change() {
    let f = Fixture::new();
    let file = open_source(&f.root, "0000/0.paz").unwrap();
    let before = stamp(&file).unwrap();
    fs::write(
        f.root.join("0000/0.paz"),
        b"own concurrent update, no write-denying lock",
    )
    .unwrap();
    assert!(before != stamp(&file).unwrap());
}
fn rebuild_manifest(f: &Fixture) {
    let names = [
        "meta/0.papgt",
        "bin64/CrimsonDesert.exe",
        "0000/0.pamt",
        "0000/0.paz",
    ];
    let files: Vec<_> = names
        .into_iter()
        .map(|p| {
            let b = fs::read(f.root.join(p)).unwrap();
            (p, b.len() as u64, Sha1::digest(&b).into())
        })
        .collect();
    fs::write(
        f.cache.join("1_2.manifest"),
        crate::installation::depot::tests::fixture_hashes(&files, 1, 2),
    )
    .unwrap();
    let size: u64 = files.iter().map(|(_, n, _)| n).sum();
    fs::write(&f.app,format!(r#""AppState" {{ "appid" "3321460" "installdir" "Game" "buildid" "123" "TargetBuildID" "123" "InstalledDepots" {{ "1" {{ "manifest" "2" "size" "{size}" }} }} }}"#)).unwrap();
}
