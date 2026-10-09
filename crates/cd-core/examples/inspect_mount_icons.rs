//! Read-only inventory of possible mount icon textures.
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let game = std::env::args_os().nth(1).ok_or("Game root required")?;
    let archive = crimson_format::Archive::open(std::path::PathBuf::from(game))?;
    let icons: Vec<_> = archive
        .list_group("0012")?
        .into_iter()
        .filter(|e| {
            e.directory.starts_with("ui/")
                && [
                    "horse",
                    "bear",
                    "lion",
                    "tiger",
                    "camel",
                    "deer",
                    "wolf",
                    "boar",
                    "elephant",
                    "riding",
                    "mercenary",
                    "dinosaur",
                    "iguana",
                    "ibex",
                    "kuku",
                    "bull",
                ]
                .iter()
                .any(|s| e.name.to_ascii_lowercase().contains(s))
        })
        .map(|e| e.path)
        .collect();
    println!("{}", serde_json::to_string_pretty(&icons)?);
    Ok(())
}
