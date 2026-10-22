//! Player riding interactions from the build-pinned CharacterInfo reader.
//! A vehicle family alone also matches corpses, calves and NPC-only animals.
use super::{Result, bad};
use crate::mods::advanced::reader::Reader;

fn skills(r: &mut Reader<'_>) -> Result<()> {
    let count = r.count()?;
    if count > 512 {
        return Err(bad("Unexpected mount skill list."));
    }
    r.take(count * 8)?; // SkillInfo key and level, both serialized as u32.
    Ok(())
}

fn flags(r: &mut Reader<'_>) -> Result<()> {
    r.take(6)?; // AllyGroupInfo u32, DetectInfo u16.
    skills(r)?;
    r.take(4)?; // Invincibility, attackable, aggro-targetable, valid.
    Ok(())
}

fn player_interaction(vehicle: u16, interactions: &[u32]) -> bool {
    let ride: &[u32] = match vehicle {
        16960 => &[400, 404], // Horse_Ride and WildHorse_Ride.
        16962 => &[1_000_101],
        16966 | 16968 | 16969 => &[468], // Wolf_Ride, also used by lions/tigers.
        16978 => &[1_000_007],
        16979 => &[463],
        16980 => &[465],
        16982 => &[467],
        16983 => &[441],
        16985 => &[1_000_006],
        16986 => &[1_000_008],
        16993 => &[1_000_085],
        16994 => &[1_000_119],
        _ => return false,
    };
    interactions.iter().any(|id| ride.contains(id))
}

/// Continue immediately after the serialized VehicleInfo u16. Field order and
/// widths follow native reader 14f01a0, through InteractionInfoList at +1b8.
/// Key wrappers store compact indices in memory but read the widths below.
pub(super) fn available(r: &mut Reader<'_>, vehicle: u16) -> Result<bool> {
    r.take(8 + 8 + 1 + 12)?; // Call timings, type and two child-vehicle pairs.
    r.take(17 * 4)?; // Faction through AI-focus refs/hashes, including appearance.
    r.take(3 + 2 + 1)?; // Target/lock/gender, PersonalityInfo, MercenaryInfo.
    r.loc()?; // Mercenary hire message.
    r.loc()?; // Display level name.
    r.take(4 + 1 + 2)?; // Owned character, spawn fix, player index.
    flags(r)?;
    flags(r)?;
    r.take(40 + 4 + 4 + 4)?; // Flags/enums, vanish tick, portrait and symbol refs.
    for _ in 0..4 {
        skills(r)?;
    }
    let count = r.count()?;
    if count > 512 {
        return Err(bad("Unexpected mount interaction list."));
    }
    let interactions = (0..count)
        .map(|_| r.num(4).map(|v| v as u32))
        .collect::<Result<Vec<_>>>()?;
    Ok(player_interaction(vehicle, &interactions))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn ai_riding_and_carcass_interactions_do_not_make_a_player_mount() {
        // The second Tiger variant has Ai_Wolf_Ride, DeadAnimal_Catch and
        // Horse_UnCarry_Dead, but no player Wolf_Ride interaction.
        assert!(!player_interaction(
            16966,
            &[
                1_000_242, 1_000_098, 1_000_132, 302, 1_000_315, 409, 1_000_092, 1_000_093
            ]
        ));
        assert!(player_interaction(16969, &[1_000_098, 468, 1_000_250, 409]));
        assert!(!player_interaction(16962, &[1_000_103])); // NPC boar riding.
        assert!(!player_interaction(16978, &[468])); // Wrong vehicle family.
        assert!(player_interaction(16978, &[1_000_007]));
    }
}
