import {useEffect,useRef,useState} from 'react';
import {FileWarning} from 'lucide-react';
import {api,asError} from './api';
import {count} from './format';
import type {ForeignInspection} from './foreign-types';

export function ForeignFiles({session,changed}:{session:number;changed:()=>void}) {
 const [inspection,setInspection]=useState<ForeignInspection|null>(null),[confirmed,setConfirmed]=useState(false),[busy,setBusy]=useState(false),[error,setError]=useState(''),[message,setMessage]=useState('');
 const generation=useRef(0),pending=useRef(false);
 useEffect(()=>{generation.current++;setInspection(null);setConfirmed(false);setBusy(false);setError('');setMessage('');pending.current=false;return()=>{generation.current++}},[session]);
 async function work(action:(gen:number)=>Promise<void>){
  if(pending.current)return;pending.current=true;setBusy(true);setError('');setMessage('');const gen=generation.current;
  try{await action(gen)}catch(e){if(gen===generation.current){setError(asError(e).message);setConfirmed(false);setInspection(null)}}finally{if(gen===generation.current){pending.current=false;setBusy(false)}}
 }
 async function load(gen:number){setInspection(null);setConfirmed(false);const next=await api.foreignPreview(session);if(gen===generation.current)setInspection(next)}
 async function confirm(gen:number){
  if(!inspection?.review||!confirmed)return;
  const token=inspection.review.review_id;setInspection(null);setConfirmed(false);
  const receipt=await api.foreignConfirm(session,token,true);
  if(gen===generation.current){setMessage(`${count(receipt.files)} Additional files approved for keeping. Game files unchanged. Reload the file list and live preview.`);changed()}
 }
 async function revoke(gen:number){
  if(!inspection?.approval_id)return;
  const id=inspection.approval_id;setInspection(null);setConfirmed(false);await api.foreignRevoke(session,id);
  if(gen===generation.current){setMessage('Approval revoked. Additional files are preserved; live operations require an approved file inventory again.');changed()}
 }
 const review=inspection?.review,hasExtras=!!review&&(review.files.length>0||review.directories.length>0);
 return <section className="recovery-center" aria-label="Third-party additional files">
  <div className="recovery-heading"><FileWarning size={21}/><div><small>B0 · ADDITIONAL FILES</small><h2>Third-party mods & additional files</h2></div></div>
  <p>Additional files may come from mods, other managers or normal runtime data. Review the exact list before approving them. Modified original files and a third-party registry remain blocked.</p>
  <p>Approval is stored only in the workspace and does not launch anything. Approved files are hashed again, protected and preserved during live operations. This does not prove compatibility.</p>
  <button className="secondary" disabled={busy} onClick={()=>void work(load)}>Check additional files</button>
  {busy&&<p role="status">Checking additional files …</p>}
  {(error||inspection?.issue)&&<p role="alert" className="craft-error">{error||inspection?.issue}</p>}
  {inspection?.approval_id&&<div className="recovery-review"><p>Existing approval: {count(inspection.approved_files)} files. Changes or new files require a new preview.</p><button className="secondary" disabled={busy} onClick={()=>void work(revoke)}>Revoke approval</button></div>}
  {review&&<div className="recovery-review"><code>{review.game_path}</code>
   {hasExtras?<><p>{count(review.files.length)} files · {count(review.directories.length)} additional folders · {(Number(review.total_bytes)/1048576).toLocaleString('en-US',{maximumFractionDigits:1})} MiB</p>
    <div className="mod-files">{review.files.map(f=><div key={f.path}><strong>{f.path}</strong><span>Keep · {count(f.bytes)} Bytes</span><code>SHA-256: {f.sha256}</code></div>)}</div>
    {review.directories.length>0&&<details><summary>Additional folders</summary><ul>{review.directories.map(p=><li key={p}><code>{p}</code></li>)}</ul></details>}
    <label className="mod-switch"><input type="checkbox" aria-label="Explicitly keep additional files" checked={confirmed} disabled={busy} onChange={e=>setConfirmed(e.target.checked)}/>I want to keep these exact additional files unchanged and approve their coexistence with Workbench mods.</label>
    <button className="primary" disabled={busy||!confirmed} onClick={()=>void work(confirm)}>Approve checked additional files</button>
   </>:<p>No unassigned additional files found. No approval needed.</p>}
  </div>}
  {message&&<p role="status" className="mod-success">{message}</p>}
 </section>;
}
