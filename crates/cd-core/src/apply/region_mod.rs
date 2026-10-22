//! Independent, additive region-script installation. No vanilla-baseline claim
//! and no authority over other archive groups, original assets or saves.
use crate::{Error, Result, fingerprint::hash_bytes};
use crimson_format::{Archive, overlay};
use serde::{Deserialize, Serialize};
use std::path::{Path, PathBuf};
#[cfg(windows)]
use std::{fs, io::Read};

#[cfg_attr(not(windows), allow(dead_code))]
mod transaction;
use transaction::{Package, Store};
pub const ID: &str = "region_return_30min";
#[cfg(windows)]
const EXE_HASH: &str = "57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7";
const PATCHED: [&str; 4] = [
    "76de78cf59387f4fdc647b1648a99b3124245542098f7dfdaada44ce077867b8",
    "76bbd029b8147037a790d1da541339e8f890f62cea5b2e653ae11f02ae66a65a",
    "21d7c63087a076d6ada4f63c4106c7170e54b7974a8b7841963ee11299f33a1e",
    "3f3cbddce2a35ae8fc7df3d5a3a5e166deb4b737e883ee1afb678ef73428798b",
];
fn bad(message: impl Into<String>) -> Error {
    Error::Invalid(message.into())
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Status {
    pub id: String,
    pub state: String,
    pub message: String,
    pub can_install: bool,
    pub can_remove: bool,
}
fn state(
    value: &str,
    message: impl Into<String>,
    install: bool,
    remove: bool,
    running: bool,
) -> Status {
    Status {
        id: ID.into(),
        state: value.into(),
        message: message.into(),
        can_install: install && !running,
        can_remove: remove && !running,
    }
}
fn root(game: &Path) -> Result<PathBuf> {
    // Check the named path before canonicalizing, so junctions cannot disappear.
    for ancestor in game.ancestors() {
        super::transaction::plain(ancestor, true)?;
    }
    let root = game.canonicalize()?;
    let registry = super::transaction::checked(&root, "meta/0.papgt")?;
    super::transaction::plain(&registry, false)?;
    Archive::open(&root)?;
    Ok(root)
}
fn build(game: &Path) -> Result<Package> {
    let archive = Archive::open(game)?;
    let language = archive
        .groups()
        .iter()
        .find(|g| g.name == "0014")
        .ok_or_else(|| bad("Region script source group is missing."))?
        .language;
    let pack = crate::mods::marketplace::region_timers(&archive, &mut Vec::new())?;
    Ok(Package {
        pamt: pack.pamt,
        paz: pack.paz,
        language,
        checksum: pack.checksum,
    })
}
fn conflicts(game: &Path, owned: Option<&str>) -> Result<()> {
    let archive = Archive::open(game)?;
    for group in archive
        .groups()
        .iter()
        .filter(|g| g.present && g.name != "0014" && Some(g.name.as_str()) != owned)
    {
        for entry in archive.list_group(&group.name)? {
            if entry.directory == overlay::REGION_TIMER_DIRECTORY
                && overlay::REGION_TIMER_FILES.contains(&entry.name.as_str())
            {
                return Err(bad(format!(
                    "Another mod overrides restricted-region scripts in group {}. It has been preserved; remove that override before installing this mod.",
                    group.name
                )));
            }
        }
    }
    Ok(())
}
fn curated(game: &Path, receipt: &transaction::Receipt) -> Result<()> {
    // Validate the complete owned payload even when original game assets have
    // changed after an update. Removal never requires restoring old originals.
    let mut empty = vec![0; 16];
    empty[4..8].copy_from_slice(&crimson_format::checksum(&[0; 4]).to_le_bytes());
    let registry = overlay::register(&empty, &receipt.group, receipt.language, receipt.checksum)?;
    let archive = Archive::with_registry(game, registry)?;
    let entries = archive.list_group(&receipt.group)?;
    if entries.len() != 4 {
        return Err(bad("Owned region archive contains unexpected files."));
    }
    for (index, name) in overlay::REGION_TIMER_FILES.iter().enumerate() {
        let entry = entries
            .iter()
            .find(|e| e.directory == overlay::REGION_TIMER_DIRECTORY && &e.name == name)
            .ok_or_else(|| bad("Owned region archive paths changed."))?;
        if hash_bytes(&archive.extract(entry)?) != PATCHED[index] {
            return Err(bad(
                "Owned region scripts changed. Nothing will be removed.",
            ));
        }
    }
    Ok(())
}
pub fn status(game: &Path) -> Status {
    let running = super::game_running().unwrap_or(true);
    let checked = (|| -> Result<Status> {
        let game = root(game)?;
        let store = Store::new(&game)?;
        if store.pending()? {
            return Ok(state(
                "pending",
                "Interrupted region-mod operation. Close the game and recover the operation.",
                true,
                false,
                running,
            ));
        }
        if let Some(receipt) = store.receipt()? {
            store.verify(&receipt, false)?;
            curated(&game, &receipt)?;
            if store.registered(&receipt)? {
                if let Err(e) = conflicts(&game, Some(&receipt.group)) {
                    return Ok(state("conflict", e.to_string(), false, true, running));
                }
                return Ok(state(
                    "installed",
                    "Installed. The 30-minute countdown loads when the game starts.",
                    false,
                    true,
                    running,
                ));
            }
            return Ok(state(
                "conflict",
                "The mod's registration is missing. Remove its verified remaining files before reinstalling.",
                false,
                true,
                running,
            ));
        }
        conflicts(&game, None)?;
        build(&game)?;
        Ok(state(
            "not_installed",
            "Not installed. Install once with the game closed; existing mods are preserved.",
            true,
            false,
            running,
        ))
    })();
    checked.unwrap_or_else(|e| state("conflict", e.to_string(), false, false, running))
}
pub fn change(game: &Path, install: bool) -> Result<Status> {
    #[cfg(not(windows))]
    {
        let _ = (game, install);
        return Err(bad("Archive mod installation requires Windows x64."));
    }
    #[cfg(windows)]
    {
        use sha2::{Digest, Sha256};
        use std::os::windows::fs::OpenOptionsExt;
        if super::game_running()? {
            return Err(bad(
                "Close Crimson Desert before installing or removing this mod.",
            ));
        }
        let game = root(game)?;
        let exe = super::transaction::checked(&game, "bin64/CrimsonDesert.exe")?;
        super::transaction::plain(&exe, false)?;
        let mut guard = fs::OpenOptions::new().read(true).share_mode(0).open(&exe)?;
        if install {
            let mut hash = Sha256::new();
            let mut block = [0; 65536];
            loop {
                let n = guard.read(&mut block)?;
                if n == 0 {
                    break;
                }
                hash.update(&block[..n]);
            }
            if format!("{:x}", hash.finalize()) != EXE_HASH {
                return Err(bad(
                    "This region mod requires the verified game build 1.0.0.2976.",
                ));
            }
        }
        let store = Store::new(&game)?;
        if !install && !store.pending()? && store.receipt()?.is_none() {
            drop(guard);
            return Ok(status(&game));
        }
        let _lock = store.lock()?;
        if store.pending()? {
            store.recover(curated)?;
        } else if install {
            if let Some(r) = store.receipt()? {
                store.verify(&r, false)?;
                curated(&game, &r)?;
                if !store.registered(&r)? {
                    return Err(bad(
                        "Remove the incomplete installation before reinstalling.",
                    ));
                }
            } else {
                conflicts(&game, None)?;
                if let Err(error) = store.install(build(&game)?, None) {
                    if store.pending()? {
                        let _ = store.recover(curated);
                    }
                    return Err(error);
                }
            }
        } else {
            if let Err(error) = store.remove(curated, None) {
                if store.pending()? {
                    let _ = store.recover(curated);
                }
                return Err(error);
            }
        }
        drop(guard);
        Ok(status(&game))
    }
}

#[cfg(test)]
mod tests;
