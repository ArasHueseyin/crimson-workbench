//! Read-only research, writes only JSON to stdout. No game mutation.
use cd_core::{
    fingerprint::Fingerprint,
    tables::{IndexedTable, TableSchema},
};
use crimson_format::Archive;
use serde_json::json;
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let root = std::env::args().nth(1).ok_or("game root required")?;
    let fp = Fingerprint::inspect(std::path::Path::new(&root))?;
    if !fp.metadata_matches {
        return Err("unrecognized installation metadata".into());
    }
    let archive = Archive::open(root)?;
    let entries = archive.list_group("0008")?;
    let mut output = vec![];
    for (name, key_bytes) in [
        ("multichangeinfo", 4),
        ("crafttoolinfo", 2),
        ("crafttoolgroupinfo", 2),
        ("itemgroupinfo", 2),
        ("dropsetinfo", 4),
        ("storeinfo", 2),
    ] {
        let mut blobs = vec![];
        let mut hashes = vec![];
        for ext in ["staticinfobody", "staticinfoheader"] {
            let filename = format!("{name}.{ext}");
            let entry = entries
                .iter()
                .find(|e| e.name == filename && e.directory == "gamedata/binarystaticinfo__/bin")
                .ok_or("table missing")?;
            let bytes = archive.extract(entry)?;
            hashes.push(json!({"group":entry.group,"directory":entry.directory,"name":entry.name,"size":bytes.len(),"sha256":cd_core::fingerprint::hash_bytes(&bytes)}));
            blobs.push(bytes);
        }
        let table = IndexedTable::parse(
            TableSchema {
                name,
                count_bytes: 2,
                key_bytes,
                interpretation: "research raw only",
            },
            &blobs[0],
            &blobs[1],
        )?;
        let rows:Vec<_>=table.records().iter().map(|r|json!({"key":r.key,"hex":cd_core::tables::hex(table.record_bytes(r.key).unwrap())})).collect();
        output.push(json!({"name":name,"hashes":hashes,"rows":rows}));
    }
    println!("{}", serde_json::to_string(&output)?);
    Ok(())
}
