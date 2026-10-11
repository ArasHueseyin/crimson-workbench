import { useEffect, useRef, useState } from 'react';
import { RotateCcw, ShieldCheck } from 'lucide-react';
import { api, asError } from './api';
import { count } from './format';
import type { RecoveryListing, RecoveryReview } from './recovery-types';

export function RecoveryCenter({session,latestDirectory}:{session:number;latestDirectory?:string}) {
  const [listing,setListing]=useState<RecoveryListing|null>(null);
  const [selected,setSelected]=useState(''),[review,setReview]=useState<RecoveryReview|null>(null);
  const [busy,setBusy]=useState(false),[error,setError]=useState(''),[success,setSuccess]=useState('');
  const generation=useRef(0);
  useEffect(()=>{
    generation.current++;setListing(null);setSelected('');setReview(null);setBusy(false);setError('');setSuccess('');
    return()=>{generation.current++};
  },[session]);
  async function run(action:'list'|'inspect'|'restore',directory=selected) {
    const id=++generation.current;
    const previous=review;
    setBusy(true);setError('');setSuccess('');setReview(null);
    try {
      if(action==='list') {
        const result=await api.rehearsalList(session);
        if(id!==generation.current)return;
        setListing(result);setSelected('');
      } else if(action==='inspect') {
        const result=await api.rehearsalReview(session,directory);
        if(id!==generation.current)return;
        setSelected(directory);setReview(result);
      } else if(previous?.can_restore) {
        const result=await api.rehearsalRestore(session,previous.directory,previous.review_id);
        if(id!==generation.current)return;
        if(!result.registry_restored)throw Error('Recovery not confirmed. Please check again.');
        setSuccess(result.pending_recovered?'Interrupted workspace rehearsal completed and original state restored.':'Workspace copy restored. Registry and backup match.');
        // A new review always comes from the backend, never optimistic local state.
        const fresh=await api.rehearsalReview(session,result.directory);
        if(id===generation.current)setReview(fresh);
      }
    } catch(e) { if(id===generation.current)setError(asError(e).message); }
    finally { if(id===generation.current)setBusy(false); }
  }
  return <section className="recovery-center" aria-label="Backup and recovery">
    <div className="recovery-heading"><ShieldCheck size={21}/><div><small>B0 · WORKSPACE COPIES</small><h2>Backup & recovery</h2></div></div>
    <p>Check existing workspace rehearsals and recover after interruption. Backup, sources and owned overlay files are checked again before each restore. Only the selected workspace copy is affected.</p>
    <div className="audit-actions"><button className="secondary" disabled={busy} onClick={()=>void run('list')}>Load workspace rehearsals</button>{latestDirectory&&<button className="secondary" disabled={busy} onClick={()=>void run('inspect',latestDirectory)}>Check latest workspace rehearsal</button>}</div>
    {listing&&<>
      {listing.entries.length===0?<p>No workspace rehearsals found. Create a mod preview and select “Rehearse on workspace copy” first.</p>:<div className="recovery-selection"><label>Workspace rehearsal<select aria-label="Select workspace rehearsal" disabled={busy} value={selected} onChange={e=>{generation.current++;setSelected(e.target.value);setReview(null);setError('');setSuccess('')}}><option value="">Please select</option>{listing.entries.map(entry=><option value={entry.directory} key={entry.directory} disabled={entry.error!==null}>{entry.name}{entry.error?' · unavailable':''}</option>)}</select></label><button className="secondary" disabled={busy||!selected} onClick={()=>void run('inspect')}>Check backup and rollback</button></div>}
      {listing.truncated&&<p>The list shows the last 200 entries sorted by folder name.</p>}
      {listing.entries.some(e=>e.error)&&<details><summary>Unavailable entries</summary><ul>{listing.entries.filter(e=>e.error).map(e=><li key={e.directory}>{e.name}: {e.error}</li>)}</ul></details>}
    </>}
    {busy&&<p role="status">Checking workspace rehearsal …</p>}
    {error&&<div className="craft-error" role="alert">{error}<p>There is no automatic retry. Check the workspace rehearsal again.</p></div>}
    {success&&<p className="recovery-success" role="status">{success}</p>}
    {review&&<div className="recovery-review">
      <strong role="status">{review.can_restore?'Backup verified · Rollback prepared':'Backup verified · Already in original state'}</strong>
      <p>{count(review.protected_source_files)} unchanged source files; registry backup: {count(review.restore.backup_bytes)} Bytes.</p>
      <code>{review.directory}</code>
      {review.restore.pending_outcome&&<p>{review.restore.pending_outcome==='committed'?'The registry has already been switched. The pending transaction will be completed, then the backup restored.':'The registry has not been switched yet. The pending transaction will be rolled back, then the original state restored.'}</p>}
      <details open={review.can_restore}><summary>Planned rollback · {review.restore.files.length} files</summary>{review.restore.files.length===0?<p>No visible file changes needed.</p>:<ol className="recovery-files">{review.restore.files.map(file=><li key={file.path}><strong>{file.path}</strong><span>{file.action==='remove'?'Remove':'Replace from backup'}</span><code>Before: {file.before_sha256}<br/>After: {file.after_sha256??'absent'}</code></li>)}</ol>}
      {review.restore.remove_directories.length>0&&<p>Then remove empty owned group folders: {review.restore.remove_directories.join(', ')}.</p>}
      <p>Recovery writes a transaction log and a result report. The backup and previous logs are preserved.</p>
      <code>Backup SHA-256: {review.restore.backup_sha256}</code></details>
      <p>{review.scope}</p>
      <button className="secondary" disabled={busy||!review.can_restore} onClick={()=>void run('restore')}><RotateCcw size={15}/>Restore workspace copy</button>
    </div>}
  </section>;
}
