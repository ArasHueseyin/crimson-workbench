//! Original, build-pinned readers. Layout evidence: docs/CRAFTING.md.
//! Every interpreted row is independently serialized and byte-compared.
use crate::{Error, Result};

#[derive(Clone, Debug)]
pub struct Text(pub u8, pub u64, pub Vec<u8>);
#[derive(Clone, Debug)]
pub struct Fixed {
    pub item: u32,
    pub character: u32,
    pub gimmick: u32,
    pub count: u64,
    pub coupon: u64,
    pub enchant: u16,
}
#[derive(Clone, Debug)]
pub struct GroupIngredient {
    pub group: u16,
    pub count: u64,
    pub unknown: u16,
}
#[derive(Clone, Debug)]
pub struct RecipeRow {
    pub key: u32,
    pub name: Vec<u8>,
    pub blocked: u8,
    pub tool: u16,
    pub consume: u8,
    pub conditions: Vec<(u32, Text)>,
    pub knowledge: u32,
    pub tag: Vec<u8>,
    pub flags: [u8; 5],
    pub fixed: Vec<Fixed>,
    pub groups: Vec<GroupIngredient>,
    pub elemental: u32,
    pub states: Vec<u32>,
    pub description: Text,
    pub name_text: Text,
    pub strings: [u32; 3],
    pub complete: Text,
    pub results: Vec<u32>,
    pub additional: Vec<u32>,
}
#[derive(Clone, Debug)]
pub struct GroupRow {
    pub key: u16,
    pub name: Vec<u8>,
    pub blocked: u8,
    pub text: Text,
    pub groups: Vec<u16>,
    pub items: Vec<u32>,
    pub unknown_types: Vec<u8>,
    pub tail: [u8; 11],
}
#[derive(Clone, Debug)]
pub struct Drop {
    pub flag: u8,
    pub item: u32,
    pub unknown: u32,
    pub kind: u32,
    pub raw: [u8; 5],
    pub condition: u32,
    pub post: u32,
    pub rate: u64,
    pub rate2: u32,
    pub unknown2: u32,
    pub max: u64,
    pub min: u64,
    pub enchant: u16,
    pub duplicate: u32,
    pub friendly_raw: Vec<u8>,
}
#[derive(Clone, Debug)]
pub struct DropRow {
    pub key: u32,
    pub name: Vec<u8>,
    pub blocked: u8,
    pub roll: u8,
    pub rolls: u32,
    pub condition: Vec<u8>,
    pub tag: u32,
    pub drops: Vec<Drop>,
    pub slots: u16,
    pub weight: u64,
    pub total_rate: u64,
    pub unknown_tail: u64,
    pub original: Vec<u8>,
    pub tail: u8,
}

