//! Read-only mount catalog/save eligibility diagnostic; never writes files.
use cd_core::{GameData, mounts};
use std::path::PathBuf;
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<_> = std::env::args_os().collect();
    if args.len() < 3 || args.len() > 4 {
        return Err("Usage: inspect_mounts GAME SAVE_ROOT [ACCOUNT/slotN]".into());
    }
    let data = GameData::open(&PathBuf::from(&args[1]).canonicalize()?, "ger")?;
    let catalog = mounts::catalog(&data)?;
    let selected = args.get(3).map(|s| s.to_string_lossy().into_owned());
    let snapshot = mounts::snapshot(
        catalog,
        &PathBuf::from(&args[2]).canonicalize()?,
        selected.as_deref(),
    )?;
    println!("{}", serde_json::to_string_pretty(&snapshot)?);
    Ok(())
}
