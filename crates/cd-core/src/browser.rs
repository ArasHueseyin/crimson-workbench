//! Read-only desktop catalog. A session pins one checked snapshot and one index.
use crate::{IndexBuildInfo, Result, Workspace};
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::{
    collections::BTreeMap,
    path::{Path, PathBuf},
};

#[derive(Debug, Clone, Serialize)]
pub struct BrowserItem {
    pub key: u32,
    pub internal_key: String,
    pub name: String,
    pub description: String,
    pub item_type: u8,
    pub category: u16,
    pub tier: u8,
    // JS must not round u64 values crossing IPC.
    pub max_stack: String,
    pub stat_keys: Vec<u32>,
    pub knowledge_keys: Vec<u32>,
    pub icon_key: Option<u32>,
    pub use_restriction: Option<String>,
}
#[derive(Debug, Clone, Deserialize, Default)]
#[serde(default, deny_unknown_fields)]
pub struct BrowserQuery {
    pub text: String,
    pub regex: bool,
    pub item_type: Option<u8>,
    pub category: Option<u16>,
    pub group: Option<u32>,
    pub tier: Option<u8>,
    pub stackable: bool,
    pub stat_key: Option<u32>,
    pub sort: BrowserSort,
    pub descending: bool,
    pub offset: usize,
}
#[derive(Debug, Clone, Copy, Deserialize, Default)]
#[serde(rename_all = "snake_case")]
pub enum BrowserSort {
    #[default]
    Name,
    Key,
    Tier,
    Stack,
}
#[derive(Debug, Serialize)]
pub struct BrowserPage {
    pub items: Vec<BrowserItem>,
    pub total: usize,
    pub offset: usize,
}
#[derive(Serialize)]
pub struct Facet {
    pub value: u32,
    pub count: usize,
}
#[derive(Serialize)]
pub struct BrowserInfo {
    pub item_count: usize,
    pub types: Vec<Facet>,
    pub categories: Vec<Facet>,
    pub groups: Vec<crate::item_groups::ItemGroup>,
    pub group_error: Option<String>,
    pub tiers: Vec<Facet>,
    pub stats: Vec<Facet>,
    pub index: IndexBuildInfo,
    pub exe_version: Option<String>,
}
pub struct BrowserSession {
    workspace: Workspace,
    save_root: Option<PathBuf>,
    items: Vec<BrowserItem>,
    index_info: IndexBuildInfo,
    icons: crate::icons::IconCatalog,
    groups: Vec<crate::item_groups::ItemGroup>,
    group_error: Option<String>,
    crafting: std::sync::Mutex<Option<crate::crafting::CraftCatalog>>,
    mods: std::sync::Mutex<Option<crate::mods::ModCatalog>>,
    mounts: std::sync::Mutex<Option<Vec<crate::mounts::Mount>>>,
}
impl BrowserSession {
    pub fn extra_sockets(&self) -> Result<crate::extra_sockets::Snapshot> {
        crate::extra_sockets::snapshot(self.workspace.data())
    }
    pub fn abyss_stone_details(&self) -> Result<Vec<crate::data::AbyssStoneDetail>> {
        self.workspace.data().abyss_stone_details()
    }
    pub fn set_extra_socket(
        &self,
        project: &Path,
        request: &crate::extra_sockets::Request,
    ) -> Result<crate::extra_sockets::Receipt> {
        crate::extra_sockets::set(self.workspace.data(), project, request)
    }
    pub fn extra_socket_candidates(
        &self,
        save: Option<&str>,
    ) -> Result<crate::extra_sockets_candidates::Candidates> {
        crate::extra_sockets_candidates::candidates(self.workspace.data(), self.save_root()?, save)
    }
    pub fn add_extra_sockets(
        &self,
        project: &Path,
        request: &crate::extra_sockets_candidates::AddRequest,
    ) -> Result<crate::extra_sockets::Receipt> {
        crate::extra_sockets::add(self.workspace.data(), project, self.save_root()?, request)
    }
    pub fn game_root(&self) -> &Path {
        self.workspace.data().game_root()
    }
    pub fn save_root(&self) -> Result<&Path> {
        self.save_root.as_deref().ok_or_else(|| crate::Error::Invalid(
            "Bitte unter Datenquellen einen Spielstandordner auswählen und die Installation neu einlesen. Kein eindeutiger Saveordner erkannt.".into()
        ))
    }
    pub fn mounts(&self) -> Result<Vec<crate::mounts::Mount>> {
        let mut catalog = self
            .mounts
            .lock()
            .map_err(|_| crate::Error::Invalid("Reittierkatalog-Sperre beschädigt".into()))?;
        if catalog.is_none() {
            *catalog = Some(crate::mounts::catalog(self.workspace.data())?);
        }
        Ok(catalog.as_ref().unwrap().clone())
    }
    pub fn mount_search(&self, text: &str, regex: bool) -> Result<Vec<u32>> {
        let matcher = crate::search::Matcher::new(text, regex)?;
        Ok(self
            .mounts()?
            .iter()
            .filter(|m| matcher.matches(m.key, &[&m.name, &m.internal, &m.family]))
            .map(|m| m.key)
            .collect())
    }
    pub fn mount_icon(&self, key: u32) -> Result<crate::IconResult> {
        let mounts = self.mounts()?;
        let mount = mounts
            .iter()
            .find(|m| m.key == key)
            .ok_or_else(|| crate::Error::Invalid("Unknown mount".into()))?;
        Ok(self.icons.mount_icon(&mount.internal))
    }
    pub fn knowledge(&self, save: Option<&str>) -> Result<crate::knowledge::Snapshot> {
        crate::knowledge::snapshot(self.workspace.data().items(), self.save_root()?, save)
    }
    pub fn open(project: &Path, game: Option<&Path>, language: &str) -> Result<Self> {
        let workspace = Workspace::open(project, game, language)?;
        let report = crate::discover_project(project, game)?;
        let icons = crate::icons::IconCatalog::open(&crate::select_game(&report)?)?;
        // A different CLI/UI language must never replace this snapshot's FTS data.
        let cache = format!(
            ".local/index/catalog-{}-{}.sqlite",
            workspace.data().language(),
            workspace.data().fingerprint().digest
        );
        let (_index, index_info) = workspace.index(Some(Path::new(&cache)))?;
        let localized = workspace.data().localized_items()?;
        let by_key: BTreeMap<_, _> = workspace
            .data()
            .items()
            .iter()
            .map(|i| (i.key, i))
            .collect();
        let items = localized
            .into_iter()
            .map(|i| {
                let raw = by_key[&i.key];
                BrowserItem {
                    key: i.key,
                    internal_key: i.internal_key,
                    name: i.name,
                    description: i.description,
                    item_type: raw.item_type,
                    category: raw.category_info,
                    tier: raw.item_tier,
                    max_stack: raw.max_stack_count.to_string(),
                    stat_keys: raw.stat_keys.clone(),
                    knowledge_keys: raw.knowledge_keys.clone(),
                    icon_key: raw.icon_path,
                    use_restriction: grant_restriction(raw.key, &raw.string_key).map(str::to_owned),
                }
            })
            .collect();
        let (groups, group_error) = match workspace.data().item_groups() {
            Ok(groups) => (groups, None),
            Err(e) => (Vec::new(), Some(e.to_string())),
        };
        Ok(Self {
            workspace,
            save_root: crate::project_save_root(project)?,
            items,
            index_info,
            icons,
            groups,
            group_error,
            crafting: std::sync::Mutex::new(None),
            mods: std::sync::Mutex::new(None),
            mounts: std::sync::Mutex::new(None),
        })
    }
    pub fn info(&self) -> BrowserInfo {
        BrowserInfo {
            item_count: self.items.len(),
            types: facets(self.items.iter().map(|i| i.item_type as u32)),
            categories: facets(self.items.iter().map(|i| i.category as u32)),
            groups: self.groups.clone(),
            group_error: self.group_error.clone(),
            tiers: facets(self.items.iter().map(|i| i.tier as u32)),
            stats: facets(self.items.iter().flat_map(|i| i.stat_keys.iter().copied())),
            index: IndexBuildInfo {
                path: self.index_info.path.clone(),
                rebuilt: self.index_info.rebuilt,
                items: self.index_info.items,
                language: self.index_info.language.clone(),
                fingerprint: self.index_info.fingerprint.clone(),
            },
            exe_version: self.workspace.data().fingerprint().exe_version.clone(),
        }
    }
    pub fn with_crafting<T>(
        &self,
        f: impl FnOnce(&crate::crafting::CraftCatalog) -> Result<T>,
    ) -> Result<T> {
        let mut guard = self
            .crafting
            .lock()
            .map_err(|_| crate::Error::Invalid("Crafting lock poisoned".into()))?;
        if guard.is_none() {
            *guard = Some(crate::crafting::CraftCatalog::open(self.workspace.data())?);
        }
        f(guard.as_ref().unwrap())
    }
    pub fn with_mods<T>(&self, f: impl FnOnce(&crate::mods::ModCatalog) -> Result<T>) -> Result<T> {
        let mut guard = self
            .mods
            .lock()
            .map_err(|_| crate::Error::Invalid("Mod lock poisoned".into()))?;
        if guard.is_none() {
            *guard = Some(crate::mods::ModCatalog::open(self.workspace.data())?);
        }
        f(guard.as_ref().unwrap())
    }
    pub fn mod_export(
        &self,
        request: crate::mods::ModRequest,
        plan_id: &str,
    ) -> Result<std::path::PathBuf> {
        self.with_mods(|c| {
            let built = c.build(request)?;
            if built.preview.plan_id != plan_id {
                return Err(crate::Error::Invalid(
                    "Vorschau ist veraltet; neu berechnen".into(),
                ));
            }
            crate::apply::export(&self.workspace, &built)
        })
    }
    pub fn mod_rehearse(
        &self,
        request: crate::mods::ModRequest,
        plan_id: &str,
    ) -> Result<crate::apply::Rehearsal> {
        self.with_mods(|c| {
            let built = c.build(request)?;
            if built.preview.plan_id != plan_id {
                return Err(crate::Error::Invalid(
                    "Vorschau ist veraltet; neu berechnen".into(),
                ));
            }
            crate::apply::rehearse(&self.workspace, &built)
        })
    }
    pub fn installation_check(&self) -> Result<crate::installation::Inventory> {
        crate::installation::inspect_managed(
            self.workspace.policy(),
            self.workspace.data().game_root(),
        )
    }
    pub fn search(&self, query: BrowserQuery) -> Result<BrowserPage> {
        let matcher = crate::search::Matcher::new(&query.text, query.regex)?;
        let keys = Some(
            self.items
                .iter()
                .filter(|i| matcher.matches(i.key, &[&i.name, &i.internal_key, &i.description]))
                .map(|i| i.key)
                .collect(),
        );
        let group_keys = query
            .group
            .map(|key| {
                self.groups
                    .iter()
                    .find(|g| g.key == key)
                    .map(|g| {
                        g.items
                            .iter()
                            .copied()
                            .collect::<std::collections::HashSet<u32>>()
                    })
                    .ok_or_else(|| crate::Error::Invalid("Unbekannte Itemgruppe".into()))
            })
            .transpose()?;
        let filtered: Vec<_> = filter_sort(&self.items, &query, keys.as_ref())
            .into_iter()
            .filter(|i| group_keys.as_ref().is_none_or(|g| g.contains(&i.key)))
            .collect();
        let total = filtered.len();
        let items = filtered
            .into_iter()
            .skip(query.offset)
            .take(200)
            .cloned()
            .collect();
        Ok(BrowserPage {
            items,
            total,
            offset: query.offset,
        })
    }
    pub fn item(&self, key: u32) -> Result<Value> {
        let mut detail = self.workspace.data().item_detail(key, true)?;
        let references:Vec<_> = detail["fields"].as_array().into_iter().flatten()
            .filter(|f| f["type_name"]=="ItemKey" && f["path"]!="key")
            .filter_map(|f| {
                let target=u32::try_from(f["value"].as_u64()?).ok()?;
                if target==0 { return None; }
                let item=self.items.iter().find(|i|i.key==target);
                Some(serde_json::json!({"path":f["path"],"key":target,"name":item.map(|i|&i.name),"available":item.is_some()}))
            }).collect();
        detail["item_references"] = serde_json::to_value(references)?;
        preserve_js_integers(&mut detail);
        Ok(detail)
    }
    pub fn validate_item_grant(&self, key: u32) -> Result<()> {
        let item = self
            .items
            .iter()
            .find(|i| i.key == key)
            .ok_or_else(|| crate::Error::Invalid("Unknown item".into()))?;
        if let Some(reason) = &item.use_restriction {
            return Err(crate::Error::Invalid(reason.clone()));
        }
        Ok(())
    }
    pub fn icon(&self, key: u32) -> Result<crate::IconResult> {
        let item = self
            .items
            .iter()
            .find(|i| i.key == key)
            .ok_or_else(|| crate::Error::Invalid("Unknown item".into()))?;
        Ok(item.icon_key.map_or_else(
            || crate::IconResult::unavailable("Keine Iconreferenz"),
            |hash| self.icons.icon(hash),
        ))
    }
    pub fn export_item(&self, key: u32) -> Result<std::path::PathBuf> {
        let detail = self.item(key)?;
        let stamp = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .map_err(|e| crate::Error::Invalid(e.to_string()))?
            .as_nanos();
        self.workspace.write_json(
            Path::new(&format!("exports/item-{key}-{stamp}.json")),
            &detail,
        )
    }
}
fn facets(values: impl Iterator<Item = u32>) -> Vec<Facet> {
    let mut counts = BTreeMap::new();
    for value in values {
        *counts.entry(value).or_default() += 1;
    }
    counts
        .into_iter()
        .map(|(value, count)| Facet { value, count })
        .collect()
}
// Verified ItemInfo notes: these three elemental arrow definitions explicitly
// say "몬스터용" (for monsters), while Bomb_Arrow says player use is allowed.
// Keep them searchable, but do not confuse a successful grant with usable ammo.
fn grant_restriction(key: u32, internal: &str) -> Option<&'static str> {
    match (key, internal) {
        (1001314, "Ice_Arrow") | (1001315, "Lightning_Arrow") | (1001316, "Fire_Arrow") => Some(
            "Monstermunition: Diese Pfeilvariante ist in den Spieldaten für NPCs vorgesehen. Die Vergabe ins Inventar macht sie nicht zu regulärer Spielermunition. Verwende Pfeil (ID 50001), Giftpfeil (ID 50003) oder Explosionspfeil (ID 1001321).",
        ),
        _ => None,
    }
}
fn filter_sort<'a>(
    items: &'a [BrowserItem],
    q: &BrowserQuery,
    keys: Option<&std::collections::HashSet<u32>>,
) -> Vec<&'a BrowserItem> {
    let mut out: Vec<_> = items
        .iter()
        .filter(|i| {
            keys.is_none_or(|keys| keys.contains(&i.key))
                && q.item_type.is_none_or(|v| v == i.item_type)
                && q.category.is_none_or(|v| v == i.category)
                && q.tier.is_none_or(|v| v == i.tier)
                && (!q.stackable || i.max_stack.parse::<u64>().unwrap_or(0) > 1)
                && q.stat_key.is_none_or(|v| i.stat_keys.contains(&v))
        })
        .collect();
    out.sort_by(|a, b| {
        let order = match q.sort {
            BrowserSort::Name => a.name.to_lowercase().cmp(&b.name.to_lowercase()),
            BrowserSort::Key => a.key.cmp(&b.key),
            BrowserSort::Tier => a.tier.cmp(&b.tier),
            BrowserSort::Stack => a
                .max_stack
                .parse::<u64>()
                .unwrap_or(0)
                .cmp(&b.max_stack.parse::<u64>().unwrap_or(0)),
        }
        .then(a.key.cmp(&b.key));
        if q.descending { order.reverse() } else { order }
    });
    out
}
/// All integer JSON values remain exact across the JavaScript bridge.
fn preserve_js_integers(value: &mut Value) {
    match value {
        Value::Number(n)
            if n.as_u64().is_some_and(|n| n > 9_007_199_254_740_991)
                || n.as_i64()
                    .is_some_and(|n| n.unsigned_abs() > 9_007_199_254_740_991) =>
        {
            *value = Value::String(n.to_string())
        }
        Value::Array(a) => a.iter_mut().for_each(preserve_js_integers),
        Value::Object(o) => o.values_mut().for_each(preserve_js_integers),
        _ => {}
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn filters_intersect_before_pagination_and_numeric_sort_keeps_u64_precision() {
        let item = |key, stack: &str, stats| BrowserItem {
            key,
            name: format!("Item {key}"),
            internal_key: String::new(),
            description: String::new(),
            item_type: 3,
            category: 7,
            tier: 2,
            max_stack: stack.into(),
            stat_keys: stats,
            knowledge_keys: Vec::new(),
            icon_key: None,
            use_restriction: None,
        };
        let items = vec![
            item(1, "9007199254740993", vec![44]),
            item(2, "9007199254740992", vec![44]),
            item(3, "1", vec![55]),
        ];
        let query = BrowserQuery {
            stackable: true,
            stat_key: Some(44),
            sort: BrowserSort::Stack,
            ..Default::default()
        };
        assert_eq!(
            filter_sort(&items, &query, None)
                .iter()
                .map(|i| i.key)
                .collect::<Vec<_>>(),
            vec![2, 1]
        );
        let keys = [1, 3].into_iter().collect();
        assert_eq!(
            filter_sort(&items, &query, Some(&keys))
                .iter()
                .map(|i| i.key)
                .collect::<Vec<_>>(),
            vec![1]
        );
    }
    #[test]
    fn large_integers_survive_nested_ipc_without_rounding() {
        let mut value = serde_json::json!({"a":[u64::MAX,i64::MIN,42]});
        preserve_js_integers(&mut value);
        assert_eq!(value["a"][0], u64::MAX.to_string());
        assert_eq!(value["a"][1], i64::MIN.to_string());
        assert_eq!(value["a"][2], 42);
    }
}
