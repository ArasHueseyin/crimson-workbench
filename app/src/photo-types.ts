export type PhotoOperation='enter'|'hide_hud'|'show_hud'|'leave';
export type PhotoState='waiting'|'ready'|'pending'|'native_queued'|'active'|'hud_hidden'|'hud_visible'|'left'|'rejected'|'uncertain';
export interface PhotoSnapshot {
 pid:number;state:PhotoState;id:string;session:string;epoch:string;observed_ms:string;reason:number;
 native_photo:boolean;owned:boolean;hud_hidden:boolean;scene_frozen:boolean;
}
export type PhotoChoice={operation:'enter';hide_on_enter:boolean}|{operation:'hide_hud'|'show_hud'|'leave'};
export interface PhotoRequest {
 pid:number;id:string;session:string;epoch:string;not_after_ms:string;operation:PhotoOperation;hide_on_enter:boolean|null;
}
