//! BuffInfo reader 0x1414f98d0 and AdditionalUseResourceStatBuffData 0x141f351f0.
//! Every field is accounted for. Only resource amounts become editor fields.
use super::{Inspect, Value, bad, layouts};
use crate::{
    Result,
    mods::advanced::{Field, Record, reader::Reader},
};

impl Inspect<'_> {
    pub(super) fn additional_costs(&mut self) -> Result<()> {
        self.array("payload.skillKeys", 4)?;
        let count = self.number("payload.resources.count", 4, false)?;
        if count > 1000 {
            return Err(bad("additional resource limit"));
        }
        for index in 0..count {
            let prefix = format!("payload.resources[{index}]");
            // Shared UseResourceStat reader 0x141518720: 1+4+1+8+4+4.
            self.number(format!("{prefix}.statType"), 1, false)?;
            self.number(format!("{prefix}.statusInfo"), 4, false)?;
            if self.number(format!("{prefix}.isRegen"), 1, false)? > 1 {
                return Err(bad("invalid resource regeneration flag"));
            }
            self.number(format!("{prefix}.varyStatAmount"), 8, true)?;
            self.number(format!("{prefix}.increaseStatusInfo"), 4, false)?;
            self.number(format!("{prefix}.decreaseStatusInfo"), 4, false)?;
        }
        Ok(())
    }
}

pub(in crate::mods::advanced) struct Detail {
    pub row: Record,
    pub fields: Vec<Value>,
}

pub(in crate::mods::advanced) fn inspect(bytes: &[u8]) -> Result<Detail> {
    let mut i = Inspect {
        r: Reader::new(bytes),
        values: vec![],
    };
    let key = i.number("key", 4, false)? as u32;
    i.text("stringKey")?;
    let name = i.values.last().unwrap().value.clone();
    if i.number("isBlocked", 1, false)? > 1 {
        return Err(bad("invalid blocked flag"));
    }
    let count = i.number("buffDataList.count", 4, false)?;
    if count > 1000 {
        return Err(bad("buff info count limit"));
    }
    let mut costs = Vec::new();
    for index in 0..count {
        let prefix = format!("buffs[{index}]");
        let first = i.values.len();
        i.number("level", 4, false)?;
        match i.number("null", 1, false)? {
            0 => {
                let typ = i.number("type", 1, false)? as u8;
                let layout = layouts()
                    .iter()
                    .find(|l| l.type_id == typ)
                    .ok_or_else(|| bad("unsupported buff info type"))?;
                i.common()?;
                let payload_first = i.values.len();
                i.tail(layout)?;
                if typ == 114 {
                    let payload = &i.values[payload_first..];
                    for field in payload
                        .iter()
                        .filter(|f| f.path.ends_with(".varyStatAmount"))
                    {
                        let resource = field.path.strip_suffix(".varyStatAmount").unwrap();
                        let find = |suffix: &str| -> Result<&str> {
                            payload
                                .iter()
                                .find(|f| f.path == format!("{resource}.{suffix}"))
                                .map(|f| f.value.as_str())
                                .ok_or_else(|| bad("missing resource field"))
                        };
                        let rule = match (find("statType")?, find("statusInfo")?) {
                            ("3", "1000026") => "stamina",
                            ("3", "1000027") => "spirit",
                            _ => continue,
                        };
                        costs.push(Field {
                            name: format!("{prefix}.{}", field.path),
                            value: field.value.clone(),
                            min: "-1000000000".into(),
                            max: "1000000000".into(),
                            rule: rule.into(),
                            offset: field.offset,
                            width: 8,
                            signed: true,
                        });
                    }
                }
            }
            1 => {}
            _ => return Err(bad("invalid null buff flag")),
        }
        for field in &mut i.values[first..] {
            field.path = format!("{prefix}.{}", field.path);
        }
    }
    i.number("minLevel", 4, false)?;
    i.number("maxLevel", 4, false)?;
    i.text("sequencerFileName")?;
    i.number("buffLevelCalculateType", 1, false)?;
    for field in ["uiTemplateName", "uiComponentName", "elementalStatusInfo"] {
        i.number(field, 4, false)?;
    }
    for field in [
        "isUseSkillInfoPatternDescription",
        "useCountingByGlobalTimer",
    ] {
        if i.number(field, 1, false)? > 1 {
            return Err(bad("invalid buff info boolean"));
        }
    }
    i.r.end()?;
    for field in &mut i.values {
        field.editable = false;
    }
    let mut row = Record::new("buffinfo", key, name, "resource", costs);
    row.category = "equipment".into();
    row.description = "Zusätzliche Eigenkosten von Ausrüstungs-Buffs. Gilt für die in diesem Buff referenzierten Skills. Positive Ressourcenwerte, Regenerationsflags und andere Buff-Effekte bleiben bei globaler Skalierung erhalten.".into();
    Ok(Detail {
        row,
        fields: i.values,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn fixture() -> Vec<u8> {
        let mut b = 42u32.to_le_bytes().to_vec();
        b.extend(4u32.to_le_bytes());
        b.extend(b"Cost");
        b.push(0);
        b.extend(2u32.to_le_bytes());
        b.extend(1u32.to_le_bytes());
        b.extend([0, 114]);
        b.extend([0; 102]);
        b.extend(2u32.to_le_bytes());
        b.extend(10u32.to_le_bytes());
        b.extend(20u32.to_le_bytes());
        b.extend(3u32.to_le_bytes());
        for (stat, amount, regen) in [
            (1000026u32, -1000i64, 0),
            (1000027, 25, 1),
            (1000000, -300, 0),
        ] {
            b.push(3);
            b.extend(stat.to_le_bytes());
            b.push(regen);
            b.extend(amount.to_le_bytes());
            b.extend([0; 8]);
        }
        b.extend(2u32.to_le_bytes());
        b.push(1); // null second entry
        b.extend(1u32.to_le_bytes());
        b.extend(2u32.to_le_bytes());
        b.extend([0; 4 + 1 + 12 + 2]);
        b
    }

    #[test]
    fn nested_cost_lists_preserve_types_flags_and_other_effects() {
        let b = fixture();
        let d = inspect(&b).unwrap();
        assert_eq!(d.row.fields.len(), 2);
        assert_eq!(d.row.fields[0].rule, "stamina");
        assert_eq!(d.row.fields[1].value, "25");
        assert_eq!(d.fields.iter().map(|f| f.bytes).sum::<usize>(), b.len());
        assert!(d.fields.iter().all(|f| !f.editable));
        for len in 0..b.len() {
            assert!(inspect(&b[..len]).is_err(), "truncation {len}");
        }
        let mut broken = b.clone();
        broken.push(0);
        assert!(inspect(&broken).is_err());
        for (path, value) in [
            ("buffs[0].null", 2),
            ("buffs[0].type", 255),
            ("buffs[0].payload.resources[0].isRegen", 2),
        ] {
            let mut broken = b.clone();
            let at = d.fields.iter().find(|f| f.path == path).unwrap().offset;
            broken[at] = value;
            assert!(inspect(&broken).is_err());
        }
    }
}
