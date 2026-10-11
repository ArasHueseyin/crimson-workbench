import { useEffect, useRef, useState } from 'react';
import { Save } from 'lucide-react';
import { api, asError } from './api';
import { count } from './format';
import type { BaselineCatalog, BaselinePreview, BaselineSaved } from './baseline-types';

export function Baseline({session}:{session:number}) {
  const [catalog,setCatalog]=useState<BaselineCatalog|null>(null);
  const [report,setReport]=useState(''),[snapshot,setSnapshot]=useState('');
  const [preview,setPreview]=useState<BaselinePreview|null>(null),[saved,setSaved]=useState<BaselineSaved|null>(null);
  const [busy,setBusy]=useState(false),[error,setError]=useState('');
  const generation=useRef(0);
  useEffect(()=>{generation.current++;setCatalog(null);setReport('');setSnapshot('');setPreview(null);setSaved(null);setBusy(false);setError('');return()=>{generation.current++}},[session]);
  async function run(action:'list'|'preview'|'save'|'inspect') {
    const gen=++generation.current;const selected=preview;
    setBusy(true);setError('');setPreview(null);setSaved(null);
    try {
      if(action==='list') {
        const value=await api.baselineCatalog(session);
        if(gen===generation.current){setCatalog(value);setReport('');setSnapshot('')}
      } else if(action==='preview') {
        const value=await api.baselinePreview(session,report);
        if(gen===generation.current)setPreview(value);
      } else {
        if(action==='save'&&!selected)return;
        const value=action==='save'?await api.baselineCapture(session,selected!.report_name,selected!.review_id):await api.baselineInspect(session,snapshot);
        if(gen!==generation.current)return;
        setSaved(value);setSnapshot(value.id);
        if(action==='save') {
          const fresh=await api.baselineCatalog(session);
          if(gen===generation.current)setCatalog(fresh);
        }
      }
    } catch(e) {if(gen===generation.current)setError(asError(e).message)}
    finally {if(gen===generation.current)setBusy(false)}
  }
  const info=preview??saved?.preview;
  return <section className="baseline-panel recovery-center" aria-label="Baseline and registry backup">
    <div className="recovery-heading"><Save size={21}/><div><small>B0 · SAVED BASELINE</small><h2>Baseline & registry backup</h2></div></div>
    <p>Imports an exported content audit and backs up its matching registry in the workspace. Large original archives are neither hashed again nor copied.</p>
    <button className="secondary" disabled={busy} onClick={()=>void run('list')}>Load audit reports</button>
    {catalog&&<>
      {catalog.reports.length===0?<p>No exported content audits found. Export a report after completing a full content audit first.</p>:<div className="recovery-selection"><label>Exported audit report<select aria-label="Select audit report" disabled={busy} value={report} onChange={e=>{generation.current++;setReport(e.target.value);setPreview(null);setSaved(null);setError('')}}><option value="">Please select</option>{catalog.reports.map(name=><option key={name} value={name}>{name}</option>)}</select></label><button className="secondary" disabled={busy||!report} onClick={()=>void run('preview')}>Check report for baseline</button></div>}
      {catalog.snapshots.length>0&&<div className="recovery-selection"><label>Saved baseline<select aria-label="Select baseline" disabled={busy} value={snapshot} onChange={e=>{generation.current++;setSnapshot(e.target.value);setPreview(null);setSaved(null);setError('')}}><option value="">Please select</option>{catalog.snapshots.map(id=><option key={id} value={id}>{id}</option>)}</select></label><button className="secondary" disabled={busy||!snapshot} onClick={()=>void run('inspect')}>Check saved baseline</button></div>}
      {catalog.truncated&&<p>Each list shows up to 200 entries sorted by filename.</p>}
    </>}
    {busy&&<p role="status">Checking baseline …</p>}
    {error&&<div className="craft-error" role="alert">{error}<p>Check again before saving. Existing baselines are not overwritten.</p></div>}
    {info&&<div className="recovery-review">
      <strong role="status">{saved?saved.current_state==='metadata_match'?'Registry backup verified · Metadata matches the saved baseline':saved.current_state==='changed'?'Registry backup preserved · Installation differs':'Registry backup preserved · Installation cannot be checked':'Report matches file list and registry · Ready to save'}</strong>
      <p>Content audit from {new Date(info.audited_at*1000).toLocaleString('en-US')} · Build {info.build_id}. {count(info.file_count)} files, including {count(info.archive_count)} PAZ archives.</p>
      <p>Registry backup: {count(info.registry_bytes)} Bytes. {info.read_schema_matches?'Saved metadata hashes match the known reading schema.':'No matching reading schema is verified for this baseline.'}</p>
      <p>This comparison checks metadata and the registry. Archive changes of the same size may go undetected. Affected contents must be checked again before live apply. Vanilla origin unconfirmed · Live apply blocked.</p>
      {saved&&<><code>{saved.directory}</code>{saved.issues.length>0&&<ul role="alert">{saved.issues.map(issue=><li key={issue}>{issue}</li>)}</ul>}</>}
      <details><summary>Baseline evidence</summary><code>Registry SHA-256: {info.registry_sha256}</code><p>Detected launchers:</p><ul>{info.executables.map(file=><li key={file}>{file}</li>)}</ul><p>Saves the unchanged audit report, a registry copy and a completion manifest. Manual game tests remain pending.</p></details>
      {preview&&<button className="secondary" disabled={busy} onClick={()=>void run('save')}>Save baseline</button>}
    </div>}
  </section>;
}
