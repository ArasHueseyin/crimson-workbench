export type TeleportOperation='save_current'|'remove_saved'|'travel_saved'|'travel_marker';
export type TeleportState='waiting'|'ready'|'pending'|'captured'|'removed'|'native_queued'|'position_observed'|'rejected'|'uncertain'|'faulted';
export type MarkerSource='map_pin'|'waypoint';
export interface SavedPosition {field_definition:number;stage:number;position:{x:number;y:number;z:number};yaw:number}
export interface TeleportBookmark {occupied:boolean;name:string;position:SavedPosition|null}
export interface TeleportMarker {id:string;position:{x:number;y:number;z:number};source:MarkerSource;layer:'unknown'|'mainland'|'alternate';shape:number;color:number}
export interface TeleportSnapshot {
 pid:number;state:TeleportState;id:string;session:string;epoch:string;observed_ms:string;reason:number;
 features:number;identity:number;world:number;native_reason:number;native_error:number;storage_status:number;bookmarks_revision:string;
 current:SavedPosition|null;bookmarks:TeleportBookmark[];marker_status:number;markers_observed_ms:string;markers:TeleportMarker[];
}
export type TeleportChoice={operation:'save_current';slot:number;name:string}|{operation:'remove_saved';slot:number}|{operation:'travel_saved';slot:number}|{operation:'travel_marker';marker:string;marker_source:MarkerSource};
export type TeleportRequest=TeleportChoice&{pid:number;id:string;session:string;epoch:string;not_after_ms:string};
