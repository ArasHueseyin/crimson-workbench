//! Read-only distinction between arrow item definitions and their internal notes.
use cd_core::GameData;
use std::{fs::OpenOptions, io::Write, path::PathBuf};
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let game =
        PathBuf::from(std::env::args_os().nth(1).ok_or("Game root required")?).canonicalize()?;
    let data = GameData::open(&game, "ger")?;
    let mut out = Vec::new();
    for item in data
        .items()
        .iter()
        .filter(|i| i.string_key.to_ascii_lowercase().contains("arrow"))
    {
        let detail = data.item_detail(item.key, true)?;
        let fields = detail["fields"].as_array().ok_or("Missing fields")?;
        let relevant: Vec<_> = fields
            .iter()
            .filter(|f| {
                f["path"].as_str().is_some_and(|s| {
                    s == "item_memo"
                        || s.starts_with("item_use_info_list[")
                        || s == "is_editor_usable"
                        || s == "item_type"
                        || s == "equipable_type"
                })
            })
            .cloned()
            .collect();
        out.push(serde_json::json!({"key":item.key,"name":detail["name"],"internal":item.string_key,"description":detail["description"],"fields":relevant}));
    }
    let path =
        std::env::current_dir()?.join(".local/live-arrow-use-20261004/arrow-definitions.json");
    OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)?
        .write_all(&serde_json::to_vec_pretty(&out)?)?;
    println!(
        "{} arrow-related definitions inspected; no game/save writes.",
        out.len()
    );
    Ok(())
}
