mod audit_service;
mod live_service;
mod marketplace_service;
mod native_pipe;
mod photo_service;
mod service;
mod settings;
mod spawn_service;
mod teleport_service;
mod world_service;
use cd_core::browser::{BrowserPage, BrowserQuery};
use service::{AppError, AppService, Bootstrap, Catalog, Result, SharedService};
use std::sync::Arc;
use tauri::{Manager, State};

async fn blocking<T: Send + 'static>(f: impl FnOnce() -> Result<T> + Send + 'static) -> Result<T> {
    tauri::async_runtime::spawn_blocking(f)
        .await
        .map_err(|e| AppError {
            code: "internal",
            message: e.to_string(),
        })?
}
#[tauri::command]
async fn bootstrap(state: State<'_, SharedService>) -> Result<Bootstrap> {
    let state = Arc::clone(&state);
    blocking(move || state.bootstrap()).await
}
#[tauri::command]
async fn marketplace_game_running(state: State<'_, SharedService>, session: u64) -> Result<bool> {
    let state = Arc::clone(&state);
    blocking(move || {
        state.with_session(session, |_| {
            cd_core::apply::game_running().map_err(cd_core::Error::from)
        })
    })
    .await
}
#[tauri::command]
async fn marketplace_mod_status(
    app: tauri::AppHandle,
    state: State<'_, SharedService>,
    session: u64,
) -> Result<marketplace_service::Snapshot> {
    let resources = app.path().resource_dir().map_err(|e| AppError {
        code: "marketplace_mod",
        message: e.to_string(),
    })?;
    let state = Arc::clone(&state);
    blocking(move || {
        let game = state.with_session(session, |s| Ok(s.game_root().to_path_buf()))?;
        marketplace_service::invoke(&resources, &game, None)
    })
    .await
}
#[tauri::command]
async fn marketplace_mod_change(
    app: tauri::AppHandle,
    state: State<'_, SharedService>,
    session: u64,
    id: String,
    enabled: bool,
) -> Result<marketplace_service::Snapshot> {
    let resources = app.path().resource_dir().map_err(|e| AppError {
        code: "marketplace_mod",
        message: e.to_string(),
    })?;
    let state = Arc::clone(&state);
    blocking(move || {
        let game = state.with_session(session, |s| Ok(s.game_root().to_path_buf()))?;
        marketplace_service::invoke(&resources, &game, Some((&id, enabled)))
    })
    .await
}
#[tauri::command]
async fn marketplace_runtime_setting(
    app: tauri::AppHandle,
    state: State<'_, SharedService>,
    session: u64,
    id: cd_core::config::runtime_options::RuntimeOption,
    value: Option<u32>,
) -> Result<cd_core::config::runtime_options::Snapshot> {
    let directory = app.path().app_local_data_dir().map_err(|e| AppError {
        code: "marketplace_mod",
        message: e.to_string(),
    })?;
    let state = Arc::clone(&state);
    blocking(move || {
        state.with_session(session, |s| {
            let version = s.info().exe_version;
            Ok(match value {
                Some(value) => cd_core::config::runtime_options::save(
                    &directory,
                    version.as_deref(),
                    id,
                    value,
                )?,
                None => {
                    cd_core::config::runtime_options::snapshot(&directory, version.as_deref(), id)?
                }
            })
        })
    })
    .await
}
#[tauri::command]
async fn marketplace_player_jump(
    app: tauri::AppHandle,
    state: State<'_, SharedService>,
    session: u64,
    height_percent: Option<u32>,
) -> Result<cd_core::config::player_jump::Snapshot> {
    let directory = app.path().app_local_data_dir().map_err(|e| AppError {
        code: "marketplace_mod",
        message: e.to_string(),
    })?;
    let state = Arc::clone(&state);
    blocking(move || {
        state.with_session(session, |s| {
            let version = s.info().exe_version;
            Ok(match height_percent {
                Some(percent) => {
                    cd_core::config::player_jump::save(&directory, version.as_deref(), percent)?
                }
                None => cd_core::config::player_jump::snapshot(&directory, version.as_deref())?,
            })
        })
    })
    .await
}
#[tauri::command]
async fn marketplace_player_speed(
    app: tauri::AppHandle,
    state: State<'_, SharedService>,
    session: u64,
    speed_percent: Option<u32>,
) -> Result<cd_core::config::player_speed::Snapshot> {
    let directory = app.path().app_local_data_dir().map_err(|e| AppError {
        code: "marketplace_mod",
        message: e.to_string(),
    })?;
    let state = Arc::clone(&state);
    blocking(move || {
        state.with_session(session, |s| {
            let version = s.info().exe_version;
            Ok(match speed_percent {
                Some(percent) => {
                    cd_core::config::player_speed::save(&directory, version.as_deref(), percent)?
                }
                None => cd_core::config::player_speed::snapshot(&directory, version.as_deref())?,
            })
        })
    })
    .await
}
#[tauri::command]
async fn marketplace_consumables(
    app: tauri::AppHandle,
    state: State<'_, SharedService>,
    session: u64,
    item_keys: Option<Vec<u32>>,
) -> Result<cd_core::config::preserve_consumables::Snapshot> {
    let directory = app.path().app_local_data_dir().map_err(|e| AppError {
        code: "marketplace_mod",
        message: e.to_string(),
    })?;
    let state = Arc::clone(&state);
    blocking(move || {
        state.with_session(session, |s| {
            let version = s.info().exe_version;
            let choices = s.consumable_choices()?;
            Ok(match item_keys {
                Some(keys) => cd_core::config::preserve_consumables::save(
                    &directory,
                    version.as_deref(),
                    &keys,
                    choices,
                )?,
                None => cd_core::config::preserve_consumables::snapshot(
                    &directory,
                    version.as_deref(),
                    choices,
                )?,
            })
        })
    })
    .await
}
#[tauri::command]
async fn contribution_preview(
    state: State<'_, SharedService>,
    session: u64,
    save: Option<String>,
    edits: Vec<cd_core::contribution_values::Edit>,
) -> Result<cd_core::contribution_action::Preview> {
    let state = Arc::clone(&state);
    blocking(move || state.contribution_preview(session, save.as_deref(), &edits)).await
}
#[tauri::command]
async fn map_fog_preview(
    state: State<'_, Arc<AppService>>,
    session: u64,
    save: Option<String>,
) -> Result<cd_core::map_fog_action::Preview> {
    let state = state.inner().clone();
    blocking(move || state.map_fog_preview(session, save.as_deref())).await
}
#[tauri::command]
async fn map_fog_apply(
    state: State<'_, Arc<AppService>>,
    session: u64,
    request: cd_core::map_fog_action::Request,
) -> Result<cd_core::map_fog_action::Receipt> {
    let state = state.inner().clone();
    blocking(move || state.map_fog_apply(session, &request)).await
}
#[tauri::command]
async fn contribution_apply(
    state: State<'_, SharedService>,
    session: u64,
    request: cd_core::contribution_action::Request,
) -> Result<cd_core::contribution_action::Receipt> {
    let state = Arc::clone(&state);
    blocking(move || state.contribution_apply(session, &request)).await
}
#[tauri::command]
async fn knowledge_unlock_preview(
    state: State<'_, Arc<AppService>>,
    session: u64,
    save: Option<String>,
) -> Result<cd_core::knowledge_unlock::Preview> {
    let state = state.inner().clone();
    blocking(move || state.knowledge_unlock_preview(session, save.as_deref())).await
}
#[tauri::command]
async fn knowledge_unlock_apply(
    state: State<'_, Arc<AppService>>,
    session: u64,
    request: cd_core::knowledge_unlock::Request,
) -> Result<cd_core::knowledge_unlock::Receipt> {
    let state = state.inner().clone();
    blocking(move || state.knowledge_unlock_apply(session, &request)).await
}
#[tauri::command]
async fn skill_unlock_preview(
    state: State<'_, SharedService>,
    session: u64,
    save: Option<String>,
    abilities: Option<std::collections::BTreeSet<u32>>,
) -> Result<cd_core::skill_unlock::Preview> {
    let state = Arc::clone(&state);
    blocking(move || state.skill_unlock_preview(session, save.as_deref(), abilities.as_ref())).await
}
#[tauri::command]
async fn skill_unlock_apply(
    state: State<'_, SharedService>,
    session: u64,
    request: cd_core::skill_unlock::Request,
) -> Result<cd_core::skill_unlock::Receipt> {
    let state = Arc::clone(&state);
    blocking(move || state.skill_unlock_apply(session, &request)).await
}
#[tauri::command]
async fn save_settings(
    state: State<'_, SharedService>,
    settings: cd_core::config::Preferences,
) -> Result<Bootstrap> {
    let state = Arc::clone(&state);
    blocking(move || state.save_settings(settings)).await
}
#[tauri::command]
async fn open_catalog(
    state: State<'_, SharedService>,
    game: Option<String>,
    language: String,
) -> Result<Catalog> {
    let state = Arc::clone(&state);
    blocking(move || state.open(game, language)).await
}
#[tauri::command]
async fn search_items(
    state: State<'_, SharedService>,
    session: u64,
    query: BrowserQuery,
) -> Result<BrowserPage> {
    let state = Arc::clone(&state);
    blocking(move || state.search(session, query)).await
}
#[tauri::command]
async fn extra_sockets_snapshot(
    state: State<'_, SharedService>,
    session: u64,
) -> Result<cd_core::extra_sockets::Snapshot> {
    let state = Arc::clone(&state);
    blocking(move || state.with_session(session, |s| s.extra_sockets())).await
}
#[tauri::command]
async fn extra_socket_set(
    state: State<'_, SharedService>,
    session: u64,
    request: cd_core::extra_sockets::Request,
) -> Result<cd_core::extra_sockets::Receipt> {
    let state = Arc::clone(&state);
    blocking(move || state.set_extra_socket(session, &request)).await
}
#[tauri::command]
async fn extra_socket_candidates(
    state: State<'_, SharedService>,
    session: u64,
    save: Option<String>,
) -> Result<cd_core::extra_sockets_candidates::Candidates> {
    let state = Arc::clone(&state);
    blocking(move || state.with_session(session, |s| s.extra_socket_candidates(save.as_deref())))
        .await
}
#[tauri::command]
async fn extra_socket_add(
    state: State<'_, SharedService>,
    session: u64,
    request: cd_core::extra_sockets_candidates::AddRequest,
) -> Result<cd_core::extra_sockets::Receipt> {
    let state = Arc::clone(&state);
    blocking(move || state.add_extra_sockets(session, &request)).await
}
#[tauri::command]
async fn knowledge_snapshot(
    state: State<'_, SharedService>,
    session: u64,
    save: Option<String>,
) -> Result<cd_core::knowledge::Snapshot> {
    let state = Arc::clone(&state);
    blocking(move || state.with_session(session, |s| s.knowledge(save.as_deref()))).await
}
#[tauri::command]
async fn mount_search(
    state: State<'_, SharedService>,
    session: u64,
    text: String,
    regex: bool,
) -> Result<Vec<u32>> {
    let state = Arc::clone(&state);
    blocking(move || state.with_session(session, |s| s.mount_search(&text, regex))).await
}
#[tauri::command]
async fn mount_catalog(
    state: State<'_, SharedService>,
    session: u64,
    save: Option<String>,
) -> Result<cd_core::mounts::Snapshot> {
    let state = Arc::clone(&state);
    blocking(move || state.mount_catalog(session, save.as_deref())).await
}
#[tauri::command]
async fn mount_icon(
    state: State<'_, SharedService>,
    session: u64,
    key: u32,
) -> Result<cd_core::IconResult> {
    let state = Arc::clone(&state);
    blocking(move || state.with_session(session, |s| s.mount_icon(key))).await
}
#[tauri::command]
async fn mount_register(
    state: State<'_, SharedService>,
    session: u64,
    request: cd_core::mounts::Request,
) -> Result<cd_core::mounts::Receipt> {
    let state = Arc::clone(&state);
    blocking(move || state.mount_register(session, &request)).await
}
#[tauri::command]
async fn item_detail(
    state: State<'_, SharedService>,
    session: u64,
    key: u32,
) -> Result<serde_json::Value> {
    let state = Arc::clone(&state);
    blocking(move || state.with_session(session, |s| s.item(key))).await
}
#[tauri::command]
async fn item_icon(
    state: State<'_, SharedService>,
    session: u64,
    key: u32,
) -> Result<cd_core::IconResult> {
    let state = Arc::clone(&state);
    blocking(move || state.with_session(session, |s| s.icon(key))).await
}
#[tauri::command]
async fn export_item(state: State<'_, SharedService>, session: u64, key: u32) -> Result<String> {
    let state = Arc::clone(&state);
    blocking(move || {
        state
            .with_session(session, |s| s.export_item(key))
            .map(|p| p.to_string_lossy().into_owned())
    })
    .await
}
#[tauri::command]
async fn craft_info(
    state: State<'_, SharedService>,
    session: u64,
) -> Result<cd_core::crafting::CraftInfo> {
    let state = Arc::clone(&state);
    blocking(move || state.with_session(session, |s| s.with_crafting(|c| Ok(c.info())))).await
}
#[tauri::command]
async fn craft_plan(
    state: State<'_, SharedService>,
    session: u64,
    request: cd_core::crafting::PlanRequest,
) -> Result<cd_core::crafting::Plan> {
    let state = Arc::clone(&state);
    blocking(move || state.with_session(session, |s| s.with_crafting(|c| c.plan(request)))).await
}
#[tauri::command]
async fn craft_item(
    state: State<'_, SharedService>,
    session: u64,
    key: u32,
) -> Result<serde_json::Value> {
    let state = Arc::clone(&state);
    blocking(move || {
        state.with_session(session, |s| {
            s.with_crafting(|c| {
                Ok(serde_json::json!({"recipes":c.recipes_for(key),"used_in":c.used_in(key)}))
            })
        })
    })
    .await
}
#[tauri::command]
async fn mod_info(state: State<'_, SharedService>, session: u64) -> Result<cd_core::mods::ModInfo> {
    let state = Arc::clone(&state);
    blocking(move || state.with_session(session, |s| s.with_mods(|c| Ok(c.info())))).await
}
#[tauri::command]
async fn marketplace_weapon_effects(
    state: State<'_, SharedService>,
    session: u64,
) -> Result<Vec<cd_core::mods::marketplace::EffectSource>> {
    let state = Arc::clone(&state);
    blocking(move || {
        state.with_session(session, |s| s.with_mods(|c| c.marketplace_weapon_effects()))
    })
    .await
}
#[tauri::command]
async fn advanced_skill(
    state: State<'_, SharedService>,
    session: u64,
    key: u32,
) -> Result<cd_core::mods::advanced::skill_detail::SkillDetail> {
    let state = Arc::clone(&state);
    blocking(move || state.with_session(session, |s| s.with_mods(|c| c.advanced_skill(key)))).await
}
#[tauri::command]
async fn advanced_item(
    state: State<'_, SharedService>,
    session: u64,
    key: u32,
) -> Result<cd_core::mods::advanced::ItemDetail> {
    let state = Arc::clone(&state);
    blocking(move || state.with_session(session, |s| s.with_mods(|c| c.advanced_item(key)))).await
}
#[tauri::command]
async fn abyss_stone_details(
    state: State<'_, SharedService>,
    session: u64,
) -> Result<Vec<cd_core::AbyssStoneDetail>> {
    let state = Arc::clone(&state);
    blocking(move || state.with_session(session, |s| s.abyss_stone_details())).await
}
#[tauri::command]
async fn mod_preview(
    state: State<'_, SharedService>,
    session: u64,
    request: cd_core::mods::ModRequest,
) -> Result<cd_core::mods::Preview> {
    let state = Arc::clone(&state);
    blocking(move || {
        state.with_session(session, |s| s.with_mods(|c| Ok(c.build(request)?.preview)))
    })
    .await
}
#[tauri::command]
async fn mod_export(
    state: State<'_, SharedService>,
    session: u64,
    request: cd_core::mods::ModRequest,
    plan_id: String,
) -> Result<String> {
    let state = Arc::clone(&state);
    blocking(move || {
        state
            .with_session(session, |s| s.mod_export(request, &plan_id))
            .map(|p| p.to_string_lossy().into_owned())
    })
    .await
}
#[tauri::command]
async fn mod_rehearse(
    state: State<'_, SharedService>,
    session: u64,
    request: cd_core::mods::ModRequest,
    plan_id: String,
) -> Result<cd_core::apply::Rehearsal> {
    let state = Arc::clone(&state);
    blocking(move || state.with_session(session, |s| s.mod_rehearse(request, &plan_id))).await
}
pub fn run() {
    let hidden = std::env::args().any(|a| a == "--background-test");
    tauri::Builder::default()
        .manage(Arc::new(AppService::new(service::project_root())))
        .invoke_handler(tauri::generate_handler![
            bootstrap,
            marketplace_mod_status,
            marketplace_game_running,
            marketplace_mod_change,
            marketplace_runtime_setting,
            marketplace_player_jump,
            marketplace_player_speed,
            marketplace_consumables,
            save_settings,
            extra_sockets_snapshot,
            extra_socket_set,
            extra_socket_candidates,
            extra_socket_add,
            live_status,
            spawn_launch,
            spawn_status,
            spawn_grant,
            spawn_cancel,
            world_status,
            world_change,
            photo_status,
            photo_change,
            teleport_status,
            teleport_change,
            mount_catalog,
            mount_search,
            mount_icon,
            knowledge_snapshot,
            mount_register,
            contribution_preview,
            contribution_apply,
            map_fog_preview,
            map_fog_apply,
            knowledge_unlock_preview,
            knowledge_unlock_apply,
            skill_unlock_preview,
            skill_unlock_apply,
            foreign_preview,
            foreign_confirm,
            foreign_revoke,
            live_setup_preview,
            live_update_preview,
            live_preview,
            live_start,
            live_job_status,
            live_cancel,
            open_catalog,
            search_items,
            item_detail,
            item_icon,
            export_item,
            craft_info,
            craft_plan,
            craft_item,
            mod_info,
            marketplace_weapon_effects,
            advanced_item,
            abyss_stone_details,
            advanced_skill,
            mod_preview,
            mod_export,
            mod_rehearse,
            rehearsal_list,
            rehearsal_review,
            rehearsal_restore,
            baseline_catalog,
            baseline_preview,
            baseline_capture,
            baseline_inspect,
            installation_check,
            installation_audit_start,
            installation_audit_status,
            installation_audit_cancel,
            installation_audit_export
        ])
        .setup(move |app| {
            if !hidden && let Some(window) = app.get_webview_window("main") {
                window.show()?;
            }
            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("Desktop runtime failed");
}
#[tauri::command]
async fn baseline_catalog(
    state: State<'_, SharedService>,
    session: u64,
) -> Result<cd_core::installation::baseline::Catalog> {
    let state = Arc::clone(&state);
    blocking(move || state.baselines(session)).await
}
#[tauri::command]
async fn baseline_preview(
    state: State<'_, SharedService>,
    session: u64,
    report_name: String,
) -> Result<cd_core::installation::baseline::Preview> {
    let state = Arc::clone(&state);
    blocking(move || state.baseline_preview(session, &report_name)).await
}
#[tauri::command]
async fn baseline_capture(
    state: State<'_, SharedService>,
    session: u64,
    report_name: String,
    review_id: String,
) -> Result<cd_core::installation::baseline::Saved> {
    let state = Arc::clone(&state);
    blocking(move || state.baseline_capture(session, &report_name, &review_id)).await
}
#[tauri::command]
async fn baseline_inspect(
    state: State<'_, SharedService>,
    session: u64,
    id: String,
) -> Result<cd_core::installation::baseline::Saved> {
    let state = Arc::clone(&state);
    blocking(move || state.baseline_inspect(session, &id)).await
}
#[tauri::command]
async fn rehearsal_list(
    state: State<'_, SharedService>,
    session: u64,
) -> Result<cd_core::apply::recovery::Listing> {
    let state = Arc::clone(&state);
    blocking(move || state.rehearsals(session)).await
}
#[tauri::command]
async fn rehearsal_review(
    state: State<'_, SharedService>,
    session: u64,
    directory: String,
) -> Result<cd_core::apply::recovery::Review> {
    let state = Arc::clone(&state);
    blocking(move || state.recovery_review(session, std::path::Path::new(&directory))).await
}
#[tauri::command]
async fn rehearsal_restore(
    state: State<'_, SharedService>,
    session: u64,
    directory: String,
    review_id: String,
) -> Result<cd_core::apply::RehearsalRecovery> {
    let state = Arc::clone(&state);
    blocking(move || state.recovery_restore(session, std::path::Path::new(&directory), &review_id))
        .await
}
#[tauri::command]
async fn installation_audit_start(
    state: State<'_, SharedService>,
    session: u64,
) -> Result<audit_service::Snapshot> {
    let state = Arc::clone(&state);
    blocking(move || state.audit_start(session)).await
}
#[tauri::command]
async fn installation_audit_status(
    state: State<'_, SharedService>,
    session: u64,
) -> Result<Option<audit_service::Snapshot>> {
    let state = Arc::clone(&state);
    blocking(move || state.audit_status(session)).await
}
#[tauri::command]
async fn installation_audit_cancel(
    state: State<'_, SharedService>,
    session: u64,
    id: u64,
) -> Result<()> {
    let state = Arc::clone(&state);
    blocking(move || state.audit_cancel(session, id)).await
}
#[tauri::command]
async fn installation_audit_export(
    state: State<'_, SharedService>,
    session: u64,
    id: u64,
) -> Result<String> {
    let state = Arc::clone(&state);
    blocking(move || state.audit_export(session, id)).await
}
#[tauri::command]
async fn installation_check(
    state: State<'_, SharedService>,
    session: u64,
) -> Result<cd_core::installation::Inventory> {
    let state = Arc::clone(&state);
    blocking(move || state.with_session(session, |s| s.installation_check())).await
}

#[tauri::command]
async fn live_status(
    state: State<'_, SharedService>,
    session: u64,
) -> Result<cd_core::apply::live::Status> {
    let state = Arc::clone(&state);
    blocking(move || state.live_status(session)).await
}

#[tauri::command]
async fn world_status(
    state: State<'_, SharedService>,
    session: u64,
) -> Result<world_service::Snapshot> {
    let state = Arc::clone(&state);
    blocking(move || {
        let game = state.with_session(session, |s| Ok(s.game_root().to_owned()))?;
        world_service::exchange(&game, None)
    })
    .await
}
#[tauri::command]
async fn world_change(
    state: State<'_, SharedService>,
    session: u64,
    request: world_service::ControlRequest,
) -> Result<world_service::Snapshot> {
    let state = Arc::clone(&state);
    blocking(move || {
        let (game, version) = state.with_session(session, |s| {
            Ok((s.game_root().to_owned(), s.info().exe_version))
        })?;
        if version.as_deref() != Some("1.0.0.2976") {
            return Err(AppError {
                code: "world_controls",
                message: "Live world controls require game version 1.0.0.2976.".into(),
            });
        }
        world_service::exchange(&game, Some(&request))
    })
    .await
}
#[tauri::command]
async fn photo_status(
    state: State<'_, SharedService>,
    session: u64,
) -> Result<photo_service::Snapshot> {
    let state = Arc::clone(&state);
    blocking(move || {
        let game = state.with_session(session, |s| Ok(s.game_root().to_owned()))?;
        photo_service::exchange(&game, None)
    })
    .await
}
#[tauri::command]
async fn photo_change(
    state: State<'_, SharedService>,
    session: u64,
    request: photo_service::ControlRequest,
) -> Result<photo_service::Snapshot> {
    let state = Arc::clone(&state);
    blocking(move || {
        let (game, version) = state.with_session(session, |s| {
            Ok((s.game_root().to_owned(), s.info().exe_version))
        })?;
        if version.as_deref() != Some("1.0.0.2976") {
            return Err(AppError {
                code: "photo_controls",
                message: "Photo controls require game version 1.0.0.2976.".into(),
            });
        }
        photo_service::exchange(&game, Some(&request))
    })
    .await
}
#[tauri::command]
async fn teleport_status(
    state: State<'_, SharedService>,
    session: u64,
) -> Result<teleport_service::Snapshot> {
    let state = Arc::clone(&state);
    blocking(move || {
        let game = state.with_session(session, |s| Ok(s.game_root().to_owned()))?;
        teleport_service::exchange(&game, None)
    })
    .await
}
#[tauri::command]
async fn teleport_change(
    state: State<'_, SharedService>,
    session: u64,
    request: teleport_service::ControlRequest,
) -> Result<teleport_service::Snapshot> {
    let state = Arc::clone(&state);
    blocking(move || {
        let (game, version) = state.with_session(session, |s| {
            Ok((s.game_root().to_owned(), s.info().exe_version))
        })?;
        if version.as_deref() != Some("1.0.0.2976") {
            return Err(AppError {
                code: "teleport_controls",
                message: "Teleport controls require game version 1.0.0.2976.".into(),
            });
        }
        teleport_service::exchange(&game, Some(&request))
    })
    .await
}
#[tauri::command]
async fn spawn_launch(
    app: tauri::AppHandle,
    state: State<'_, SharedService>,
    session: u64,
) -> Result<serde_json::Value> {
    #[cfg(target_os = "macos")]
    {
        let resources = app.path().resource_dir().map_err(|e| AppError {
            code: "live_items",
            message: e.to_string(),
        })?;
        let data = app.path().app_local_data_dir().map_err(|e| AppError {
            code: "live_items",
            message: e.to_string(),
        })?;
        let state = Arc::clone(&state);
        blocking(move || {
            let game = state.with_session(session, |s| Ok(s.game_root().to_owned()))?;
            let launch = spawn_service::macos::launch(&game, &resources, &data)?;
            serde_json::to_value(launch).map_err(|e| AppError {
                code: "live_items",
                message: e.to_string(),
            })
        })
        .await
    }
    #[cfg(not(target_os = "macos"))]
    {
        let _ = (app, state, session);
        Err(AppError {
            code: "live_items",
            message: "This launcher is available on macOS.".into(),
        })
    }
}
#[tauri::command]
async fn spawn_status(
    state: State<'_, SharedService>,
    session: u64,
    request: Option<spawn_service::GrantRequest>,
) -> Result<spawn_service::Snapshot> {
    let state = Arc::clone(&state);
    blocking(move || {
        state
            .with_session(session, |s| Ok(s.game_root().to_owned()))
            .and_then(|game| {
                spawn_service::exchange(
                    &game,
                    if request.is_some() { 3 } else { 1 },
                    request.as_ref(),
                )
            })
    })
    .await
}
#[tauri::command]
async fn spawn_grant(
    state: State<'_, SharedService>,
    session: u64,
    request: spawn_service::GrantRequest,
) -> Result<spawn_service::Snapshot> {
    let state = Arc::clone(&state);
    blocking(move || {
        state
            .with_session(session, |s| {
                s.validate_item_grant(request.key)?;
                Ok(s.game_root().to_owned())
            })
            .and_then(|game| spawn_service::exchange(&game, 2, Some(&request)))
    })
    .await
}
#[tauri::command]
async fn spawn_cancel(
    state: State<'_, SharedService>,
    session: u64,
    request: spawn_service::GrantRequest,
) -> Result<spawn_service::Snapshot> {
    let state = Arc::clone(&state);
    blocking(move || {
        state
            .with_session(session, |s| Ok(s.game_root().to_owned()))
            .and_then(|game| spawn_service::exchange(&game, 4, Some(&request)))
    })
    .await
}
#[tauri::command]
async fn live_setup_preview(
    state: State<'_, SharedService>,
    session: u64,
    baseline_id: String,
) -> Result<cd_core::apply::live::SetupPreview> {
    let state = Arc::clone(&state);
    blocking(move || state.live_setup_preview(session, &baseline_id)).await
}
#[tauri::command]
async fn live_update_preview(
    state: State<'_, SharedService>,
    session: u64,
    baseline_id: String,
) -> Result<cd_core::apply::live::updates::Review> {
    let state = Arc::clone(&state);
    blocking(move || state.live_update_preview(session, &baseline_id)).await
}
#[tauri::command]
async fn live_preview(
    state: State<'_, SharedService>,
    session: u64,
    request: cd_core::apply::live::Request,
) -> Result<cd_core::apply::live::Review> {
    let state = Arc::clone(&state);
    blocking(move || state.live_preview(session, &request)).await
}
#[tauri::command]
async fn live_start(
    state: State<'_, SharedService>,
    session: u64,
    operation: live_service::Operation,
) -> Result<live_service::Snapshot> {
    let state = Arc::clone(&state);
    blocking(move || state.live_start(session, operation)).await
}
#[tauri::command]
async fn live_job_status(
    state: State<'_, SharedService>,
    session: u64,
) -> Result<Option<live_service::Snapshot>> {
    let state = Arc::clone(&state);
    blocking(move || state.live_job_status(session)).await
}
#[tauri::command]
async fn live_cancel(state: State<'_, SharedService>, session: u64, id: u64) -> Result<()> {
    let state = Arc::clone(&state);
    blocking(move || state.live_cancel(session, id)).await
}

#[tauri::command]
async fn foreign_preview(
    state: State<'_, SharedService>,
    session: u64,
) -> Result<cd_core::apply::live::foreign::Inspection> {
    let state = Arc::clone(&state);
    blocking(move || state.foreign_preview(session)).await
}
#[tauri::command]
async fn foreign_confirm(
    state: State<'_, SharedService>,
    session: u64,
    review_id: String,
    preserve: bool,
) -> Result<cd_core::apply::live::foreign::Receipt> {
    let state = Arc::clone(&state);
    blocking(move || state.foreign_confirm(session, &review_id, preserve)).await
}
#[tauri::command]
async fn foreign_revoke(
    state: State<'_, SharedService>,
    session: u64,
    approval_id: String,
) -> Result<cd_core::apply::live::foreign::Receipt> {
    let state = Arc::clone(&state);
    blocking(move || state.foreign_revoke(session, &approval_id)).await
}
