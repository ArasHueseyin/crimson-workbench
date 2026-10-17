import {Dialog} from './Dialog';
import {SaveSelector,SaveTarget} from './SaveSelector';
import {useEffect,useRef,useState} from 'react';
import {api,asError} from './api';
import type {Catalog} from './types';
import type {MapFogPreview,MapFogReceipt,MapFogRequest} from './map-fog-types';
const percent=(value:number,total:number)=>new Intl.NumberFormat('en-US',{style:'percent',maximumFractionDigits:1}).format(value/total);

export function MarketplaceMap({catalog,close}:{catalog:Catalog;close:()=>void}) {
 const [snapshot,setSnapshot]=useState<MapFogPreview|null>(null),[save,setSave]=useState('');
 const [checked,setChecked]=useState(false),[busy,setBusy]=useState(false),[error,setError]=useState(''),[receipt,setReceipt]=useState<MapFogReceipt|null>(null);
 const storage=`crimson-map-reveal-pending:${catalog.game_path}`;
 const [pending,setPending]=useState<MapFogRequest|null>(()=>{try{return JSON.parse(localStorage.getItem(storage)??'null') as MapFogRequest|null}catch{return null}});
 const mounted=useRef(true),generation=useRef(0),lock=useRef(false);
 useEffect(()=>{mounted.current=true;return()=>{mounted.current=false;generation.current++}},[]);
 async function load(id:string|null) {
  const current=++generation.current;setBusy(true);setChecked(false);setError('');setSnapshot(s=>s?{...s,plan:null}:s);
  try{const next=await api.mapFogPreview(catalog.session,id);if(mounted.current&&current===generation.current)setSnapshot(next)}
  catch(e){if(mounted.current&&current===generation.current)setError(asError(e).message)}
  finally{if(mounted.current&&current===generation.current)setBusy(false)}
 }
 useEffect(()=>{void load(null)},[catalog.session]);
 async function apply(request:MapFogRequest) {
  if(lock.current)return;lock.current=true;setBusy(true);setError('');setChecked(false);
  try{const result=await api.mapFogApply(catalog.session,request);localStorage.removeItem(storage);if(mounted.current){setPending(null);setReceipt(result);setSave(request.save);await load(request.save)}}
  catch(e){if(mounted.current)setError(asError(e).message)}
  finally{lock.current=false;if(mounted.current)setBusy(false)}
 }
 function confirm() {
  const plan=snapshot?.plan;if(!plan||!snapshot.selected_save||!checked)return;
  const request:MapFogRequest={save_root:snapshot.save_root,id:crypto.randomUUID().replaceAll('-',''),save:snapshot.selected_save,save_sha256:plan.save_sha256,lobby_sha256:plan.lobby_sha256,plan_id:plan.plan_id};
  try{localStorage.setItem(storage,JSON.stringify(request));setPending(request);void apply(request)}catch(e){setError(asError(e).message)}
 }
 const plan=snapshot?.plan,changed=!!plan&&plan.before.revealed_cells<plan.after.revealed_cells;
 return <Dialog title="Reveal world map" eyebrow="SAVE ACTION" onClose={close} closeDisabled={busy}>
  <p>Remove world map fog in one save. Quest progress, knowledge, discovered locations and fast-travel unlocks keep their existing values. Map markers still follow their normal discovery rules.</p>
  <SaveSelector saves={snapshot?.saves??[]} value={save||null} ariaLabel="Map reveal save" disabled={busy||!!pending} onChange={value=>{setSave(value);setReceipt(null);void load(value||null)}}/>
  {snapshot&&<p>Save folder: <code>{snapshot.save_root}</code></p>}
  {snapshot&&!snapshot.saves.length&&<p>No complete saves found. Select your save folder under Data sources.</p>}
  {plan&&<div className="recovery-review"><strong>Map fog in {snapshot?.selected_save}</strong><p>Revealed map: {percent(plan.before.revealed_cells,plan.before.total_cells)} → {percent(plan.after.revealed_cells,plan.after.total_cells)}</p>{!changed&&<p>This map is already fully revealed.</p>}</div>}
  {snapshot?.game_running&&<p role="alert" className="craft-error">Save and fully close Crimson Desert before applying this action.</p>}
  {busy&&<p role="status">Checking the save and preparing the map preview …</p>}
  {error&&<p role="alert" className="craft-error">{error}</p>}
  {pending&&<div className="recovery-review"><p>A saved map reveal request for {pending.save} needs a result check. Retry the same request to recover its result.</p><button className="secondary" disabled={busy} onClick={()=>void apply(pending)}>Check / recover pending map reveal</button><button className="secondary" disabled={busy} onClick={()=>{localStorage.removeItem(storage);setPending(null);void load(save||null)}}>Discard saved request & reload map</button></div>}
  {receipt&&<div className="recovery-success" role="status"><p>{receipt.message}</p><strong>Backup folder</strong><code>{receipt.backup}</code></div>}
  <SaveTarget saves={snapshot?.saves??[]} value={snapshot?.selected_save??null}/>
  <label className="mod-switch"><input type="checkbox" aria-label="Confirm map reveal save edit" checked={checked} disabled={busy||!!pending||!changed||snapshot?.game_running} onChange={e=>setChecked(e.target.checked)}/>I want to reveal world map fog in this save.</label>
  <button className="primary" disabled={busy||!!pending||!checked||!changed||snapshot?.game_running} onClick={confirm}>Back up & reveal map</button>
 </Dialog>;
}
