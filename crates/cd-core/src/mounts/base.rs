//! A blank mount record built from the target save's own reflection layout.
//! No foreign raw template or animal state is transplanted. The engine initializes
//! the selected character definition on load is intended; exotic behavior needs
//! an in-game test before it can be called confirmed.
use super::{Result, bad};
use crimson_format::save::{FieldKind, FieldValue, ObjectBlock, ScalarValue};

type Shape = (&'static str, &'static str, u16, u16, u32);
const MERCENARY: &[Shape] = &[
    ("_characterKey", "CharacterKey", 0, 4, 0),
    ("_factionKey", "FactionKey", 0, 4, 0),
    ("_mercenaryNo", "MercenaryNo", 0, 8, 0),
    ("_ownedCharacterKey", "CharacterKey", 0, 4, 0),
    ("_mercenaryName", "staticstringA", 1, 1, 0),
    (
        "_nudeAppearanceIndexKey",
        "CharacterAppearanceIndexKey",
        0,
        8,
        0,
    ),
    (
        "_customizationAppearanceIndexKey",
        "CharacterAppearanceIndexKey",
        0,
        8,
        0,
    ),
    ("_armorDyeAppearanceIndexKey", "uint8", 0, 1, 0),
    ("_levelData", "ReflectObject", 4, 8, 8),
    ("_remainSkillPoint", "TSkillPoint", 0, 2, 0),
    ("_deadTime", "Ctc64", 0, 8, 0),
    ("_lastPaidTime", "uint64", 0, 8, 0),
    ("_lastBreedingTime", "uint64", 0, 8, 0),
    ("_nextFeedFromCampStrawTime", "uint64", 0, 8, 0),
    ("_prevFeedFromCampStrawTime", "uint64", 0, 8, 0),
    ("_workPlaceFactionNodeKey", "FactionNodeKey", 0, 4, 0),
    ("_workStartTime", "uint64", 0, 8, 0),
    ("_onlyWorkStartTime", "uint64", 0, 8, 0),
    ("_onlyWorkCompleteTime", "uint64", 0, 8, 0),
    ("_workCompleteTime", "uint64", 0, 8, 0),
    ("_workKeyNameHash", "HashCode32", 0, 4, 0),
    ("_movedDistance", "float", 0, 4, 0),
    ("_totalDistance", "float", 0, 4, 0),
    ("_moveVelocity", "float", 0, 4, 0),
    ("_lastSummoned", "bool", 0, 1, 0),
    ("_spawnPosition", "float3", 0, 12, 0),
    ("_spawnYaw", "float", 0, 4, 0),
    ("_spawnFieldInfoKey", "FieldInfoKey", 0, 4, 0),
    ("_isMainMercenary", "bool", 0, 1, 0),
    ("_isInitialize", "bool", 0, 1, 0),
    ("_isDead", "bool", 0, 1, 0),
    ("_isBlockedAbility", "bool", 0, 1, 0),
    ("_isHyosiMercenary", "bool", 0, 1, 0),
    ("_equipItemList", "ReflectObject", 6, 0, 4104),
    ("_inventoryItemList", "ReflectObject", 6, 0, 4104),
    ("_customizationSaveData", "ReflectObjectPtr", 5, 8, 4104),
    ("_remainTimeBuffSaveDataList", "ReflectObject", 6, 0, 4104),
    ("_recoveryItemNo", "ItemNo", 0, 8, 0),
    (
        "_sealedDropResultSubSaveItemList",
        "ReflectObject",
        6,
        0,
        4104,
    ),
    ("_useItemReserveSlotSaveList", "ReflectObject", 6, 0, 4104),
    ("_currentHp", "TStat", 0, 8, 0),
    ("_currentMp", "TStat", 0, 8, 0),
    ("_lastEquipSlotNameKey", "EquipSlotNameKey", 0, 4, 0),
    ("_shipStationSaveList", "ReflectObject", 6, 0, 4104),
];
const LEVEL: &[Shape] = &[
    ("_level", "TLevel", 0, 4, 0),
    ("_exp", "TExperience", 0, 8, 0),
    ("_gimmickEventDailyCountData", "ReflectObject", 4, 8, 8),
    ("_actionFrameEventDailyCountData", "ReflectObject", 4, 8, 8),
    ("_talkEventDailyCountData", "ReflectObject", 4, 8, 8),
    (
        "_shareKnowledgeRewardDailyCountData",
        "ReflectObject",
        4,
        8,
        8,
    ),
];
const DAILY: &[Shape] = &[
    ("_lastUpdateTime", "uint64", 0, 8, 0),
    ("_dailyCount", "uint32", 0, 4, 0),
];

