//! Print metadata-only locale fingerprints; reads only registered item namespaces.
use crimson_format::{Archive, Paloc};
use sha2::{Digest, Sha256};
use std::{env, io};
fn main() -> io::Result<()> {
    let game = env::var("CD_GAME_DIR").map_err(|_| io::Error::other("CD_GAME_DIR required"))?;
    let archive = Archive::open(game)?;
    let mut rows = Vec::new();
    for group in archive
        .groups()
        .iter()
        .filter(|g| g.present && !g.is_optional && g.language.is_power_of_two())
    {
        for entry in archive.list_group(&group.name)?.iter().filter(|e| {
            e.name == "item.paloc" && e.directory.starts_with("gamedata/stringtable/binary__/")
        }) {
            let language = entry.directory.rsplit('/').next().unwrap();
            let data = archive.extract(entry)?;
            let parsed = Paloc::parse(&data)?;
            if parsed.serialize()? != data {
                return Err(io::Error::other("PALOC roundtrip mismatch"));
            }
            let sha256 = format!("{:x}", Sha256::digest(&data));
            rows.push(serde_json::json!({"language":language,"group":group.name,"directory":entry.directory,"name":entry.name,"size":data.len(),"sha256":sha256}));
        }
    }
    rows.sort_by(|a, b| a["language"].as_str().cmp(&b["language"].as_str()));
    println!("{}", serde_json::to_string_pretty(&rows)?);
    Ok(())
}
