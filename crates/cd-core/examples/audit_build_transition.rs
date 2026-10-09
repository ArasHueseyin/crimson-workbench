//! Read-only comparison of every currently used table/localization against the
//! newly observed executable. Does not admit a schema or write game files.
use cd_core::fingerprint::{TableHash, hash_bytes, hash_file};
use crimson_format::Archive;
use serde_json::{Value, json};
use std::{collections::BTreeMap, path::PathBuf};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let root = PathBuf::from(std::env::args().nth(1).ok_or("game directory required")?);
    let exe = root.join("bin64/CrimsonDesert.exe");
    let expected_exe = "a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a";
    if hash_file(&exe)?.1 != expected_exe {
        return Err("build-transition probe requires exactly EXE 1.0.0.2949".into());
    }
    let old: Value = serde_json::from_str(
        include_str!("../../../docs/builds/steam-25381195.observed.json")
            .trim_start_matches('\u{feff}'),
    )?;
    let mut references: Vec<TableHash> = serde_json::from_value(old["tables"].clone())?;
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
    let mut unique = BTreeMap::new();
    for reference in references {
        let key = (
            reference.group.clone(),
            reference.directory.clone(),
            reference.name.clone(),
        );
        if let Some(previous) = unique.insert(key, reference.clone()) {
            assert_eq!(
                previous.sha256, reference.sha256,
                "conflicting old reference"
            );
        }
    }
    let archive = Archive::open(&root)?;
    let mut groups = BTreeMap::new();
    let mut output = Vec::new();
    let mut blobs = BTreeMap::new();
    for reference in unique.values() {
        if !groups.contains_key(&reference.group) {
            groups.insert(
                reference.group.clone(),
                archive.list_group(&reference.group)?,
            );
        }
        let matches: Vec<_> = groups[&reference.group]
            .iter()
            .filter(|entry| entry.directory == reference.directory && entry.name == reference.name)
            .collect();
        if matches.len() != 1 {
            return Err(format!("ambiguous/missing {}", reference.name).into());
        }
        let bytes = archive.extract(matches[0])?;
        let sha256 = hash_bytes(&bytes);
        if reference.group == "0008" {
            blobs.insert(reference.name.clone(), bytes.clone());
        }
        output.push(
            json!({"group":reference.group,"directory":reference.directory,"name":reference.name,
            "size":bytes.len(),"sha256":sha256,"previous_sha256":reference.sha256,
            "unchanged":sha256 == reference.sha256 && bytes.len() as u64 == reference.size}),
        );
    }
    let mut structures = Vec::new();
    let old_root =
        PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../.local/archive-probe/extracted");
    for schema in cd_core::tables::TABLE_SCHEMAS {
        let body_name = format!("{}.staticinfobody", schema.name);
        let header_name = format!("{}.staticinfoheader", schema.name);
        let body = &blobs[&body_name];
        let header = &blobs[&header_name];
        let table = cd_core::tables::IndexedTable::parse(*schema, body, header)?;
        assert_eq!(table.serialize_body(), *body, "current body roundtrip");
        assert_eq!(
            table.serialize_header()?,
            *header,
            "current header roundtrip"
        );
        let mut changed = Vec::new();
        let mut added = Vec::new();
        let mut removed = Vec::new();
        if schema.name == "stageinfo" || schema.name == "questinfo" {
            let previous_body = std::fs::read(old_root.join(&body_name))?;
            let previous_header = std::fs::read(old_root.join(&header_name))?;
            for (name, bytes) in [
                (&body_name, &previous_body),
                (&header_name, &previous_header),
            ] {
                let reference = unique
                    .values()
                    .find(|r| r.name == *name)
                    .ok_or("old reference missing")?;
                assert_eq!(
                    hash_bytes(bytes),
                    reference.sha256,
                    "previous extracted table must match old build"
                );
            }
            let previous =
                cd_core::tables::IndexedTable::parse(*schema, &previous_body, &previous_header)?;
            for record in table.records() {
                match previous.record_bytes(record.key) {
                    None => added.push(record.key),
                    Some(old) if Some(old) != table.record_bytes(record.key) => {
                        changed.push(record.key)
                    }
                    _ => {}
                }
            }
            for record in previous.records() {
                if table.record_bytes(record.key).is_none() {
                    removed.push(record.key);
                }
            }
            if schema.name == "stageinfo" {
                for key in [1017811, 1002224] {
                    let original = previous.record_bytes(key).ok_or("old patrol absent")?;
                    assert_eq!(
                        table.record_bytes(key),
                        Some(original),
                        "editable patrol record changed"
                    );
                }
            }
        }
        structures.push(
            json!({"table":schema.name,"records":table.records().len(),"body_header_roundtrip":true,
            "compared_records":schema.name=="stageinfo" || schema.name=="questinfo",
            "changed_records":changed,"added_records":added,"removed_records":removed}),
        );
    }
    if hash_file(&exe)?.1 != expected_exe {
        return Err("EXE changed during observation".into());
    }
    println!(
        "{}",
        serde_json::to_string_pretty(&json!({
            "exe_version":"1.0.0.2949","exe_sha256":expected_exe,"steam_buildid":"25455892",
            "read_only":true,"admitted":false,"files":output,"structures":structures,"editable_patrols_unchanged":true
        }))?
    );
    Ok(())
}
