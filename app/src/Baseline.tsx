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
  return <section className="baseline-panel recovery-center" aria-label="Ausgangsbasis und Registry-Sicherung">
    <div className="recovery-heading"><Save size={21}/><div><small>B0 · GESPEICHERTER AUSGANGSSTAND</small><h2>Ausgangsbasis & Registry-Sicherung</h2></div></div>
    <p>Übernimmt einen exportierten Inhaltsprüfbericht und sichert die dazu passende Registry im Projekt. Die großen Originalarchive werden dabei weder erneut gehasht noch kopiert.</p>
    <button className="secondary" disabled={busy} onClick={()=>void run('list')}>Prüfstände laden</button>
    {catalog&&<>
      {catalog.reports.length===0?<p>Keine exportierten Inhaltsprüfberichte vorhanden. Nach einer vollständigen Inhaltsprüfung zuerst den Prüfbericht exportieren.</p>:<div className="recovery-selection"><label>Exportierter Prüfbericht<select aria-label="Prüfbericht auswählen" disabled={busy} value={report} onChange={e=>{generation.current++;setReport(e.target.value);setPreview(null);setSaved(null);setError('')}}><option value="">Bitte auswählen</option>{catalog.reports.map(name=><option key={name} value={name}>{name}</option>)}</select></label><button className="secondary" disabled={busy||!report} onClick={()=>void run('preview')}>Bericht für Ausgangsbasis prüfen</button></div>}
      {catalog.snapshots.length>0&&<div className="recovery-selection"><label>Gespeicherter Ausgangsstand<select aria-label="Ausgangsstand auswählen" disabled={busy} value={snapshot} onChange={e=>{generation.current++;setSnapshot(e.target.value);setPreview(null);setSaved(null);setError('')}}><option value="">Bitte auswählen</option>{catalog.snapshots.map(id=><option key={id} value={id}>{id}</option>)}</select></label><button className="secondary" disabled={busy||!snapshot} onClick={()=>void run('inspect')}>Gespeicherten Stand prüfen</button></div>}
      {catalog.truncated&&<p>Je Liste werden höchstens 200 Einträge nach Dateiname angezeigt.</p>}
    </>}
    {busy&&<p role="status">Ausgangsstand wird geprüft …</p>}
    {error&&<div className="craft-error" role="alert">{error}<p>Erneut prüfen, bevor du speicherst. Vorhandene Ausgangsstände werden nicht überschrieben.</p></div>}
    {info&&<div className="recovery-review">
      <strong role="status">{saved?saved.current_state==='metadata_match'?'Registry-Sicherung geprüft · Metadaten passen zum gespeicherten Stand':saved.current_state==='changed'?'Registry-Sicherung erhalten · Installation weicht ab':'Registry-Sicherung erhalten · Installation nicht prüfbar':'Bericht passt zu Dateiliste und Registry · bereit zum Speichern'}</strong>
      <p>Inhaltsprüfung vom {new Date(info.audited_at*1000).toLocaleString('de-DE')} · Build {info.build_id}. {count(info.file_count)} Dateien, darunter {count(info.archive_count)} PAZ-Archive.</p>
      <p>Registry-Sicherung: {count(info.registry_bytes)} Bytes. {info.read_schema_matches?'Die gespeicherten Metadatenhashes passen zum bekannten Leseschema.':'Für diesen Stand ist kein passendes Leseschema bestätigt.'}</p>
      <p>Der aktuelle Vergleich prüft Metadaten und Registry. Gleich große Archivänderungen können dabei unentdeckt bleiben. Vor Live-Apply sind die betroffenen Inhalte erneut zu prüfen. Vanilla-Herkunft unbestätigt · Live-Apply gesperrt.</p>
      {saved&&<><code>{saved.directory}</code>{saved.issues.length>0&&<ul role="alert">{saved.issues.map(issue=><li key={issue}>{issue}</li>)}</ul>}</>}
      <details><summary>Nachweise des Ausgangsstands</summary><code>Registry SHA-256: {info.registry_sha256}</code><p>Erfasste Startprogramme:</p><ul>{info.executables.map(file=><li key={file}>{file}</li>)}</ul><p>Gespeichert werden der unveränderte Prüfbericht, die Registry-Kopie und ein Abschlussmanifest. Manuelle Spieltests sind bis zum Ende aller Entwicklungsphasen zurückgestellt.</p></details>
      {preview&&<button className="secondary" disabled={busy} onClick={()=>void run('save')}>Ausgangsbasis speichern</button>}
    </div>}
  </section>;
}
