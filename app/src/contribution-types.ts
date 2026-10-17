export interface ContributionEdit {key:number;target:string}
export interface ContributionBalance {key:number;name:string;amount:string|null}
export interface ContributionChange {key:number;name:string;before:string;after:string}
export interface ContributionPlan {save_sha256:string;lobby_sha256:string;plan_id:string;balances:ContributionBalance[];changes:ContributionChange[]}
export interface ContributionPreview {save_root:string;saves:{id:string;label:string;modified:number}[];selected_save:string|null;balances:ContributionBalance[];plan:ContributionPlan|null;game_running:boolean}
export interface ContributionRequest {save_root:string;id:string;save:string;save_sha256:string;lobby_sha256:string;plan_id:string;edits:ContributionEdit[]}
export interface ContributionReceipt {request:ContributionRequest;plan:ContributionPlan;backup:string;message:string}
