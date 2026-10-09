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
        if(!result.registry_restored)throw Error('Wiederherstellung nicht bestätigt. Bitte erneut prüfen.');
        setSuccess(result.pending_recovered?'Unterbrochene Projektprobe abgeschlossen und Ausgangszustand wiederhergestellt.':'Projektkopie wiederhergestellt. Registry und Sicherung stimmen überein.');
        // A new review always comes from the backend, never optimistic local state.
        const fresh=await api.rehearsalReview(session,result.directory);
        if(id===generation.current)setReview(fresh);
      }
    } catch(e) { if(id===generation.current)setError(asError(e).message); }
    finally { if(id===generation.current)setBusy(false); }
  }
  return <section className="recovery-center" aria-label="Sicherung und Wiederherstellung">
    <div className="recovery-heading"><ShieldCheck size={21}/><div><small>B0 · PROJEKTKOPIEN</small><h2>Sicherung & Wiederherstellung</h2></div></div>
    <p>Vorhandene Projektproben prüfen und nach einem Abbruch wiederherstellen. Backup, Quellen und eigene Overlaydateien werden vor jedem Restore erneut geprüft. Das betrifft ausschließlich die ausgewählte Projektkopie.</p>
    <div className="audit-actions"><button className="secondary" disabled={busy} onClick={()=>void run('list')}>Projektproben laden</button>{latestDirectory&&<button className="secondary" disabled={busy} onClick={()=>void run('inspect',latestDirectory)}>Letzte Projektprobe prüfen</button>}</div>
    {listing&&<>
      {listing.entries.length===0?<p>Keine Projektproben vorhanden. Erstelle zuerst eine Modvorschau und wähle „Probe an Projektkopie“.</p>:<div className="recovery-selection"><label>Projektprobe<select aria-label="Projektprobe auswählen" disabled={busy} value={selected} onChange={e=>{generation.current++;setSelected(e.target.value);setReview(null);setError('');setSuccess('')}}><option value="">Bitte auswählen</option>{listing.entries.map(entry=><option value={entry.directory} key={entry.directory} disabled={entry.error!==null}>{entry.name}{entry.error?' · nicht verfügbar':''}</option>)}</select></label><button className="secondary" disabled={busy||!selected} onClick={()=>void run('inspect')}>Backup und Rücknahme prüfen</button></div>}
      {listing.truncated&&<p>Die Liste zeigt die letzten 200 Einträge nach Ordnername.</p>}
      {listing.entries.some(e=>e.error)&&<details><summary>Nicht verfügbare Einträge</summary><ul>{listing.entries.filter(e=>e.error).map(e=><li key={e.directory}>{e.name}: {e.error}</li>)}</ul></details>}
    </>}
    {busy&&<p role="status">Projektprobe wird geprüft …</p>}
    {error&&<div className="craft-error" role="alert">{error}<p>Es erfolgt kein automatischer Wiederholungsversuch. Prüfe die Projektprobe erneut.</p></div>}
    {success&&<p className="recovery-success" role="status">{success}</p>}
    {review&&<div className="recovery-review">
      <strong role="status">{review.can_restore?'Sicherung geprüft · Rücknahme vorbereitet':'Sicherung geprüft · bereits im Ausgangszustand'}</strong>
      <p>{count(review.protected_source_files)} unveränderte Quelldateien; Registry-Sicherung: {count(review.restore.backup_bytes)} Bytes.</p>
      <code>{review.directory}</code>
      {review.restore.pending_outcome&&<p>{review.restore.pending_outcome==='committed'?'Die Registry wurde bereits umgeschaltet. Die offene Transaktion wird abgeschlossen, danach wird die Sicherung wiederhergestellt.':'Die Registry wurde noch nicht umgeschaltet. Die offene Transaktion wird zurückgerollt, danach wird der Ausgangszustand wiederhergestellt.'}</p>}
      <details open={review.can_restore}><summary>Geplante Rücknahme · {review.restore.files.length} Dateien</summary>{review.restore.files.length===0?<p>Keine sichtbaren Dateiänderungen nötig.</p>:<ol className="recovery-files">{review.restore.files.map(file=><li key={file.path}><strong>{file.path}</strong><span>{file.action==='remove'?'Entfernen':'Aus Sicherung ersetzen'}</span><code>Vorher: {file.before_sha256}<br/>Nachher: {file.after_sha256??'nicht vorhanden'}</code></li>)}</ol>}
      {review.restore.remove_directories.length>0&&<p>Anschließend leere eigene Gruppenordner entfernen: {review.restore.remove_directories.join(', ')}.</p>}
      <p>Die Wiederherstellung schreibt ein Transaktionsprotokoll und einen Ergebnisbericht. Die Sicherung und frühere Protokolle bleiben erhalten.</p>
      <code>SHA-256 der Sicherung: {review.restore.backup_sha256}</code></details>
      <p>{review.scope}</p>
      <button className="secondary" disabled={busy||!review.can_restore} onClick={()=>void run('restore')}><RotateCcw size={15}/>Projektkopie wiederherstellen</button>
    </div>}
  </section>;
}
