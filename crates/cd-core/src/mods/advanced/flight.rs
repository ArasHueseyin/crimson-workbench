//! One pinned town-flight rule. No generic condition editor or executable patch.
use super::{Record, bad, reader::Reader};
use crate::Result;

const KEY: u32 = 1011130;
const EXPRESSION: &str = "IsInTown() && !IsAboveRoad(Bird,20)";
// ContentsLogicFunction: AND, predicate 131 with its u8 payload, NOT,
// predicate 132 with road type Bird, f32 radius 20, and its u8 payload.
const TREE: &[u8] = &[1, 3, 131, 0, 0, 2, 3, 132, 0, 2, 0, 0, 160, 65, 0];
// NOT + ConditionData_CheckNone (predicate 2, no payload). CheckNone returns
// Yes (0); Not maps that result to No (1), preserving the engine's tri-state.
const FALSE_TREE: &[u8] = &[2, 3, 2, 0];
const FALSE_EXPRESSION: &str = "!CheckNone()";

fn read(bytes: &[u8], disabled: bool) -> Result<(Record, usize, [u8; 3])> {
    let mut r = Reader::new(bytes);
    let (key, name) = r.head(4)?;
    if key != KEY || name != format!("{EXPRESSION}, parserType:0") || bytes[r.at - 1] != 0 {
        return Err(bad("unexpected town-flight rule identity"));
    }
    let start = r.at;
    let tree = if disabled { FALSE_TREE } else { TREE };
    if r.take(tree.len())? != tree {
        return Err(bad("unexpected town-flight condition tree"));
    }
    let flags: [u8; 3] = r.take(3)?.try_into().unwrap();
    if flags != [0, 0, 1] {
        return Err(bad("unexpected town-flight condition flags"));
    }
    if r.text()?
        != if disabled {
            FALSE_EXPRESSION
        } else {
            EXPRESSION
        }
        || r.num(1)? != 0
    {
        return Err(bad("unexpected town-flight expression or parser"));
    }
    r.end()?;
    let mut row = Record::new("conditioninfo", key, name, "world", vec![]);
    row.town_dismount = true;
    row.description="Gemeinsame Stadtflug-Regel für Flugreittiere. Wird durch die Stadt- oder Drachen-Regionsoption aufgehoben; andere Stadtbedingungen bleiben erhalten.".into();
    Ok((row, start, flags))
}
pub(super) fn inspect(bytes: &[u8]) -> Result<Record> {
    Ok(read(bytes, false)?.0)
}
pub(super) fn disable(bytes: &[u8]) -> Result<Vec<u8>> {
    let (_, start, flags) = read(bytes, false)?;
    let mut out = bytes[..start].to_vec();
    out.extend_from_slice(FALSE_TREE);
    out.extend_from_slice(&flags);
    out.extend_from_slice(&(FALSE_EXPRESSION.len() as u32).to_le_bytes());
    out.extend_from_slice(FALSE_EXPRESSION.as_bytes());
    out.push(0);
    read(&out, true)?;
    Ok(out)
}

#[cfg(test)]
mod tests {
    use super::*;
    fn fixture() -> Vec<u8> {
        let name = format!("{EXPRESSION}, parserType:0");
        let mut out = KEY.to_le_bytes().to_vec();
        out.extend((name.len() as u32).to_le_bytes());
        out.extend(name.as_bytes());
        out.push(0);
        out.extend(TREE);
        out.extend([0, 0, 1]);
        out.extend((EXPRESSION.len() as u32).to_le_bytes());
        out.extend(EXPRESSION.as_bytes());
        out.push(0);
        out
    }
    #[test]
    fn town_rule_changes_only_the_condition_and_preserves_identity_flags_and_parser() {
        let b = fixture();
        let (_, start, flags) = read(&b, false).unwrap();
        let out = disable(&b).unwrap();
        let (row, new_start, new_flags) = read(&out, true).unwrap();
        assert_eq!(row.key, KEY);
        assert_eq!(start, new_start);
        assert_eq!(flags, new_flags);
        assert_eq!(&b[..start], &out[..start]);
        assert_eq!(&out[start..start + 4], FALSE_TREE);
        // Already modified or foreign conditions must never become baselines.
        assert!(disable(&out).is_err());
        for end in 0..b.len() {
            assert!(inspect(&b[..end]).is_err());
        }
        for pos in [
            0,
            start,
            start + 4,
            start + 10,
            start + TREE.len(),
            b.len() - 1,
        ] {
            let mut bad = b.clone();
            bad[pos] ^= 1;
            assert!(disable(&bad).is_err());
        }
    }
}
