//! Bounded format facade for Crimson Workbench.
//! MIT port provenance: docs/FORMAT_PORT.md. Serialization returns bytes;
//! only the guarded mount service may replace a user-selected save pair.
mod archive;
#[allow(dead_code)]
mod binary;
mod crypto;
#[allow(dead_code)]
mod item_info;
pub mod item_mods;
mod items;
mod localization;
pub mod overlay;
pub mod save;
pub use archive::{Archive, ArchiveEntry, ArchiveGroup};
pub use items::{ItemRecord, ItemTable, LocalizableText, RawField, RawRecord, RawTable};
pub use localization::{Paloc, PalocEntry};
pub fn checksum(bytes: &[u8]) -> u32 {
    crypto::checksum::calculate_checksum(bytes)
}
pub const MAX_FILE_BYTES: usize = 128 * 1024 * 1024;
pub(crate) fn invalid(message: impl Into<String>) -> std::io::Error {
    std::io::Error::new(std::io::ErrorKind::InvalidData, message.into())
}
