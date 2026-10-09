//! Pinned EXE evidence: roll type 0 compares each entry independently with a
//! freshly sampled integer in 0..1_000_000. Types 1/2 have caps/weighted selection.
//! See docs/research/PHASE4_FIELDS.md; this never changes roll type or conditions.
use crate::crafting::formats::DropRow;
pub const SCALE: u64 = 1_000_000;
pub fn eligible(row: &DropRow) -> bool {
    row.roll == 0
        && row.blocked == 0
        && row.condition.is_empty()
        && row.tag == 0
        && row.weight == 0
        && row.unknown_tail == 0
        && row.tail == 0
        && !row.drops.is_empty()
        && row.drops.iter().all(|d| {
            d.flag == 1
                && d.kind == 0
                && d.item == d.duplicate
                && d.unknown == 0
                && d.raw == [0; 5]
                && d.condition == 0
                && d.post == 0
                && d.rate <= SCALE
                && d.rate2 == 0
                && d.unknown2 == 0
                && d.min <= d.max
                && d.max <= i64::MAX as u64
        })
        && row.drops.iter().map(|d| d.rate).sum::<u64>() == row.total_rate
}
pub fn guarantee_eligible(row: &DropRow) -> bool {
    eligible(row) && row.drops.iter().all(|d| d.min > 0)
}
pub fn scaled(rate: u64, multiplier: u32) -> u64 {
    (u128::from(rate) * u128::from(multiplier)).min(u128::from(SCALE)) as u64
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn scaling_is_exact_and_saturates_at_the_independent_roll_denominator() {
        assert_eq!(scaled(35000, 2), 70000);
        assert_eq!(scaled(700000, 2), 1000000);
        assert_eq!(scaled(1, 0), 0);
        assert_eq!(scaled(0, 1000), 0);
        assert_eq!(scaled(u64::MAX, u32::MAX), 1000000);
    }
}
