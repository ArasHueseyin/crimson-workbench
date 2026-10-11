//! Display groups from the independently hash-checked ItemGroupInfo table.
//! Unknown suffix fields remain raw in IndexedTable; there is no table editor.
use crate::{
    Error, Result,
    tables::{IndexedTable, TableSchema},
};
use serde::Serialize;

#[derive(Clone, Debug, Serialize)]
pub struct ItemGroup {
    pub key: u32,
    pub internal_key: String,
    pub name: String,
    pub order: u16,
    pub items: Vec<u32>,
}
pub const SCHEMA: TableSchema = TableSchema {
    name: "itemgroupinfo",
    count_bytes: 2,
    key_bytes: 2,
    interpretation: "display-group-prefix-and-raw-suffix",
};
struct Reader<'a> {
    bytes: &'a [u8],
    at: usize,
}
impl<'a> Reader<'a> {
    fn take(&mut self, n: usize) -> Result<&'a [u8]> {
        let end = self
            .at
            .checked_add(n)
            .filter(|&v| v <= self.bytes.len())
            .ok_or_else(|| Error::Invalid("Truncated ItemGroupInfo record".into()))?;
        let b = &self.bytes[self.at..end];
        self.at = end;
        Ok(b)
    }
    fn byte(&mut self) -> Result<u8> {
        Ok(self.take(1)?[0])
    }
    fn word(&mut self) -> Result<u16> {
        Ok(u16::from_le_bytes(self.take(2)?.try_into().unwrap()))
    }
    fn dword(&mut self) -> Result<u32> {
        Ok(u32::from_le_bytes(self.take(4)?.try_into().unwrap()))
    }
    fn string(&mut self) -> Result<String> {
        let n = self.dword()? as usize;
        if n > 4096 {
            return Err(Error::Invalid("Oversized group text".into()));
        }
        String::from_utf8(self.take(n)?.to_vec()).map_err(|e| Error::Invalid(e.to_string()))
    }
    fn count(&mut self) -> Result<usize> {
        let n = self.dword()? as usize;
        if n > 65536 {
            return Err(Error::Invalid("Oversized group vector".into()));
        }
        Ok(n)
    }
}
pub fn parse(
    body: &[u8],
    header: &[u8],
    resolve: impl Fn(u64, &str) -> String,
) -> Result<Vec<ItemGroup>> {
    let t = IndexedTable::parse(SCHEMA, body, header)?;
    if t.serialize_body() != body || t.serialize_header()? != header {
        return Err(Error::Invalid("ItemGroupInfo roundtrip differs".into()));
    }
    let mut result = Vec::new();
    for row in t.records() {
        let mut r = Reader {
            bytes: t.record_bytes(row.key).unwrap(),
            at: 0,
        };
        if u32::from(r.word()?) != row.key {
            return Err(Error::Invalid("Group key mismatch".into()));
        }
        let internal_key = r.string()?;
        let _locale_category = r.byte()?;
        if r.byte()? != 8 {
            return Err(Error::Invalid(
                "Unknown group localization integer width".into(),
            ));
        }
        let index = u64::from_le_bytes(r.take(8)?.try_into().unwrap());
        let default = r.string()?;
        let children = r.count()?;
        r.take(children * 2)?;
        let n = r.count()?;
        let mut items = Vec::with_capacity(n);
        for _ in 0..n {
            items.push(r.dword()?);
        }
        // An observed byte-vector precedes _orderIndex. Its meaning remains raw.
        let n = r.count()?;
        if n > 16 {
            return Err(Error::Invalid("Unknown group suffix prefix".into()));
        }
        r.take(n)?;
        let order = r.word()?;
        if order == u16::MAX
            || !(order <= 5 || order % 100 == 0)
            || !(internal_key.starts_with("ItemGroup_Category_")
                || internal_key.starts_with("ItemGroup_SubCategory_")
                || internal_key.starts_with("ItemGroup_Collection"))
        {
            continue;
        }
        let name = resolve(index, &default);
        result.push(ItemGroup {
            key: row.key,
            name: if name.is_empty() {
                display_name(order).unwrap_or(&internal_key).into()
            } else {
                name
            },
            internal_key,
            order,
            items,
        });
    }
    result.sort_by_key(|r| (r.order, r.key));
    Ok(result)
}

/// English Workbench fallback labels for observed named groups; not item-category IDs.
fn display_name(order: u16) -> Option<&'static str> {
    Some(match order {
        1 => "Equipment",
        2 => "Food",
        3 => "Materials",
        4 => "Documents",
        5 => "Other",
        1300 => "One-handed weapons",
        1400 => "Shields",
        1500 => "Two-handed weapons",
        1600 => "Ranged weapons",
        1700 => "Daggers",
        1800 => "Tools",
        2000 => "Helmets",
        2100 => "Armor",
        2200 => "Cloaks",
        2300 => "Gloves",
        2400 => "Boots",
        2600 => "Necklaces",
        2700 => "Earrings",
        2800 => "Rings",
        3000 => "Glasses",
        3100 => "Masks",
        3300 => "Backpacks",
        3500 => "Mount equipment",
        3600 => "Animal equipment",
        3700 => "Special vehicles",
        4000 => "Books",
        4100 => "Recipe books",
        4200 => "Blueprints",
        4300 => "Treasure maps",
        4400 => "Documents",
        4500 => "Notices",
        4600 => "Wanted posters",
        5000 => "Dishes",
        5100 => "Potions",
        5200 => "Horse feed",
        6000 => "Ingredients",
        6500 => "Remedy materials",
        6600 => "Crafting materials",
        7000 => "Currencies",
        7100 => "Quest mementos",
        7200 => "Special quest equipment",
        7300 => "Keys",
        7400 => "Sealed artifacts",
        7500 => "Controls",
        7600 => "Abyss equipment",
        7800 => "Ammunition",
        7900 => "Furniture",
        8000 => "Bags",
        8100 => "Collectibles",
        8200 => "Lamps",
        8300 => "Kuku pots and accessories",
        8400 => "Kuku pots",
        8500 => "Bait",
        8600 => "Packed trade goods",
        8700 => "Unpacked trade goods",
        8800 => "Animal items",
        8900 => "Goods",
        _ => return None,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn rejects_truncation_and_oversized_group_strings() {
        let h = [1, 0, 7, 0, 0, 0, 0, 0];
        assert!(parse(&[7, 0, 255, 255, 255, 255], &h, |_, s| s.into()).is_err());
        assert!(parse(&[7], &h, |_, s| s.into()).is_err());
    }
}
