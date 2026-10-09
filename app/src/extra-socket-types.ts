export interface ExtraGem {key:number;endurance:number;name?:string|null}
export interface ExtraRecord {uid:string;item_key:number;name:string;baseline:number;additional:number;gems:ExtraGem[]}
export interface ExtraSnapshot {revision:string;game_running:boolean;records:ExtraRecord[];runtime_log:string}
export interface ExtraRequest {revision:string;uid:string;index:number;gem_key:number}
export interface ExtraReceipt {backup:string;snapshot:ExtraSnapshot}
export interface ExtraCandidate {uid:string;item_key:number;name:string;baseline:number;location:string;eligible:boolean;reason:string|null;extended:boolean}
export interface ExtraCandidates {saves:{id:string;label:string;modified:number}[];selected_save:string|null;save_revision:string|null;items:ExtraCandidate[]}
export interface ExtraAddRequest {revision:string;save:string;save_revision:string;uid:string}
