use super::*;
use serde_json::json;
fn catalog() -> CraftCatalog {
    let items = (1..=4)
        .map(|key| {
            (
                key,
                ItemRef {
                    key,
                    name: format!("Item {key}"),
                    internal_key: format!("item_{key}"),
                },
            )
        })
        .collect::<BTreeMap<_, _>>();
    let recipe = |key, output, qty, inputs: Vec<(u32, u64)>| Recipe {
        key,
        internal_key: format!("recipe_{key}"),
        output: items[&output].clone(),
        output_quantity: Quantity(qty),
        tool_key: 1,
        knowledge_key: 0,
        condition_keys: vec![],
        ingredients: inputs
            .into_iter()
            .enumerate()
            .map(|(i, (item, n))| Ingredient {
                quantity: Quantity(n),
                choices: vec![items[&item].clone()],
                group: None,
                group_name: None,
                slot: format!("{key}:{i}"),
            })
            .collect(),
        result_dropset: 1,
    };
    let recipes = BTreeMap::from([
        (1, vec![recipe(10, 1, 1, vec![(2, 1), (3, 1)])]),
        (2, vec![recipe(20, 2, 3, vec![(4, 5)])]),
        (3, vec![recipe(30, 3, 1, vec![(2, 1), (4, 2)])]),
    ]);
    CraftCatalog {
        items,
        recipes,
        drops: BTreeMap::new(),
        coverage: Coverage {
            recipe_rows: 3,
            group_rows: 0,
            dropset_rows: 0,
            interpreted_dropsets: 0,
            usable_recipes: 3,
            excluded: BTreeMap::new(),
            source_note: String::new(),
        },
    }
}
#[test]
fn shared_inventory_and_batch_surplus_are_used_once() {
    let c = catalog();
    let p = c
        .plan(serde_json::from_value(json!({"target":1,"quantity":"1","owned":{"4":"3"}})).unwrap())
        .unwrap();
    assert_eq!(p.materials.len(), 1);
    assert_eq!(p.materials[0].item.key, 4);
    assert_eq!(p.materials[0].needed.0, 4);
    assert_eq!(p.root.children[1].children[0].from_surplus.0, 1);
    assert_eq!(
        p.inventory.iter().find(|r| r.item.key == 4).unwrap().used.0,
        3
    );
    assert_eq!(
        p.inventory
            .iter()
            .find(|r| r.item.key == 2)
            .unwrap()
            .surplus
            .0,
        1
    );
}
#[test]
fn owned_intermediates_reduce_recursive_work() {
    let p = catalog()
        .plan(serde_json::from_value(json!({"target":1,"quantity":"1","owned":{"2":"2"}})).unwrap())
        .unwrap();
    assert_eq!(p.materials[0].needed.0, 2);
    assert_eq!(p.root.children[0].from_owned.0, 1);
}
#[test]
fn cycles_are_visible_acquisition_requirements() {
    let mut c = catalog();
    c.recipes.get_mut(&2).unwrap()[0].ingredients[0].choices = vec![c.items[&1].clone()];
    let p = c
        .plan(serde_json::from_value(json!({"target":1,"quantity":"1"})).unwrap())
        .unwrap();
    assert!(!p.warnings.is_empty());
    assert!(p.materials.iter().any(|m| m.item.key == 1));
}
#[test]
fn exact_u64_and_overflow_are_not_rounded() {
    let q: Quantity = serde_json::from_value(json!("18446744073709551615")).unwrap();
    assert_eq!(q.0, u64::MAX);
    assert_eq!(
        serde_json::to_value(q).unwrap(),
        json!("18446744073709551615")
    );
    for v in [
        json!(2),
        json!("1.5"),
        json!("-1"),
        json!("18446744073709551616"),
    ] {
        assert!(serde_json::from_value::<Quantity>(v).is_err());
    }
    let mut c = catalog();
    c.recipes.get_mut(&1).unwrap()[0].ingredients[0].quantity = Quantity(u64::MAX);
    assert!(
        c.plan(serde_json::from_value(json!({"target":1,"quantity":"2"})).unwrap())
            .is_err()
    );
}
#[test]
fn invalid_choices_and_recipes_are_rejected() {
    for input in [
        json!({"target":1,"quantity":"1","choices":{"10:0":4}}),
        json!({"target":1,"quantity":"1","recipes":{"1":20}}),
        json!({"target":1,"quantity":"0"}),
    ] {
        assert!(
            catalog()
                .plan(serde_json::from_value(input).unwrap())
                .is_err()
        );
    }
}
#[test]
fn manual_acquisition_stops_expansion() {
    let p = catalog()
        .plan(
            serde_json::from_value(
                json!({"target":2,"quantity":"4","acquire":[2],"owned":{"2":"1"}}),
            )
            .unwrap(),
        )
        .unwrap();
    assert_eq!(p.materials[0].item.key, 2);
    assert_eq!(p.materials[0].needed.0, 3);
    assert_eq!(p.root.reason, "acquire");
}
