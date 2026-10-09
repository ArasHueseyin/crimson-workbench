export interface BaselineCatalog { reports:string[];snapshots:string[];truncated:boolean }
export interface BaselinePreview {
  report_name:string;review_id:string;game_path:string;build_id:string;audited_at:number;
  file_count:number;archive_count:number;total_bytes:string;registry_bytes:number;registry_sha256:string;
  executables:string[];read_schema_matches:boolean;certified_vanilla:boolean;can_apply:boolean;
}
export interface BaselineSaved {
  id:string;directory:string;created_at:number;preview:BaselinePreview;
  current_state:'metadata_match'|'changed'|'unavailable';issues:string[];
}
