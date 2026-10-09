//! Crimson Workbench Phase 1: read-only game access and guarded local output.
//!
//! File changes are guarded; mount registration edits only a selected save pair.
pub mod apply;
pub mod browser;
pub mod config;
pub mod crafting;
pub mod extra_sockets;
pub mod extra_sockets_candidates;
mod icons;
pub mod mods;
pub mod mounts;
mod search;
pub use icons::IconResult;
mod data;
pub mod discovery;
pub mod fingerprint;
mod index;
pub mod installation;
pub mod item_groups;
pub mod knowledge;
pub mod paths;
pub mod tables;
mod workspace;

pub use data::{
    AbyssStoneDetail, Error, GameData, Language, Result, RoundtripCheck, RoundtripReport,
    TableStatus, supported_languages,
};
pub use workspace::{
    DiscoveryReport, IndexBuildInfo, Workspace, discover_project, output_policy, project_save_root,
    select_game, write_generated,
};

pub use index::{IndexError, IndexIdentity, IndexedItem, SearchPage, SearchQuery};
