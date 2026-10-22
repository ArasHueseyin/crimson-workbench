export interface WeaponEffectEdit {source:number;replace_existing:boolean}
export interface WeaponEffectTarget {key:number;name:string;existing:{skill:number;level:number}[]}
export interface WeaponEffectSource {source:number;source_name:string;effect_name:string;skill:number;level:number;targets:WeaponEffectTarget[]}
export interface MarketplaceSelection { larger_lion:boolean; larger_rokade:boolean; faster_mounts:boolean; no_mount_stamina:boolean;inventory_999:boolean;no_durability_loss:boolean;region_return_30min:boolean;automatic_fishing:boolean;free_crafting_currency:boolean;free_upgrade_materials:boolean;item_stacks:Record<number,number>;weapon_effects:Record<number,WeaponEffectEdit> }
export const emptySelection:MarketplaceSelection={larger_lion:false,larger_rokade:false,faster_mounts:false,no_mount_stamina:false,inventory_999:false,no_durability_loss:false,region_return_30min:false,automatic_fishing:false,free_crafting_currency:false,free_upgrade_materials:false,item_stacks:{},weapon_effects:{}};
export type NativeModId='blackstar'|'sleep_cooldown'|'sleep_duration'|'no_fall'|'player_stamina'|'mount_jump'|'player_jump'|'player_speed'|'preserve_consumables'|'mount_protection'|'free_crafting_materials'|'force_palm'|'force_palm_height'|'axiom_reach'|'glide_speed'|'mount_dismount'|'auto_loot'|'world_controls'|'photo_mode'|'teleport'|'region_return_30min';
export interface ConsumableChoice {key:number;name:string;kind:string}
export interface ConsumableSettings {item_keys:number[];choices:ConsumableChoice[];game_running:boolean;can_change:boolean;message:string}
export interface PlayerJumpSettings {height_percent:number;game_running:boolean;can_change:boolean;message:string}
export interface PlayerSpeedSettings {speed_percent:number;game_running:boolean;can_change:boolean;message:string}
export type RuntimeOptionId='force_palm_height'|'axiom_reach'|'glide_speed'|'player_resources';
export interface RuntimeOptionSettings {id:RuntimeOptionId;value:number;invalid:boolean;game_running:boolean;can_change:boolean;message:string}
export interface NativeModStatus {id:NativeModId;state:'not_installed'|'installed'|'unmanaged'|'update_available'|'conflict'|'pending';message:string;can_install:boolean;can_remove:boolean}
export interface NativeModSnapshot {mods:NativeModStatus[];game_running:boolean}
export interface SkillSummary { total:number;new_abilities:number;upgraded_abilities:number;already_maxed:number;gifted_points:number[];available_points:number[] }
export interface AbilityChoice {key:number;name:string;owner:number;maximum:number}
export interface SkillPreview { abilities:AbilityChoice[];save_root:string; saves:{id:string;label:string;modified:number}[];selected_save:string|null;save_sha256:string|null;lobby_sha256:string|null;plan_id:string|null;summary:SkillSummary|null;game_running:boolean }
export interface SkillRequest { save_root:string; id:string;save:string;save_sha256:string;lobby_sha256:string;plan_id:string;abilities?:number[]|null }
export interface SkillReceipt { request:SkillRequest;summary:SkillSummary;backup:string;message:string }

export interface KnowledgeSummary {total:number;new_entries:number;upgraded_entries:number;already_complete:number}
export interface KnowledgePlan {save_sha256:string;lobby_sha256:string;plan_id:string;summary:KnowledgeSummary}
export interface KnowledgePreview {save_root:string;saves:{id:string;label:string;modified:number}[];selected_save:string|null;plan:KnowledgePlan|null;game_running:boolean}
export interface KnowledgeRequest {save_root:string;id:string;save:string;save_sha256:string;lobby_sha256:string;plan_id:string}
export interface KnowledgeReceipt {request:KnowledgeRequest;plan:KnowledgePlan;backup:string;message:string}
