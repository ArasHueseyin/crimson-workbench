import type {AdvancedInfo,AdvancedRequest} from './advanced-types';
export interface VendorOptions { items:number[]|null;daily_refresh:boolean|null }
export interface AuditReport { foreign_approval?:string|null; version:number;kind:string;game_path:string;build_id:string;started_at:number;finished_at:number;all_files_match_cache:boolean;metadata_stable:boolean;certified_vanilla:boolean;can_apply:boolean;manifest_sha256:string[];files:{path:string;bytes:string;expected_sha1:string;sha1:string;sha256:string;matches_cache:boolean}[];limitations:string[] }
export interface AuditSnapshot { id:number;session:number;phase:string;progress:{phase:string;files_done:number;total_files:number;bytes_done:string;total_bytes:string;current_path:string|null};error:string|null;report:AuditReport|null }
export interface InstallationInventory {
 version:number;mode:string;game_path:string;observed_at:number;build_id:string|null;game_running:boolean|null;
 directory_scan_complete:boolean;depot_comparison_available:boolean;content_verified:boolean;certified_vanilla:boolean;can_apply:boolean;
 expected_files:number;actual_files:number;expected_bytes:string;registry_sha256:string|null;registry_matches_observed_build:boolean|null;
 depots:{id:number;manifest_id:string;manifest_sha256:string;files:number;bytes:string;signature_bytes:number;authenticated:boolean}[];
 executables:string[];groups:{name:string;optional:boolean;installed:boolean;in_depots:boolean}[];
 files:{path:string;state:string;expected_bytes:string|null;actual_bytes:string|null;expected_sha1:string|null}[];
 foreign_files?:InstallationInventory["files"];foreign_approval?:string|null;
 managed_files?:InstallationInventory["files"];managed_registry?:boolean;
 issues:{code:string;path:string|null;message:string}[];limitations:string[];
}
export interface ModRequest { advanced?:AdvancedRequest; shop_stock: number | null; vendors: number[]; vendor_overrides: Record<number,number>;vendor_options:Record<number,VendorOptions>; quantity_multiplier: number; dropsets: number[]; quantity_overrides: Record<number,number>; trust_multiplier: number; daily_refresh:boolean;shop_items:number[];chance_multiplier:number;chance_overrides:Record<number,number>;guaranteed_dropsets:number[] }
export const emptyMod: ModRequest = { shop_stock:null,vendors:[],vendor_overrides:{},vendor_options:{},quantity_multiplier:1,dropsets:[],quantity_overrides:{},trust_multiplier:1,daily_refresh:false,shop_items:[],chance_multiplier:1,chance_overrides:{},guaranteed_dropsets:[] };
export interface ModChoice { key:number;name:string;entries:number }
export interface ModInfo { advanced?:AdvancedInfo; vendors:ModChoice[];dropsets:ModChoice[];store_rows:number;stock_rows:number;opaque_stores:number;opaque_dropsets:number;quantity_excluded:number;trust_rows:number;limitations:string[];items:ModChoice[];append_vendors:number[];daily_vendors:number[];chance_dropsets:ModChoice[];guarantee_dropsets:ModChoice[] }
export interface ModChange { module:string;table:string;key:number;name:string;field:string;before:string;after:string }
export interface ModPreview { request:ModRequest;fingerprint:string;plan_id:string;changes:ModChange[];files:{path:string;action:string;before_sha256:string|null;after_sha256:string;bytes:number}[];warnings:string[];gates:{game_running:boolean|null;can_apply:boolean;reasons:string[]};credits:string }
export interface ModRehearsal { directory:string;plan_id:string;cycles:number;registry_restored:boolean;archive_files_untouched:boolean;scope:string;reapply_passed:boolean;recovery_passed:boolean;launch_guard_held:boolean;protected_source_files:number;update_refusal_passed:boolean;transitions:{name:string;files:{path:string;action:string;before_sha256:string|null;after_sha256:string|null}[]}[] }
