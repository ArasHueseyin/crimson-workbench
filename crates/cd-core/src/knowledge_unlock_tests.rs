use super::*;
use crimson_format::save::SaveHeader;

fn scalar(index: u32, name: &str, value: u32) -> DecodedField {
    DecodedField {
        field_index: index,
        name: name.into(),
        type_name: "uint32".into(),
        meta_kind: 0,
        meta_size: 4,
        meta_aux: 0,
        present: true,
        absent_marker: false,
        kind: FieldKind::FixedPrefix,
        value: FieldValue::Scalar(ScalarValue::U32(value)),
        start: 0,
        end: 0,
        note: String::new(),
    }
}
fn fixture(rows: &[(u32, u32)], absent: bool) -> Vec<u8> {
    fn string(out: &mut Vec<u8>, s: &str) {
        out.extend((s.len() as u32).to_le_bytes());
        out.extend(s.as_bytes());
    }
    let mut raw = vec![255, 255, 4, 0];
    raw.resize(14, 0);
    raw.extend(1u16.to_le_bytes());
    raw.extend(0u16.to_le_bytes());
    raw.extend(2u16.to_le_bytes());
    for (name, fields) in [
        (
            "KnowledgeSaveData",
            vec![
                ("_list", "ReflectObject", 6, 0, 4104),
                ("_points", "uint32", 0, 4, 0),
                ("_questProgress", "uint32", 0, 4, 0),
            ],
        ),
        (
            "KnowledgeElementSaveData",
            vec![
                ("_key", "KnowledgeKey", 0, 4, 0),
                ("_level", "TLevel", 0, 4, 0),
                ("_learnedFieldTime", "uint64", 0, 8, 0),
                ("_isNewMark", "bool", 0, 1, 0),
            ],
        ),
    ] {
        string(&mut raw, name);
        raw.extend((fields.len() as u16).to_le_bytes());
        for (field, ty, kind, size, aux) in fields {
            string(&mut raw, field);
            string(&mut raw, ty);
            raw.extend((kind as u16).to_le_bytes());
            raw.extend((size as u16).to_le_bytes());
            raw.extend((aux as u32).to_le_bytes());
        }
    }
    let offset = raw.len() + 32;
    for n in [0, 1, offset as u32, 0, u32::MAX, u32::MAX, offset as u32, 0] {
        raw.extend(n.to_le_bytes());
    }
    let body = Body::parse(&raw).unwrap();
    let mut elements = Vec::new();
    for (key, level) in rows {
        let mut e = blank(&body).unwrap();
        set_number(&mut e, "_key", (*key).into()).unwrap();
        set_number(&mut e, "_level", (*level).into()).unwrap();
        set_number(&mut e, "_learnedFieldTime", 123).unwrap();
        elements.push(e);
    }
    let mut header = vec![0; 18];
    header[4..8].copy_from_slice(&(elements.len() as u32).to_le_bytes());
    let list = DecodedField {
        field_index: 0,
        name: "_list".into(),
        type_name: "ReflectObject".into(),
        meta_kind: 6,
        meta_size: 0,
        meta_aux: 4104,
        present: !absent,
        absent_marker: absent,
        kind: if absent {
            FieldKind::Absent
        } else {
            FieldKind::ObjectList
        },
        value: if absent {
            FieldValue::None
        } else {
            FieldValue::ObjectList {
                count: elements.len() as u32,
                elements,
                header_bytes: header,
                header_variant: "zero4_count_u32",
            }
        },
        start: 0,
        end: 0,
        note: String::new(),
    };
    let object = ObjectBlock {
        class_index: 0,
        class_name: "KnowledgeSaveData".into(),
        data_offset: offset as u32,
        data_size: 0,
        mask_byte_count: 1,
        mask_bytes: vec![if absent { 6 } else { 7 }],
        reserved_u32: 0,
        fields: vec![
            list,
            scalar(1, "_points", 57),
            scalar(2, "_questProgress", 1234),
        ],
        trailing_pad: vec![],
        undecoded_ranges: vec![],
        locator_wrapper: None,
    };
    let mut header = [0; 128];
    header[..4].copy_from_slice(b"SAVE");
    header[4] = 2;
    Save {
        header: SaveHeader::from_bytes(header).unwrap(),
        body: body.write(&raw, &[object]).unwrap(),
        hmac_ok: true,
    }
    .write_with_nonce([0; 16])
    .unwrap()
}
#[test]
fn recipe_is_journal_only_and_disjoint_from_skills() {
    let skills: Vec<serde_json::Value> =
        serde_json::from_str(include_str!("../schemas/marketplace-skills-2976.json")).unwrap();
    let keys = targets().unwrap();
    assert_eq!(keys.len(), 3939);
    assert!(skills.iter().all(|s| {
        !keys
            .iter()
            .any(|k| k.0 as u64 == s["key"].as_u64().unwrap())
    }));
}
#[test]
fn empty_partial_complete_and_absent_lists_work_without_a_foreign_donor() {
    let recipe = targets().unwrap();
    let first = recipe[0];
    let upgrade = *recipe.iter().find(|k| k.1 > 1 && k.0 != first.0).unwrap();
    for (rows, absent) in [
        (vec![], false),
        (vec![], true),
        (
            vec![(first.0, first.1), (upgrade.0, 1), (4_000_000_000, 9)],
            false,
        ),
        (recipe.clone(), false),
    ] {
        let raw = fixture(&rows, absent);
        let (candidate, summary) = transform(&raw).unwrap();
        assert_eq!(summary.total, 3939);
        assert_eq!(
            summary.total,
            summary.new_entries + summary.upgraded_entries + summary.already_complete
        );
        let (_, _, actual) = decode(&candidate).unwrap();
        assert_eq!(number(&actual[0], "_points").unwrap(), 57);
        assert_eq!(number(&actual[0], "_questProgress").unwrap(), 1234);
        let known: BTreeMap<_, _> = entries(&actual[0])
            .unwrap()
            .iter()
            .map(|e| (number(e, "_key").unwrap(), number(e, "_level").unwrap()))
            .collect();
        for (key, level) in &rows {
            assert!(known[&u64::from(*key)] >= u64::from(*level));
        }
        let (again, repeat) = transform(&candidate).unwrap();
        assert_eq!(repeat.already_complete, 3939);
        assert_eq!(candidate, again);
        if rows.len() == 3939 {
            assert_eq!(raw, candidate);
        }
    }
}
#[test]
fn preserves_higher_levels_and_rejects_duplicates_corruption_and_stale_pairs() {
    let first = targets().unwrap()[0];
    let raw = fixture(&[(first.0, first.1 + 3)], false);
    let (candidate, _) = transform(&raw).unwrap();
    let (_, _, actual) = decode(&candidate).unwrap();
    assert_eq!(
        number(&entries(&actual[0]).unwrap()[0], "_level").unwrap(),
        u64::from(first.1 + 3)
    );
    assert!(transform(&fixture(&[(first.0, 1), (first.0, 1)], false)).is_err());
    assert!(transform(b"invalid").is_err());
    let lobby = fixture(&[], false);
    let review = plan(&raw, &lobby).unwrap();
    assert!(build(&raw, &fixture(&[(first.0, 1)], false), &review.plan_id).is_err());
    assert!(build(&candidate, &lobby, &review.plan_id).is_err());
    let mut damaged = raw.clone();
    let last = damaged.len() - 1;
    damaged[last] ^= 1;
    assert!(transform(&damaged).is_err());
}
#[test]
#[ignore = "private pre-unlock save; reads only and writes no installed files"]
fn private_production_candidate_matches_the_independently_verified_ingame_test() {
    let root =
        PathBuf::from(std::env::var_os("CD_KNOWLEDGE_FIXTURE").expect("private fixture directory"));
    let raw = std::fs::read(root.join("source.save")).unwrap();
    let lobby = std::fs::read(root.join("source-lobby.save")).unwrap();
    let review = plan(&raw, &lobby).unwrap();
    let (candidate, actual) = build(&raw, &lobby, &review.plan_id).unwrap();
    assert_eq!(actual, review);
    assert_eq!(review.summary.new_entries, 1966);
    assert_eq!(review.summary.upgraded_entries, 111);
    assert_eq!(
        candidate,
        std::fs::read(root.join("candidate.save")).unwrap()
    );
    assert_eq!(transform(&candidate).unwrap().0, candidate);
}
