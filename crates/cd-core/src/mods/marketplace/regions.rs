//! The compiled branch_warning(SCENE, UI_Forbidden_Region, 30) transitions.
//! This changes their shared logic/UI duration, not an unrelated clock or label.
use super::super::{Change, bad};
use crate::{Result, fingerprint::hash_bytes};
use crimson_format::{
    Archive,
    overlay::{self, Overlay},
};
use std::collections::BTreeMap;

// Original build 2976 files. No copyrighted script bodies are embedded.
const RECIPES: [(u32, &str, &str, usize, usize); 4] = [
    (
        60301,
        "Restricted desert region",
        "3a3bb041fb3b7b835e59a64275f97c2ba9ad06631bb49cb41a922eb066fb8bc5",
        9633,
        1659,
    ),
    (
        60101,
        "Restricted snow region",
        "343fde48a424df71cc2566d08c763921d23a7911de1e17a275be801525f117f5",
        9627,
        1653,
    ),
    (
        60201,
        "Restricted water region",
        "ffdd48d3a2e84877817eabd3f2c96a791a2f1ae1495ec312ec090a6ece3de77b",
        9630,
        1656,
    ),
    (
        60001,
        "Restricted general region",
        "5abc2bb00e15400e8e91fda74ad47e60c16b2e255ef7d1be4cb8b94d10670e55",
        9612,
        1638,
    ),
];
const ORIGINAL_MS: u64 = 30_000;
const EXTENDED_MS: u64 = 1_800_000;

fn extend(original: &[u8], recipe: usize) -> Result<Vec<u8>> {
    let (_, _, hash, size, at) = RECIPES[recipe];
    if original.len() != size
        || hash_bytes(original) != hash
        || original.get(at..at + 8) != Some(ORIGINAL_MS.to_le_bytes().as_slice())
    {
        return Err(bad("Restricted-region script changed; nothing written"));
    }
    let mut after = original.to_vec();
    after[at..at + 8].copy_from_slice(&EXTENDED_MS.to_le_bytes());
    // Preserve the entire surrounding state machine, conditions and return scene.
    let mut inverse = after.clone();
    inverse[at..at + 8].copy_from_slice(&ORIGINAL_MS.to_le_bytes());
    if inverse != original {
        return Err(bad("Restricted-region timer inverse validation failed"));
    }
    Ok(after)
}

pub(crate) fn region_timers(archive: &Archive, changes: &mut Vec<Change>) -> Result<Overlay> {
    let entries = archive.list_group("0014")?;
    let mut files = BTreeMap::new();
    for (index, (key, label, _, _, _)) in RECIPES.iter().enumerate() {
        let name = overlay::REGION_TIMER_FILES[index];
        let entry = entries
            .iter()
            .find(|e| e.directory == overlay::REGION_TIMER_DIRECTORY && e.name == name)
            .ok_or_else(|| bad("Verified restricted-region script absent"))?;
        files.insert(name.into(), extend(&archive.extract(entry)?, index)?);
        changes.push(Change {
            module: "Marketplace",
            table: "region return script",
            key: *key,
            name: (*label).into(),
            field: "Warning and return delay (seconds)".into(),
            before: "30".into(),
            after: "1800".into(),
        });
    }
    let source = archive.serialize_group("0014")?;
    Ok(overlay::build_region_timers(
        &files,
        source[8..12].try_into().unwrap(),
    )?)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn unknown_or_truncated_scripts_are_rejected() {
        for (index, recipe) in RECIPES.iter().enumerate() {
            assert!(extend(&[], index).is_err());
            assert!(extend(&vec![0; recipe.3], index).is_err());
        }
    }
    #[test]
    #[ignore = "requires private original script fixtures; reads only, never installs"]
    fn exact_scripts_change_only_the_warning_transition_duration() {
        let root = std::path::PathBuf::from(
            std::env::var_os("CD_REGION_TIMER_FIXTURE")
                .expect("private original fixtures required"),
        );
        for (index, name) in overlay::REGION_TIMER_FILES.iter().enumerate() {
            let source = std::fs::read(root.join(name)).unwrap();
            let result = extend(&source, index).unwrap();
            let at = RECIPES[index].4;
            assert_eq!(&source[..at], &result[..at]);
            assert_eq!(&source[at + 8..], &result[at + 8..]);
            assert_eq!(
                u64::from_le_bytes(result[at..at + 8].try_into().unwrap()),
                EXTENDED_MS
            );
            assert!(
                extend(&result, index).is_err(),
                "never extend an already modified source"
            );
            let mut changed = source.clone();
            changed[0] ^= 1;
            assert!(extend(&changed, index).is_err());
            assert!(extend(&source[..source.len() - 1], index).is_err());
            assert_eq!(std::fs::read(root.join(name)).unwrap(), source);
        }
    }
}
