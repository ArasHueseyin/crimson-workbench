import type { ModRequest } from './mod-types';
export interface LiveStatus { configured:boolean;initialized:boolean;game_running:boolean|null;baseline_id:string|null;pending_basis?:string|null;active_overlay:boolean;recovery_required:boolean;issues:string[] }
export interface LiveSetup { preserved_foreign_files?:number; review_id:string;baseline_id:string;game_path:string;files:number;total_bytes:string;audited_at:number;provenance:string }
export type LiveRequest = {action:'apply';settings:ModRequest}|{action:'restore'};
export interface LiveReview { preserved_foreign_files?:number; review_id:string;action:'apply'|'restore';game_path:string;baseline_id:string;files:{path:string;action:string;before_sha256:string|null;after_sha256:string|null}[];recovery_required:boolean;protected_source_files:number;total_bytes:string }
export type LiveOperation = {kind:'setup'|'update';baseline_id:string;review_id:string;steam_verified_before_audit:boolean}|{kind:'execute';request:LiveRequest;review_id:string};
export interface LiveJob { id:number;session:number;phase:'running'|'complete'|'failed'|'cancelled';error:string|null;result:LiveStatus|{action:string;review_id:string;registry_sha256:string;source_files_verified:number;pending_recovered:boolean}|null }

export interface LiveUpdate {review_id:string;previous_baseline:string;next_baseline:string;resume:boolean;game_path:string;archive_path:string;files:LiveReview["files"];source_files:number;total_bytes:string}
