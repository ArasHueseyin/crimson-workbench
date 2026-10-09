//! Named, read-only SummonCharacterData fields from the pinned EXE reader.
// 0x1414ed330 + nested 0x1414ecd40; buff suffix 0x141f2d790.
// A known width/name does not establish editable numeric semantics or units.
use super::{Inspect, bad};
use crate::Result;

impl Inspect<'_> {
    pub(super) fn summon(&mut self) -> Result<()> {
        let count = self.number("payload.summon.selectDataList.count", 4, false)?;
        if count > 1000 {
            return Err(bad("summon select data limit"));
        }
        for index in 0..count {
            self.number(
                format!("payload.summon.selectDataList[{index}].characterGroupInfo"),
                2,
                false,
            )?;
            self.number(
                format!("payload.summon.selectDataList[{index}].regionInfo"),
                2,
                false,
            )?;
        }
        // This build contains no embedded player condition in these buffs.
        // Reject an unimplemented polymorphic condition rather than lose alignment.
        if self.number("payload.summon.playerCondition.present", 1, false)? != 0 {
            return Err(bad("unsupported summon player condition"));
        }
        self.number("payload.summon.characterKey", 4, false)?;
        self.number("payload.summon.characterGroupKey", 2, false)?;
        self.raw("payload.summon.position", 12)?;
        self.raw("payload.summon.yaw", 4)?;
        self.number("payload.summon.isDead", 1, false)?;
        self.text("payload.summon.appearanceName")?;
        self.number("payload.summon.summonDestroyType", 1, false)?;
        self.number("payload.summon.rotateType", 1, false)?;
        self.number("payload.summon.summonSpawnType", 1, false)?;
        self.number("payload.summon.deadLimitTime", 8, false)?;
        for index in 0..4 {
            self.number(
                format!("payload.summon.summonTagNameHash[{index}]"),
                4,
                false,
            )?;
        }
        self.number("payload.summon.summoneeCatchType", 1, false)?;
        self.number("payload.summon.summoneeDockingType", 1, false)?;
        self.text("payload.summon.summonerSocketName")?;
        self.text("payload.summon.summoneeSocketName")?;
        self.number("payload.summon.summoneeIsBagDocking", 1, false)?;
        self.number("payload.summon.summoneeEnableCollision", 1, false)?;
        self.number("payload.summon.summoneeActionNameHash", 4, false)?;
        for index in 0..4 {
            self.number(
                format!("payload.summon.summoneeDockingTagHashList[{index}]"),
                4,
                false,
            )?;
        }
        self.number("payload.summon.spawnReason", 4, false)?;
        self.number("payload.summon.summoneeCatchPresetNameHash", 4, false)?;
        self.number("payload.summon.specialType", 1, false)?;
        self.number(
            "payload.summon.minigameCharacterOverrideDataIndex",
            4,
            false,
        )?;
        self.number(
            "payload.summon.terrainRegionAutoSpawnData.infoKey",
            4,
            false,
        )?;
        self.text("payload.summon.terrainRegionAutoSpawnData.tag")?;
        self.number(
            "payload.summon.terrainRegionAutoSpawnData.nearSpawnPosition",
            1,
            false,
        )?;
        self.number(
            "payload.summon.terrainRegionAutoSpawnData.excludedCharacterGroupKey",
            2,
            false,
        )?;
        self.number(
            "payload.summon.terrainRegionAutoSpawnData.spawnableCheckInterval",
            8,
            false,
        )?;
        self.text("payload.summon.factionSpawnTag")?;
        self.number("payload.summon.spawnPercent", 8, false)?;
        self.number("payload.summon.fromOperationReward", 1, false)?;
        self.number("payload.summon.interactionKey", 4, false)?;
        self.text("payload.summon.interactionPivotKey")?;
        self.number("payload.summon.isCaged", 1, false)?;
        self.number(
            "payload.summon.isLogoutWhenSummonerSequencerControl",
            1,
            false,
        )?;
        self.number("payload.summon.isLockedOnlySummon", 1, false)?;
        self.number("payload.summon.ignoreSummonerObstacle", 1, false)?;
        self.number("payload.summon.findValidPositionType", 1, false)?;
        self.raw("payload.summon.findValidHeightRange", 8)?;
        self.number("payload.summon.summonFormationKey", 4, false)?;
        self.number("payload.summon.summonFormationLeaderActorKey", 4, false)?;
        self.number("payload.summon.summonerEquipSlotNo", 2, false)?;
        self.number("payload.summon.isCloneActor", 1, false)?;
        self.number("payload.summon.isHideActorHelm", 1, false)?;
        self.number("payload.summon.isUseOwnerStat", 1, false)?;
        self.number("payload.summon.logoutDistanceType", 1, false)?;
        self.number("payload.summon.summonAllyGroupType", 1, false)?;
        self.number("payload.mem_384", 1, false)?;
        self.number("payload.mem_388", 4, false)?;
        self.number("payload.mem_392", 4, false)?;
        Ok(())
    }
}