fn blank(source: &ObjectBlock, class: &str, shape: &[Shape]) -> Result<ObjectBlock> {
    if source.class_name != class
        || source.fields.len() != shape.len()
        || source.mask_bytes.len() != shape.len().div_ceil(8)
        || source.mask_byte_count as usize != source.mask_bytes.len()
        || source.reserved_u32 != 0
        || !source.trailing_pad.is_empty()
        || !source.undecoded_ranges.is_empty()
        || !source.locator_wrapper.as_ref().is_some_and(|w| {
            w.type_index as u32 == source.class_index
                && w.child_reserved == 0
                && w.sentinel1 == u32::MAX
                && w.sentinel2 == u32::MAX
        })
    {
        return Err(bad("Unbestätigtes Layout für den Reittier-Basiseintrag."));
    }
    let mut out = source.clone();
    out.mask_bytes.fill(0);
    for (index, f) in out.fields.iter_mut().enumerate() {
        let Some(&(_, ty, kind, size, aux)) = shape.iter().find(|s| s.0 == f.name) else {
            return Err(bad("Unbekanntes Feld im Reittier-Basiseintrag."));
        };
        if f.field_index as usize != index
            || f.type_name != ty
            || f.meta_kind != kind
            || f.meta_size != size
            || f.meta_aux != aux
            || source.fields.iter().filter(|x| x.name == f.name).count() != 1
        {
            return Err(bad("Geändertes Feldformat im Reittier-Basiseintrag."));
        }
        f.start = 0;
        f.end = 0;
        f.note.clear();
        if kind == 4 {
            let FieldValue::Locator {
                child: Some(child),
                inline_child: true,
                child_type_name,
                child_type_index,
                wrapper_prefix,
                child_reserved,
                child_sentinel1,
                child_sentinel2,
                ..
            } = &mut f.value
            else {
                return Err(bad("Fehlende Inline-Struktur im Reittier-Basiseintrag."));
            };
            if !f.present
                || *child_reserved != 0
                || *child_sentinel1 != u32::MAX
                || *child_sentinel2 != u32::MAX
                || !wrapper_prefix.is_empty()
                || *child_type_index as u32 != child.class_index
                || *child_type_name != child.class_name
            {
                return Err(bad(
                    "Unbestätigte Inline-Struktur im Reittier-Basiseintrag.",
                ));
            }
            let (expected, child_shape) = if class == "MercenarySaveData" {
                ("ExperienceLevelSaveData", LEVEL)
            } else if class == "ExperienceLevelSaveData" {
                ("FriendlyDailyCountSaveData", DAILY)
            } else {
                return Err(bad("Unerwartete verschachtelte Tierstruktur."));
            };
            **child = blank(child, expected, child_shape)?;
            f.kind = FieldKind::ObjectLocator;
            f.present = true;
            f.absent_marker = false;
            out.mask_bytes[index / 8] |= 1 << (index % 8);
        } else {
            f.kind = FieldKind::Absent;
            f.present = false;
            f.absent_marker = matches!(kind, 3 | 6 | 7);
            f.value = FieldValue::None;
        }
    }
    Ok(out)
}
fn scalar(o: &mut ObjectBlock, name: &str, value: ScalarValue) -> Result<()> {
    let f = o
        .fields
        .iter_mut()
        .find(|f| f.name == name)
        .ok_or_else(|| bad("Basisfeld fehlt."))?;
    if f.meta_kind != 0 {
        return Err(bad("Basisfeld ist kein Skalar."));
    }
    f.present = true;
    f.absent_marker = false;
    f.kind = FieldKind::FixedPrefix;
    f.value = FieldValue::Scalar(value);
    o.mask_bytes[f.field_index as usize / 8] |= 1 << (f.field_index % 8);
    Ok(())
}
/// Use a source only as a schema/wrapper witness; every persisted value is reset.
pub(super) fn build(source: &ObjectBlock, key: u32) -> Result<ObjectBlock> {
    let mut out = blank(source, "MercenarySaveData", MERCENARY)?;
    scalar(&mut out, "_characterKey", ScalarValue::U32(key))?;
    scalar(&mut out, "_mercenaryNo", ScalarValue::U64(0))?; // caller assigns a fresh number
    // Ownership makes the new entry available to Kliff, without copying the
    // donor's active/main/last-summoned flags or replacing an existing mount.
    scalar(&mut out, "_ownedCharacterKey", ScalarValue::U32(1))?;
    scalar(&mut out, "_isInitialize", ScalarValue::Bool(0))?;
    Ok(out)
}
pub(super) fn verify(o: &ObjectBlock) -> Result<()> {
    // Validate the current layout again after encryption and list relocation.
    blank(o, "MercenarySaveData", MERCENARY)?;
    let expected = [
        "_characterKey",
        "_mercenaryNo",
        "_ownedCharacterKey",
        "_levelData",
        "_isInitialize",
    ];
    if o.fields.iter().filter(|f| f.present).count() != expected.len()
        || o.fields
            .iter()
            .any(|f| f.present && !expected.contains(&f.name.as_str()))
        || !o.fields.iter().any(|f| {
            f.name == "_ownedCharacterKey"
                && matches!(f.value, FieldValue::Scalar(ScalarValue::U32(1)))
        })
        || !o.fields.iter().any(|f| {
            f.name == "_isInitialize" && matches!(f.value, FieldValue::Scalar(ScalarValue::Bool(0)))
        })
    {
        return Err(bad("Basiseintrag enthält unerwarteten Tierzustand."));
    }
    let level = o.fields.iter().find(|f| f.name == "_levelData").unwrap();
    let FieldValue::Locator {
        child: Some(level), ..
    } = &level.value
    else {
        return Err(bad("Basiseintrag ohne Levelstruktur."));
    };
    for f in &level.fields {
        if f.present && f.meta_kind != 4 {
            return Err(bad("Kopierte Levelwerte im Basiseintrag."));
        }
        if let FieldValue::Locator { child: Some(c), .. } = &f.value
            && c.fields.iter().any(|f| f.present)
        {
            return Err(bad("Kopierte Tageszähler im Basiseintrag."));
        }
    }
    Ok(())
}
/// Riding families only. Livestock and a camel calf need a separate verified route.
pub(super) fn target(m: &super::Mount) -> bool {
    matches!(
        m.vehicle,
        16960
            | 16962
            | 16966
            | 16968
            | 16969
            | 16978
            | 16979
            | 16980
            | 16982
            | 16983
            | 16985
            | 16986
            | 16993
    ) && !m.internal.contains("Baby_")
        && (m.internal.starts_with("Animal_") || m.internal.starts_with("Riding_"))
}
