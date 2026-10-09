use super::*;
use serde::{Deserialize, Deserializer, Serializer};
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Quantity(pub u64);
impl Serialize for Quantity {
    fn serialize<S: Serializer>(&self, s: S) -> std::result::Result<S::Ok, S::Error> {
        s.serialize_str(&self.0.to_string())
    }
}
impl<'de> Deserialize<'de> for Quantity {
    fn deserialize<D: Deserializer<'de>>(d: D) -> std::result::Result<Self, D::Error> {
        let s = String::deserialize(d)?;
        if s.is_empty() || !s.bytes().all(|b| b.is_ascii_digit()) {
            return Err(serde::de::Error::custom(
                "Menge muss eine ganze positive Zahl sein",
            ));
        }
        s.parse().map(Self).map_err(serde::de::Error::custom)
    }
}
#[derive(Clone, Debug, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct PlanRequest {
    pub target: u32,
    pub quantity: Quantity,
    #[serde(default)]
    pub owned: BTreeMap<u32, Quantity>,
    #[serde(default)]
    pub recipes: BTreeMap<u32, u32>,
    #[serde(default)]
    pub choices: BTreeMap<String, u32>,
    #[serde(default)]
    pub acquire: BTreeSet<u32>,
}
#[derive(Debug, Serialize)]
pub struct PlanNode {
    pub item: ItemRef,
    pub requested: Quantity,
    pub from_owned: Quantity,
    pub from_surplus: Quantity,
    pub batches: Quantity,
    pub produced: Quantity,
    pub recipe: Option<Recipe>,
    pub alternatives: Vec<Recipe>,
    pub reason: String,
    pub children: Vec<PlanNode>,
}
#[derive(Debug, Serialize)]
pub struct Material {
    pub item: ItemRef,
    pub needed: Quantity,
    pub craftable: bool,
    pub drops: Vec<DropSource>,
    pub vendor_status: String,
}
#[derive(Debug, Serialize)]
pub struct InventoryRow {
    pub item: ItemRef,
    pub owned: Quantity,
    pub used: Quantity,
    pub surplus: Quantity,
}
#[derive(Debug, Serialize)]
pub struct Plan {
    pub root: PlanNode,
    pub materials: Vec<Material>,
    pub inventory: Vec<InventoryRow>,
    pub warnings: Vec<String>,
}
struct State<'a> {
    catalog: &'a CraftCatalog,
    request: &'a PlanRequest,
    owned: BTreeMap<u32, u64>,
    surplus: BTreeMap<u32, u64>,
    used: BTreeMap<u32, u64>,
    needed: BTreeMap<u32, u64>,
    seen: BTreeSet<u32>,
    warnings: BTreeSet<String>,
    nodes: usize,
}
fn add(map: &mut BTreeMap<u32, u64>, key: u32, amount: u64) -> Result<()> {
    let n = map.entry(key).or_default();
    *n = n
        .checked_add(amount)
        .ok_or_else(|| invalid("Mengenüberlauf"))?;
    Ok(())
}
fn take(map: &mut BTreeMap<u32, u64>, key: u32, amount: u64) -> u64 {
    let v = map.entry(key).or_default();
    let n = (*v).min(amount);
    *v -= n;
    n
}
impl State<'_> {
    fn expand(&mut self, key: u32, quantity: u64, path: &mut BTreeSet<u32>) -> Result<PlanNode> {
        self.nodes += 1;
        if self.nodes > 1000 || path.len() > 32 {
            return Err(invalid(
                "Rezeptbaum überschreitet 1.000 Knoten oder 32 Ebenen",
            ));
        }
        let item = self
            .catalog
            .items
            .get(&key)
            .ok_or_else(|| invalid("Unbekanntes Material"))?
            .clone();
        self.seen.insert(key);
        let from_owned = take(&mut self.owned, key, quantity);
        add(&mut self.used, key, from_owned)?;
        let from_surplus = take(&mut self.surplus, key, quantity - from_owned);
        let remaining = quantity - from_owned - from_surplus;
        let alternatives = self.catalog.recipes_for(key);
        let mut node = PlanNode {
            item,
            requested: Quantity(quantity),
            from_owned: Quantity(from_owned),
            from_surplus: Quantity(from_surplus),
            batches: Quantity(0),
            produced: Quantity(0),
            recipe: None,
            alternatives: alternatives.clone(),
            reason: "owned".into(),
            children: vec![],
        };
        if remaining == 0 {
            return Ok(node);
        }
        let selected = if let Some(wanted) = self.request.recipes.get(&key) {
            Some(
                alternatives
                    .iter()
                    .find(|r| r.key == *wanted)
                    .ok_or_else(|| invalid("Rezept gehört nicht zum Zielitem"))?
                    .clone(),
            )
        } else {
            alternatives.first().cloned()
        };
        let cycle = path.contains(&key);
        if selected.is_none() || self.request.acquire.contains(&key) || cycle {
            node.reason = if cycle {
                "cycle"
            } else if self.request.acquire.contains(&key) {
                "acquire"
            } else {
                "no_supported_recipe"
            }
            .into();
            if cycle {
                self.warnings.insert(format!(
                    "Rezeptzyklus bei {}: Restmenge muss beschafft werden.",
                    node.item.name
                ));
            }
            add(&mut self.needed, key, remaining)?;
            return Ok(node);
        }
        let recipe = selected.unwrap();
        let batches = remaining.div_ceil(recipe.output_quantity.0);
        let produced = batches
            .checked_mul(recipe.output_quantity.0)
            .ok_or_else(|| invalid("Mengenüberlauf"))?;
        path.insert(key);
        for ingredient in &recipe.ingredients {
            let choice = self
                .request
                .choices
                .get(&ingredient.slot)
                .copied()
                .unwrap_or(ingredient.choices[0].key);
            if !ingredient.choices.iter().any(|c| c.key == choice) {
                return Err(invalid(
                    "Ausgewähltes Material gehört nicht zur Rezeptgruppe",
                ));
            }
            let amount = batches
                .checked_mul(ingredient.quantity.0)
                .ok_or_else(|| invalid("Mengenüberlauf"))?;
            node.children.push(self.expand(choice, amount, path)?);
        }
        path.remove(&key);
        add(&mut self.surplus, key, produced - remaining)?;
        node.reason = "craft".into();
        node.batches = Quantity(batches);
        node.produced = Quantity(produced);
        node.recipe = Some(recipe);
        Ok(node)
    }
}
pub(super) fn calculate(catalog: &CraftCatalog, request: PlanRequest) -> Result<Plan> {
    if request.quantity.0 == 0 || request.quantity.0 > 1_000_000_000_000 {
        return Err(invalid(
            "Zielmenge muss zwischen 1 und 1.000.000.000.000 liegen",
        ));
    }
    if request.owned.len() > 10_000
        || request.choices.len() > 10_000
        || request.recipes.len() > 10_000
    {
        return Err(invalid("Zu viele Einstellungen"));
    }
    for key in request
        .owned
        .keys()
        .chain(request.acquire.iter())
        .chain(request.recipes.keys())
    {
        if !catalog.items.contains_key(key) {
            return Err(invalid("Unbekanntes Item in Einstellungen"));
        }
    }
    let mut state = State {
        catalog,
        request: &request,
        owned: request.owned.iter().map(|(k, v)| (*k, v.0)).collect(),
        surplus: BTreeMap::new(),
        used: BTreeMap::new(),
        needed: BTreeMap::new(),
        seen: BTreeSet::new(),
        warnings: BTreeSet::new(),
        nodes: 0,
    };
    let root = state.expand(request.target, request.quantity.0, &mut BTreeSet::new())?;
    let mut materials: Vec<_> = state
        .needed
        .into_iter()
        .map(|(key, n)| Material {
            item: catalog.items[&key].clone(),
            needed: Quantity(n),
            craftable: catalog.recipes.contains_key(&key),
            drops: catalog.drops.get(&key).cloned().unwrap_or_default(),
            vendor_status: "not_verified".into(),
        })
        .collect();
    materials.sort_by_cached_key(|m| m.item.name.to_lowercase());
    let inventory = state
        .seen
        .into_iter()
        .map(|key| InventoryRow {
            item: catalog.items[&key].clone(),
            owned: request.owned.get(&key).copied().unwrap_or_default(),
            used: Quantity(*state.used.get(&key).unwrap_or(&0)),
            surplus: Quantity(*state.surplus.get(&key).unwrap_or(&0)),
        })
        .collect();
    Ok(Plan {
        root,
        materials,
        inventory,
        warnings: state.warnings.into_iter().collect(),
    })
}
