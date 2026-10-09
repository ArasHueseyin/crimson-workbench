import { useEffect, useRef, useState } from 'react';
import { api, asError } from './api';
import type { AuditSnapshot, InstallationInventory } from './mod-types';

const active = (s: AuditSnapshot | null) => s !== null && ['preparing','hashing','validating'].includes(s.phase);
function size(value: string) { const gib=Number(value)/1073741824;return gib>=1?`${gib.toLocaleString('de-DE',{maximumFractionDigits:2})} GiB`:`${(Number(value)/1048576).toLocaleString('de-DE',{maximumFractionDigits:1})} MiB`; }
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
  return <section className="content-audit" aria-label="Inhaltsprüfung">
    <h3>Vollständige Inhaltsprüfung</h3>
    <p>Liest sämtliche Dateien aus den Depotlisten und vergleicht ihre Inhalte. {inventory?.expected_bytes&&`Umfang: ${size(inventory.expected_bytes)}. `}Nur nach dem Spielen starten; der Lauf kann länger dauern. Ein erkannter Spielstart bricht die Prüfung ab.</p>
    <div className="audit-actions"><button className="secondary" type="button" disabled={!eligible||running||pending} onClick={()=>void start()}>Dateiinhalte prüfen</button>
      {running&&<button className="secondary" type="button" disabled={cancelling} onClick={()=>void cancel()}>{cancelling?'Abbruch angefordert …':'Inhaltsprüfung abbrechen'}</button>}
      {result&&<button className="secondary" type="button" disabled={pending} onClick={()=>void save()}>Prüfbericht exportieren</button>}</div>
    {!eligible&&!running&&<p>{!inventory?'Zuerst die Dateiliste prüfen.':inventory.game_running!==false?'Spiel läuft oder Status unbekannt. Nach dem Beenden die Dateiliste erneut prüfen.':'Die Dateiliste ist unvollständig oder abweichend. Hinweise oben prüfen.'}</p>}
    {running&&progress&&<div role="status"><p>{job?.phase==='preparing'?'Prüfung wird vorbereitet …':job?.phase==='validating'?'Abschließender Metadatenvergleich …':`${progress.files_done} / ${progress.total_files} Dateien · ${size(progress.bytes_done)} / ${size(progress.total_bytes)}`}</p><progress aria-label="Inhaltsprüfung Fortschritt" max={100} value={fraction}/><code>{progress.current_path}</code><p>Die Prüfung läuft bei einem Wechsel der Ansicht weiter. Ein Wechsel der Installation oder Sprache bricht sie ab.</p></div>}
    {(error||job?.error)&&<p role="alert" className="craft-error">{error||job?.error}</p>}
    {result&&<div className="audit-result" role="status"><strong>{result.all_files_match_cache?'Alle Dateiinhalte stimmen mit dem lokalen Steam-Cache überein.':'Inhaltsabweichungen gefunden.'}</strong><p>{result.files.length} Depotdateien geprüft. Stand: {new Date(result.finished_at*1000).toLocaleString('de-DE')}. Kein Vanilla-Zertifikat · Live-Apply bleibt gesperrt.</p>{result.foreign_approval&&<p>Zusätzliche Fremddateien sind separat bestätigt. Dieser Bericht prüft ausschließlich die Depot-Originale und bestätigt keine Kompatibilität.</p>}<ul>{result.files.filter(f=>!f.matches_cache).map(f=><li key={f.path}><code>{f.path}</code> · Inhalt weicht ab</li>)}</ul><details><summary>Grenzen dieses Berichts</summary><ul>{result.limitations.map(s=><li key={s}>{s}</li>)}</ul></details></div>}
    {exported&&<p className="audit-export" role="status">Prüfbericht gespeichert: <code>{exported}</code></p>}
  </section>;
}
