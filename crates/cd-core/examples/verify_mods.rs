use cd_core::{
    Workspace,
    mods::{ModCatalog, ModRequest},
};
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let workspace = Workspace::open(std::path::Path::new("."), None, "ger")?;
    let catalog = ModCatalog::open(workspace.data())?;
    let info = catalog.info();
    println!(
        "{}",
        serde_json::json!({"stores":info.store_rows,"stocks":info.stock_rows,"vendors":info.vendors.len(),"dropsets":info.dropsets.len(),"trust":info.trust_rows,"opaque_dropsets":info.opaque_dropsets})
    );
    let built = catalog.build(ModRequest {
        shop_stock: Some(999),
        quantity_multiplier: 2,
        trust_multiplier: 3,
        ..Default::default()
    })?;
    println!(
        "{}",
        serde_json::json!({"changes":built.preview.changes.len(),"files":built.preview.files,"gates":built.preview.gates,"sample_trust":built.preview.changes.iter().filter(|c| c.module=="trust").take(8).collect::<Vec<_>>()})
    );
    let rehearsal = cd_core::apply::rehearse(&workspace, &built)?;
    println!("{}", serde_json::to_string(&rehearsal)?);
    Ok(())
}
