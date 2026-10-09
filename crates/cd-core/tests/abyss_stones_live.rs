//! Explicit read-only check of the pinned socket-stone catalog. No save reads.
use cd_core::GameData;
use crimson_format::item_mods::StatList;
#[test]
#[ignore = "requires explicit CD_ABYSS_STONES_GAME; read-only"]
fn socket_stones_have_complete_base_values_and_localized_effects() {
    let game = std::path::PathBuf::from(std::env::var_os("CD_ABYSS_STONES_GAME").unwrap());
    let data = GameData::open(&game, "ger").unwrap();
    let stones = data.abyss_stone_details().unwrap();
    assert_eq!(stones.len(), 190);
    let keys = stones
        .iter()
        .map(|s| s.values.key)
        .collect::<std::collections::BTreeSet<_>>();
    assert_eq!(keys.len(), 190);
    assert!(
        stones
            .iter()
            .all(|s| s.values.category == 2501 && !s.name.is_empty() && !s.description.is_empty())
    );
    for (key, name, list, stat, value) in [
        (1002787, "Zerstörung III", StatList::Static, 1000002, "3000"),
        (1002812, "Sturmwind III", StatList::PerLevel, 1000010, "3"),
        (1002793, "Einsicht III", StatList::PerLevel, 1000007, "3"),
    ] {
        let stone = stones.iter().find(|s| s.values.key == key).unwrap();
        assert_eq!(stone.name, name);
        assert!(
            stone.values.stats.iter().any(|s| s.enchant_level == 0
                && s.list == list
                && s.stat == stat
                && s.value == value)
        );
    }
    let flame = stones.iter().find(|s| s.values.key == 1000121).unwrap();
    assert!(flame.description.contains("zusätzlichen Feuerschaden"));
    assert_eq!(flame.values.buffs[0].buff, 1000120);
    assert!(!keys.contains(&6003) && !keys.contains(&1001315));
    println!(
        "PASS 190 unique localized stones, fixed-point attack and level bonuses, passive flame effect; no game/save writes"
    );
}
