fn main() {
    if std::env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("macos") {
        for path in [
            "../../runtime/macos-items/src",
            "../../runtime/macos-items/Makefile",
            "../../.local/macos-items/libCrimsonItems.dylib",
        ] {
            println!("cargo:rerun-if-changed={path}");
        }
        let status = std::process::Command::new("make")
            .args(["-B", "-C", "../../runtime/macos-items", "all"])
            .status()
            .expect("Mac item module build could not start");
        assert!(status.success(), "Mac item module build failed");
        let hash = std::process::Command::new("/usr/bin/shasum")
            .args([
                "-a",
                "256",
                "../../.local/macos-items/libCrimsonItems.dylib",
            ])
            .output()
            .expect("Mac item module hashing could not start");
        assert!(hash.status.success(), "Mac item module hashing failed");
        let output = String::from_utf8(hash.stdout).expect("Invalid module hash output");
        let digest = output
            .split_whitespace()
            .next()
            .expect("Missing module hash");
        assert!(digest.len() == 64 && digest.bytes().all(|b| b.is_ascii_hexdigit()));
        println!("cargo:rustc-env=CRIMSON_MAC_ITEMS_SHA256={digest}");
    }
    let manifest = tauri_build::AppManifest::new().commands(&[
        "bootstrap",
        "marketplace_mod_status",
        "installed_mods",
        "marketplace_game_running",
        "marketplace_mod_change",
        "marketplace_runtime_setting",
        "marketplace_player_jump",
        "marketplace_player_speed",
        "marketplace_consumables",
        "marketplace_weapon_effects",
        "contribution_preview",
        "contribution_apply",
        "knowledge_unlock_preview",
        "knowledge_unlock_apply",
        "map_fog_preview",
        "map_fog_apply",
        "save_settings",
        "extra_sockets_snapshot",
        "extra_socket_set",
        "extra_socket_candidates",
        "extra_socket_add",
        "live_status",
        "spawn_launch",
        "spawn_status",
        "spawn_grant",
        "spawn_cancel",
        "world_status",
        "world_change",
        "photo_status",
        "photo_change",
        "teleport_status",
        "teleport_change",
        "mount_catalog",
        "mount_search",
        "mount_icon",
        "knowledge_snapshot",
        "mount_register",
        "mount_live_grant",
        "mount_live_status",
        "skill_unlock_preview",
        "skill_unlock_apply",
        "foreign_preview",
        "foreign_confirm",
        "foreign_revoke",
        "live_setup_preview",
        "live_update_preview",
        "live_preview",
        "live_start",
        "live_job_status",
        "live_cancel",
        "open_catalog",
        "search_items",
        "item_detail",
        "item_icon",
        "export_item",
        "craft_info",
        "craft_plan",
        "craft_item",
        "mod_info",
        "advanced_item",
        "abyss_stone_details",
        "advanced_skill",
        "mod_preview",
        "mod_export",
        "mod_rehearse",
        "rehearsal_list",
        "rehearsal_review",
        "rehearsal_restore",
        "baseline_catalog",
        "baseline_preview",
        "baseline_capture",
        "baseline_inspect",
        "installation_check",
        "installation_audit_start",
        "installation_audit_status",
        "installation_audit_cancel",
        "installation_audit_export",
    ]);
    tauri_build::try_build(tauri_build::Attributes::new().app_manifest(manifest))
        .expect("Tauri build failed");
}
