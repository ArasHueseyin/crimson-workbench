use super::{Mount, Result, bad, base};
use crimson_format::save::{
    Body, DecodedField, FieldKind, FieldValue, ObjectBlock, Save, ScalarValue,
};
use std::collections::{BTreeMap, BTreeSet};

fn field<'a>(o: &'a ObjectBlock, name: &str) -> Result<&'a DecodedField> {
    o.fields
        .iter()
        .find(|f| f.name == name)
        .ok_or_else(|| bad(format!("Save-Feld fehlt: {name}")))
}
fn field_mut<'a>(o: &'a mut ObjectBlock, name: &str) -> Result<&'a mut DecodedField> {
    o.fields
        .iter_mut()
        .find(|f| f.name == name)
        .ok_or_else(|| bad(format!("Save-Feld fehlt: {name}")))
}
fn number(o: &ObjectBlock, name: &str) -> Result<u64> {
    match &field(o, name)?.value {
        FieldValue::Scalar(ScalarValue::U8(n)) => Ok(*n as u64),
        FieldValue::Scalar(ScalarValue::U16(n)) => Ok(*n as u64),
        FieldValue::Scalar(ScalarValue::U32(n)) => Ok(*n as u64),
        FieldValue::Scalar(ScalarValue::U64(n)) => Ok(*n),
        FieldValue::Scalar(ScalarValue::I32(n)) if *n >= 0 => Ok(*n as u64),
        FieldValue::Scalar(ScalarValue::I64(n)) if *n >= 0 => Ok(*n as u64),
        _ => Err(bad(format!("Kein gültiger Zähler: {name}"))),
    }
}
fn set_number(o: &mut ObjectBlock, name: &str, n: u64) -> Result<()> {
    let f = field_mut(o, name)?;
    if !f.present {
        return Err(bad(format!("Zähler nicht gesetzt: {name}")));
    }
    let value = match &f.value {
        FieldValue::Scalar(ScalarValue::U32(_)) if n <= u32::MAX as u64 => {
            ScalarValue::U32(n as u32)
        }
        FieldValue::Scalar(ScalarValue::U64(_)) => ScalarValue::U64(n),
        FieldValue::Scalar(ScalarValue::I32(_)) if n <= i32::MAX as u64 => {
            ScalarValue::I32(n as i32)
        }
        FieldValue::Scalar(ScalarValue::I64(_)) if n <= i64::MAX as u64 => {
            ScalarValue::I64(n as i64)
        }
        _ => return Err(bad(format!("Unbestätigte Zählerbreite: {name}"))),
    };
    f.value = FieldValue::Scalar(value);
    Ok(())
}
fn list<'a>(o: &'a ObjectBlock, name: &str) -> Result<&'a [ObjectBlock]> {
    match &field(o, name)?.value {
        FieldValue::ObjectList {
            count, elements, ..
        } if *count as usize == elements.len() => Ok(elements),
        _ => Err(bad(format!("Ungültige Liste: {name}"))),
    }
}
// Ported from crimson-rs's validated length-changing list editor. The codec
// preserves list header bytes; its separate count field must be patched too.
fn update_count(header: &mut [u8], variant: &str, count: u32) -> Result<()> {
    if variant == "marker_run_plus_zeros" {
        let off = header
            .len()
            .checked_sub(17)
            .ok_or_else(|| bad("Liste zu kurz"))?;
        header[off..off + 4].copy_from_slice(&count.to_le_bytes());
        return Ok(());
    }
    let (size, offset, width) = match variant {
        "zero1_count_u24" => (18, 1, 3),
        "zero4_count_u32" => (18, 4, 4),
        "ones_then_count" => (21, 4, 4),
        "one_count_u16be" => (19, 1, 2),
        _ => return Err(bad("Unbestätigter Listenheader")),
    };
    let off = header
        .len()
        .checked_sub(size)
        .ok_or_else(|| bad("Liste zu kurz"))?
        + offset;
    if width == 2 {
        let n = u16::try_from(count).map_err(|_| bad("Liste zu groß"))?;
        header[off..off + 2].copy_from_slice(&n.to_be_bytes());
    } else {
        if width == 3 && count > 0xffffff {
            return Err(bad("Liste zu groß"));
        }
        header[off..off + width].copy_from_slice(&count.to_le_bytes()[..width]);
    }
    Ok(())
}
fn visit(o: &ObjectBlock, f: &mut impl FnMut(&ObjectBlock) -> Result<()>) -> Result<()> {
    f(o)?;
    for field in &o.fields {
        match &field.value {
            FieldValue::Locator { child: Some(c), .. } => visit(c, f)?,
            FieldValue::ObjectList { elements, .. } => {
                for c in elements {
                    visit(c, f)?
                }
            }
            _ => {}
        }
    }
    Ok(())
}
fn visit_mut(
    o: &mut ObjectBlock,
    f: &mut impl FnMut(&mut ObjectBlock) -> Result<()>,
) -> Result<()> {
    f(o)?;
    for field in &mut o.fields {
        match &mut field.value {
            FieldValue::Locator { child: Some(c), .. } => visit_mut(c, f)?,
            FieldValue::ObjectList { elements, .. } => {
                for c in elements {
                    visit_mut(c, f)?
                }
            }
            _ => {}
        }
    }
    Ok(())
}
fn decode(bytes: &[u8]) -> Result<(Save, Body, Vec<ObjectBlock>)> {
    let save = Save::parse(bytes).map_err(|e| bad(e.to_string()))?;
    let body = Body::parse(&save.body)?;
    let blocks = body.decode_blocks(&save.body);
    if blocks.len() != body.toc.entries.len() {
        return Err(bad("Spielstand konnte nicht vollständig gelesen werden."));
    }
    for b in &blocks {
        visit(b, &mut |o| {
            if !o.undecoded_ranges.is_empty() {
                Err(bad("Unbekannte Spielstandfelder: keine Änderung."))
            } else {
                Ok(())
            }
        })?;
    }
    if body.write(&save.body, &blocks)? != save.body {
        return Err(bad(
            "Spielstand-Roundtrip ist nicht bytegleich: keine Änderung.",
        ));
    }
    Ok((save, body, blocks))
}
fn clan(blocks: &[ObjectBlock]) -> Result<usize> {
    let indices: Vec<_> = blocks
        .iter()
        .enumerate()
        .filter(|(_, b)| b.class_name == "MercenaryClanSaveData")
        .map(|(i, _)| i)
        .collect();
    if indices.len() != 1 {
        return Err(bad("Stallzuordnung ist nicht eindeutig."));
    }
    Ok(indices[0])
}
// Only these build-confirmed assignment fields can be omitted from a NEW
// clone. Jobs, feed timers and other actor state still disqualify the donor.
const ASSIGNMENT: &[&str] = &["_ownedCharacterKey", "_lastSummoned", "_isMainMercenary"];
fn supported_assignment(o: &ObjectBlock) -> bool {
    o.fields
        .iter()
        .filter(|f| f.present && ASSIGNMENT.contains(&f.name.as_str()))
        .all(|f| {
            f.meta_kind == 0
                && f.meta_aux == 0
                && !f.absent_marker
                && f.kind == FieldKind::FixedPrefix
                && (f.field_index as usize / 8) < o.mask_bytes.len()
                && o.mask_bytes[f.field_index as usize / 8] & (1 << (f.field_index % 8)) != 0
                && if f.name == "_ownedCharacterKey" {
                    f.type_name == "CharacterKey"
                        && f.meta_size == 4
                        && matches!(f.value, FieldValue::Scalar(ScalarValue::U32(_)))
                } else {
                    f.type_name == "bool"
                        && f.meta_size == 1
                        && matches!(f.value, FieldValue::Scalar(ScalarValue::Bool(_)))
                }
        })
}
fn has_assignment(o: &ObjectBlock) -> bool {
    o.fields
        .iter()
        .any(|f| f.present && ASSIGNMENT.contains(&f.name.as_str()))
}
fn supported_name(o: &ObjectBlock) -> bool {
    let Ok(f) = field(o, "_mercenaryName") else {
        return true;
    };
    if !f.present {
        return true;
    }
    matches!(&f.value, FieldValue::InlineBytes { count, bytes }
        if f.type_name == "staticstringA" && f.meta_kind == 1 && f.meta_size == 1
            && f.kind == FieldKind::InlineBytes && *count as usize == bytes.len()
            && bytes.len() <= 256 && !bytes.contains(&0) && std::str::from_utf8(bytes).is_ok())
        && (f.field_index as usize / 8) < o.mask_bytes.len()
}
fn omit_name(o: &mut ObjectBlock) -> Result<()> {
    if !supported_name(o) {
        return Err(bad("Unbestätigtes Namensformat der Stallvorlage."));
    }
    let Some(f) = o
        .fields
        .iter_mut()
        .find(|f| f.name == "_mercenaryName" && f.present)
    else {
        return Ok(());
    };
    let index = f.field_index as usize;
    o.mask_bytes[index / 8] &= !(1 << (index % 8));
    f.present = false;
    f.absent_marker = false; // Inline strings have no absent array marker.
    f.kind = FieldKind::Absent;
    f.value = FieldValue::None;
    Ok(())
}
fn normalize_clone(o: &mut ObjectBlock) -> Result<()> {
    if !clean(o) {
        return Err(bad("Unbestätigte Stallvorlage: keine Registrierung."));
    }
    omit_name(o)?;
    for f in o
        .fields
        .iter_mut()
        .filter(|f| f.present && ASSIGNMENT.contains(&f.name.as_str()))
    {
        let index = f.field_index as usize;
        o.mask_bytes[index / 8] &= !(1 << (index % 8));
        f.present = false;
        f.absent_marker = false; // Confirmed fixed scalars have no absent marker.
        f.kind = FieldKind::Absent;
        f.value = FieldValue::None;
    }
    Ok(())
}
fn clean(o: &ObjectBlock) -> bool {
    const ALLOWED: &[&str] = &[
        "_characterKey",
        "_mercenaryNo",
        "_mercenaryName",
        "_levelData",
        "_lastPaidTime",
        "_lastBreedingTime",
        "_spawnPosition",
        "_spawnYaw",
        "_spawnFieldInfoKey",
        "_isInitialize",
        "_equipItemList",
        "_currentHp",
        "_currentMp",
        "_ownedCharacterKey",
        "_lastSummoned",
        "_isMainMercenary",
    ];
    o.class_name == "MercenarySaveData"
        && o.fields
            .iter()
            .filter(|f| f.present)
            .all(|f| ALLOWED.contains(&f.name.as_str()))
        && supported_name(o)
        && supported_assignment(o)
        && number(o, "_currentHp").is_ok_and(|n| n > 0)
        && number(o, "_mercenaryNo").is_ok()
}
fn donors<'a>(mercs: &'a [ObjectBlock], mounts: &[Mount]) -> BTreeMap<u16, &'a ObjectBlock> {
    let by_key: BTreeMap<_, _> = mounts.iter().map(|m| (m.key, m.vehicle)).collect();
    let mut out = BTreeMap::new();
    for merc in mercs {
        if clean(merc)
            && let Ok(key) = number(merc, "_characterKey")
            && let Some(vehicle) = by_key.get(&(key as u32))
        {
            let previous = out.entry(*vehicle).or_insert(merc);
            // Keep the original unassigned-donor route whenever available.
            if has_assignment(previous) && !has_assignment(merc) {
                *previous = merc;
            }
        }
    }
    out
}
fn base_witness<'a>(mercs: &'a [ObjectBlock], mounts: &[Mount]) -> Option<&'a ObjectBlock> {
    mercs.iter().find(|o| {
        clean(o)
            && number(o, "_characterKey")
                .is_ok_and(|key| mounts.iter().any(|m| m.key as u64 == key))
            && base::build(o, 1).is_ok()
    })
}
pub(super) fn annotate(bytes: &[u8], mounts: &mut [Mount]) -> Result<()> {
    let (_, _, blocks) = decode(bytes)?;
    let mercs = list(&blocks[clan(&blocks)?], "_mercenaryDataList")?;
    let available = donors(mercs, mounts);
    let base_available = base_witness(mercs, mounts).is_some();
    let mut owned = BTreeMap::new();
    for m in mercs {
        *owned
            .entry(number(m, "_characterKey")? as u32)
            .or_insert(0usize) += 1;
    }
    for m in mounts {
        m.owned = *owned.get(&m.key).unwrap_or(&0);
        let matching = available.contains_key(&m.vehicle);
        let blank = base_available && base::target(m);
        m.supported = m.owned == 0 && (matching || blank);
        m.registration = if m.owned > 0 {
            "owned"
        } else if matching {
            "same_family"
        } else if blank {
            "base"
        } else {
            "unavailable"
        }
        .into();
        m.reason = if m.owned > 0 {
            "Bereits im gewählten Spielstand registriert."
        } else if matching {
            "Passende Stallvorlage vorhanden."
        } else if blank {
            "Basiseintrag ohne Tierart-Vorlage möglich. Keine fremde Ausrüstung oder Werte werden übernommen. Herbeirufen und Reiten dieser Variante sind im Spiel noch zu prüfen."
        } else {
            "In diesem Spielstand fehlt eine bestätigte Stallvorlage dieser Tierart. Registriere zuerst ein gesundes Tier dieser Gruppe im Spiel und speichere."
        }
        .into();
    }
    Ok(())
}
pub(super) struct Candidate {
    pub save: Vec<u8>,
    pub lobby: Vec<u8>,
    pub mercenary_no: u64,
    pub donor_key: u32,
    pub count_before: usize,
    pub count_after: usize,
}
pub(super) fn build(
    save_bytes: &[u8],
    lobby_bytes: &[u8],
    mount: &Mount,
    mounts: &[Mount],
) -> Result<Candidate> {
    let (mut save, body, mut blocks) = decode(save_bytes)?;
    let (mut lobby, lobby_body, mut lb) = decode(lobby_bytes)?;
    if lb.len() != 1 || lb[0].class_name != "SlotSaveData" {
        // Some builds call the top level LobbySaveData.
        if lb.len() != 1 || !lb[0].fields.iter().any(|f| f.name == "_generateNo") {
            return Err(bad("Lobby-Zähler nicht eindeutig."));
        }
    }
    let ci = clan(&blocks)?;
    let mercs = list(&blocks[ci], "_mercenaryDataList")?;
    let before = mercs.len();
    if before >= 4096 {
        return Err(bad("Stall-Limit erreicht."));
    }
    if mercs
        .iter()
        .any(|o| number(o, "_characterKey").ok() == Some(mount.key as u64))
    {
        return Err(bad("Dieses Reittier ist bereits registriert."));
    }
    let available = donors(mercs, mounts);
    let (donor_key, mut added) = if let Some(donor) = available.get(&mount.vehicle) {
        let mut clone = (**donor).clone();
        normalize_clone(&mut clone)?;
        (number(donor, "_characterKey")? as u32, clone)
    } else if base::target(mount) {
        let witness = base_witness(mercs, mounts)
            .ok_or_else(|| bad("Spielstandlayout erlaubt keinen Reittier-Basiseintrag."))?;
        (0, base::build(witness, mount.key)?)
    } else {
        return Err(bad(
            "Keine bestätigte Stallvorlage oder unterstützte Basiseintrag-Route.",
        ));
    };
    let mut max_no = number(&lb[0], "_generateNo")?;
    let mut merc_nos = BTreeSet::new();
    for o in mercs {
        if !merc_nos.insert(number(o, "_mercenaryNo")?) {
            return Err(bad("Doppelte bestehende Reittiernummer."));
        }
    }
    for b in blocks.iter().chain(lb.iter()) {
        visit(b, &mut |o| {
            for f in &o.fields {
                if f.present
                    && f.name.to_ascii_lowercase().ends_with("no")
                    && let Ok(n) = number(o, &f.name)
                    && n != u64::MAX
                {
                    max_no = max_no.max(n);
                }
            }
            Ok(())
        })?;
    }
    let first = max_no
        .checked_add(1)
        .ok_or_else(|| bad("Nummernraum erschöpft."))?;
    let mut next = first
        .checked_add(1)
        .ok_or_else(|| bad("Nummernraum erschöpft."))?;
    set_number(&mut added, "_characterKey", mount.key as u64)?;
    set_number(&mut added, "_mercenaryNo", first)?;
    // Every embedded item, including nested socket items, gets a fresh global
    // number. Never copy a donor's item-instance identity into a second animal.
    visit_mut(&mut added, &mut |o| {
        if o.fields.iter().any(|f| f.name == "_itemNo" && f.present) {
            set_number(o, "_itemNo", next)?;
            next = next
                .checked_add(1)
                .ok_or_else(|| bad("Nummernraum erschöpft."))?;
        }
        Ok(())
    })?;
    let f = field_mut(&mut blocks[ci], "_mercenaryDataList")?;
    let FieldValue::ObjectList {
        count,
        elements,
        header_bytes,
        header_variant,
    } = &mut f.value
    else {
        return Err(bad("Stall-Liste fehlt."));
    };
    elements.push(added);
    *count += 1;
    update_count(header_bytes, header_variant, *count)?;
    let candidate_body = body.write(&save.body, &blocks)?;
    // Decode the complete changed tree, then remove exactly the new animal.
    // Returning byte-for-byte to the old body proves all existing state stayed
    // intact, including opaque fields and every recomputed absolute pointer.
    let parsed = Body::parse(&candidate_body)?;
    let mut check = parsed.decode_blocks(&candidate_body);
    let FieldValue::ObjectList {
        count,
        elements,
        header_bytes,
        header_variant,
    } = &mut field_mut(&mut check[ci], "_mercenaryDataList")?.value
    else {
        return Err(bad("Neue Stall-Liste ungültig."));
    };
    if elements.len() != before + 1
        || number(elements.last().unwrap(), "_characterKey")? != mount.key as u64
        || number(elements.last().unwrap(), "_mercenaryNo")? != first
    {
        return Err(bad("Neue Registrierung stimmt nicht."));
    }
    if donor_key == 0 {
        base::verify(elements.last().unwrap())?;
    } else if elements
        .last()
        .unwrap()
        .fields
        .iter()
        .any(|f| f.present && (f.name == "_mercenaryName" || ASSIGNMENT.contains(&f.name.as_str())))
    {
        return Err(bad(
            "Name oder aktive Zuweisung der Vorlage wurde unerwartet kopiert.",
        ));
    }
    let mut new_ids = BTreeSet::from([first]);
    visit(elements.last().unwrap(), &mut |o| {
        if o.fields.iter().any(|f| f.name == "_itemNo" && f.present) {
            let n = number(o, "_itemNo")?;
            if n <= max_no || n >= next || !new_ids.insert(n) {
                return Err(bad("Neue Itemnummern sind nicht eindeutig."));
            }
        }
        Ok(())
    })?;
    elements.pop();
    *count -= 1;
    update_count(header_bytes, header_variant, *count)?;
    if parsed.write(&candidate_body, &check)? != save.body {
        return Err(bad(
            "Bestehender Spielstand wurde verändert: keine Installation.",
        ));
    }
    save.body = candidate_body;
    set_number(&mut lb[0], "_generateNo", next)?;
    lobby.body = lobby_body.write(&lobby.body, &lb)?;
    let save_out = save
        .write_with_nonce(save.header.nonce())
        .map_err(|e| bad(e.to_string()))?;
    let lobby_out = lobby
        .write_with_nonce(lobby.header.nonce())
        .map_err(|e| bad(e.to_string()))?;
    let (rs, _, _) = decode(&save_out)?;
    let (rl, _, rblocks) = decode(&lobby_out)?;
    if rs.body != save.body || rl.body != lobby.body || number(&rblocks[0], "_generateNo")? != next
    {
        return Err(bad("Verschlüsselter Spielstand-Readback fehlgeschlagen."));
    }
    Ok(Candidate {
        save: save_out,
        lobby: lobby_out,
        mercenary_no: first,
        donor_key,
        count_before: before,
        count_after: before + 1,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    #[ignore = "requires explicit private save fixture and read-only game catalog"]
    fn real_save_registration_inverse_and_duplicate_rejection() {
        let source = std::path::PathBuf::from(
            std::env::var_os("CD_MOUNT_FIXTURE").expect("private source fixture required"),
        );
        let game = std::path::PathBuf::from(
            std::env::var_os("CD_MOUNT_GAME").expect("read-only catalog root required"),
        );
        let data = crate::GameData::open(&game, "ger").unwrap();
        let mounts = crate::mounts::catalog(&data).unwrap();
        assert!(mounts.len() > 30);
        assert!(
            mounts
                .iter()
                .all(|m| !m.internal.contains("Sequencer") && !m.internal.contains("Dummy"))
        );
        let save = std::fs::read(&source).unwrap();
        let lobby = std::fs::read(source.with_file_name("lobby.save")).unwrap();
        let mut annotated = mounts.clone();
        annotate(&save, &mut annotated).unwrap();
        let (_, _, blocks) = decode(&save).unwrap();
        let mercs = list(&blocks[clan(&blocks).unwrap()], "_mercenaryDataList").unwrap();
        for donor in mercs
            .iter()
            .filter(|m| clean(m) && field(m, "_mercenaryName").is_ok_and(|f| f.present))
        {
            if has_assignment(donor) {
                let mut malformed = donor.clone();
                field_mut(&mut malformed, "_ownedCharacterKey")
                    .unwrap()
                    .meta_size = 8;
                assert!(
                    !clean(&malformed),
                    "Unconfirmed assignment format must be refused"
                );
            }
            let mut dead = donor.clone();
            set_number(&mut dead, "_currentHp", 0).unwrap();
            assert!(!clean(&dead));
            let mut malformed = donor.clone();
            if let FieldValue::InlineBytes { count, .. } =
                &mut field_mut(&mut malformed, "_mercenaryName").unwrap().value
            {
                *count += 1;
            }
            assert!(!clean(&malformed));
            let mut named_copy = donor.clone();
            omit_name(&mut named_copy).unwrap();
            assert!(!field(&named_copy, "_mercenaryName").unwrap().present);
            assert!(field(donor, "_mercenaryName").unwrap().present);
            let mut normalized = donor.clone();
            normalize_clone(&mut normalized).unwrap();
            assert!(!has_assignment(&normalized));
            assert!(clean(&normalized));
            // Normalization only receives a clone; the named source is intact.
            assert!(field(donor, "_mercenaryName").unwrap().present);
        }
        let candidates: Vec<_> = annotated.iter().filter(|m| m.supported).collect();
        assert!(candidates.len() > 10);
        let bear = candidates.iter().find(|m| m.vehicle == 16979).unwrap();
        let horse = candidates.iter().find(|m| m.vehicle == 16960).unwrap();
        let mut tested = vec![*bear, *horse];
        if let Some(lion) = candidates.iter().find(|m| m.vehicle == 16968) {
            tested.push(*lion);
        }
        if std::env::var_os("CD_MOUNT_REQUIRE_NAMED_LION").is_some() {
            assert_eq!(tested.len(), 3);
            assert!(mercs.iter().any(|m| clean(m)
                && has_assignment(m)
                && number(m, "_characterKey").ok() == Some(1002316)));
        }
        for mount in &tested {
            let new = build(&save, &lobby, mount, &mounts).unwrap();
            assert_eq!(new.count_after, new.count_before + 1);
            let mut view = mounts.clone();
            annotate(&new.save, &mut view).unwrap();
            let owned = view.iter().find(|m| m.key == mount.key).unwrap();
            assert_eq!(owned.owned, 1);
            assert!(!owned.supported);
            assert!(build(&new.save, &new.lobby, mount, &mounts).is_err());
            if let Some(out) = std::env::var_os("CD_MOUNT_CANDIDATES") {
                let p = std::path::PathBuf::from(out);
                std::fs::write(p.join(format!("{}.save", mount.key)), &new.save).unwrap();
                std::fs::write(p.join(format!("{}.lobby", mount.key)), &new.lobby).unwrap();
            }
        }
        println!(
            "Read-only catalog: {} selectable mounts; {} have a valid donor. {} encrypted candidates passed full inverse comparison, new-item identity and duplicate rejection. No game or live save writes.",
            mounts.len(),
            candidates.len(),
            tested.len()
        );
    }
    #[test]
    fn damaged_save_is_rejected_before_editing() {
        assert!(decode(b"not a save").is_err());
    }
    #[test]
    #[ignore = "requires explicit private save fixture and read-only game catalog"]
    fn real_save_blank_species_inverse_and_state_isolation() {
        let source = std::path::PathBuf::from(std::env::var_os("CD_MOUNT_FIXTURE").unwrap());
        let game = std::path::PathBuf::from(std::env::var_os("CD_MOUNT_GAME").unwrap());
        let data = crate::GameData::open(&game, "ger").unwrap();
        let mounts = crate::mounts::catalog(&data).unwrap();
        let save = std::fs::read(&source).unwrap();
        let lobby = std::fs::read(source.with_file_name("lobby.save")).unwrap();
        let (_, _, blocks) = decode(&save).unwrap();
        let mercs = list(&blocks[clan(&blocks).unwrap()], "_mercenaryDataList").unwrap();
        let witness = base_witness(mercs, &mounts).expect("Current-schema witness required");
        let blank = base::build(witness, 1000265).unwrap();
        base::verify(&blank).unwrap();
        for name in [
            "_currentHp",
            "_currentMp",
            "_equipItemList",
            "_spawnPosition",
            "_spawnYaw",
            "_lastPaidTime",
            "_lastBreedingTime",
            "_mercenaryName",
            "_isMainMercenary",
            "_lastSummoned",
        ] {
            assert!(
                !field(&blank, name).unwrap().present,
                "Borrowed state: {name}"
            );
        }
        // A malformed or new schema must fail closed rather than copy a state
        // field at a legacy bit offset or silently skip a required child.
        for name in [
            "_characterKey",
            "_currentHp",
            "_ownedCharacterKey",
            "_isInitialize",
        ] {
            let mut malformed = witness.clone();
            field_mut(&mut malformed, name).unwrap().meta_size += 1;
            assert!(base::build(&malformed, 1000265).is_err());
        }
        let mut changed = witness.clone();
        changed.fields[0].name = "_unconfirmedActorState".into();
        assert!(base::build(&changed, 1000265).is_err());
        let mut no_level = witness.clone();
        field_mut(&mut no_level, "_levelData").unwrap().value = FieldValue::None;
        assert!(base::build(&no_level, 1000265).is_err());
        let mut unexpected_hp = blank.clone();
        // Deliberately reintroduce a source animal's HP; post-write validation
        // must detect state leaking back into the neutral registration.
        let hp = field(witness, "_currentHp").unwrap().clone();
        *field_mut(&mut unexpected_hp, "_currentHp").unwrap() = hp;
        assert!(base::verify(&unexpected_hp).is_err());
        let mut view = mounts.clone();
        annotate(&save, &mut view).unwrap();
        // Every requested family plus all adult camel variants. No candidates
        // are written to the game's save folder or installed by this test.
        let keys = [
            1000265, 1000523, 1000363, 1000254, 1000264, 1000520, 1003918, 1002059, 29448, 1003912,
            1001077,
        ];
        let tested: Vec<_> = view
            .iter()
            .filter(|m| {
                keys.contains(&m.key) || (m.vehicle == 16978 && !m.internal.contains("Baby_"))
            })
            .collect();
        assert_eq!(tested.len(), 24);
        assert!(
            view.iter()
                .find(|m| m.key == 32387)
                .is_some_and(|m| !m.supported)
        );
        for mount in &tested {
            assert!(mount.supported, "Unsupported: {}", mount.name);
            assert_eq!(mount.registration, "base");
            let new = build(&save, &lobby, mount, &mounts).unwrap();
            assert_eq!(new.donor_key, 0);
            assert_eq!(new.count_after, new.count_before + 1);
            let (_, _, parsed) = decode(&new.save).unwrap();
            base::verify(
                list(&parsed[clan(&parsed).unwrap()], "_mercenaryDataList")
                    .unwrap()
                    .last()
                    .unwrap(),
            )
            .unwrap();
            let mut owned = mounts.clone();
            annotate(&new.save, &mut owned).unwrap();
            assert_eq!(owned.iter().find(|m| m.key == mount.key).unwrap().owned, 1);
            assert!(build(&new.save, &new.lobby, mount, &mounts).is_err());
            if let Some(out) = std::env::var_os("CD_MOUNT_BASE_CANDIDATES") {
                let p = std::path::PathBuf::from(out);
                std::fs::write(p.join(format!("{}.save", mount.key)), &new.save).unwrap();
                std::fs::write(p.join(format!("{}.lobby", mount.key)), &new.lobby).unwrap();
            }
            println!(
                "Private base registration passed: {} ({})",
                mount.name, mount.key
            );
        }
        println!(
            "{} neutral species/variant candidates passed inverse preservation, encryption, fresh identity and duplicate rejection; {} catalog entries eligible. Actual game behavior unconfirmed.",
            tested.len(),
            view.iter().filter(|m| m.supported).count()
        );
    }
}
