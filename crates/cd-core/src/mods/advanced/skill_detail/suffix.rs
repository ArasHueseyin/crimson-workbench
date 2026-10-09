//! SkillInfo header/suffix inspection from pinned reader 0x141518830.
//! ResourceStat: 0x141518720; ResourceItem list: 0x14152cfe0.
//! Names describe serialized fields, not inferred gameplay units.
use super::Inspect;
use crate::Result;

impl Inspect<'_> {
    fn reference(&mut self, path: impl Into<String>, width: usize) -> Result<()> {
        self.number(path, width, false)?;
        self.values.last_mut().unwrap().kind = "reference";
        Ok(())
    }

    fn reference_list(&mut self, path: &str, width: usize) -> Result<()> {
        let count = self.r.count()?;
        self.r.at -= 4;
        self.number(format!("{path}.count"), 4, false)?;
        for index in 0..count {
            self.reference(format!("{path}[{index}]"), width)?;
        }
        Ok(())
    }

    fn resource_stats(&mut self, path: &str) -> Result<()> {
        let count = self.r.count()?;
        self.r.at -= 4;
        self.number(format!("{path}.count"), 4, false)?;
        for index in 0..count {
            let path = format!("{path}[{index}]");
            self.number(format!("{path}.statType"), 1, false)?;
            self.reference(format!("{path}.statusInfo"), 4)?;
            self.number(format!("{path}.isRegen"), 1, false)?;
            self.number(format!("{path}.varyStatAmount"), 8, true)?;
            self.reference(format!("{path}.increaseStatusInfo"), 4)?;
            self.reference(format!("{path}.decreaseStatusInfo"), 4)?;
        }
        Ok(())
    }

    pub(super) fn suffix(&mut self) -> Result<()> {
        self.reference("skillGroupKey", 4)?;
        self.reference("parentSkill", 4)?;
        self.number("learnLevel", 4, false)?;
        self.number("applyType", 1, false)?;
        self.reference("iconPath", 4)?;
        self.reference("needUpgradeItemInfo", 4)?;
        for name in ["needUpgradeItemCountGraph", "needUpgradeExperienceGraph"] {
            for part in ["val0", "val1", "val2"] {
                self.number(format!("{name}.{part}"), 8, true)?;
            }
            self.number(format!("{name}.curve"), 4, false)?;
        }
        self.reference_list("usableCharacterInfoList", 4)?;
        self.reference_list("usableCondition", 4)?;
        self.reference("learnKnowledgeInfo", 4)?;
        self.reference("factionInfo", 4)?;
        self.resource_stats("useResourceStatList")?;
        let count = self.r.count()?;
        self.r.at -= 4;
        self.number("useResourceItemList.count", 4, false)?;
        for index in 0..count {
            self.reference(format!("useResourceItemList[{index}].itemInfo"), 4)?;
            // Width is known; signedness/valid edit range is not established.
            self.raw(&format!("useResourceItemList[{index}].useItemCount"), 8)?;
        }
        self.resource_stats("useDriverResourceStatList")?;
        self.number("useBatteryStat", 8, true)?;
        for name in [
            "isUiUseAllowed",
            "isLearnUseArtifact",
            "allowSkillWithLowResource",
            "isUseChildPatternDescriptionBuffData",
            "isNoAlert",
            "damageType",
            "uiType",
        ] {
            self.number(name, 1, false)?;
        }
        self.reference_list("reserveSlotInfoList", 4)?;
        self.number("maxLevel", 4, false)?;
        self.reference_list("skillGroupKeyList", 2)?;
        self.number("buffSustainFlag", 4, false)?;
        self.text("devSkillName")?;
        self.text("devSkillDesc")?;
        self.reference("videoPath", 4)?;
        Ok(())
    }
}
