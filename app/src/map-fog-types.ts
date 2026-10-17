export interface MapFogState {revealed_cells:number;total_cells:number}
export interface MapFogPlan {save_sha256:string;lobby_sha256:string;plan_id:string;before:MapFogState;after:MapFogState}
export interface MapFogPreview {save_root:string;saves:{id:string;label:string;modified:number}[];selected_save:string|null;plan:MapFogPlan|null;game_running:boolean}
export interface MapFogRequest {save_root:string;id:string;save:string;save_sha256:string;lobby_sha256:string;plan_id:string}
export interface MapFogReceipt {request:MapFogRequest;plan:MapFogPlan;backup:string;message:string}
