use cd_core::{
    GameData,
    crafting::{CraftCatalog, PlanRequest},
};
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let root = std::env::args().nth(1).ok_or("game root required")?;
    let data = GameData::open(std::path::Path::new(&root), "ger")?;
    let catalog = CraftCatalog::open(&data)?;
    let request: PlanRequest = serde_json::from_value(
        serde_json::json!({"target":50001,"quantity":"31","owned":{"710001":"3"}}),
    )?;
    println!(
        "{}",
        serde_json::to_string_pretty(
            &serde_json::json!({"info":catalog.info(),"plan":catalog.plan(request)?})
        )?
    );
    Ok(())
}
