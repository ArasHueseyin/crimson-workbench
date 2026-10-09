//! Read-only recipe graph and integer material planner for the pinned build.
pub(crate) mod formats;
mod planner;
#[cfg(test)]
mod tests;
use crate::{
    Error, GameData, Result,
    tables::{IndexedTable, TableSchema},
};
use formats::{DropRow, GroupRow, RecipeRow};
pub use planner::{Plan, PlanRequest, Quantity};
use serde::Serialize;
use std::collections::{BTreeMap, BTreeSet};

#[derive(Clone, Debug, Serialize)]
pub struct ItemRef {
    pub key: u32,
    pub name: String,
    pub internal_key: String,
}
#[derive(Clone, Debug, Serialize)]
pub struct Ingredient {
    pub quantity: Quantity,
    pub choices: Vec<ItemRef>,
    pub group: Option<u16>,
    pub group_name: Option<String>,
    pub slot: String,
}
#[derive(Clone, Debug, Serialize)]
pub struct Recipe {
    pub key: u32,
    pub internal_key: String,
    pub output: ItemRef,
    pub output_quantity: Quantity,
    pub tool_key: u16,
    pub knowledge_key: u32,
    pub condition_keys: Vec<u32>,
    pub ingredients: Vec<Ingredient>,
    pub result_dropset: u32,
}
#[derive(Clone, Debug, Serialize)]
pub struct DropSource {
    pub key: u32,
    pub internal_key: String,
    pub minimum: Quantity,
    pub maximum: Quantity,
    pub rate: Quantity,
    pub total_rate: Quantity,
    pub roll_type: u8,
    pub roll_count: u32,
    pub chance_percent: Option<f64>,
    pub world_source: Option<String>,
}
#[derive(Clone, Debug, Serialize)]
pub struct Coverage {
    pub recipe_rows: usize,
    pub group_rows: usize,
    pub dropset_rows: usize,
    pub interpreted_dropsets: usize,
    pub usable_recipes: usize,
    pub excluded: BTreeMap<String, usize>,
    pub source_note: String,
}
#[derive(Clone, Debug, Serialize)]
pub struct CraftInfo {
    pub targets: Vec<ItemRef>,
    pub coverage: Coverage,
}
pub struct CraftCatalog {
    pub(crate) items: BTreeMap<u32, ItemRef>,
    pub(crate) recipes: BTreeMap<u32, Vec<Recipe>>,
    pub(crate) drops: BTreeMap<u32, Vec<DropSource>>,
    coverage: Coverage,
}
impl CraftCatalog {
    pub fn open(data: &GameData) -> Result<Self> {
        let blobs = data.crafting_blobs()?;
        let table = |name: &'static str, width| -> Result<IndexedTable<'_>> {
            let body = &blobs[&format!("{name}.staticinfobody")];
            let header = &blobs[&format!("{name}.staticinfoheader")];
            let parsed = IndexedTable::parse(
                TableSchema {
                    name,
                    count_bytes: 2,
                    key_bytes: width,
                    interpretation: "phase3 versioned read-only",
                },
                body,
                header,
            )?;
            if parsed.serialize_body() != *body || parsed.serialize_header()? != *header {
                return Err(invalid(
                    "Craft table index/raw boundaries did not roundtrip",
                ));
            }
            Ok(parsed)
        };
        let mut groups = BTreeMap::new();
        let group_table = table("itemgroupinfo", 2)?;
        for row in group_table.records() {
            let g = GroupRow::parse(group_table.record_bytes(row.key).unwrap())?;
            if u32::from(g.key) != row.key {
                return Err(invalid("Group key differs from header"));
            }
            groups.insert(g.key, g);
        }
        let item_refs: BTreeMap<_, _> = data
            .localized_items()?
            .into_iter()
            .map(|v| {
                (
                    v.key,
                    ItemRef {
                        key: v.key,
                        name: v.name,
                        internal_key: v.internal_key,
                    },
                )
            })
            .collect();
        let drop_table = table("dropsetinfo", 4)?;
        let mut drop_rows = BTreeMap::new();
        let mut drops: BTreeMap<u32, Vec<DropSource>> = BTreeMap::new();
        for row in drop_table.records() {
            // Unsupported variants stay opaque; never resynchronize or scan for item IDs.
            if let Ok(d) = DropRow::parse_items(drop_table.record_bytes(row.key).unwrap()) {
                if d.key != row.key {
                    return Err(invalid("Drop key differs from header"));
                }
                for entry in &d.drops {
                    if entry.item != entry.duplicate
                        || !item_refs.contains_key(&entry.item)
                        || d.blocked != 0
                    {
                        continue;
                    }
                    let chance = if d.deterministic().is_some() {
                        Some(100.0)
                    } else {
                        None
                    };
                    drops.entry(entry.item).or_default().push(DropSource {
                        key: d.key,
                        internal_key: lossy(&d.name),
                        minimum: Quantity(entry.min),
                        maximum: Quantity(entry.max),
                        rate: Quantity(entry.rate),
                        total_rate: Quantity(d.total_rate),
                        roll_type: d.roll,
                        roll_count: d.rolls,
                        chance_percent: chance,
                        world_source: None,
                    });
                }
                drop_rows.insert(d.key, d);
            }
        }
        let recipe_table = table("multichangeinfo", 4)?;
        let mut recipes: BTreeMap<u32, Vec<Recipe>> = BTreeMap::new();
        let mut excluded = BTreeMap::new();
        for row in recipe_table.records() {
            let r = RecipeRow::parse(recipe_table.record_bytes(row.key).unwrap())?;
            if r.key != row.key {
                return Err(invalid("Recipe key differs from header"));
            }
            match build_recipe(&r, &groups, &drop_rows, &item_refs) {
                Ok(recipe) => recipes.entry(recipe.output.key).or_default().push(recipe),
                Err(reason) => *excluded.entry(reason.to_string()).or_insert(0) += 1,
            }
        }
        for variants in recipes.values_mut() {
            variants.sort_by_key(|r| r.key);
        }
        let coverage=Coverage{recipe_rows:recipe_table.records().len(),group_rows:groups.len(),dropset_rows:drop_table.records().len(),interpreted_dropsets:drop_rows.len(),usable_recipes:recipes.values().map(Vec::len).sum(),excluded,source_note:"Händler, Regionen und Weltquellen sind noch nicht verifiziert. Dropset-Verweise sind Tabellenquellen; eine unbekannte Chance bedeutet nicht 0 %. Kein Save-Import.".into()};
        Ok(Self {
            items: item_refs,
            recipes,
            drops,
            coverage,
        })
    }
    pub fn info(&self) -> CraftInfo {
        let mut targets: Vec<_> = self.recipes.keys().map(|k| self.items[k].clone()).collect();
        targets.sort_by_cached_key(|v| v.name.to_lowercase());
        CraftInfo {
            targets,
            coverage: self.coverage.clone(),
        }
    }
    pub fn recipes_for(&self, item: u32) -> Vec<Recipe> {
        self.recipes.get(&item).cloned().unwrap_or_default()
    }
    pub fn used_in(&self, item: u32) -> Vec<Recipe> {
        self.recipes
            .values()
            .flatten()
            .filter(|r| {
                r.ingredients
                    .iter()
                    .any(|i| i.choices.iter().any(|c| c.key == item))
            })
            .cloned()
            .collect()
    }
    pub fn plan(&self, request: PlanRequest) -> Result<Plan> {
        planner::calculate(self, request)
    }
}
fn invalid(s: &str) -> Error {
    Error::Invalid(s.into())
}
fn lossy(bytes: &[u8]) -> String {
    String::from_utf8_lossy(bytes).into_owned()
}
fn group_members(
    key: u16,
    groups: &BTreeMap<u16, GroupRow>,
    path: &mut BTreeSet<u16>,
    out: &mut BTreeSet<u32>,
) -> std::result::Result<(), &'static str> {
    if path.len() > 32 || !path.insert(key) {
        return Err("cyclic_material_group");
    }
    let g = groups.get(&key).ok_or("missing_material_group")?;
    if g.blocked != 0 || !g.unknown_types.is_empty() {
        return Err("unsupported_material_group");
    }
    out.extend(&g.items);
    for child in &g.groups {
        group_members(*child, groups, path, out)?;
    }
    path.remove(&key);
    Ok(())
}
fn build_recipe(
    r: &RecipeRow,
    groups: &BTreeMap<u16, GroupRow>,
    drops: &BTreeMap<u32, DropRow>,
    items: &BTreeMap<u32, ItemRef>,
) -> std::result::Result<Recipe, &'static str> {
    if r.blocked != 0 {
        return Err("blocked");
    }
    if r.consume != 1 {
        return Err("enhancement_or_special_consumption");
    }
    if r.flags[4] != 0 || r.elemental != 0 || !r.states.is_empty() {
        return Err("special_material_state");
    }
    if r.results.len() != 1 || !r.additional.is_empty() {
        return Err("multiple_or_bonus_results");
    }
    let output = drops
        .get(&r.results[0])
        .and_then(DropRow::deterministic)
        .ok_or("non_deterministic_or_special_result")?;
    let output_item = items.get(&output.0).ok_or("missing_output_item")?.clone();
    let mut ingredients = vec![];
    for (idx, f) in r.fixed.iter().enumerate() {
        if f.character != 0 || f.gimmick != 0 || f.enchant != 0 || f.coupon != 0 || f.count == 0 {
            return Err("special_fixed_material");
        }
        let item = items.get(&f.item).ok_or("missing_material_item")?.clone();
        ingredients.push(Ingredient {
            quantity: Quantity(f.count),
            choices: vec![item],
            group: None,
            group_name: None,
            slot: format!("{}:fixed:{idx}", r.key),
        });
    }
    for (idx, g) in r.groups.iter().enumerate() {
        if g.count == 0 || g.unknown != 0 {
            return Err("special_group_material");
        }
        let mut members = BTreeSet::new();
        group_members(g.group, groups, &mut BTreeSet::new(), &mut members)?;
        if members.is_empty() {
            return Err("empty_material_group");
        }
        let mut choices: Vec<_> = members
            .into_iter()
            .map(|k| items.get(&k).cloned().ok_or("missing_group_member"))
            .collect::<std::result::Result<_, _>>()?;
        choices.sort_by_cached_key(|c| (c.name.to_lowercase(), c.key));
        ingredients.push(Ingredient {
            quantity: Quantity(g.count),
            choices,
            group: Some(g.group),
            group_name: Some(lossy(&groups[&g.group].name)),
            slot: format!("{}:group:{idx}", r.key),
        });
    }
    if ingredients.is_empty() {
        return Err("no_materials");
    }
    Ok(Recipe {
        key: r.key,
        internal_key: lossy(&r.name),
        output: output_item,
        output_quantity: Quantity(output.1),
        tool_key: r.tool,
        knowledge_key: r.knowledge,
        condition_keys: r.conditions.iter().map(|v| v.0).collect(),
        ingredients,
        result_dropset: r.results[0],
    })
}
