import type {ForeignInspection,ForeignReceipt} from './foreign-types';
import type {ExtraSnapshot,ExtraRequest,ExtraReceipt,ExtraCandidates,ExtraAddRequest} from './extra-socket-types';
import type {AdvancedItem,AdvancedSkill} from './advanced-types';
import type {StoneDetail} from './abyss-stone-effects';
import type { SpawnRequest, SpawnSnapshot } from './spawn-types';
import type { MountRequest, MountSnapshot, MountReceipt } from './mount-types';
import type { KnowledgeSnapshot } from './knowledge-types';
import { invoke, isTauri } from '@tauri-apps/api/core';
import type {LiveUpdate,LiveStatus,LiveSetup,LiveRequest,LiveReview,LiveOperation,LiveJob} from './live-types';
import type { AppError, Bootstrap, Catalog, Detail, ItemIcon, Page, Query, UserSettings } from './types';
import type { CraftInfo, CraftPlan, CraftRequest, Recipe } from './craft-types';
import type { RecoveryListing, RecoveryReview, RecoveryReceipt } from './recovery-types';
import type { BaselineCatalog, BaselinePreview, BaselineSaved } from './baseline-types';
import type { AuditSnapshot, InstallationInventory, ModInfo, ModPreview, ModRehearsal, ModRequest } from './mod-types';
function call<T>(command: string, args?: Record<string, unknown>): Promise<T> {
  if (!isTauri()) return Promise.reject({ code: 'desktop_required', message: 'Öffne Crimson Workbench als Desktop-App, um deine lokale Installation zu lesen.' });
  return invoke<T>(command, args);
}
export const api = {
  extraSockets:(session:number)=>call<ExtraSnapshot>('extra_sockets_snapshot',{session}),
  setExtraSocket:(session:number,request:ExtraRequest)=>call<ExtraReceipt>('extra_socket_set',{session,request}),
  extraSocketCandidates:(session:number,save:string|null=null)=>call<ExtraCandidates>('extra_socket_candidates',{session,save}),
  addExtraSockets:(session:number,request:ExtraAddRequest)=>call<ExtraReceipt>('extra_socket_add',{session,request}),
  knowledge:(session:number,save:string|null=null)=>call<KnowledgeSnapshot>('knowledge_snapshot',{session,save}),
  mountCatalog:(session:number,save:string|null=null)=>call<MountSnapshot>('mount_catalog',{session,save}),
  mountSearch:(session:number,text:string,regex:boolean)=>call<number[]>('mount_search',{session,text,regex}),
  mountIcon:(session:number,key:number)=>call<ItemIcon>('mount_icon',{session,key}),
  mountRegister:(session:number,request:MountRequest)=>call<MountReceipt>('mount_register',{session,request}),
  spawnStatus:(session:number,request:SpawnRequest|null=null)=>call<SpawnSnapshot>('spawn_status',{session,request}),
  spawnGrant:(session:number,request:SpawnRequest)=>call<SpawnSnapshot>('spawn_grant',{session,request}),
  spawnCancel:(session:number,request:SpawnRequest)=>call<SpawnSnapshot>('spawn_cancel',{session,request}),
  advancedSkill:(session:number,key:number)=>call<AdvancedSkill>('advanced_skill',{session,key}),
  advancedItem:(session:number,key:number)=>call<AdvancedItem>('advanced_item',{session,key}),
  abyssStoneDetails:(session:number)=>call<StoneDetail[]>('abyss_stone_details',{session}),
  foreignPreview:(session:number)=>call<ForeignInspection>('foreign_preview',{session}),
  foreignConfirm:(session:number,reviewId:string,preserve:boolean)=>call<ForeignReceipt>('foreign_confirm',{session,reviewId,preserve}),
  foreignRevoke:(session:number,approvalId:string)=>call<ForeignReceipt>('foreign_revoke',{session,approvalId}),
  liveStatus:(session:number)=>call<LiveStatus>('live_status',{session}),
  liveUpdatePreview:(session:number,baselineId:string)=>call<LiveUpdate>('live_update_preview',{session,baselineId}),
  liveSetupPreview:(session:number,baselineId:string)=>call<LiveSetup>('live_setup_preview',{session,baselineId}),
  livePreview:(session:number,request:LiveRequest)=>call<LiveReview>('live_preview',{session,request}),
  liveStart:(session:number,operation:LiveOperation)=>call<LiveJob>('live_start',{session,operation}),
  liveJobStatus:(session:number)=>call<LiveJob|null>('live_job_status',{session}),
  liveCancel:(session:number,id:number)=>call<void>('live_cancel',{session,id}),
  baselineCatalog: (session:number)=>call<BaselineCatalog>('baseline_catalog',{session}),
  baselinePreview: (session:number,reportName:string)=>call<BaselinePreview>('baseline_preview',{session,reportName}),
  baselineCapture: (session:number,reportName:string,reviewId:string)=>call<BaselineSaved>('baseline_capture',{session,reportName,reviewId}),
  baselineInspect: (session:number,id:string)=>call<BaselineSaved>('baseline_inspect',{session,id}),
  rehearsalList: (session:number)=>call<RecoveryListing>('rehearsal_list',{session}),
  rehearsalReview: (session:number,directory:string)=>call<RecoveryReview>('rehearsal_review',{session,directory}),
  rehearsalRestore: (session:number,directory:string,reviewId:string)=>call<RecoveryReceipt>('rehearsal_restore',{session,directory,reviewId}),
  auditStart: (session:number)=>call<AuditSnapshot>('installation_audit_start',{session}),
  auditStatus: (session:number)=>call<AuditSnapshot|null>('installation_audit_status',{session}),
  auditCancel: (session:number,id:number)=>call<void>('installation_audit_cancel',{session,id}),
  auditExport: (session:number,id:number)=>call<string>('installation_audit_export',{session,id}),
  installationCheck: (session: number) => call<InstallationInventory>('installation_check', { session }),
  modInfo: (session: number) => call<ModInfo>('mod_info', { session }),
  modPreview: (session: number, request: ModRequest) => call<ModPreview>('mod_preview', { session, request }),
  modExport: (session: number, request: ModRequest, planId: string) => call<string>('mod_export', { session, request, planId }),
  modRehearse: (session: number, request: ModRequest, planId: string) => call<ModRehearsal>('mod_rehearse', { session, request, planId }),
  craftInfo: (session: number) => call<CraftInfo>('craft_info', { session }),
  craftPlan: (session: number, request: CraftRequest) => call<CraftPlan>('craft_plan', { session, request }),
  craftItem: (session: number, key: number) => call<{ recipes: Recipe[]; used_in: Recipe[] }>('craft_item', { session, key }),
  bootstrap: () => call<Bootstrap>('bootstrap'),
  saveSettings: (settings: UserSettings) => call<Bootstrap>('save_settings', { settings }),
  open: (game: string | null, language: string) => call<Catalog>('open_catalog', { game, language }),
  search: (session: number, query: Query) => call<Page>('search_items', { session, query }),
  detail: (session: number, key: number) => call<Detail>('item_detail', { session, key }),
  icon: (session: number, key: number) => call<ItemIcon>('item_icon', { session, key }),
  export: (session: number, key: number) => call<string>('export_item', { session, key }),
};
export function asError(error: unknown): AppError {
  if (typeof error === 'object' && error && 'message' in error) return { code: 'code' in error ? String(error.code) : 'read_error', message: String(error.message) };
  return { code: 'read_error', message: String(error) };
}
