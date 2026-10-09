//! Edits are rebuilt from one immutable, hash-pinned snapshot, in memory only.
//! Unsupported semantics are errors, never silently approximated.
pub mod advanced;
mod chance;
mod store;
use crate::{
    Error, GameData, Result,
    crafting::formats::DropRow,
    fingerprint::hash_bytes,
    tables::{self, IndexedTable},
};
use serde::{Deserialize, Serialize};
use std::collections::{BTreeMap, BTreeSet};
use store::Store;

pub const CREDITS: &str = "Crimson Workbench: eigene Implementierung. Formatrecherche: GildyBoye – Crimson Desert Shop Editor (https://github.com/GildyBoye/CrimsonDesertShopEditor); NattKh – Crimson Desert Editor; CDUMM – faisalkindi. Archivformat/Krypto: crimson-rs – Tommy Tran (MIT). Keine Ausgabe dieser Referenzprogramme.";

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(default, deny_unknown_fields)]
pub struct ModRequest {
    pub shop_stock: Option<u32>,
    pub vendors: BTreeSet<u32>,
    pub vendor_overrides: BTreeMap<u32, u32>,
    pub vendor_options: BTreeMap<u32, VendorOptions>,
    pub quantity_multiplier: u32,
    pub dropsets: BTreeSet<u32>,
    pub quantity_overrides: BTreeMap<u32, u32>,
    pub trust_multiplier: u32,
    pub daily_refresh: bool,
    pub shop_items: BTreeSet<u32>,
    pub chance_multiplier: u32,
    pub chance_overrides: BTreeMap<u32, u32>,
    pub guaranteed_dropsets: BTreeSet<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub advanced: Option<advanced::AdvancedRequest>,
}
impl Default for ModRequest {
    fn default() -> Self {
        Self {
            shop_stock: None,
            vendors: BTreeSet::new(),
            vendor_overrides: BTreeMap::new(),
            vendor_options: BTreeMap::new(),
            quantity_multiplier: 1,
            dropsets: BTreeSet::new(),
            quantity_overrides: BTreeMap::new(),
            trust_multiplier: 1,
            daily_refresh: false,
            shop_items: BTreeSet::new(),
            chance_multiplier: 1,
            chance_overrides: BTreeMap::new(),
            guaranteed_dropsets: BTreeSet::new(),
            advanced: None,
        }
    }
}
#[derive(Clone, Debug, Default, Deserialize, Serialize)]
#[serde(default, deny_unknown_fields)]
pub struct VendorOptions {
    /// None inherits the global selection; Some(empty) suppresses extra items.
    pub items: Option<BTreeSet<u32>>,
    /// None inherits; false preserves the original interval.
    pub daily_refresh: Option<bool>,
}
impl ModRequest {
    fn items_for(&self, key: u32, selected: bool) -> Option<&BTreeSet<u32>> {
        self.vendor_options
            .get(&key)
            .and_then(|v| v.items.as_ref())
            .or(if selected {
                Some(&self.shop_items)
            } else {
                None
            })
    }
    fn daily_for(&self, key: u32, selected: bool) -> bool {
        self.vendor_options
            .get(&key)
            .and_then(|v| v.daily_refresh)
            .unwrap_or(selected && self.daily_refresh)
    }
}
#[derive(Clone, Serialize)]
pub struct Choice {
    pub key: u32,
    pub name: String,
    pub entries: usize,
}
#[derive(Serialize)]
pub struct ModInfo {
    pub advanced: advanced::AdvancedInfo,
    pub items: Vec<Choice>,
    pub append_vendors: BTreeSet<u32>,
    pub daily_vendors: BTreeSet<u32>,
    pub chance_dropsets: Vec<Choice>,
    pub guarantee_dropsets: Vec<Choice>,
    pub vendors: Vec<Choice>,
    pub dropsets: Vec<Choice>,
    pub store_rows: usize,
    pub stock_rows: usize,
    pub opaque_stores: usize,
    pub opaque_dropsets: usize,
    pub quantity_excluded: usize,
    pub trust_rows: usize,
    pub limitations: Vec<String>,
}
#[derive(Clone, Serialize)]
pub struct Change {
    pub module: &'static str,
    pub table: &'static str,
    pub key: u32,
    pub name: String,
    pub field: String,
    pub before: String,
    pub after: String,
}
#[derive(Serialize)]
pub struct PlannedFile {
    pub path: String,
    pub action: &'static str,
    pub before_sha256: Option<String>,
    pub after_sha256: String,
    pub bytes: usize,
}
#[derive(Serialize)]
pub struct Preview {
    pub request: ModRequest,
    pub fingerprint: String,
    pub plan_id: String,
    pub changes: Vec<Change>,
    pub files: Vec<PlannedFile>,
    pub warnings: Vec<String>,
    pub gates: crate::apply::ApplyStatus,
    pub credits: &'static str,
}
pub struct BuiltMod {
    pub preview: Preview,
    pub files: BTreeMap<String, Vec<u8>>,
}
pub struct ModCatalog {
    advanced: advanced::Catalog,
    blobs: BTreeMap<String, Vec<u8>>,
    registry: Vec<u8>,
    encryption: [u8; 4],
    stores: BTreeMap<u32, Store>,
    drops: BTreeMap<u32, DropRow>,
    trust: BTreeMap<u32, DropRow>,
    fingerprint: String,
    game: std::path::PathBuf,
    store_rows: usize,
    drop_rows: usize,
    items: Vec<Choice>,
}
fn bad(s: impl Into<String>) -> Error {
    Error::Invalid(s.into())
}
fn table<'a>(blobs: &'a BTreeMap<String, Vec<u8>>, name: &str) -> Result<IndexedTable<'a>> {
    Ok(IndexedTable::parse(
        tables::schema(name)
            .or_else(|| advanced::schema(name))
            .ok_or_else(|| bad("unknown table"))?,
        &blobs[&format!("{name}.staticinfobody")],
        &blobs[&format!("{name}.staticinfoheader")],
    )?)
}
impl ModCatalog {
    pub fn advanced_skill(&self, key: u32) -> Result<advanced::skill_detail::SkillDetail> {
        let t = table(&self.blobs, "skill")?;
        advanced::skill_detail::inspect(t.record_bytes(key).ok_or_else(|| bad("unknown skill"))?)
    }
    pub fn advanced_item(&self, key: u32) -> Result<advanced::ItemDetail> {
        self.advanced.item(key)
    }
    pub fn open(data: &GameData) -> Result<Self> {
        let (blobs, registry, encryption) = data.mod_inputs()?;
        let mut stores = BTreeMap::new();
        let mut drops = BTreeMap::new();
        let mut trust = BTreeMap::new();
        let store_table = table(&blobs, "storeinfo")?;
        let store_rows = store_table.records().len();
        for row in store_table.records() {
            if let Some(store) = Store::parse(store_table.record_bytes(row.key).unwrap())? {
                if store.key != row.key {
                    return Err(bad("store index/body mismatch"));
                }
                stores.insert(row.key, store);
            }
        }
        let drop_table = table(&blobs, "dropsetinfo")?;
        let drop_rows = drop_table.records().len();
        for row in drop_table.records() {
            let bytes = drop_table.record_bytes(row.key).unwrap();
            if let Ok(drop) = DropRow::parse_items(bytes) {
                if drop.key != row.key {
                    return Err(bad("dropset index/body mismatch"));
                }
                drops.insert(row.key, drop);
            } else if let Ok(drop) = DropRow::parse_friendly(bytes)
                && drop.key == row.key
                && drop.name.starts_with(b"DropSet_Friendly_")
            {
                trust.insert(row.key, drop);
            }
        }
        // These counts are part of the pinned schema contract, not a best-fit threshold.
        if (
            store_rows,
            stores.len(),
            stores.values().map(|s| s.stocks.len()).sum::<usize>(),
            drop_rows,
            drops.len(),
        ) != (436, 397, 6376, 14747, 13035)
        {
            return Err(bad(format!(
                "mod schema coverage differs: stores {store_rows}/{}, drops {drop_rows}/{}",
                stores.len(),
                drops.len()
            )));
        }
        for name in ["storeinfo", "dropsetinfo"] {
            let t = table(&blobs, name)?;
            let (b, h) = t.rebuild(&BTreeMap::new())?;
            if b != blobs[&format!("{name}.staticinfobody")]
                || h != blobs[&format!("{name}.staticinfoheader")]
            {
                return Err(bad("table pair failed roundtrip"));
            }
        }
        Ok(Self {
            advanced: advanced::Catalog::open(&blobs)?,
            blobs,
            registry,
            encryption,
            stores,
            drops,
            trust,
            fingerprint: data.fingerprint().digest.clone(),
            game: data.game_root().to_owned(),
            store_rows,
            drop_rows,
            items: data
                .localized_items()?
                .into_iter()
                .map(|i| Choice {
                    key: i.key,
                    name: i.name,
                    entries: 0,
                })
                .collect(),
        })
    }
    pub fn info(&self) -> ModInfo {
        ModInfo {
            advanced: self.advanced.info(),
            items: self.items.clone(),
            append_vendors: self.stores.values().filter(|s|s.can_append()).map(|s|s.key).collect(),
            daily_vendors: self.stores.values().filter(|s|s.reset_days().is_some()).map(|s|s.key).collect(),
            chance_dropsets: self.drops.values().filter(|r|chance::eligible(r)).map(drop_choice).collect(),
            guarantee_dropsets: self.drops.values().filter(|r|chance::guarantee_eligible(r)).map(drop_choice).collect(),
            vendors: self.stores.values().map(|s| Choice { key: s.key, name: s.name.clone(), entries: s.stocks.len() }).collect(),
            dropsets: self.drops.values().filter(|d| quantity_eligible(d)).map(|d| Choice { key: d.key, name: String::from_utf8_lossy(&d.name).into_owned(), entries: d.drops.len() }).collect(),
            store_rows: self.store_rows, stock_rows: self.stores.values().map(|s| s.stocks.len()).sum(), opaque_stores: self.store_rows-self.stores.len(), opaque_dropsets: self.drop_rows-self.drops.len()-self.trust.len(), trust_rows: self.trust.len(),
            quantity_excluded: self.drops.values().filter(|d| !quantity_eligible(d)).count(),
            limitations: vec!["Zusätzliche Artikel nur bei Händlern mit gewöhnlicher, bedingungsfreier Angebotsvorlage und eindeutigen Save-Indizes. Bestehende Angebote bleiben erhalten; neue übernehmen die Preisfaktoren der Vorlage. Beliebige Itemtypen und große Sortimente sind noch nicht im Spiel abgenommen.".into(), "Täglicher Refresh ändert belegte 3-/7-Tage-Intervalle auf 1; dauerhafte und unbekannte Intervalle sowie einmalige Artikel bleiben unverändert.".into(), "Chancen skalieren ausschließlich unbedingte Item-Dropsets mit unabhängigem Rolltyp 0, gedeckelt bei 100 %. Begrenzte/gewichtete Rolltypen bleiben unverändert; Bosssets müssen manuell zugeordnet werden.".into(), "Mengen und globale Chancen betreffen auch freigegebene Rezept- und Questbelohnungen. Eine Garantie gilt nur für den einzelnen Eintrag beim Auslösen des Sets; vorgelagerte Ereignisse werden nicht verändert.".into(), "Trust skaliert nur positive Friendly-Mengen; Strafen bleiben erhalten. In-game-Wirkung noch ungeprüft.".into()],
        }
    }
    pub fn build(&self, request: ModRequest) -> Result<BuiltMod> {
        if request.quantity_multiplier > 1000
            || request.chance_multiplier > 1000
            || request.chance_overrides.values().any(|v| *v > 1000)
            || request.trust_multiplier > 1000
            || request.quantity_overrides.values().any(|v| *v > 1000)
            || request.shop_stock.is_some_and(|v| v > 1_000_000)
            || request.vendor_overrides.values().any(|v| *v > 1_000_000)
        {
            return Err(bad(
                "Workbench-Prüfgrenze: Multiplikatoren 0..1000, Shopbestand 0..1000000 (keine behaupteten Engine-Maxima)",
            ));
        }
        for key in request
            .vendors
            .iter()
            .chain(request.vendor_overrides.keys())
            .chain(request.vendor_options.keys())
        {
            if !self.stores.contains_key(key) {
                return Err(bad(format!(
                    "Händler {key} hat kein freigegebenes Stockschema"
                )));
            }
        }
        for key in request
            .dropsets
            .iter()
            .chain(request.quantity_overrides.keys())
        {
            if !self.drops.get(key).is_some_and(quantity_eligible) {
                return Err(bad(format!(
                    "Dropset {key} hat kein freigegebenes Mengenschema"
                )));
            }
        }
        let mut changes = Vec::new();
        let mut feature_warnings = Vec::new();
        let item_keys: BTreeSet<_> = self.items.iter().map(|i| i.key).collect();
        if !request.shop_items.is_subset(&item_keys)
            || request
                .vendor_options
                .values()
                .filter_map(|v| v.items.as_ref())
                .any(|items| !items.is_subset(&item_keys))
        {
            return Err(bad("Artikelauswahl enthält unbekannte Item-IDs"));
        }
        for key in request.chance_overrides.keys() {
            if !self.drops.get(key).is_some_and(chance::eligible) {
                return Err(bad(format!(
                    "Dropset {key} hat kein freigegebenes unabhängiges Chancenschema"
                )));
            }
        }
        if request.chance_multiplier != 1
            && request
                .dropsets
                .iter()
                .any(|k| !self.drops.get(k).is_some_and(chance::eligible))
        {
            return Err(bad(
                "Die Dropsetauswahl enthält begrenzte, gewichtete oder bedingte Sets ohne freigegebene Prozentchance",
            ));
        }
        for key in &request.guaranteed_dropsets {
            if !self.drops.get(key).is_some_and(chance::guarantee_eligible) {
                return Err(bad(format!(
                    "Dropset {key} kann nicht garantiert werden: Rolltyp, Bedingung oder Nullmenge"
                )));
            }
            let multiplier = request.quantity_overrides.get(key).copied().unwrap_or(
                if request.dropsets.is_empty() || request.dropsets.contains(key) {
                    request.quantity_multiplier
                } else {
                    1
                },
            );
            if multiplier == 0 {
                return Err(bad(format!(
                    "Dropset {key}: Eine Garantie ist mit Mengenmultiplikator 0 nicht möglich"
                )));
            }
        }
        let mut combinations = 0usize;
        let mut skipped = 0usize;
        for store in self.stores.values() {
            let selected = request.vendors.is_empty() || request.vendors.contains(&store.key);
            let options = request.vendor_options.get(&store.key);
            if let Some(items) = request.items_for(store.key, selected)
                && !items.is_empty()
            {
                if store.can_append() {
                    combinations = combinations.saturating_add(items.len());
                } else if options.is_some_and(|v| v.items.is_some()) || !request.vendors.is_empty()
                {
                    return Err(bad(format!(
                        "Händler {} hat kein freigegebenes Einfügeschema",
                        store.key
                    )));
                } else {
                    skipped += 1;
                }
            }
            if request.daily_for(store.key, selected)
                && store.reset_days().is_none()
                && (options.is_some_and(|v| v.daily_refresh.is_some())
                    || !request.vendors.is_empty())
            {
                return Err(bad(format!(
                    "Händler {} hat ein dauerhaftes oder unbekanntes Refresh-Intervall",
                    store.key
                )));
            }
        }
        if combinations > 100_000 {
            return Err(bad(
                "Vorschaugrenze: höchstens 100.000 zusätzliche Positionen pro Plan; Händler- oder Artikelauswahl eingrenzen (kein behauptetes Engine-Limit)",
            ));
        }
        if skipped > 0 {
            feature_warnings.push(format!("Zusätzliche Artikel: {skipped} Händler ohne freigegebenes Einfügeschema bleiben beim bisherigen Sortiment."));
        }
        let mut replacements: BTreeMap<&str, BTreeMap<u32, Vec<u8>>> = BTreeMap::new();
        for store in self.stores.values() {
            let selected = request.vendors.is_empty() || request.vendors.contains(&store.key);
            let stock = request
                .vendor_overrides
                .get(&store.key)
                .copied()
                .or(if selected { request.shop_stock } else { None });
            let mut edited = store.clone();
            if let Some(stock) = stock {
                for (i, row) in edited.stocks.iter_mut().enumerate() {
                    if row.count != stock {
                        changes.push(Change {
                            module: "shops",
                            table: "storeinfo",
                            key: store.key,
                            name: store.name.clone(),
                            field: format!("stocks[{i}].stock_count (item {})", row.item),
                            before: row.count.to_string(),
                            after: stock.to_string(),
                        });
                        row.count = stock;
                    }
                }
            }
            if request.daily_for(store.key, selected)
                && let Some(days) = edited.reset_days()
                && days != 1
            {
                edited.set_daily()?;
                changes.push(Change {
                    module: "shops",
                    table: "storeinfo",
                    key: store.key,
                    name: store.name.clone(),
                    field: "reset_days".into(),
                    before: days.to_string(),
                    after: "1".into(),
                });
            }
            if let Some(items) = request.items_for(store.key, selected)
                && !items.is_empty()
                && edited.can_append()
            {
                let before = edited.stocks.len();
                let added = edited.append_items(items, stock.unwrap_or(999))?;
                for (i, item) in added.iter().enumerate() {
                    changes.push(Change {
                        module: "shops",
                        table: "storeinfo",
                        key: store.key,
                        name: store.name.clone(),
                        field: format!("stocks[{}].item", before + i),
                        before: "nicht vorhanden".into(),
                        after: format!("{item} (Bestand {})", stock.unwrap_or(999)),
                    });
                }
                if !added.is_empty() {
                    changes.push(Change {
                        module: "shops",
                        table: "storeinfo",
                        key: store.key,
                        name: store.name.clone(),
                        field: "buyable_stock_count".into(),
                        before: store.offer_count().to_string(),
                        after: edited.offer_count().to_string(),
                    });
                }
            }
            if edited.serialize() == store.serialize() {
                continue;
            }
            if Store::parse(&edited.serialize())?.is_none() {
                return Err(bad(
                    "Geänderter Händler lässt sich nicht vollständig zurücklesen",
                ));
            }
            replacements
                .entry("storeinfo")
                .or_default()
                .insert(store.key, edited.serialize());
        }
        for (key, row) in &self.drops {
            if !quantity_eligible(row) {
                continue;
            }
            let selected = request.dropsets.is_empty() || request.dropsets.contains(key);
            let multiplier = request
                .quantity_overrides
                .get(key)
                .copied()
                .unwrap_or(if selected {
                    request.quantity_multiplier
                } else {
                    1
                });
            let chance_multiplier =
                request
                    .chance_overrides
                    .get(key)
                    .copied()
                    .unwrap_or(if selected {
                        request.chance_multiplier
                    } else {
                        1
                    });
            let guaranteed = request.guaranteed_dropsets.contains(key);
            if multiplier == 1 && (chance_multiplier == 1 || !chance::eligible(row)) && !guaranteed
            {
                continue;
            }
            let mut edited = row.clone();
            if (chance_multiplier != 1 && chance::eligible(row)) || guaranteed {
                for (i, drop) in edited.drops.iter_mut().enumerate() {
                    let after = if guaranteed {
                        chance::SCALE
                    } else {
                        chance::scaled(drop.rate, chance_multiplier)
                    };
                    if after != drop.rate {
                        changes.push(Change {
                            module: "drops",
                            table: "dropsetinfo",
                            key: *key,
                            name: String::from_utf8_lossy(&row.name).into_owned(),
                            field: format!("drops[{i}].chance_per_million"),
                            before: drop.rate.to_string(),
                            after: after.to_string(),
                        });
                        drop.rate = after;
                    }
                }
                let total: u64 = edited.drops.iter().map(|d| d.rate).sum();
                if total != edited.total_rate {
                    changes.push(Change {
                        module: "drops",
                        table: "dropsetinfo",
                        key: *key,
                        name: String::from_utf8_lossy(&row.name).into_owned(),
                        field: "total_drop_rate".into(),
                        before: edited.total_rate.to_string(),
                        after: total.to_string(),
                    });
                    edited.total_rate = total;
                }
            }
            for (i, drop) in edited.drops.iter_mut().enumerate() {
                if drop.min > drop.max || drop.max > i64::MAX as u64 {
                    return Err(bad(format!(
                        "Dropset {key}: Menge/Sentinel nicht freigegeben"
                    )));
                }
                for (field, value) in [("min", &mut drop.min), ("max", &mut drop.max)] {
                    let after = value
                        .checked_mul(u64::from(multiplier))
                        .filter(|n| *n <= i64::MAX as u64)
                        .ok_or_else(|| bad("quantity overflow"))?;
                    if *value != after {
                        changes.push(Change {
                            module: "drops",
                            table: "dropsetinfo",
                            key: *key,
                            name: String::from_utf8_lossy(&row.name).into_owned(),
                            field: format!("drops[{i}].{field}"),
                            before: value.to_string(),
                            after: after.to_string(),
                        });
                        *value = after;
                    }
                }
            }
            DropRow::parse_items(&edited.serialize())?;
            replacements
                .entry("dropsetinfo")
                .or_default()
                .insert(*key, edited.serialize());
        }
        if request.trust_multiplier != 1 {
            for (key, row) in &self.trust {
                let mut edited = row.clone();
                for (i, drop) in edited.drops.iter_mut().enumerate() {
                    for (field, value) in [("min", &mut drop.min), ("max", &mut drop.max)] {
                        let before = *value as i64;
                        if before <= 0 {
                            continue;
                        }
                        let after = before
                            .checked_mul(i64::from(request.trust_multiplier))
                            .ok_or_else(|| bad("trust overflow"))?;
                        changes.push(Change {
                            module: "trust",
                            table: "dropsetinfo",
                            key: *key,
                            name: String::from_utf8_lossy(&row.name).into_owned(),
                            field: format!("friendly[{i}].{field}"),
                            before: before.to_string(),
                            after: after.to_string(),
                        });
                        *value = after as u64;
                    }
                }
                DropRow::parse_friendly(&edited.serialize())?;
                if replacements
                    .entry("dropsetinfo")
                    .or_default()
                    .insert(*key, edited.serialize())
                    .is_some()
                {
                    return Err(bad("conflicting drops/trust record"));
                }
            }
        }
        if let Some(q) = &request.advanced {
            let first = changes.len();
            self.advanced
                .apply(&self.blobs, q, &mut replacements, &mut changes)?;
            let names: BTreeMap<_, _> = self
                .items
                .iter()
                .map(|i| (i.key, i.name.as_str()))
                .collect();
            for change in &mut changes[first..] {
                if change.table == "iteminfo"
                    && let Some(name) = names.get(&change.key)
                {
                    change.name = (*name).into();
                }
            }
            feature_warnings.extend(self.advanced.info().limitations);
        }
        let mut files = BTreeMap::new();
        if !changes.is_empty() {
            let mut payload = BTreeMap::new();
            for (name, rows) in replacements {
                let (body, header) = table(&self.blobs, name)?.rebuild(&rows)?;
                if body != self.blobs[&format!("{name}.staticinfobody")] {
                    payload.insert(format!("{name}.staticinfobody"), body);
                    payload.insert(format!("{name}.staticinfoheader"), header);
                }
            }
            let overlay = crimson_format::overlay::build(&payload, self.encryption)?;
            let archive = match crate::apply::original_registry(&self.game)? {
                Some(bytes) => crimson_format::Archive::with_registry(&self.game, bytes)?,
                None => crimson_format::Archive::open(&self.game)?,
            };
            if archive.serialize_registry()? != self.registry {
                return Err(bad(
                    "Installation seit dem Einlesen geändert; Datenquelle neu einlesen",
                ));
            }
            let group = (1..=9999)
                .map(|n| format!("{n:04}"))
                .find(|n| {
                    !archive.groups().iter().any(|g| &g.name == n) && !self.game.join(n).exists()
                })
                .ok_or_else(|| bad("no free overlay group"))?;
            let language = archive
                .groups()
                .iter()
                .find(|g| g.name == "0008")
                .ok_or_else(|| bad("source group absent"))?
                .language;
            let registry = crimson_format::overlay::register(
                &self.registry,
                &group,
                language,
                overlay.checksum,
            )?;
            files.insert(format!("{group}/0.paz"), overlay.paz);
            files.insert(format!("{group}/0.pamt"), overlay.pamt);
            files.insert("meta/0.papgt".into(), registry);
        }
        let planned: Vec<_> = files
            .iter()
            .map(|(path, b)| PlannedFile {
                path: path.clone(),
                action: if path == "meta/0.papgt" {
                    "replace"
                } else {
                    "create"
                },
                before_sha256: (path == "meta/0.papgt").then(|| hash_bytes(&self.registry)),
                after_sha256: hash_bytes(b),
                bytes: b.len(),
            })
            .collect();
        let plan_id = hash_bytes(&serde_json::to_vec(&(
            &request,
            &planned,
            &self.fingerprint,
        ))?);
        let mut warnings = self.info().limitations;
        warnings.extend(feature_warnings);
        warnings.push(format!("Unverändert: {} opake Händler, {} Sonder-Dropsets, {} leere oder Sentinel-Item-Dropsets.", self.info().opaque_stores, self.info().opaque_dropsets, self.info().quantity_excluded));
        let gates = crate::apply::status(&self.game);
        Ok(BuiltMod {
            preview: Preview {
                request,
                fingerprint: self.fingerprint.clone(),
                plan_id,
                changes,
                files: planned,
                warnings,
                gates,
                credits: CREDITS,
            },
            files,
        })
    }
}
fn quantity_eligible(row: &DropRow) -> bool {
    !row.drops.is_empty()
        && row
            .drops
            .iter()
            .all(|d| d.min <= d.max && d.max <= i64::MAX as u64)
}
fn drop_choice(row: &DropRow) -> Choice {
    Choice {
        key: row.key,
        name: String::from_utf8_lossy(&row.name).into_owned(),
        entries: row.drops.len(),
    }
}
