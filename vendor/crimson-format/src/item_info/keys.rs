// Adapted from crimson-rs b1b687bec8e38097dcb3a6dd39e5e96128d4f9c0.
// Copyright (c) 2026 Tommy Tran. MIT; see LICENSE and docs/FORMAT_PORT.md.
use crate::binary::{BinaryRead, BinaryReadTracked, BinaryWrite, FieldRange};

use std::io::{self, Write};

macro_rules! define_key {
    ($name:ident, $inner:ty) => {
        #[derive(Debug, Clone, Copy, PartialEq, Eq)]
        pub struct $name(pub $inner);

        impl<'a> BinaryRead<'a> for $name {
            fn read_from(data: &'a [u8], offset: &mut usize) -> io::Result<Self> {
                <$inner>::read_from(data, offset).map($name)
            }
        }

        impl<'a> BinaryReadTracked<'a> for $name {
            fn read_tracked(
                data: &'a [u8],
                offset: &mut usize,
                path: &mut String,
                ranges: &mut Vec<FieldRange>,
            ) -> io::Result<Self> {
                let start = *offset;
                let value = Self::read_from(data, offset)?;
                ranges.push(FieldRange {
                    path: path.clone(),
                    start,
                    end: *offset,
                    ty: stringify!($name),
                });
                Ok(value)
            }
        }

        impl BinaryWrite for $name {
            fn write_to(&self, w: &mut dyn Write) -> io::Result<()> {
                self.0.write_to(w)
            }
        }
    };
}

// u32 keys
define_key!(ItemKey, u32);
define_key!(BuffKey, u32);
define_key!(CharacterKey, u32);
define_key!(ConditionKey, u32);
define_key!(EffectKey, u32);
define_key!(EquipTypeKey, u32);
define_key!(GameAdviceInfoKey, u32);
define_key!(GimmickInfoKey, u32);
define_key!(ItemUseKey, u32);
define_key!(KnowledgeKey, u32);
define_key!(LocalStringInfoKey, u32);
define_key!(MaterialMatchKey, u32);
define_key!(MissionKey, u32);
define_key!(MultiChangeKey, u32);
define_key!(ReserveSlotKey, u32);
define_key!(SkillKey, u32);
define_key!(StatusKey, u32);
define_key!(StringInfoKey, u32);
define_key!(TribeInfoKey, u32);

// u16 keys
define_key!(CategoryKey, u16);
define_key!(CharacterGroupKey, u16);
define_key!(CraftToolKey, u16);
define_key!(InventoryKey, u16);
define_key!(ItemGroupKey, u16);
define_key!(SkillGroupKey, u16);