fn bad(s: &str) -> Error {
    Error::Invalid(format!("Craft table: {s}"))
}
struct Reader<'a> {
    bytes: &'a [u8],
    at: usize,
}
impl<'a> Reader<'a> {
    fn take<const N: usize>(&mut self) -> Result<[u8; N]> {
        let b = self
            .bytes
            .get(self.at..self.at + N)
            .ok_or_else(|| bad("truncated record"))?;
        self.at += N;
        Ok(b.try_into().unwrap())
    }
    fn u8(&mut self) -> Result<u8> {
        Ok(self.take::<1>()?[0])
    }
    fn u16(&mut self) -> Result<u16> {
        Ok(u16::from_le_bytes(self.take()?))
    }
    fn u32(&mut self) -> Result<u32> {
        Ok(u32::from_le_bytes(self.take()?))
    }
    fn u64(&mut self) -> Result<u64> {
        Ok(u64::from_le_bytes(self.take()?))
    }
    fn bytes(&mut self) -> Result<Vec<u8>> {
        let n = self.u32()? as usize;
        if n > 65_536 {
            return Err(bad("string too long"));
        }
        let b = self
            .bytes
            .get(self.at..self.at + n)
            .ok_or_else(|| bad("truncated string"))?;
        self.at += n;
        Ok(b.to_vec())
    }
    fn text(&mut self) -> Result<Text> {
        Ok(Text(self.u8()?, self.u64()?, self.bytes()?))
    }
    fn array<T>(&mut self, mut f: impl FnMut(&mut Self) -> Result<T>) -> Result<Vec<T>> {
        let n = self.u32()? as usize;
        if n > 16_384 || n > self.bytes.len().saturating_sub(self.at) {
            return Err(bad("array exceeds record"));
        }
        (0..n).map(|_| f(self)).collect()
    }
    fn finish(&self, serialized: &[u8]) -> Result<()> {
        if self.at != self.bytes.len() || serialized != self.bytes {
            return Err(bad("record did not roundtrip completely"));
        }
        Ok(())
    }
}
#[derive(Default)]
struct Writer(Vec<u8>);
impl Writer {
    fn u8(&mut self, v: u8) {
        self.0.push(v)
    }
    fn u16(&mut self, v: u16) {
        self.0.extend(v.to_le_bytes())
    }
    fn u32(&mut self, v: u32) {
        self.0.extend(v.to_le_bytes())
    }
    fn u64(&mut self, v: u64) {
        self.0.extend(v.to_le_bytes())
    }
    fn bytes(&mut self, v: &[u8]) {
        self.u32(v.len() as u32);
        self.0.extend(v)
    }
    fn text(&mut self, v: &Text) {
        self.u8(v.0);
        self.u64(v.1);
        self.bytes(&v.2)
    }
    fn array<T>(&mut self, v: &[T], mut f: impl FnMut(&mut Self, &T)) {
        self.u32(v.len() as u32);
        for t in v {
            f(self, t)
        }
    }
}
impl RecipeRow {
    pub fn parse(bytes: &[u8]) -> Result<Self> {
        let mut r = Reader { bytes, at: 0 };
        let v = Self {
            key: r.u32()?,
            name: r.bytes()?,
            blocked: r.u8()?,
            tool: r.u16()?,
            consume: r.u8()?,
            conditions: r.array(|r| Ok((r.u32()?, r.text()?)))?,
            knowledge: r.u32()?,
            tag: r.bytes()?,
            flags: r.take()?,
            fixed: r.array(|r| {
                Ok(Fixed {
                    item: r.u32()?,
                    character: r.u32()?,
                    gimmick: r.u32()?,
                    count: r.u64()?,
                    coupon: r.u64()?,
                    enchant: r.u16()?,
                })
            })?,
            groups: r.array(|r| {
                Ok(GroupIngredient {
                    group: r.u16()?,
                    count: r.u64()?,
                    unknown: r.u16()?,
                })
            })?,
            elemental: r.u32()?,
            states: r.array(|r| r.u32())?,
            description: r.text()?,
            name_text: r.text()?,
            strings: [r.u32()?, r.u32()?, r.u32()?],
            complete: r.text()?,
            results: r.array(|r| r.u32())?,
            additional: r.array(|r| r.u32())?,
        };
        r.finish(&v.serialize())?;
        Ok(v)
    }
    pub fn serialize(&self) -> Vec<u8> {
        let mut w = Writer::default();
        w.u32(self.key);
        w.bytes(&self.name);
        w.u8(self.blocked);
        w.u16(self.tool);
        w.u8(self.consume);
        w.array(&self.conditions, |w, (k, t)| {
            w.u32(*k);
            w.text(t)
        });
        w.u32(self.knowledge);
        w.bytes(&self.tag);
        w.0.extend(self.flags);
        w.array(&self.fixed, |w, v| {
            w.u32(v.item);
            w.u32(v.character);
            w.u32(v.gimmick);
            w.u64(v.count);
            w.u64(v.coupon);
            w.u16(v.enchant)
        });
        w.array(&self.groups, |w, v| {
            w.u16(v.group);
            w.u64(v.count);
            w.u16(v.unknown)
        });
        w.u32(self.elemental);
        w.array(&self.states, |w, v| w.u32(*v));
        w.text(&self.description);
        w.text(&self.name_text);
        for k in self.strings {
            w.u32(k)
        }
        w.text(&self.complete);
        w.array(&self.results, |w, v| w.u32(*v));
        w.array(&self.additional, |w, v| w.u32(*v));
        w.0
    }
}
impl GroupRow {
    pub fn parse(bytes: &[u8]) -> Result<Self> {
        let mut r = Reader { bytes, at: 0 };
        let v = Self {
            key: r.u16()?,
            name: r.bytes()?,
            blocked: r.u8()?,
            text: r.text()?,
            groups: r.array(|r| r.u16())?,
            items: r.array(|r| r.u32())?,
            unknown_types: r.array(|r| r.u8())?,
            tail: r.take()?,
        };
        let mut w = Writer::default();
        w.u16(v.key);
        w.bytes(&v.name);
        w.u8(v.blocked);
        w.text(&v.text);
        w.array(&v.groups, |w, v| w.u16(*v));
        w.array(&v.items, |w, v| w.u32(*v));
        w.array(&v.unknown_types, |w, v| w.u8(*v));
        w.0.extend(v.tail);
        r.finish(&w.0)?;
        Ok(v)
    }
}
impl DropRow {
    /// Only the item variant is interpreted. Other variants remain opaque records.
    pub fn parse_items(bytes: &[u8]) -> Result<Self> {
        Self::parse_variant(bytes, 0)
    }
    pub fn parse_friendly(bytes: &[u8]) -> Result<Self> {
        Self::parse_variant(bytes, 7)
    }
    fn parse_variant(bytes: &[u8], expected_kind: u32) -> Result<Self> {
        let mut r = Reader { bytes, at: 0 };
        let key = r.u32()?;
        let name = r.bytes()?;
        let blocked = r.u8()?;
        let roll = r.u8()?;
        let rolls = r.u32()?;
        let condition = r.bytes()?;
        let tag = r.u32()?;
        let drops = r.array(|r| {
            let flag = r.u8()?;
            let item = r.u32()?;
            let unknown = r.u32()?;
            let kind = r.u32()?;
            if flag != 1 || kind != expected_kind {
                return Err(bad("unsupported drop variant"));
            }
            Ok(Drop {
                flag,
                item,
                unknown,
                kind,
                raw: r.take()?,
                condition: r.u32()?,
                post: r.u32()?,
                rate: r.u64()?,
                rate2: r.u32()?,
                unknown2: r.u32()?,
                max: r.u64()?,
                min: r.u64()?,
                enchant: r.u16()?,
                duplicate: r.u32()?,
                friendly_raw: if kind == 7 {
                    r.take::<28>()?.to_vec()
                } else {
                    Vec::new()
                },
            })
        })?;
        let v = Self {
            key,
            name,
            blocked,
            roll,
            rolls,
            condition,
            tag,
            drops,
            slots: r.u16()?,
            weight: r.u64()?,
            total_rate: r.u64()?,
            unknown_tail: r.u64()?,
            original: r.bytes()?,
            tail: r.u8()?,
        };
        r.finish(&v.serialize())?;
        Ok(v)
    }
    pub fn serialize(&self) -> Vec<u8> {
        let v = self;
        let mut w = Writer::default();
        w.u32(v.key);
        w.bytes(&v.name);
        w.u8(v.blocked);
        w.u8(v.roll);
        w.u32(v.rolls);
        w.bytes(&v.condition);
        w.u32(v.tag);
        w.array(&v.drops, |w, d| {
            w.u8(d.flag);
            w.u32(d.item);
            w.u32(d.unknown);
            w.u32(d.kind);
            w.0.extend(d.raw);
            w.u32(d.condition);
            w.u32(d.post);
            w.u64(d.rate);
            w.u32(d.rate2);
            w.u32(d.unknown2);
            w.u64(d.max);
            w.u64(d.min);
            w.u16(d.enchant);
            w.u32(d.duplicate);
            w.0.extend(&d.friendly_raw)
        });
        w.u16(v.slots);
        w.u64(v.weight);
        w.u64(v.total_rate);
        w.u64(v.unknown_tail);
        w.bytes(&v.original);
        w.u8(v.tail);
        w.0
    }
    pub fn deterministic(&self) -> Option<(u32, u64)> {
        if self.blocked != 0
            || !self.condition.is_empty()
            || self.tag != 0
            || self.drops.len() != 1
            || self.unknown_tail != 0
            || self.tail != 0
            || self.weight != 0
        {
            return None;
        }
        if !((self.roll == 2 && self.rolls == 1) || (self.roll == 0 && self.rolls <= 1)) {
            return None;
        }
        let d = &self.drops[0];
        if d.item != d.duplicate
            || d.unknown != 0
            || d.raw != [0; 5]
            || d.condition != 0
            || d.post != 0
            || d.rate != 1_000_000
            || d.rate2 != 0
            || d.unknown2 != 0
            || d.min == 0
            || d.min != d.max
            || ![0, u16::MAX].contains(&d.enchant)
            || self.total_rate != 1_000_000
        {
            return None;
        }
        Some((d.item, d.min))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn truncated_or_excessive_inputs_are_rejected() {
        assert!(RecipeRow::parse(&[0; 12]).is_err());
        assert!(GroupRow::parse(&[0xff; 16]).is_err());
        assert!(DropRow::parse_items(&[0; 80]).is_err());
    }
}
