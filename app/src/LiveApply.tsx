import {useEffect,useRef,useState} from 'react';
import {ShieldCheck} from 'lucide-react';
import {api,asError} from './api';
import {count} from './format';
import type {ModRequest} from './mod-types';
import type {LiveUpdate,LiveStatus,LiveSetup,LiveReview,LiveRequest,LiveOperation,LiveJob} from './live-types';

export function LiveApply({session,settings,onStatus}:{session:number;settings:ModRequest|null;onStatus?:(status:LiveStatus|null)=>void}) {
 const [status,setStatus]=useState<LiveStatus|null>(null),[ids,setIds]=useState<string[]>([]),[id,setId]=useState('');
 const [update,setUpdate]=useState<LiveUpdate|null>(null);
 const [setup,setSetup]=useState<LiveSetup|null>(null),[verified,setVerified]=useState(false);
 const [review,setReview]=useState<LiveReview|null>(null),[request,setRequest]=useState<LiveRequest|null>(null);
 const [job,setJob]=useState<LiveJob|null>(null),[busy,setBusy]=useState(false),[error,setError]=useState('');
 const generation=useRef(0),pending=useRef(false),settingsKey=JSON.stringify(settings),latest=useRef(settingsKey);
 latest.current=settingsKey;
 useEffect(()=>{onStatus?.(status)},[status,onStatus]);
 useEffect(()=>{generation.current++;setStatus(null);setIds([]);setId('');setSetup(null);setUpdate(null);setVerified(false);setReview(null);setRequest(null);setJob(null);setBusy(false);pending.current=false;setError('');return()=>{generation.current++}},[session]);
 useEffect(()=>{setReview(null);setRequest(null)},[settingsKey]);
 const running=job?.phase==='running';
 useEffect(()=>{
  if(!running)return;
  const gen=generation.current;let closed=false;let timer:ReturnType<typeof setTimeout>;
  async function poll(){
   try {
    const next=await api.liveJobStatus(session);
    if(closed||gen!==generation.current)return;
    if(!next){setError('Live job no longer available. Reload status.');return;}
    if(next.phase==='running'){setJob(next);timer=setTimeout(()=>void poll(),700)}
    else {
     try{const fresh=await api.liveStatus(session);if(!closed&&gen===generation.current)setStatus(fresh)}catch(e){if(!closed&&gen===generation.current)setError(asError(e).message)}
     if(!closed&&gen===generation.current)setJob(next);
    }
   }catch(e){if(!closed&&gen===generation.current){setError(asError(e).message);timer=setTimeout(()=>void poll(),2000)}}
  }
  timer=setTimeout(()=>void poll(),300);
  return()=>{closed=true;clearTimeout(timer)};
 },[running,session]);
 async function work(action:()=>Promise<void>){
  if(pending.current)return;
  pending.current=true;setBusy(true);setError('');const gen=generation.current;
  try{await action()}catch(e){if(gen===generation.current)setError(asError(e).message)}finally{if(gen===generation.current){setBusy(false);pending.current=false}}
 }
 async function load(){
  const gen=generation.current;setSetup(null);setUpdate(null);setVerified(false);setReview(null);setRequest(null);
  const [s,c,j]=await Promise.all([api.liveStatus(session),api.baselineCatalog(session),api.liveJobStatus(session)]);
  if(gen===generation.current){setStatus(s);setIds(c.snapshots);setJob(j);setId(s.pending_basis??s.baseline_id??'')}
 }
 async function prepareSetup(){const gen=generation.current;setSetup(null);setUpdate(null);setVerified(false);const value=await api.liveSetupPreview(session,id);if(gen===generation.current)setSetup(value)}
 async function prepareUpdate(){const gen=generation.current;setUpdate(null);setSetup(null);setReview(null);setRequest(null);setVerified(false);const value=await api.liveUpdatePreview(session,id);if(gen===generation.current)setUpdate(value)}
 async function prepare(action:'apply'|'restore'){
  if(action==='apply'&&!settings)return;
  const gen=generation.current,key=latest.current;setUpdate(null);setVerified(false);setReview(null);setRequest(null);
  const req:LiveRequest=action==='restore'?{action:'restore'}:{action:'apply',settings:settings!};
  const value=await api.livePreview(session,req);
  if(gen===generation.current&&latest.current===key){setReview(value);setRequest(req)}
 }
 async function start(operation:LiveOperation){
  const gen=generation.current;setReview(null);setRequest(null);setSetup(null);setUpdate(null);setVerified(false);setJob(null);
  const value=await api.liveStart(session,operation);if(gen===generation.current)setJob(value);
 }
 const blocked=busy||running||status?.game_running!==false;
 return <section className="recovery-center live-panel" aria-label="Live apply and restore">
  <div className="recovery-heading"><ShieldCheck size={21}/><div><small>B0 · INSTALLATION</small><h2>Live apply & restore</h2></div></div>
  <p>Applies the calculated mod in a dedicated archive group. Every application starts from the original tables. Before each write, original files are fully checked and protected against changes; the game must be closed.</p>
  <p>Review the file changes before applying. Save actions such as All skills are managed separately in the marketplace.</p>
  <button className="secondary" disabled={busy||running} onClick={()=>void work(load)}>Load live status</button>
  {status&&<>
   <p role="status">{status.game_running===true?'Game running · Live writes blocked':status.game_running===false?'Game closed':'Game status unknown · Live writes blocked'} · {status.pending_basis?'Baseline change interrupted':status.initialized?status.active_overlay?'Workbench mod active':'Registry in original state':status.configured?'Setup incomplete':'Live setup missing'}</p>
   {status.issues.length>0&&<ul role="alert">{status.issues.map(s=><li key={s}>{s}</li>)}</ul>}
   {(!status.initialized&&!status.pending_basis)&&<div className="recovery-review">
    <label>Saved baseline for Live B0<select aria-label="Live baseline" disabled={busy||running} value={id} onChange={e=>{setId(e.target.value);setSetup(null);setVerified(false)}}><option value="">Please select</option>{ids.map(v=><option key={v} value={v}>{v}</option>)}</select></label>
    <button className="secondary" disabled={busy||running||!id} onClick={()=>void work(prepareSetup)}>Check live setup</button>
    {setup&&<><p>{count(setup.files)} files · Content report from {new Date(setup.audited_at*1000).toLocaleString('en-US')}. Setup hashes all contents again and backs up the registry.</p><code>{setup.game_path}</code>
     {(setup.preserved_foreign_files??0)>0&&<p>{count(setup.preserved_foreign_files!)} approved additional files are also checked and preserved unchanged.</p>}
     <label className="mod-switch"><input aria-label="Steam file verification confirmed" type="checkbox" disabled={blocked} checked={verified} onChange={e=>setVerified(e.target.checked)}/>I fully completed Steam “Verify integrity of game files” before creating this content report.</label>
     <p>If not: let Steam verify the files, then create a new content report and baseline. The existing report alone does not confirm this step.</p>
     <button className="secondary" disabled={blocked||!verified} onClick={()=>void work(()=>start({kind:'setup',baseline_id:setup.baseline_id,review_id:setup.review_id,steam_verified_before_audit:verified}))}>Verify and set up live baseline</button>
    </>}
   </div>}
   {status.configured&&<details className="recovery-review" open={status.pending_basis?true:undefined}>
    <summary>Change baseline after game update</summary>
    <p>After Steam verification completes, save a new content report and baseline. The change archives previous Workbench history and mod files. The existing original registry of the new baseline is preserved.</p>
    {status.pending_basis&&<p>Interrupted change: continue with the intended baseline.</p>}
    <label>New saved baseline<select aria-label="New live baseline" disabled={busy||running||!!status.pending_basis} value={id} onChange={e=>{setId(e.target.value);setUpdate(null);setSetup(null);setVerified(false)}}><option value="">Please select</option>{ids.map(v=><option key={v} value={v}>{v}</option>)}</select></label>
    <button className="secondary" disabled={busy||running||!id||id===status.baseline_id} onClick={()=>void work(prepareUpdate)}>Check baseline change</button>
    {update&&<><p>{update.resume?'Continue pending baseline change':'Reviewed baseline change'}: {update.previous_baseline} → {update.next_baseline}</p><p>{count(update.files.length)} files to archive · {count(update.source_files)} original files to verify again.</p><code>{update.archive_path}</code>
     <details><summary>Files to archive</summary><ul>{update.files.map(f=><li key={f.path}><code>{f.path}</code></li>)}</ul></details>
     <label className="mod-switch"><input aria-label="Steam verification for new baseline confirmed" type="checkbox" disabled={blocked} checked={verified} onChange={e=>setVerified(e.target.checked)}/>I fully completed Steam file verification before creating the content report for this new baseline.</label>
     <button className="primary" disabled={blocked||!verified} onClick={()=>void work(()=>start({kind:'update',baseline_id:update.next_baseline,review_id:update.review_id,steam_verified_before_audit:verified}))}>{update.resume?'Continue reviewed baseline change':'Execute reviewed baseline change'}</button>
    </>}
   </details>}
   {status.initialized&&!status.pending_basis&&<div className="mod-actions">
    <button className="secondary" disabled={busy||running||!settings||status.recovery_required} onClick={()=>void work(()=>prepare('apply'))}>Calculate live file preview</button>
    <button className="secondary" disabled={busy||running} onClick={()=>void work(()=>prepare('restore'))}>Check live restore</button>
    {!settings&&<span>Calculate a change plan below first.</span>}
    {status.recovery_required&&<span>Interrupted transaction: check live restore first.</span>}
   </div>}
  </>}
  {busy&&<p role="status">Preparing live request …</p>}
  {error&&<div role="alert" className="craft-error">{error}</div>}
  {review&&request&&<div className="recovery-review">
   <strong>{review.action==='apply'?'File changes for live apply':'File changes for live restore'}</strong><code>{review.game_path}</code>
   <p>{count(review.protected_source_files)} Original files are checked again before writing. This takes a few minutes for large installations.</p>
   {(review.preserved_foreign_files??0)>0&&<p>{count(review.preserved_foreign_files!)} approved third-party additional files are preserved; their hashes and approval must still match.</p>}
   <div className="mod-files">{review.files.map((f,i)=><div key={`${f.path}-${i}`}><strong>{f.path}</strong><span>{f.action==='create'?'New':f.action==='remove'?'Remove':'Replace'}</span><code>Before: {f.before_sha256??'absent'}<br/>After: {f.after_sha256??'absent'}</code></div>)}</div>
   {review.files.length===0&&!review.recovery_required?<p>No file changes required.</p>:<button className="primary" disabled={blocked} onClick={()=>void work(()=>start({kind:'execute',request,review_id:review.review_id}))}>{review.action==='apply'?'Apply reviewed mod to game':'Execute reviewed live restore'}</button>}
  </div>}
  {job&&<div className="recovery-review" role="status">
   {running?<><strong>Live operation running · Checking sources and completing transaction</strong><p>The game cannot start during this operation. Cancellation may leave a recoverable transaction.</p><button className="secondary" disabled={busy} onClick={()=>void work(()=>api.liveCancel(session,job.id))}>Cancel live operation</button></>:job.phase==='complete'?<><strong>Live operation completed.</strong>{job.result&&'registry_sha256' in job.result&&<><p>{job.result.action==='restore'?'Original registry restored.':job.result.action==='basis_update'?'New baseline active; previous Workbench files archived.':'Mod registered; original archives preserved.'}</p><code>Registry SHA-256: {job.result.registry_sha256}</code></>}</>:<><strong>{job.phase==='cancelled'?'Live operation cancelled':'Live operation failed'}</strong><p>{job.error}</p><p>Reload status and, if needed, continue the baseline change or check live restore. Writes are not retried automatically.</p></>}
  </div>}
 </section>;
}
