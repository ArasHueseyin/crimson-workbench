import {useEffect,useRef,useState} from 'react';
import {LoaderCircle,MapPin,RefreshCw} from 'lucide-react';
import {api,asError} from './api';
import {clearTeleportPending,markerReason,replacementProof,saveTeleportPending,teleportContext,teleportHistory,teleportOutcome,teleportReason,teleportRequest,validTeleportName,type PendingTeleport} from './teleport-controls';
import type {TeleportChoice,TeleportSnapshot} from './teleport-types';

export function MarketplaceTeleportControls({session,enabled}:{session:number;enabled:boolean}){
 const [snapshot,setSnapshot]=useState<TeleportSnapshot|null>(null),[name,setName]=useState(''),[saveSlot,setSaveSlot]=useState(0),[savedSlot,setSavedSlot]=useState(''),[marker,setMarker]=useState(''),[replaceProof,setReplaceProof]=useState('');
 const [busy,setBusy]=useState(false),[error,setError]=useState(''),[historyError,setHistoryError]=useState(''),[message,setMessage]=useState(''),[pending,setPending]=useState<PendingTeleport|null>(null),[uncertain,setUncertain]=useState(false);
 const revision=useRef(0),lock=useRef(false),reading=useRef(false),counter=useRef('0'),pendingRef=useRef<PendingTeleport|null>(null),initialised=useRef(false);
 function clear(){try{clearTeleportPending(localStorage);pendingRef.current=null;setPending(null);setUncertain(false)}catch{setHistoryError('The previous teleport command could not be cleared. Teleport changes are disabled.')}}
 function accept(next:TeleportSnapshot,direct=false){
  setSnapshot(next);setError('');
  if(!initialised.current){
   const empty=next.bookmarks.findIndex(b=>!b.occupied),saved=next.bookmarks.findIndex(b=>b.occupied);
   setSaveSlot(empty<0?0:empty);setSavedSlot(saved<0?'':String(saved));initialised.current=true;
  }
  const p=pendingRef.current;
  if(p){const outcome=teleportOutcome(next,p,direct);setMessage(outcome.message);setUncertain(outcome.kind==='uncertain');if(outcome.kind==='done')clear()}
 }
 async function refresh(token=revision.current){
  if(!enabled||reading.current||lock.current)return;reading.current=true;
  try{const next=await api.teleportStatus(session);if(revision.current===token)accept(next)}
  catch(e){if(revision.current===token){setError(asError(e).message);if(pendingRef.current){setUncertain(true);setMessage('The teleport result could not be checked. This command will not be sent again.')}}}
  finally{reading.current=false}
 }
 useEffect(()=>{
  const token=++revision.current;setSnapshot(null);setBusy(false);setError('');setHistoryError('');setMessage('');setUncertain(false);setName('');setReplaceProof('');setMarker('');initialised.current=false;
  try{const saved=teleportHistory(localStorage);counter.current=saved.counter;pendingRef.current=saved.pending;setPending(saved.pending);if(saved.pending)setMessage('Checking the previous teleport command without sending it again.')}
  catch{setHistoryError('The previous teleport command history could not be read. Teleport changes are disabled.')}
  void refresh(token);const timer=window.setInterval(()=>void refresh(token),1000);
  return()=>{revision.current++;window.clearInterval(timer)};
 },[session,enabled]);
 async function change(choice:TeleportChoice){
  if(lock.current||pendingRef.current||historyError||!enabled)return;
  lock.current=true;setBusy(true);setError('');setMessage('');const token=revision.current;
  try{
   const current=await api.teleportStatus(session);if(revision.current!==token)return;
   const contextChanged=!snapshot||teleportContext(current)!==teleportContext(snapshot);
   accept(current);
   if(contextChanged){setReplaceProof('');throw Error('The game, player or field changed. Review the current context before continuing.');}
   // Recheck replacement against the fresh status, not only the earlier view.
   if(choice.operation==='save_current'&&current.bookmarks[choice.slot]?.occupied){
    if(!replaceProof||replaceProof!==replacementProof(current,choice.slot)){setReplaceProof('');throw Error('Review this saved position and confirm replacing it first.');}
   }
   if((choice.operation==='travel_saved'||choice.operation==='remove_saved')&&JSON.stringify(current.bookmarks[choice.slot])!==JSON.stringify(snapshot?.bookmarks[choice.slot]))throw Error('This saved position changed. Review it before continuing.');
   if(choice.operation==='travel_marker'){
    const fresh=current.markers.find(m=>m.id===choice.marker&&m.source===choice.marker_source);
    const shown=snapshot?.markers.find(m=>m.id===choice.marker&&m.source===choice.marker_source);
    if(JSON.stringify(fresh)!==JSON.stringify(shown))throw Error('This map marker changed. Review its current position before travelling.');
   }
   const request=teleportRequest(current,choice,counter.current),saved=saveTeleportPending(localStorage,request);
   counter.current=request.id;pendingRef.current=saved;setPending(saved);setUncertain(false);
   const next=await api.teleportChange(session,request);if(revision.current===token){accept(next,true);if(choice.operation==='save_current'){setSavedSlot(String(choice.slot));setReplaceProof('')}}
  }catch(e){if(revision.current===token){setError(asError(e).message);if(pendingRef.current){setUncertain(true);setMessage('The teleport result was not confirmed. Check the game; this command will not be sent again.')}}}
  finally{lock.current=false;if(revision.current===token)setBusy(false)}
 }
 const locked=!enabled||busy||!!pending||!!error||!!historyError||!snapshot||!snapshot.current||['waiting','faulted','pending','native_queued'].includes(snapshot.state);
 const replace=!!replaceProof&&replaceProof===replacementProof(snapshot,saveSlot);
 const selectedBookmark=savedSlot===''?null:snapshot?.bookmarks[Number(savedSlot)],selectedMarker=snapshot?.markers.find(m=>`${m.source}:${m.id}`===marker);
 const sameField=!!selectedBookmark?.position&&selectedBookmark.position.field_definition===snapshot?.current?.field_definition&&selectedBookmark.position.stage===snapshot?.current?.stage;
 const coordinates=(p:{x:number;y:number;z:number})=>`${p.x.toFixed(1)}, ${p.y.toFixed(1)}, ${p.z.toFixed(1)}`;
 return <div className="market-teleport-controls" aria-label="Live teleport controls">
  <p className="teleport-current">{snapshot?.current?`Current position: ${coordinates(snapshot.current.position)}`:snapshot?teleportReason(snapshot.reason):'Waiting for the teleport module…'}</p>
  {!enabled&&<p>Install this mod with the game closed, then start the game and load a save.</p>}
  <fieldset disabled={locked}><legend>Save current position</legend>
   <label>Position name<input aria-label="Position name" value={name} maxLength={47} placeholder="Camp, viewpoint, entrance…" onChange={e=>{setName(e.target.value);setReplaceProof('')}}/></label>
   <label>Save slot<select aria-label="Save slot" value={saveSlot} onChange={e=>{setSaveSlot(Number(e.target.value));setReplaceProof('')}}>{Array.from({length:16},(_,slot)=><option key={slot} value={slot}>{slot+1}. {snapshot?.bookmarks[slot]?.occupied?snapshot.bookmarks[slot].name:'Empty slot'}</option>)}</select></label>
   {snapshot?.bookmarks[saveSlot]?.occupied&&<label className="mod-switch"><input type="checkbox" checked={replace} onChange={e=>setReplaceProof(e.target.checked?replacementProof(snapshot,saveSlot):'')}/>Replace the saved position in this slot</label>}
   <button className="secondary" disabled={locked||!validTeleportName(name)||!!snapshot?.bookmarks[saveSlot]?.occupied&&!replace||!((snapshot?.features??0)&1)} onClick={()=>void change({operation:'save_current',slot:saveSlot,name})}>Save current position</button>
  </fieldset>
  <fieldset disabled={locked}><legend>Saved positions</legend>
   <label>Destination<select aria-label="Saved destination" value={savedSlot} onChange={e=>setSavedSlot(e.target.value)}><option value="">Choose a saved position</option>{snapshot?.bookmarks.map((b,slot)=>b.occupied&&<option key={slot} value={slot}>{slot+1}. {b.name}{b.position?.field_definition!==snapshot.current?.field_definition||b.position?.stage!==snapshot.current?.stage?' · Other field':''}</option>)}</select></label>
   <div className="mod-actions"><button className="primary" disabled={locked||!selectedBookmark?.occupied||!sameField||!((snapshot?.features??0)&2)} onClick={()=>void change({operation:'travel_saved',slot:Number(savedSlot)})}><MapPin size={15}/>Teleport to saved position</button><button className="secondary" disabled={locked||!selectedBookmark?.occupied} onClick={()=>void change({operation:'remove_saved',slot:Number(savedSlot)})}>Remove saved position</button></div>
   {selectedBookmark?.occupied&&!sameField&&<p>This position belongs to another field. Return to that field before travelling here.</p>}
  </fieldset>
  <fieldset disabled={locked}><legend>Map pins and waypoints</legend>
   <p>{snapshot?markerReason(snapshot.marker_status):'Waiting for current map markers…'}</p>
   <label>Map marker<select aria-label="Map marker" value={marker} onChange={e=>setMarker(e.target.value)}><option value="">Choose a map marker</option>{snapshot?.markers.map((m,i)=><option key={`${m.source}:${m.id}`} value={`${m.source}:${m.id}`}>{m.source==='map_pin'?'Map pin':'Waypoint'} {i+1} · X {m.position.x.toFixed(1)}, Z {m.position.z.toFixed(1)}{m.layer!=='mainland'?' · Unsupported layer':''}</option>)}</select></label>
   <button className="primary" disabled={locked||!selectedMarker||selectedMarker.layer!=='mainland'||snapshot?.marker_status!==0||!((snapshot?.features??0)&4)} onClick={()=>{if(selectedMarker)void change({operation:'travel_marker',marker:selectedMarker.id,marker_source:selectedMarker.source})}}><MapPin size={15}/>Teleport to map marker</button>
   <p>The game checks ground and placement before moving you. Only mainland markers in the current main field are supported. Unloaded geometry, unsupported map layers and unsuitable destinations are refused.</p>
  </fieldset>
  <p>Saved positions use local Workbench storage for this Windows user. Travel uses the game’s field-transition queue and may trigger loading. Native movement and quest checks still apply. A queued destination is not a confirmed arrival.</p>
  {snapshot?.storage_status===2&&<p role="alert" className="craft-error">Bookmark storage failed. Current changes exist only in this game session. Save a position again explicitly to make another storage attempt.</p>}
  <button className="secondary" disabled={!enabled||busy} onClick={()=>void refresh()}>{busy?<LoaderCircle size={14} className="spin"/>:<RefreshCw size={14}/>}Refresh teleport status</button>
  {pending&&uncertain&&<button className="secondary" disabled={busy||!!historyError} onClick={()=>{clear();setMessage('Previous teleport command dismissed. It will not be sent again.')}}>I checked the result in game</button>}
  {message&&<p role="status">{message}</p>}{error&&<p role="alert" className="craft-error">{error}</p>}{historyError&&<p role="alert" className="craft-error">{historyError}</p>}
 </div>;
}
