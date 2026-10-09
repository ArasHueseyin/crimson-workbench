export interface ForeignFile {path:string;bytes:number;sha256:string}
export interface ForeignReview {review_id:string;game_path:string;files:ForeignFile[];directories:string[];total_bytes:string;previous_approval:string|null;game_running:boolean|null}
export interface ForeignInspection {approval_id:string|null;approved_files:number;review:ForeignReview|null;issue:string|null}
export interface ForeignReceipt {approval_id:string;files:number;directories:number;action:'preserve_foreign'|'revoke_foreign'}
