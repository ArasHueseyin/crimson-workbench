import {useEffect,useRef,useState} from 'react';
import {ShieldCheck} from 'lucide-react';
import {api,asError} from './api';
import {count} from './format';
import type {ModRequest} from './mod-types';
import type {LiveUpdate,LiveStatus,LiveSetup,LiveReview,LiveRequest,LiveOperation,LiveJob} from './live-types';

export function LiveApply({session,settings}:{session:number;settings:ModRequest|null}) {
 const [status,setStatus]=useState<LiveStatus|null>(null),[ids,setIds]=useState<string[]>([]),[id,setId]=useState('');
 const [update,setUpdate]=useState<LiveUpdate|null>(null);
 const [setup,setSetup]=useState<LiveSetup|null>(null),[verified,setVerified]=useState(false);
 const [review,setReview]=useState<LiveReview|null>(null),[request,setRequest]=useState<LiveRequest|null>(null);
 const [job,setJob]=useState<LiveJob|null>(null),[busy,setBusy]=useState(false),[error,setError]=useState('');
 const generation=useRef(0),pending=useRef(false),settingsKey=JSON.stringify(settings),latest=useRef(settingsKey);
 latest.current=settingsKey;
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
    if(!next){setError('Live-Auftrag nicht mehr verfügbar. Status erneut laden.');return;}
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
 return <section className="recovery-center live-panel" aria-label="Live-Anwendung und Restore">
  <div className="recovery-heading"><ShieldCheck size={21}/><div><small>B0 · INSTALLATION</small><h2>Live-Anwendung & Restore</h2></div></div>
  <p>Übernimmt den berechneten Mod in eine eigene Archivgruppe. Jede Anwendung entsteht aus den Originaltabellen. Vor jedem Schreibvorgang werden die Originaldateien vollständig geprüft und gegen Änderungen geschützt; das Spiel muss beendet sein.</p>
  <p>Die manuelle Prüfung im Spiel bleibt bis zum Abschluss aller Entwicklungsphasen offen.</p>
  <button className="secondary" disabled={busy||running} onClick={()=>void work(load)}>Live-Status laden</button>
  {status&&<>
   <p role="status">{status.game_running===true?'Spiel läuft · Live-Schreibzugriffe gesperrt':status.game_running===false?'Spiel beendet':'Spielstatus unbekannt · Live-Schreibzugriffe gesperrt'} · {status.pending_basis?'Basiswechsel unterbrochen':status.initialized?status.active_overlay?'Eigener Mod aktiv':'Registry im Originalzustand':status.configured?'Einrichtung unvollständig':'Live-Einrichtung fehlt'}</p>
   {status.issues.length>0&&<ul role="alert">{status.issues.map(s=><li key={s}>{s}</li>)}</ul>}
   {(!status.initialized&&!status.pending_basis)&&<div className="recovery-review">
    <label>Gespeicherte Basis für Live-B0<select aria-label="Live-Ausgangsstand" disabled={busy||running} value={id} onChange={e=>{setId(e.target.value);setSetup(null);setVerified(false)}}><option value="">Bitte auswählen</option>{ids.map(v=><option key={v} value={v}>{v}</option>)}</select></label>
    <button className="secondary" disabled={busy||running||!id} onClick={()=>void work(prepareSetup)}>Live-Einrichtung prüfen</button>
    {setup&&<><p>{count(setup.files)} Dateien · Inhaltsbericht vom {new Date(setup.audited_at*1000).toLocaleString('de-DE')}. Bei Einrichtung werden alle Inhalte erneut gehasht und die Registry gesichert.</p><code>{setup.game_path}</code>
     {(setup.preserved_foreign_files??0)>0&&<p>{count(setup.preserved_foreign_files!)} bestätigte Zusatzdateien werden ebenfalls geprüft und unverändert erhalten.</p>}
     <label className="mod-switch"><input aria-label="Steam-Dateiprüfung bestätigt" type="checkbox" disabled={blocked} checked={verified} onChange={e=>setVerified(e.target.checked)}/>Ich habe Steams „Dateien auf Fehler überprüfen“ vor diesem Inhaltsbericht vollständig abgeschlossen.</label>
     <p>Falls das noch nicht geschehen ist: später Steam prüfen lassen und anschließend einen neuen Inhaltsbericht mit Ausgangsstand erstellen. Der vorhandene Bericht allein bestätigt diesen Schritt nicht.</p>
     <button className="secondary" disabled={blocked||!verified} onClick={()=>void work(()=>start({kind:'setup',baseline_id:setup.baseline_id,review_id:setup.review_id,steam_verified_before_audit:verified}))}>Live-Basis prüfen und einrichten</button>
    </>}
   </div>}
   {status.configured&&<details className="recovery-review" open={status.pending_basis?true:undefined}>
    <summary>Basis nach Spielupdate wechseln</summary>
    <p>Nach einer abgeschlossenen Steam-Dateiprüfung einen neuen Inhaltsbericht und Ausgangsstand speichern. Der Wechsel archiviert die bisherige Workbench-Historie und eigene Moddateien. Die bereits vorhandene Original-Registry der neuen Basis bleibt erhalten.</p>
    {status.pending_basis&&<p>Unterbrochener Wechsel: mit der vorgesehenen Basis fortsetzen.</p>}
    <label>Neue gespeicherte Basis<select aria-label="Neue Live-Basis" disabled={busy||running||!!status.pending_basis} value={id} onChange={e=>{setId(e.target.value);setUpdate(null);setSetup(null);setVerified(false)}}><option value="">Bitte auswählen</option>{ids.map(v=><option key={v} value={v}>{v}</option>)}</select></label>
    <button className="secondary" disabled={busy||running||!id||id===status.baseline_id} onClick={()=>void work(prepareUpdate)}>Basiswechsel prüfen</button>
    {update&&<><p>{update.resume?'Offenen Basiswechsel fortsetzen':'Geprüfter Basiswechsel'}: {update.previous_baseline} → {update.next_baseline}</p><p>{count(update.files.length)} Dateien archivieren · {count(update.source_files)} Originaldateien frisch prüfen.</p><code>{update.archive_path}</code>
     <details><summary>Zu archivierende Dateien</summary><ul>{update.files.map(f=><li key={f.path}><code>{f.path}</code></li>)}</ul></details>
     <label className="mod-switch"><input aria-label="Steam-Prüfung für neue Basis bestätigt" type="checkbox" disabled={blocked} checked={verified} onChange={e=>setVerified(e.target.checked)}/>Ich habe Steams Dateiprüfung vor dem Inhaltsbericht dieser neuen Basis vollständig abgeschlossen.</label>
     <button className="primary" disabled={blocked||!verified} onClick={()=>void work(()=>start({kind:'update',baseline_id:update.next_baseline,review_id:update.review_id,steam_verified_before_audit:verified}))}>{update.resume?'Geprüften Basiswechsel fortsetzen':'Geprüften Basiswechsel ausführen'}</button>
    </>}
   </details>}
   {status.initialized&&!status.pending_basis&&<div className="mod-actions">
    <button className="secondary" disabled={busy||running||!settings||status.recovery_required} onClick={()=>void work(()=>prepare('apply'))}>Live-Dateivorschau berechnen</button>
    <button className="secondary" disabled={busy||running} onClick={()=>void work(()=>prepare('restore'))}>Live-Restore prüfen</button>
    {!settings&&<span>Zuerst unten einen Änderungsplan berechnen.</span>}
    {status.recovery_required&&<span>Unterbrochene Transaktion: zuerst Live-Restore prüfen.</span>}
   </div>}
  </>}
  {busy&&<p role="status">Live-Anfrage wird vorbereitet …</p>}
  {error&&<div role="alert" className="craft-error">{error}</div>}
  {review&&request&&<div className="recovery-review">
   <strong>{review.action==='apply'?'Dateiänderungen für Live-Apply':'Dateiänderungen für Live-Restore'}</strong><code>{review.game_path}</code>
   <p>{count(review.protected_source_files)} Originaldateien werden vor dem Schreiben erneut geprüft. Große Installationen benötigen dafür einige Minuten.</p>
   {(review.preserved_foreign_files??0)>0&&<p>{count(review.preserved_foreign_files!)} bestätigte fremde Zusatzdateien bleiben erhalten; auch deren Hashes und Bestätigung müssen weiterhin passen.</p>}
   <div className="mod-files">{review.files.map((f,i)=><div key={`${f.path}-${i}`}><strong>{f.path}</strong><span>{f.action==='create'?'Neu':f.action==='remove'?'Entfernen':'Ersetzen'}</span><code>Vorher: {f.before_sha256??'nicht vorhanden'}<br/>Nachher: {f.after_sha256??'nicht vorhanden'}</code></div>)}</div>
   {review.files.length===0&&!review.recovery_required?<p>Keine Dateiänderungen erforderlich.</p>:<button className="primary" disabled={blocked} onClick={()=>void work(()=>start({kind:'execute',request,review_id:review.review_id}))}>{review.action==='apply'?'Geprüften Mod auf Spiel anwenden':'Geprüften Live-Restore ausführen'}</button>}
  </div>}
  {job&&<div className="recovery-review" role="status">
   {running?<><strong>Live-Vorgang läuft · Quellen prüfen und Transaktion abschließen</strong><p>Währenddessen kann das Spiel nicht gestartet werden. Ein Abbruch kann eine wiederherstellbare Transaktion hinterlassen.</p><button className="secondary" disabled={busy} onClick={()=>void work(()=>api.liveCancel(session,job.id))}>Live-Vorgang abbrechen</button></>:job.phase==='complete'?<><strong>Live-Vorgang abgeschlossen.</strong>{job.result&&'registry_sha256' in job.result&&<><p>{job.result.action==='restore'?'Original-Registry wiederhergestellt.':job.result.action==='basis_update'?'Neue Basis aktiv; bisherige Workbench-Dateien archiviert.':'Mod registriert; Originalarchive erhalten.'}</p><code>Registry SHA-256: {job.result.registry_sha256}</code></>}</>:<><strong>{job.phase==='cancelled'?'Live-Vorgang abgebrochen':'Live-Vorgang fehlgeschlagen'}</strong><p>{job.error}</p><p>Status erneut laden und gegebenenfalls Basiswechsel fortsetzen oder Live-Restore prüfen. Kein automatischer erneuter Schreibversuch.</p></>}
  </div>}
 </section>;
}
