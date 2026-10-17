import {useEffect,useRef,useState} from 'react';
import {Camera,LoaderCircle,RefreshCw} from 'lucide-react';
import {api,asError} from './api';
import {clearPhotoPending,photoHistory,photoOutcome,photoReason,photoRequest,savePhotoPending,type PendingPhoto} from './photo-controls';
import type {PhotoChoice,PhotoSnapshot} from './photo-types';

export function MarketplacePhotoControls({session,enabled}:{session:number;enabled:boolean}) {
 const [snapshot,setSnapshot]=useState<PhotoSnapshot|null>(null),[hideOnEnter,setHideOnEnter]=useState(true);
 const [busy,setBusy]=useState(false),[error,setError]=useState(''),[message,setMessage]=useState(''),[historyError,setHistoryError]=useState('');
 const [pending,setPending]=useState<PendingPhoto|null>(null),[uncertain,setUncertain]=useState(false);
 const revision=useRef(0),lock=useRef(false),reading=useRef(false),counter=useRef('0'),pendingRef=useRef<PendingPhoto|null>(null);
 function clear(){
  try{clearPhotoPending(localStorage);pendingRef.current=null;setPending(null);setUncertain(false)}
  catch{setHistoryError('The previous photo command could not be cleared. Photo changes are disabled.')}
 }
 function accept(next:PhotoSnapshot,direct=false){
  setSnapshot(next);setError('');const p=pendingRef.current;
  if(p){const outcome=photoOutcome(next,p,direct);setMessage(outcome.message);setUncertain(outcome.kind==='uncertain');if(outcome.kind==='done')clear()}
 }
 async function refresh(token=revision.current){
  if(!enabled||reading.current||lock.current)return;reading.current=true;
  try{const next=await api.photoStatus(session);if(revision.current===token)accept(next)}
  catch(e){if(revision.current===token){setError(asError(e).message);if(pendingRef.current){setUncertain(true);setMessage('The photo result could not be checked. This command will not be sent again.')}}}
  finally{reading.current=false}
 }
 useEffect(()=>{
  const token=++revision.current;setSnapshot(null);setBusy(false);setError('');setHistoryError('');setMessage('');setUncertain(false);
  try{const saved=photoHistory(localStorage);counter.current=saved.counter;pendingRef.current=saved.pending;setPending(saved.pending);if(saved.pending)setMessage('Checking the previous photo command without sending it again.')}
  catch{setHistoryError('The previous photo command history could not be read. Photo changes are disabled.')}
  void refresh(token);const timer=window.setInterval(()=>void refresh(token),750);
  return()=>{revision.current++;window.clearInterval(timer)};
 },[session,enabled]);
 async function change(choice:PhotoChoice){
  if(lock.current||pendingRef.current||historyError||!enabled)return;
  lock.current=true;setBusy(true);setError('');setMessage('');const token=revision.current;
  try{
   const current=await api.photoStatus(session);if(revision.current!==token)return;
   accept(current);const request=photoRequest(current,choice,counter.current);
   const saved=savePhotoPending(localStorage,request);counter.current=request.id;pendingRef.current=saved;setPending(saved);setUncertain(false);
   const next=await api.photoChange(session,request);if(revision.current===token)accept(next,true);
  }catch(e){if(revision.current===token){setError(asError(e).message);if(pendingRef.current){setUncertain(true);setMessage('The photo result was not confirmed. Check the game; this command will not be sent again.')}}}
  finally{lock.current=false;if(revision.current===token)setBusy(false)}
 }
 const locked=!enabled||busy||!!pending||!!historyError||!!error||!snapshot||snapshot.state==='waiting';
 const owned=!!snapshot?.owned&&snapshot.native_photo;
 return <div className="market-photo-controls" aria-label="Live photo controls">
  <p className="photo-status">{snapshot?.native_photo?`${owned?'Workbench photo session':'Photo mode opened in game'} · ${snapshot.scene_frozen?'Scene frozen':'Waiting for the scene pause'} · ${snapshot.hud_hidden?'HUD hidden':'Photo controls visible'}`:snapshot?photoReason(snapshot.reason):'Waiting for the photo module…'}</p>
  {!enabled&&<p>Install this mod with the game closed, then start the game and load a save.</p>}
  <label className="mod-switch"><input aria-label="Hide HUD on entering photo mode" type="checkbox" checked={hideOnEnter} disabled={locked||!!snapshot?.native_photo} onChange={e=>setHideOnEnter(e.target.checked)}/>Hide HUD when entering photo mode</label>
  <div className="mod-actions">
   <button className="primary" disabled={locked||!!snapshot?.native_photo} onClick={()=>void change({operation:'enter',hide_on_enter:hideOnEnter})}><Camera size={16}/> Enter photo mode</button>
   <button className="secondary" disabled={locked||!owned} onClick={()=>void change({operation:snapshot?.hud_hidden?'show_hud':'hide_hud'})}>{snapshot?.hud_hidden?'Show HUD':'Hide HUD'}</button>
   <button className="secondary" disabled={locked||!owned} onClick={()=>void change({operation:'leave'})}>Leave photo mode</button>
  </div>
  <p>Move and rotate the camera using the game’s photo controls. The normal photo camera and its game limits are retained. Closing this panel keeps photo mode active; leave it here or use the game’s normal photo-mode exit.</p>
  {snapshot?.native_photo&&!snapshot.owned&&<p>Leave the current photo mode normally in the game before starting a Workbench photo session.</p>}
  <button className="secondary" disabled={!enabled||busy} onClick={()=>void refresh()}>{busy?<LoaderCircle size={14} className="spin"/>:<RefreshCw size={14}/>} Check photo status</button>
  {pending&&uncertain&&<button className="secondary" disabled={busy||!!historyError} onClick={()=>{clear();setMessage('Previous photo command dismissed. It will not be sent again.')}}>I checked the result in game</button>}
  {message&&<p role="status">{message}</p>}{error&&<p role="alert" className="craft-error">{error}</p>}{historyError&&<p role="alert" className="craft-error">{historyError}</p>}
 </div>;
}
