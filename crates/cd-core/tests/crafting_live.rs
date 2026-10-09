use cd_core::{
    GameData,
    config::LocalConfig,
    crafting::{CraftCatalog, PlanRequest},
};
use std::path::Path;
#[test]
fn real_recipe_group_and_item_drop_rows_roundtrip_and_plan()
-> Result<(), Box<dyn std::error::Error>> {
    let project = Path::new(env!("CARGO_MANIFEST_DIR"))
        .parent()
        .unwrap()
        .parent()
        .unwrap();
    let config = LocalConfig::load(project)?;
    let Some(root) = config.game_dir else {
        eprintln!("SKIP crafting_live: CD_GAME_DIR not configured");
        return Ok(());
    };
    let data = GameData::open(&root, "ger")?;
    let catalog = CraftCatalog::open(&data)?;
    let info = catalog.info();
    assert_eq!(info.coverage.recipe_rows, 18576);
    assert_eq!(info.coverage.group_rows, 1602);
    assert_eq!(info.coverage.interpreted_dropsets, 13035);
    assert_eq!(info.coverage.usable_recipes, 1108);
    let request: PlanRequest = serde_json::from_value(
        serde_json::json!({"target":50001,"quantity":"31","recipes":{"50001":1},"owned":{"710001":"3"}}),
    )?;
    let plan = catalog.plan(request)?;
    assert_eq!(plan.root.batches.0, 2);
    assert_eq!(plan.root.produced.0, 60);
    assert_eq!(
        plan.materials
            .iter()
            .find(|m| m.item.key == 710001)
            .unwrap()
            .needed
            .0,
        7
    );
    assert_eq!(
        plan.materials
            .iter()
            .find(|m| m.item.key == 720001)
            .unwrap()
            .needed
            .0,
        2
    );
    let soup = catalog.recipes_for(1002066);
    let recipe = soup.iter().find(|r| r.key == 343).unwrap();
    assert_eq!(recipe.ingredients.len(), 5);
    assert_eq!(recipe.ingredients[0].quantity.0, 5);
    assert!(recipe.ingredients.iter().any(|i| i.choices.len() > 20));
    // Every enabled recipe must produce a bounded, valid graph, including cycles.
    for target in info.targets {
        for recipe in catalog.recipes_for(target.key) {
            catalog.plan(serde_json::from_value(serde_json::json!({"target":target.key,"quantity":"1","recipes":{target.key.to_string():recipe.key}}))?)?;
        }
    }
    Ok(())
}
