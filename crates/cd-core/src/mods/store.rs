//! Build 25381195 stock list shape. Unknown bytes are retained, never named.
use crate::{Error, Result};
#[derive(Clone)]
pub(super) struct Stock {
    pub item: u32,
    pub count: u32,
    before_count: Vec<u8>,
    after_count: Vec<u8>,
}
#[derive(Clone)]
pub(super) struct Store {
    pub key: u32,
    pub name: String,
    pub stocks: Vec<Stock>,
    prefix: Vec<u8>,
    suffix: Vec<u8>,
}
fn bad() -> Error {
    Error::Invalid("Unrecognized stock list for pinned store schema".into())
}
fn n32(b: &[u8], p: usize) -> Result<u32> {
    Ok(u32::from_le_bytes(
        b.get(p..p + 4).ok_or_else(bad)?.try_into().unwrap(),
    ))
}
fn n16(b: &[u8], p: usize) -> Result<u16> {
    Ok(u16::from_le_bytes(
        b.get(p..p + 2).ok_or_else(bad)?.try_into().unwrap(),
    ))
}
impl Stock {
    fn bytes(&self) -> Vec<u8> {
        let mut bytes = Vec::new();
        self.serialize(&mut bytes);
        bytes
    }
    fn ordinary_offer(&self) -> bool {
        let b = self.bytes();
        // An existing unconditional, replenishable item offer. Preserve its
        // price factors and opaque item-value interior when cloning it.
        b.len() == 127 && b[38..42] == [1, 0, 1, 0] && b[51..55] == [0; 4] && b[110..127] == [0; 17]
    }
    fn parse(b: &[u8], p: &mut usize, owner: u16) -> Result<Self> {
        let start = *p;
        if n16(b, start)? != owner || b.get(start + 42) != Some(&1) {
            return Err(bad());
        }
        let item = n32(b, start + 43)?;
        if n32(b, start + 102)? != item {
            return Err(bad());
        }
        let count = n32(b, start + 18)?;
        let flag = *b.get(start + 114).ok_or_else(bad)?;
        if flag > 1 {
            return Err(bad());
        }
        let count_offset = start + 115 + 13 * usize::from(flag) + 8;
        let effects = n32(b, count_offset)? as usize;
        if effects > 4096 {
            return Err(bad());
        }
        let end = count_offset + 4 + effects * 12;
        let after_count = b.get(start + 22..end).ok_or_else(bad)?.to_vec();
        *p = end;
        Ok(Self {
            item,
            count,
            before_count: b[start..start + 18].to_vec(),
            after_count,
        })
    }
    fn serialize(&self, out: &mut Vec<u8>) {
        out.extend(&self.before_count);
        out.extend(self.count.to_le_bytes());
        out.extend(&self.after_count);
    }
}
impl Store {
    pub fn offer_count(&self) -> u32 {
        n32(&self.prefix, self.prefix.len() - 9).unwrap()
    }
    pub fn reset_days(&self) -> Option<u32> {
        let days = n32(&self.prefix, self.prefix.len().checked_sub(13)?).ok()?;
        [1, 3, 7].contains(&days).then_some(days)
    }
    pub fn set_daily(&mut self) -> Result<()> {
        if self.reset_days().is_none() {
            return Err(bad());
        }
        let offset = self.prefix.len() - 13;
        self.prefix[offset..offset + 4].copy_from_slice(&1u32.to_le_bytes());
        Ok(())
    }
    pub fn can_append(&self) -> bool {
        let ids: std::collections::BTreeSet<_> = self
            .stocks
            .iter()
            .map(|s| n32(&s.bytes(), 30).unwrap())
            .collect();
        self.stocks.iter().any(Stock::ordinary_offer)
            && ids.len() == self.stocks.len()
            && self
                .stocks
                .iter()
                .enumerate()
                .all(|(i, s)| n32(&s.bytes(), 26).ok() == Some(i as u32))
            && self.counts_match()
    }
    fn counts_match(&self) -> bool {
        let n = self.prefix.len();
        n32(&self.prefix, n - 9).ok()
            == Some(self.stocks.iter().map(|s| u32::from(s.bytes()[40])).sum())
            && n32(&self.prefix, n - 5).ok()
                == Some(self.stocks.iter().map(|s| u32::from(s.bytes()[39])).sum())
    }
    /// Append to a supported merchant, preserving existing indices and conditions.
    /// Returns added item keys; an existing item offer is never duplicated.
    pub fn append_items(
        &mut self,
        items: &std::collections::BTreeSet<u32>,
        count: u32,
    ) -> Result<Vec<u32>> {
        if !self.can_append() {
            return Err(bad());
        }
        let template = self
            .stocks
            .iter()
            .find(|s| s.ordinary_offer())
            .unwrap()
            .bytes();
        let mut next_save = self
            .stocks
            .iter()
            .map(|s| n32(&s.bytes(), 30).unwrap())
            .max()
            .unwrap()
            .checked_add(1)
            .ok_or_else(bad)?;
        let existing: std::collections::BTreeSet<_> = self
            .stocks
            .iter()
            .filter(|s| matches!(n32(&s.bytes(), 51), Ok(0)) && s.bytes()[40] == 1)
            .map(|s| s.item)
            .collect();
        let mut added = Vec::new();
        for &item in items.difference(&existing) {
            if self.stocks.len() >= 9999 {
                return Err(bad());
            }
            let mut b = template.clone();
            b[18..22].copy_from_slice(&count.to_le_bytes());
            b[26..30].copy_from_slice(&(self.stocks.len() as u32).to_le_bytes());
            b[30..34].copy_from_slice(&next_save.to_le_bytes());
            b[43..47].copy_from_slice(&item.to_le_bytes());
            b[102..106].copy_from_slice(&item.to_le_bytes());
            let stock = Stock::parse(&b, &mut 0, self.key as u16)?;
            self.stocks.push(stock);
            next_save = next_save.checked_add(1).ok_or_else(bad)?;
            added.push(item);
        }
        let at = self.prefix.len() - 9;
        let offers = n32(&self.prefix, at)?
            .checked_add(added.len() as u32)
            .ok_or_else(bad)?;
        self.prefix[at..at + 4].copy_from_slice(&offers.to_le_bytes());
        if !self.counts_match() {
            return Err(bad());
        }
        Ok(added)
    }
    pub fn parse(bytes: &[u8]) -> Result<Option<Self>> {
        let key = n16(bytes, 0)?;
        let name_end = 6 + n32(bytes, 2)? as usize;
        let name = std::str::from_utf8(bytes.get(6..name_end).ok_or_else(bad)?)
            .map_err(|_| bad())?
            .to_owned();
        let mut candidates = Vec::new();
        // Four measured positions, never best-fit or legacy layout guessing.
        for delta in [47, 95, 99, 115] {
            let offset = name_end + delta;
            let Ok(count) = n32(bytes, offset) else {
                continue;
            };
            if count == 0 || count >= 10000 {
                continue;
            }
            let mut p = offset + 4;
            let stocks: Result<Vec<_>> = (0..count)
                .map(|_| Stock::parse(bytes, &mut p, key))
                .collect();
            let Ok(stocks) = stocks else {
                continue;
            };
            let value = Self {
                key: key.into(),
                name: name.clone(),
                stocks,
                prefix: bytes[..offset].to_vec(),
                suffix: bytes[p..].to_vec(),
            };
            if value.serialize() != bytes {
                return Err(bad());
            }
            candidates.push(value);
        }
        match candidates.len() {
            0 => Ok(None),
            1 => Ok(candidates.pop()),
            _ => Err(bad()),
        }
    }
    pub fn serialize(&self) -> Vec<u8> {
        let mut out = self.prefix.clone();
        out.extend((self.stocks.len() as u32).to_le_bytes());
        for stock in &self.stocks {
            stock.serialize(&mut out);
        }
        out.extend(&self.suffix);
        out
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn ordinary_store() -> Store {
        let mut bytes = 71u16.to_le_bytes().to_vec();
        bytes.extend(15u32.to_le_bytes());
        bytes.extend(b"Store_Synthetic");
        let mut prefix = vec![0; 47];
        prefix[34..38].copy_from_slice(&7u32.to_le_bytes());
        prefix[38..42].copy_from_slice(&2u32.to_le_bytes());
        bytes.extend(prefix);
        bytes.extend(2u32.to_le_bytes());
        for i in 0..2u32 {
            let mut row = vec![0; 127];
            row[..2].copy_from_slice(&71u16.to_le_bytes());
            row[2..10].copy_from_slice(&1000000u64.to_le_bytes());
            row[10..18].copy_from_slice(&1250000u64.to_le_bytes());
            row[18..22].copy_from_slice(&5u32.to_le_bytes());
            row[26..30].copy_from_slice(&i.to_le_bytes());
            row[30..34].copy_from_slice(&(10 + i * 2).to_le_bytes());
            row[34..38].fill(0xff);
            row[38] = 1;
            row[40] = 1;
            row[42] = 1;
            row[43..47].copy_from_slice(&(42 + i).to_le_bytes());
            row[100..102].fill(0xff);
            row[102..106].copy_from_slice(&(42 + i).to_le_bytes());
            bytes.extend(row);
        }
        bytes.extend([0xa5; 17]);
        Store::parse(&bytes).unwrap().unwrap()
    }
    #[test]
    fn new_offers_preserve_original_rows_conditions_prices_and_save_indices() {
        let mut store = ordinary_store();
        let original = store.clone();
        assert!(store.can_append());
        assert_eq!(
            store.append_items(&[42, 50, 51].into(), 999).unwrap(),
            [50, 51]
        );
        assert_eq!(store.offer_count(), 4);
        assert_eq!(store.serialize().len(), original.serialize().len() + 254);
        for i in 0..2 {
            assert_eq!(store.stocks[i].bytes(), original.stocks[i].bytes());
        }
        for (i, s) in store.stocks[2..].iter().enumerate() {
            let b = s.bytes();
            assert_eq!(&b[2..18], &original.stocks[0].bytes()[2..18]);
            assert_eq!(n32(&b, 26).unwrap(), i as u32 + 2);
            assert_eq!(n32(&b, 30).unwrap(), i as u32 + 13);
            assert_eq!(n32(&b, 43).unwrap(), i as u32 + 50);
            assert_eq!(n32(&b, 102).unwrap(), i as u32 + 50);
            assert_eq!(s.count, 999);
        }
        assert_eq!(
            Store::parse(&store.serialize())
                .unwrap()
                .unwrap()
                .serialize(),
            store.serialize()
        );
        assert_eq!(store.suffix, original.suffix);
        let stable = store.serialize();
        assert!(
            store
                .append_items(&[42, 50, 51].into(), 1)
                .unwrap()
                .is_empty()
        );
        assert_eq!(stable, store.serialize());
        let mut duplicate = ordinary_store();
        duplicate.stocks[1].after_count[8..12].copy_from_slice(&10u32.to_le_bytes());
        assert!(!duplicate.can_append());
        let mut reordered = ordinary_store();
        reordered.stocks.swap(0, 1);
        assert!(!reordered.can_append());
    }
    #[test]
    fn buyback_only_item_does_not_block_a_new_sale_offer() {
        let mut store = ordinary_store();
        // Convert the second synthetic item to a sell-to-vendor-only position.
        store.stocks[1].after_count[17] = 1;
        store.stocks[1].after_count[18] = 0;
        let n = store.prefix.len();
        store.prefix[n - 9..n - 5].copy_from_slice(&1u32.to_le_bytes());
        store.prefix[n - 5..n - 1].copy_from_slice(&1u32.to_le_bytes());
        let old = store.stocks[1].bytes();
        assert_eq!(store.append_items(&[43].into(), 999).unwrap(), [43]);
        assert_eq!(store.stocks[1].bytes(), old);
        assert_eq!(store.stocks[2].bytes()[39..41], [0, 1]);
        assert_eq!(store.offer_count(), 2);
    }
    #[test]
    fn daily_refresh_changes_only_reset_day_and_refuses_permanent_intervals() {
        let mut store = ordinary_store();
        let before = store.serialize();
        store.set_daily().unwrap();
        let after = store.serialize();
        let at = store.prefix.len() - 13;
        assert_eq!(&before[..at], &after[..at]);
        assert_eq!(&before[at + 4..], &after[at + 4..]);
        assert_eq!(store.reset_days(), Some(1));
        store.prefix[at..at + 4].fill(0xff);
        let permanent = store.serialize();
        assert!(store.set_daily().is_err());
        assert_eq!(permanent, store.serialize());
    }
    #[test]
    fn stock_edit_preserves_september_gap_optional_data_effects_and_outer_unknowns() {
        let mut bytes = 71u16.to_le_bytes().to_vec();
        bytes.extend(15u32.to_le_bytes());
        bytes.extend(b"Store_Synthetic");
        bytes.extend([0xa5; 47]);
        bytes.extend(1u32.to_le_bytes());
        let start = bytes.len();
        let mut head = vec![0; 114];
        head[..2].copy_from_slice(&71u16.to_le_bytes());
        head[18..22].copy_from_slice(&5u32.to_le_bytes());
        head[42] = 1;
        head[43..47].copy_from_slice(&42u32.to_le_bytes());
        head[102..106].copy_from_slice(&42u32.to_le_bytes());
        bytes.extend(head);
        bytes.push(1);
        bytes.extend([0x91; 13]);
        bytes.extend([0x82; 8]);
        bytes.extend(2u32.to_le_bytes());
        bytes.extend([0x73; 24]);
        bytes.extend([0x64; 17]);
        let mut parsed = Store::parse(&bytes).unwrap().unwrap();
        assert_eq!(parsed.serialize(), bytes);
        parsed.stocks[0].count = 999;
        let mut expected = bytes;
        expected[start + 18..start + 22].copy_from_slice(&999u32.to_le_bytes());
        assert_eq!(parsed.serialize(), expected);
        assert!(Store::parse(&expected[..12]).is_err());
    }
}
