import {nativeInteger} from './world-controls';
import type {TeleportChoice,TeleportOperation,TeleportRequest,TeleportSnapshot} from './teleport-types';
const maximum=18446744073709551615n;
const pendingKey='crimson-workbench.teleport.pending.v1',counterKey='crimson-workbench.teleport.counter.v1';
const operations:TeleportOperation[]=['save_current','remove_saved','travel_saved','travel_marker'];
export interface PendingTeleport {pid:number;id:string;session:string;epoch:string;not_after_ms:string;operation:TeleportOperation;recorded_at:number}
export function teleportContext(s:TeleportSnapshot){return JSON.stringify([s.pid,s.session,s.epoch,s.identity,s.world,s.current?.field_definition,s.current?.stage])}
export function replacementProof(s:TeleportSnapshot|null,slot:number){return s?JSON.stringify([teleportContext(s),slot,s.bookmarks[slot]]):''}
export function validTeleportName(name:string){return name.length>0&&!/\p{Cc}/u.test(name)&&![...name].some(c=>c.length===1&&c.charCodeAt(0)>=0xd800&&c.charCodeAt(0)<=0xdfff)&&new TextEncoder().encode(name).length<=47}
function valid(value:unknown):value is PendingTeleport {
 if(!value||typeof value!=='object')return false;
 const p=value as PendingTeleport;
 return Number.isInteger(p.pid)&&p.pid>0&&operations.includes(p.operation)&&Number.isSafeInteger(p.recorded_at)&&p.recorded_at>0&&[p.id,p.session,p.epoch,p.not_after_ms].every(n=>typeof n==='string'&&nativeInteger(n)>0n);
}
export function teleportRequest(s:TeleportSnapshot,choice:TeleportChoice,counter='0'):TeleportRequest {
 if(!Number.isInteger(s.pid)||s.pid<=0||nativeInteger(s.session)===0n||nativeInteger(s.epoch)===0n||!s.current||s.state==='waiting'||s.state==='faulted')throw Error('Teleport is waiting for the active player and field.');
 if(s.state==='pending'||s.state==='native_queued')throw Error('Another teleport command is waiting for the game.');
 if(choice.operation==='travel_marker'){
  const marker=nativeInteger(choice.marker);
  if(!(s.features&4)||marker===maximum||!['map_pin','waypoint'].includes(choice.marker_source)||s.marker_status!==0||nativeInteger(s.markers_observed_ms)>nativeInteger(s.observed_ms)||nativeInteger(s.observed_ms)-nativeInteger(s.markers_observed_ms)>3000n)throw Error('Map markers are not available in the current field.');
  const row=s.markers.find(m=>m.id===choice.marker&&m.source===choice.marker_source);
  if(!row||row.layer!=='mainland')throw Error('Choose a current mainland map pin or waypoint.');
 }else{
  if(!Number.isInteger(choice.slot)||choice.slot<0||choice.slot>=16)throw Error('Choose a saved-position slot.');
  const bookmark=s.bookmarks[choice.slot];
  if(choice.operation==='save_current'){
   if(!(s.features&1)||!validTeleportName(choice.name))throw Error('Enter a position name using 1–47 UTF-8 bytes without control characters.');
  }else if(!bookmark?.occupied)throw Error('This saved position is empty.');
  if(choice.operation==='travel_saved'&&(!(s.features&2)||!bookmark.position||bookmark.position.field_definition!==s.current.field_definition||bookmark.position.stage!==s.current.stage))throw Error('This saved position belongs to another field.');
 }
 const highest=[nativeInteger(s.id),nativeInteger(counter)].reduce((a,b)=>a>b?a:b),deadline=nativeInteger(s.observed_ms)+2500n;
 if(highest===maximum||deadline>maximum)throw Error('The teleport command counter is exhausted.');
 return{...choice,pid:s.pid,id:(highest+1n).toString(),session:s.session,epoch:s.epoch,not_after_ms:deadline.toString()};
}
export function teleportHistory(storage:Storage):{pending:PendingTeleport|null;counter:string}{
 const raw=storage.getItem(pendingKey),counter=storage.getItem(counterKey)??'0';nativeInteger(counter);
 if(raw===null)return{pending:null,counter};
 const pending:unknown=JSON.parse(raw);if(!valid(pending))throw Error('Previous teleport command history is invalid.');
 return{pending,counter};
}
export function saveTeleportPending(storage:Storage,r:TeleportRequest):PendingTeleport {
 const p:PendingTeleport={pid:r.pid,id:r.id,session:r.session,epoch:r.epoch,not_after_ms:r.not_after_ms,operation:r.operation,recorded_at:Date.now()};
 if(!valid(p))throw Error('Invalid teleport command.');
 storage.setItem(counterKey,r.id);storage.setItem(pendingKey,JSON.stringify(p));return p;
}
export function clearTeleportPending(storage:Storage){storage.removeItem(pendingKey)}
export function teleportReason(reason:number):string {
 return ['Ready.','The teleport protocol does not match this Workbench build.','The game instance changed.','Return to normal gameplay briefly, then refresh teleport status.','This command was already seen.','A native movement or teleport is already waiting.','Choose an occupied saved-position slot.','Enter a valid position name.','The player or destination is not available.','The game rejected this movement.','The teleport module stopped after a native fault.','The game did not confirm the queued destination.','The teleport result was not confirmed before its deadline.','The player or field changed.','The map marker does not have a verified destination.','This map marker no longer exists.','Only mainland map markers are supported.','Destination geometry is not loaded or no ground was found.','The game could not find a safe placement at this map marker.','The marker changed or its placement differs too far from the selected point.','Marker travel is available in the main field only.','The current player marker list is not available.'][reason]??'Teleport is not available.';
}
export function markerReason(status:number):string {
 return ['Mainland map markers are available.','Marker travel is not supported by this module.','Return to normal gameplay to refresh map markers.','Marker travel is available in the main field only.','The player marker component is not available.','The game marker list is not available.','This marker no longer exists.','Only mainland map markers are supported.','Destination geometry is not loaded or no ground was found.','The game could not find a safe placement at this marker.','The safe placement differs too far from this marker.','Marker controls stopped after a native fault.'][status]??'Map markers are not available.';
}
export function teleportOutcome(s:TeleportSnapshot,p:PendingTeleport,direct=false,now=Date.now()):{kind:'waiting'|'done'|'uncertain';message:string}{
 if(direct&&s.state==='rejected')return{kind:'done',message:teleportReason(s.reason)};
 if(s.pid!==p.pid||s.session!==p.session||s.epoch!==p.epoch)return{kind:'uncertain',message:'The game, player or field changed. The previous command will not be sent again.'};
 if(s.id===p.id){
  if(s.state==='uncertain'||s.state==='faulted')return{kind:'uncertain',message:teleportReason(s.reason)};
  if(s.state==='rejected')return{kind:'done',message:teleportReason(s.reason)};
  if(p.operation==='save_current'&&s.state==='captured')return{kind:'done',message:s.storage_status===1?'Position saved.':s.storage_status===2?'Position captured for this game session. Bookmark storage failed; it may be lost when the game closes.':'Position captured for this game session.'};
  if(p.operation==='remove_saved'&&s.state==='removed')return{kind:'done',message:s.storage_status===2?'Position removed for this game session. Bookmark storage failed; it may return after a restart.':'Saved position removed.'};
  if((p.operation==='travel_saved'||p.operation==='travel_marker')&&s.state==='position_observed')return{kind:'done',message:'The player was observed at the destination. Check the game before continuing.'};
 }
 if(nativeInteger(s.id)>nativeInteger(p.id))return{kind:'uncertain',message:'Another teleport command replaced the receipt. Check the game; the previous command will not be sent again.'};
 const wait=p.operation==='travel_saved'||p.operation==='travel_marker'?70000:6000;
 if(now<p.recorded_at||now-p.recorded_at>wait)return{kind:'uncertain',message:'The teleport result was not confirmed. Check the game; this command will not be sent again.'};
 return{kind:'waiting',message:s.id===p.id&&s.state==='native_queued'?'Destination queued by the game. Waiting to observe the player there; this is not an arrival confirmation.':'Waiting for the game. Return to normal gameplay briefly; this command will not be sent again.'};
}
