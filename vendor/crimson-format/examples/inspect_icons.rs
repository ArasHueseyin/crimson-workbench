//! Read-only format research; output contains metadata, never texture bytes.
use crimson_format::Archive;
use sha2::{Digest, Sha256};
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let root = std::env::args().nth(1).ok_or("pass game root")?;
    let archive = Archive::open(root)?;
    let entries = archive.list_group("0008")?;
    let mut files = vec![];
    let mut matched = None;
    for name in ["stringinfo.staticinfobody", "stringinfo.staticinfoheader"] {
        let entry = entries
            .iter()
            .find(|e| e.name == name)
            .ok_or("missing stringinfo")?;
        let bytes = archive.extract(entry)?;
        if name.ends_with("body")
            && let Some(wanted) = std::env::args().nth(2).and_then(|v| v.parse::<u32>().ok())
        {
            let mut offset = 0;
            while offset + 13 <= bytes.len() {
                let key = u32::from_le_bytes(bytes[offset..offset + 4].try_into()?);
                let len = u32::from_le_bytes(bytes[offset + 9..offset + 13].try_into()?) as usize;
                let end = offset.checked_add(13 + len).ok_or("overflow")?;
                if end > bytes.len() {
                    return Err("string out of bounds".into());
                }
                if key == wanted {
                    matched = Some(String::from_utf8_lossy(&bytes[offset + 13..end]).into_owned());
                }
                offset = end;
            }
        }
        files.push(serde_json::json!({"group":entry.group,"directory":entry.directory,"name":entry.name,"size":bytes.len(),"sha256":format!("{:x}",Sha256::digest(&bytes)),"prefix":&bytes[..16.min(bytes.len())]}));
    }
    let icons = archive.list_group("0012")?;
    let sample: Vec<_> = icons
        .iter()
        .filter(|e| e.path.starts_with("ui/texture/icon") && e.name.contains("arrow"))
        .take(12)
        .collect();
    let icon = archive.extract(sample[0])?;
    println!(
        "{}",
        serde_json::to_string_pretty(
            &serde_json::json!({"files":files,"icons":sample,"icon_entry_count":icons.len(),"dds_header":&icon[..128],"matched":matched})
        )?
    );
    Ok(())
}
