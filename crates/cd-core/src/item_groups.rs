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

/// German Workbench labels for observed named groups; not item-category IDs.
fn display_name(order: u16) -> Option<&'static str> {
    Some(match order {
        1 => "Ausrüstung",
        2 => "Nahrung",
        3 => "Materialien",
        4 => "Dokumente",
        5 => "Sonstiges",
        1300 => "Einhandwaffen",
        1400 => "Schilde",
        1500 => "Zweihandwaffen",
        1600 => "Fernkampfwaffen",
        1700 => "Dolche",
        1800 => "Werkzeuge",
        2000 => "Helme",
        2100 => "Rüstungen",
        2200 => "Umhänge",
        2300 => "Handschuhe",
        2400 => "Stiefel",
        2600 => "Halsketten",
        2700 => "Ohrringe",
        2800 => "Ringe",
        3000 => "Brillen",
        3100 => "Masken",
        3300 => "Rucksäcke",
        3500 => "Reittierausrüstung",
        3600 => "Tierausrüstung",
        3700 => "Besondere Fahrzeuge",
        4000 => "Bücher",
        4100 => "Rezeptbücher",
        4200 => "Bauanleitungen",
        4300 => "Schatzkarten",
        4400 => "Schriftstücke",
        4500 => "Aushänge",
        4600 => "Steckbriefe",
        5000 => "Gerichte",
        5100 => "Tränke",
        5200 => "Pferdefutter",
        6000 => "Zutaten",
        6500 => "Heilmittel-Materialien",
        6600 => "Herstellungsmaterialien",
        7000 => "Währungen",
        7100 => "Quest-Erinnerungen",
        7200 => "Besondere Quest-Ausrüstung",
        7300 => "Schlüssel",
        7400 => "Versiegelte Artefakte",
        7500 => "Steuerung",
        7600 => "Abyss-Ausrüstung",
        7800 => "Munition",
        7900 => "Einrichtung",
        8000 => "Beutel",
        8100 => "Sammlerstücke",
        8200 => "Lampen",
        8300 => "Kuku-Töpfe und Zubehör",
        8400 => "Kuku-Töpfe",
        8500 => "Köder",
        8600 => "Verpackte Handelswaren",
        8700 => "Offene Handelswaren",
        8800 => "Tier-Gegenstände",
        8900 => "Waren",
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
