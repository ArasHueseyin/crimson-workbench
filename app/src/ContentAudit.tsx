import { useEffect, useRef, useState } from 'react';
import { api, asError } from './api';
import type { AuditSnapshot, InstallationInventory } from './mod-types';

const active = (s: AuditSnapshot | null) => s !== null && ['preparing','hashing','validating'].includes(s.phase);
function size(value: string) { const gib=Number(value)/1073741824;return gib>=1?`${gib.toLocaleString('en-US',{maximumFractionDigits:2})} GiB`:`${(Number(value)/1048576).toLocaleString('en-US',{maximumFractionDigits:1})} MiB`; }
export function ContentAudit({session,inventory}:{session:number;inventory:InstallationInventory|null}) {
  const [job,setJob]=useState<AuditSnapshot|null>(null),[error,setError]=useState(''),[exported,setExported]=useState('');
  const [pending,setPending]=useState(false),[cancelling,setCancelling]=useState(false);
  const generation=useRef(0),action=useRef(0);
  useEffect(()=>{
    const gen=++generation.current;let timer:ReturnType<typeof setTimeout>|undefined;
    setJob(null);setError('');setExported('');setPending(false);setCancelling(false);
    async function poll(){
      const seq=action.current;
      try{const value=await api.auditStatus(session);if(gen===generation.current&&seq===action.current){setJob(value);if(!active(value))setCancelling(false);}}
      catch(e){if(gen===generation.current)setError(asError(e).message);}
      finally{if(gen===generation.current)timer=setTimeout(()=>void poll(),500);}
    }
    void poll();return()=>{generation.current++;if(timer)clearTimeout(timer);};
  },[session]);
  const eligible=inventory!==null&&inventory.game_running===false&&inventory.directory_scan_complete&&inventory.depot_comparison_available&&inventory.issues.length===0&&inventory.files.every(f=>f.state==='size_matches');
  async function start(){
    const gen=generation.current;action.current++;setPending(true);setError('');setExported('');
    try{const value=await api.auditStart(session);if(gen===generation.current)setJob(value);}
    catch(e){if(gen===generation.current)setError(asError(e).message);}
    finally{if(gen===generation.current)setPending(false);}
  }
  async function cancel(){
    if(!job)return;const gen=generation.current;setCancelling(true);setError('');
    try{await api.auditCancel(session,job.id);}catch(e){if(gen===generation.current){setError(asError(e).message);setCancelling(false);}}
  }
  async function save(){
    if(!job)return;const gen=generation.current;setPending(true);setError('');
    try{const file=await api.auditExport(session,job.id);if(gen===generation.current)setExported(file);}
    catch(e){if(gen===generation.current)setError(asError(e).message);}
    finally{if(gen===generation.current)setPending(false);}
  }
  const running=active(job), result=job?.phase==='complete'?job.report:null;
  const progress=job?.progress, fraction=progress&&Number(progress.total_bytes)>0?Math.min(100,Number(progress.bytes_done)/Number(progress.total_bytes)*100):0;
  return <section className="content-audit" aria-label="Content audit">
    <h3>Full content audit</h3>
    <p>Reads every file in the depot lists and compares their contents. {inventory?.expected_bytes&&`Scope: ${size(inventory.expected_bytes)}. `}Start after closing the game; this may take a while. A detected game launch cancels the audit.</p>
    <div className="audit-actions"><button className="secondary" type="button" disabled={!eligible||running||pending} onClick={()=>void start()}>Check file contents</button>
      {running&&<button className="secondary" type="button" disabled={cancelling} onClick={()=>void cancel()}>{cancelling?'Cancellation requested …':'Cancel content audit'}</button>}
      {result&&<button className="secondary" type="button" disabled={pending} onClick={()=>void save()}>Export audit report</button>}</div>
    {!eligible&&!running&&<p>{!inventory?'Check the file list first.':inventory.game_running!==false?'Game running or status unknown. Check the file list again after closing the game.':'The file list is incomplete or differs. Check the notes above.'}</p>}
    {running&&progress&&<div role="status"><p>{job?.phase==='preparing'?'Preparing audit …':job?.phase==='validating'?'Final metadata comparison …':`${progress.files_done} / ${progress.total_files} files · ${size(progress.bytes_done)} / ${size(progress.total_bytes)}`}</p><progress aria-label="Content audit progress" max={100} value={fraction}/><code>{progress.current_path}</code><p>The audit continues when switching views. Changing the installation or item language cancels it.</p></div>}
    {(error||job?.error)&&<p role="alert" className="craft-error">{error||job?.error}</p>}
    {result&&<div className="audit-result" role="status"><strong>{result.all_files_match_cache?'All file contents match the local Steam cache.':'Content differences found.'}</strong><p>{result.files.length} depot files checked. As of: {new Date(result.finished_at*1000).toLocaleString('en-US')}. Not a vanilla certificate · Live apply remains blocked.</p>{result.foreign_approval&&<p>Additional third-party files were approved separately. This report checks only depot originals and does not confirm compatibility.</p>}<ul>{result.files.filter(f=>!f.matches_cache).map(f=><li key={f.path}><code>{f.path}</code> · Content differs</li>)}</ul><details><summary>Report limitations</summary><ul>{result.limitations.map(s=><li key={s}>{s}</li>)}</ul></details></div>}
    {exported&&<p className="audit-export" role="status">Audit report saved: <code>{exported}</code></p>}
  </section>;
}
