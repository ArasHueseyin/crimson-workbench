export interface MarketplaceSelection { larger_lion:boolean; faster_mounts:boolean; no_mount_stamina:boolean }
export const emptySelection:MarketplaceSelection={larger_lion:false,faster_mounts:false,no_mount_stamina:false};
export type NativeModId='blackstar'|'sleep_cooldown'|'sleep_duration';
export interface NativeModStatus {id:NativeModId;state:'not_installed'|'installed'|'unmanaged'|'update_available'|'conflict'|'pending';message:string;can_install:boolean;can_remove:boolean}
export interface NativeModSnapshot {mods:NativeModStatus[];game_running:boolean}
export interface SkillSummary { total:number;new_abilities:number;upgraded_abilities:number;already_maxed:number;gifted_points:number[];available_points:number[] }
export interface SkillPreview { save_root:string; saves:{id:string;label:string;modified:number}[];selected_save:string|null;save_sha256:string|null;lobby_sha256:string|null;plan_id:string|null;summary:SkillSummary|null;game_running:boolean }
export interface SkillRequest { save_root:string; id:string;save:string;save_sha256:string;lobby_sha256:string;plan_id:string }
export interface SkillReceipt { request:SkillRequest;summary:SkillSummary;backup:string;message:string }
