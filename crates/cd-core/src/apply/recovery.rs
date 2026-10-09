//! Reviewed restore capability restricted to existing, protected project copies.
use super::{protected, read_identity, read_protection, recovery_receipt, transaction};
use crate::{Error, Result, fingerprint::hash_bytes, paths::PathPolicy};
use serde::Serialize;
use std::{
    fs,
    path::{Path, PathBuf},
};

#[derive(Serialize)]
pub struct Entry {
    pub directory: PathBuf,
    pub name: String,
    pub format: Option<u32>,
    pub error: Option<String>,
}
#[derive(Serialize)]
pub struct Listing {
    pub entries: Vec<Entry>,
    pub truncated: bool,
}
#[derive(Serialize)]
pub struct Review {
    pub directory: PathBuf,
    pub review_id: String,
    pub plan_id: String,
    pub protected_source_files: usize,
    pub can_restore: bool,
    pub scope: &'static str,
    pub restore: transaction::RestoreReview,
}

/// Enumeration reads only markers. Backup/source verification is explicit per entry.
pub fn list(policy: &PathPolicy) -> Result<Listing> {
    // PathPolicy validates file destinations, so check an unwritten child and
    // then inspect its parent. No sentinel or directory is created.
    let boundary = policy.check_cache(Path::new(".local/rehearsals/.inventory-boundary"))?;
    let base = boundary.parent().unwrap();
    if !base.try_exists()? {
        return Ok(Listing {
            entries: Vec::new(),
            truncated: false,
        });
    }
    transaction::plain(base, true)?;
    let mut directories = Vec::new();
    for item in fs::read_dir(base)? {
        let item = item?;
        if directories.len() >= 2000 {
            return Err(Error::Invalid(
                "Mehr als 2.000 Projektproben; Liste nicht vollständig lesbar.".into(),
            ));
        }
        directories.push(item.path());
    }
    directories.sort_by(|a, b| b.file_name().cmp(&a.file_name()));
    let truncated = directories.len() > 200;
    let entries = directories.into_iter().take(200).map(|directory| {
        let name = directory.file_name().unwrap().to_string_lossy().into_owned();
        match read_identity(policy, &directory) {
            Ok((_, identity)) => Entry { directory, name, format: Some(identity.format), error: (identity.format != 3).then(|| "Ältere ungeschützte Probe; nicht für diese Wiederherstellung freigegeben.".into()) },
            Err(error) => Entry { directory, name, format: None, error: Some(error.to_string()) },
        }
    }).collect();
    Ok(Listing { entries, truncated })
}

fn checked_identity(
    policy: &PathPolicy,
    directory: &Path,
) -> Result<(PathBuf, super::RehearsalIdentity, protected::Baseline)> {
    let (root, identity) = read_identity(policy, directory)?;
    if identity.format != 3 {
        return Err(Error::Invalid(
            "Wiederherstellungsvorschau benötigt eine geschützte v3-Projektprobe.".into(),
        ));
    }
    let baseline = read_protection(policy, &root, &identity)?;
    Ok((root, identity, baseline))
}
fn make_review(
    root: &Path,
    identity: &super::RehearsalIdentity,
    sources: usize,
    restore: transaction::RestoreReview,
) -> Result<Review> {
    // Bind approval to the selected directory, marker, backup, history generation,
    // commit decision and exact current visible-file changes, never just a pathname.
    let review_id = hash_bytes(&serde_json::to_vec(&(root, identity, &restore))?);
    Ok(Review {
        directory: root.to_owned(),
        review_id,
        plan_id: identity.plan_id.clone(),
        protected_source_files: sources,
        can_restore: restore.pending_outcome.is_some() || !restore.files.is_empty(),
        scope: "Ausschließlich diese Projektkopie. Registry aus geprüfter Sicherung wiederherstellen und eigene Overlaydateien entfernen; Originalarchive bleiben erhalten. Kein Live-Restore.",
        restore,
    })
}
pub fn inspect(policy: &PathPolicy, directory: &Path) -> Result<Review> {
    let (root, identity, baseline) = checked_identity(policy, directory)?;
    let mut session = protected::Session::open_existing(&root, &baseline, || Ok(()))?;
    make_review(
        &root,
        &identity,
        baseline.file_count(),
        session.restore_review()?,
    )
}
pub fn restore(
    policy: &PathPolicy,
    directory: &Path,
    expected_review: &str,
) -> Result<super::RehearsalRecovery> {
    if expected_review.len() != 64
        || !expected_review
            .bytes()
            .all(|b| b.is_ascii_hexdigit() && !b.is_ascii_uppercase())
    {
        return Err(Error::Invalid(
            "Gültige Wiederherstellungsvorschau erforderlich.".into(),
        ));
    }
    let (root, identity, baseline) = checked_identity(policy, directory)?;
    let mut session = protected::Session::open_existing(&root, &baseline, || Ok(()))?;
    let review = make_review(
        &root,
        &identity,
        baseline.file_count(),
        session.restore_review()?,
    )?;
    if review.review_id != expected_review {
        return Err(Error::Invalid("Projektprobe seit der Vorschau geändert. Bitte erneut prüfen; nichts wiederhergestellt.".into()));
    }
    let pending = !session.recover(None)?.is_empty();
    session.restore(None)?;
    let after = session.restore_review()?;
    if !after.files.is_empty() || after.pending_outcome.is_some() {
        return Err(Error::Invalid(
            "Wiederherstellung nicht vollständig; Projektprobe erneut prüfen.".into(),
        ));
    }
    // The receipt is emitted before dropping source/backup/directory protection.
    recovery_receipt(policy, &root, &identity, pending)
}

#[cfg(all(test, windows))]
#[path = "recovery_tests.rs"]
mod tests;
