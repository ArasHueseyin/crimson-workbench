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
  if(gen===generation.current){setMessage(`${count(receipt.files)} Zusatzdateien zum Beibehalten bestätigt. Spieldateien unverändert. Dateiliste und Live-Vorschau neu laden.`);changed()}
 }
 async function revoke(gen:number){
  if(!inspection?.approval_id)return;
  const id=inspection.approval_id;setInspection(null);setConfirmed(false);await api.foreignRevoke(session,id);
  if(gen===generation.current){setMessage('Bestätigung zurückgenommen. Zusatzdateien bleiben erhalten; Live-Vorgänge benötigen erneut einen zulässigen Dateibestand.');changed()}
 }
 const review=inspection?.review,hasExtras=!!review&&(review.files.length>0||review.directories.length>0);
 return <section className="recovery-center" aria-label="Fremde Zusatzdateien">
  <div className="recovery-heading"><FileWarning size={21}/><div><small>B0 · ZUSATZDATEIEN</small><h2>Fremde Mods & Zusatzdateien</h2></div></div>
  <p>Zusätzliche Dateien können von Mods, anderen Managern oder regulären Laufzeitdaten stammen. Prüfe die genaue Liste, bevor du ihr Beibehalten bestätigst. Veränderte Originaldateien und eine fremde Registry bleiben gesperrt.</p>
  <p>Die Bestätigung wird nur im Projekt gespeichert und startet keine Anwendung. Bestätigte Dateien werden bei Live-Vorgängen erneut gehasht, geschützt und unverändert erhalten. Ihre Kompatibilität ist damit nicht bewiesen.</p>
  <button className="secondary" disabled={busy} onClick={()=>void work(load)}>Zusatzdateien prüfen</button>
  {busy&&<p role="status">Zusatzdateien werden geprüft …</p>}
  {(error||inspection?.issue)&&<p role="alert" className="craft-error">{error||inspection?.issue}</p>}
  {inspection?.approval_id&&<div className="recovery-review"><p>Vorhandene Bestätigung: {count(inspection.approved_files)} Dateien. Änderungen oder neue Dateien benötigen eine neue Vorschau.</p><button className="secondary" disabled={busy} onClick={()=>void work(revoke)}>Bestätigung zurücknehmen</button></div>}
  {review&&<div className="recovery-review"><code>{review.game_path}</code>
   {hasExtras?<><p>{count(review.files.length)} Dateien · {count(review.directories.length)} zusätzliche Ordner · {(Number(review.total_bytes)/1048576).toLocaleString('de-DE',{maximumFractionDigits:1})} MiB</p>
    <div className="mod-files">{review.files.map(f=><div key={f.path}><strong>{f.path}</strong><span>Beibehalten · {count(f.bytes)} Bytes</span><code>SHA-256: {f.sha256}</code></div>)}</div>
    {review.directories.length>0&&<details><summary>Zusätzliche Ordner</summary><ul>{review.directories.map(p=><li key={p}><code>{p}</code></li>)}</ul></details>}
    <label className="mod-switch"><input type="checkbox" aria-label="Zusatzdateien ausdrücklich beibehalten" checked={confirmed} disabled={busy} onChange={e=>setConfirmed(e.target.checked)}/>Ich möchte genau diese Zusatzdateien unverändert beibehalten und bestätige ihr Nebeneinander mit Workbench-Mods.</label>
    <button className="primary" disabled={busy||!confirmed} onClick={()=>void work(confirm)}>Geprüfte Zusatzdateien bestätigen</button>
   </>:<p>Keine unzugeordneten Zusatzdateien gefunden. Keine Bestätigung erforderlich.</p>}
  </div>}
  {message&&<p role="status" className="mod-success">{message}</p>}
 </section>;
}
