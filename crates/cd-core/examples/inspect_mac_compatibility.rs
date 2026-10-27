//! Read-only comparison; matching data never authorizes writes or admits a build.
use cd_core::fingerprint::{TableHash, hash_bytes};
use crimson_format::Archive;
use serde_json::json;
use std::{collections::BTreeMap, path::PathBuf};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let root = PathBuf::from(
        std::env::args()
            .nth(1)
            .ok_or("packages directory required")?,
    );
    let current: serde_json::Value = serde_json::from_str(
        include_str!("../../../docs/builds/steam-25477059.observed.json")
            .trim_start_matches('\u{feff}'),
    )?;
    let mut references: Vec<TableHash> = serde_json::from_value(current["tables"].clone())?;
    for source in [
        include_str!("../schemas/steam-25381195.advanced.json"),
        include_str!("../schemas/steam-25381195.crafting.json"),
        include_str!("../schemas/steam-25381195.icons.json"),
        include_str!("../schemas/steam-25381195.locales.json"),
    ] {
        references.extend(serde_json::from_str::<Vec<TableHash>>(
            source.trim_start_matches('\u{feff}'),
        )?);
    }
    let archive = Archive::open(root)?;
    let mut groups = BTreeMap::new();
    let mut output = Vec::new();
    for reference in references {
        if !groups.contains_key(&reference.group) {
            groups.insert(
                reference.group.clone(),
                archive.list_group(&reference.group)?,
            );
        }
        let entries: Vec<_> = groups[&reference.group]
            .iter()
            .filter(|e| e.directory == reference.directory && e.name == reference.name)
            .collect();
        if entries.len() != 1 {
            output.push(json!({"name":reference.name,"matches":false,"entries":entries.len()}));
            continue;
        }
        let bytes = archive.extract(entries[0])?;
        let sha256 = hash_bytes(&bytes);
        output.push(
            json!({"name":reference.name,"size":bytes.len(),"sha256":sha256,
            "matches":bytes.len() as u64 == reference.size && sha256 == reference.sha256}),
        );
    }
    println!(
        "{}",
        serde_json::to_string_pretty(&json!({
            "reference":"Windows EXE 1.0.0.2976",
            "all_match":output.iter().all(|v| v["matches"] == true),
            "tables":output
        }))?
    );
    Ok(())
}
