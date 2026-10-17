import {nativeInteger} from './world-controls';
import type {PhotoChoice,PhotoOperation,PhotoRequest,PhotoSnapshot} from './photo-types';

const maximum=18446744073709551615n;
const pendingKey='crimson-workbench.photo.pending.v1',counterKey='crimson-workbench.photo.counter.v1';
const operations:PhotoOperation[]=['enter','hide_hud','show_hud','leave'];
export type PendingPhoto={pid:number;id:string;session:string;epoch:string;not_after_ms:string;operation:PhotoOperation;recorded_at:number};
function valid(value:unknown):value is PendingPhoto {
 if(!value||typeof value!=='object')return false;
 const p=value as PendingPhoto;
 return Number.isInteger(p.pid)&&p.pid>0&&operations.includes(p.operation)&&Number.isSafeInteger(p.recorded_at)&&p.recorded_at>0&&[p.id,p.session,p.epoch,p.not_after_ms].every(n=>typeof n==='string'&&nativeInteger(n)>0n);
}
export function photoRequest(s:PhotoSnapshot,choice:PhotoChoice,counter='0'):PhotoRequest {
 if(!Number.isInteger(s.pid)||s.pid<=0||nativeInteger(s.session)===0n||nativeInteger(s.epoch)===0n||s.state==='waiting')throw Error('Photo controls are waiting for the active game view.');
 if(s.state==='pending'||s.state==='native_queued')throw Error('Another photo command is waiting for the game.');
 if(choice.operation==='enter'&&s.native_photo)throw Error('Photo mode is already active. Leave it normally before starting a Workbench session.');
 if(choice.operation!=='enter'&&(!s.owned||!s.native_photo))throw Error('These controls require a photo session started through Workbench.');
 const highest=[nativeInteger(s.id),nativeInteger(counter)].reduce((a,b)=>a>b?a:b),deadline=nativeInteger(s.observed_ms)+2500n;
 if(highest===maximum||deadline>maximum)throw Error('The photo command counter is exhausted.');
 return{pid:s.pid,id:(highest+1n).toString(),session:s.session,epoch:s.epoch,not_after_ms:deadline.toString(),operation:choice.operation,hide_on_enter:choice.operation==='enter'?choice.hide_on_enter:null};
}
export function photoHistory(storage:Storage):{pending:PendingPhoto|null;counter:string} {
 const raw=storage.getItem(pendingKey),counter=storage.getItem(counterKey)??'0';nativeInteger(counter);
 if(raw===null)return{pending:null,counter};
 const pending:unknown=JSON.parse(raw);if(!valid(pending))throw Error('Previous photo command history is invalid.');
 return{pending,counter};
}
export function savePhotoPending(storage:Storage,r:PhotoRequest):PendingPhoto {
 const p:PendingPhoto={pid:r.pid,id:r.id,session:r.session,epoch:r.epoch,not_after_ms:r.not_after_ms,operation:r.operation,recorded_at:Date.now()};
 if(!valid(p))throw Error('Invalid photo command.');
 storage.setItem(counterKey,r.id);storage.setItem(pendingKey,JSON.stringify(p));return p;
}
export function clearPhotoPending(storage:Storage){storage.removeItem(pendingKey)}
export function photoReason(reason:number):string {
 return ['Ready.','The photo protocol does not match this Workbench build.','The game instance changed.','The game view changed.','Return to the game briefly, then check photo status.','Another photo operation is waiting.','This command was already seen.','Photo controls are waiting for the active game view.','Photo mode was opened outside Workbench. Leave it normally before starting a Workbench session.','Another camera mode is active. Return to normal gameplay first.','This photo session was not started through Workbench.','The photo context changed.','The game did not confirm the photo change before the deadline.'][reason]??'Photo controls are not available.';
}
export function photoOutcome(s:PhotoSnapshot,p:PendingPhoto,direct=false,now=Date.now()):{kind:'waiting'|'done'|'uncertain';message:string} {
 if(direct&&s.state==='rejected')return{kind:'done',message:photoReason(s.reason)};
 if(s.pid!==p.pid||s.session!==p.session||s.epoch!==p.epoch)return{kind:'uncertain',message:'The game or photo context changed. The previous command will not be sent again.'};
 // A paused/unavailable UI callback can leave observed_ms unchanged. A local
 // timeout only exposes the unresolved result; it never resends the action.
 if((s.state==='pending'||s.state==='native_queued'||s.state==='waiting')&&(now<p.recorded_at||now-p.recorded_at>5000))return{kind:'uncertain',message:'The photo change was not confirmed. Check the game; this command will not be sent again.'};
 if(s.id===p.id){
  if(s.state==='pending'||s.state==='native_queued'||s.state==='waiting')return{kind:'waiting',message:'Waiting for the game. Return to the game briefly; this command will not be sent again.'};
  if(s.state==='uncertain'||s.state==='rejected')return{kind:'uncertain',message:photoReason(s.reason)};
  const success=p.operation==='enter'?s.owned&&s.native_photo&&s.scene_frozen:p.operation==='leave'?!s.native_photo:p.operation==='hide_hud'?s.owned&&s.hud_hidden:s.owned&&!s.hud_hidden;
  if(success)return{kind:'done',message:p.operation==='enter'?'Photo mode active. Move the camera with the game’s photo controls.':p.operation==='leave'?'Photo mode closed.':p.operation==='hide_hud'?'HUD hidden.':'HUD restored.'};
 }
 if(nativeInteger(s.id)>nativeInteger(p.id))return{kind:'uncertain',message:'Another photo command replaced the receipt. Check the game; the previous command will not be sent again.'};
 if(nativeInteger(s.observed_ms)>nativeInteger(p.not_after_ms))return{kind:'uncertain',message:'The photo command was not confirmed. Check the game; this command will not be sent again.'};
 return{kind:'waiting',message:'Checking the previous photo command without sending it again.'};
}
