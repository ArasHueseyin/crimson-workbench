export interface RecoveryListing {
  entries: { directory:string; name:string; format:number|null; error:string|null }[];
  truncated:boolean;
}
export interface RecoveryReview {
  directory:string; review_id:string; plan_id:string; protected_source_files:number;
  can_restore:boolean; scope:string;
  restore: {
    backup_sha256:string; backup_bytes:number; current_registry_sha256:string;
    pending_outcome:'committed'|'rolled_back'|null; pending_intent_sha256:string|null;
    next_transaction_id:string;
    files:{path:string;action:string;before_sha256:string|null;after_sha256:string|null}[];
    remove_directories:string[];
  };
}
export interface RecoveryReceipt { directory:string; registry_restored:boolean; pending_recovered:boolean; plan_id:string }
