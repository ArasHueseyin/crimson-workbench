//! Read-only, build-checked character records; writes only a new diagnostic file.
use cd_core::{GameData, mounts};
use std::{fs::OpenOptions, io::Write, path::PathBuf};
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<_> = std::env::args_os().collect();
    if args.len() != 3 {
        return Err("Usage: inspect_mount_references GAME NEW_OUTPUT".into());
    }
    let data = GameData::open(&PathBuf::from(&args[1]), "ger")?;
    let mut records = Vec::new();
    for mount in mounts::catalog(&data)? {
        records.push(serde_json::json!({"mount":mount,"record":data.dump_table("characterinfo",Some(mount.key),false)?}));
    }
    OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(&args[2])?
        .write_all(&serde_json::to_vec_pretty(&records)?)?;
    println!(
        "Exported {} checked raw mount records. No game/save writes.",
        records.len()
    );
    Ok(())
}
