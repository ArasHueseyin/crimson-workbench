import { useEffect, useLayoutEffect, useRef, useState } from 'react';
import { Archive, ArrowRight, Check, FlaskConical, LoaderCircle, LockKeyhole, Package, ShieldCheck, Sparkles, X } from 'lucide-react';
import { api, asError } from './api';
import { count } from './format';
import { useMods } from './store';
import type { ModChoice, ModInfo, ModPreview, ModRehearsal, VendorOptions } from './mod-types';
import './mods.css';
import { InstallationCheck } from './InstallationCheck';
import { RecoveryCenter } from './RecoveryCenter';
import { Baseline } from './Baseline';
import { LiveApply } from './LiveApply';
import { ForeignFiles } from './ForeignFiles';
import {AdvancedMods} from './AdvancedMods';
import {canonical} from './advanced-types';

export function Mods({session}:{session:number}) {
  const [info,setInfo]=useState<ModInfo|null>(null),[preview,setPreview]=useState<ModPreview|null>(null);
  const [busy,setBusy]=useState(false),[error,setError]=useState(''),[exported,setExported]=useState(''),[rehearsal,setRehearsal]=useState<ModRehearsal|null>(null),[filter,setFilter]=useState('');
  const request=useMods(s=>s.request),set=useMods(s=>s.set),reset=useMods(s=>s.reset);
  const generation=useRef(0),current=useRef(request);
  const form=useRef<HTMLFormElement>(null);
  const [invalidInput,setInvalidInput]=useState(false);
  const [resetId,setResetId]=useState(0);
  const [consentEpoch,setConsentEpoch]=useState(0);
  current.current=request;
  // Numeric controls retain the last valid request while displaying a draft.
  // Include DOM validity so an invalid draft also revokes the live review.
  const updateValidity=()=>setInvalidInput(form.current!==null&&!form.current.checkValidity());
  useLayoutEffect(updateValidity);
  const dirty=preview!==null&&(invalidInput||canonical(preview.request)!==canonical(request));
  useEffect(()=>{let active=true;api.modInfo(session).then(v=>{if(active)setInfo(v)}).catch(e=>{if(active)setError(asError(e).message)});return()=>{active=false;generation.current++}},[session]);
  async function calculate() {
    if(!form.current?.reportValidity())return;
    const id=++generation.current, snapshot=request;setBusy(true);setError('');setPreview(null);setExported('');setRehearsal(null);
    try { const value=await api.modPreview(session,snapshot);if(id===generation.current&&current.current===snapshot)setPreview(value); }
    catch(e){if(id===generation.current)setError(asError(e).message)}
    finally{if(id===generation.current)setBusy(false)}
  }
  async function output(kind:'export'|'rehearse') {
    if(!preview||dirty||!form.current?.reportValidity())return;
    const id=++generation.current;setBusy(true);setError('');
    try { if(kind==='export'){const path=await api.modExport(session,preview.request,preview.plan_id);if(id===generation.current)setExported(path)}
      else{const result=await api.modRehearse(session,preview.request,preview.plan_id);if(id===generation.current)setRehearsal(result)} }
    catch(e){if(id===generation.current)setError(asError(e).message)}finally{if(id===generation.current)setBusy(false)}
  }
  const changes=preview?.changes.filter(c=>`${c.name} ${c.key} ${c.field} ${c.module}`.toLowerCase().includes(filter.toLowerCase()))??[];
  const enabled=!!preview&&!dirty&&!busy&&preview.changes.length>0;
  return <div className="mods-page">
    <div className="page-heading"><div><div className="eyebrow">PLANEN · PRÜFEN · VORBEREITEN</div><h1>Modwerkstatt<span>.</span></h1><p>Shops, Dropchancen, Mengen und Trust. Jede Änderung vorab im Blick.</p></div><span className="mod-preview-badge"><FlaskConical size={15}/>Vorschauversion</span></div>
    <div className="mod-assurance"><ShieldCheck size={20}/><p><strong>Vorschau und Proben bleiben im Projekt.</strong> Änderungen an der Installation erfolgen ausschließlich über die separate Live-Anwendung mit Prüfung und beendetem Spiel.</p></div>
    {error&&<div role="alert" className="craft-error">{error}</div>}
    <ForeignFiles session={session} changed={()=>setConsentEpoch(v=>v+1)}/>
    <InstallationCheck key={`inventory-${consentEpoch}`} session={session}/>
    <Baseline session={session}/>
    <LiveApply key={`live-${consentEpoch}`} session={session} settings={preview&&!dirty?preview.request:null}/>
    <RecoveryCenter session={session} latestDirectory={rehearsal?.directory}/>
    {!info?(!error&&<p role="status">Modschemas werden geprüft …</p>):<>
    <form ref={form} onChange={updateValidity} onSubmit={e=>{e.preventDefault();void calculate()}}><fieldset key={resetId} className="mod-settings" disabled={busy}>
      <section className="mod-card"><div className="mod-card-title"><Package size={22}/><div><small>B1 · SHOPS</small><h2>Warenbestand</h2></div><span>{count(info.stock_rows)} Positionen</span></div>
        <p>Vorhandene Warenpositionen von {count(info.vendors.length)} Händlern. {info.opaque_stores} weitere Händler bleiben unverändert.</p>
        <label className="mod-switch"><input type="checkbox" checked={request.shop_stock!==null} onChange={e=>set({shop_stock:e.target.checked?999:null})}/>Shopbestand ändern</label>
        <label className="mod-number">Bestand pro Position<NumberInput label="Shopbestand" value={request.shop_stock??999} max={1000000} disabled={request.shop_stock===null} change={shop_stock=>set({shop_stock})}/></label>
        <Scope choices={info.vendors} selected={request.vendors} onChange={vendors=>set({vendors})} label="Händler"/>
        <Overrides choices={info.vendors} values={request.vendor_overrides} change={vendor_overrides=>set({vendor_overrides})} label="Händler" defaultValue={999} max={1000000}/>
        <label className="mod-switch"><input type="checkbox" checked={request.daily_refresh} onChange={e=>set({daily_refresh:e.target.checked})}/>Täglicher Shop-Refresh</label>
        <p>3- und 7-Tage-Intervalle werden täglich. {count(info.daily_vendors.length)} Händler haben ein bestätigtes Intervall; einmalige Waren bleiben einmalig.</p>
        <Scope choices={info.items} selected={request.shop_items} onChange={shop_items=>set({shop_items})} label="Zusatzartikel" emptyAll={false}/>
        <p>Zusatzartikel werden ergänzt, vorhandene Angebote bleiben erhalten. {count(info.append_vendors.length)} Händler unterstützen das Einfügen. Neue Positionen erhalten den eingestellten Bestand (sonst 999) und übernehmen Preisfaktoren eines normalen Angebots des Händlers. Für alle Artikel bitte einzelne Händler auswählen: Pro Vorschau sind höchstens 100.000 Kombinationen aus Händler und Artikel zulässig. Wirkung und große Sortimente sind noch nicht im Spiel geprüft.</p>
        <VendorDetails info={info} values={request.vendor_options} change={vendor_options=>set({vendor_options})} globalItems={request.shop_items}/>
      </section>
      <section className="mod-card"><div className="mod-card-title"><Archive size={22}/><div><small>B2 · DROPS</small><h2>Ertragsmengen</h2></div><span>{count(info.dropsets.length)} Dropsets</span></div>
        <p>Minimum und Maximum werden ganzzahlig multipliziert. Enthält auch Rezept- und Questbelohnungen.</p>
        <label className="mod-number">Mengenmultiplikator<NumberInput label="Dropmengenmultiplikator" value={request.quantity_multiplier} max={1000} change={quantity_multiplier=>set({quantity_multiplier})}/></label>
        <Scope choices={info.dropsets} selected={request.dropsets} onChange={dropsets=>set({dropsets})} label="Dropsets"/>
        <Overrides choices={info.dropsets} values={request.quantity_overrides} change={quantity_overrides=>set({quantity_overrides})} label="Dropsets" defaultValue={2} max={1000}/>
        <label className="mod-number">Chancenmultiplikator<NumberInput label="Dropchancenmultiplikator" value={request.chance_multiplier} max={1000} change={chance_multiplier=>set({chance_multiplier})}/></label>
        <p>{count(info.chance_dropsets.length)} unbedingte Dropsets mit unabhängigen Einzelchancen. Die gemeinsame Dropsetauswahl gilt auch hier; jede Chance wird bei 100 % begrenzt. Gewichtete oder begrenzte Auswahl bleibt unverändert.</p>
        <Overrides choices={info.chance_dropsets} values={request.chance_overrides} change={chance_overrides=>set({chance_overrides})} label="Dropchancen" defaultValue={2} max={1000}/>
        <Scope choices={info.guarantee_dropsets} selected={request.guaranteed_dropsets} onChange={guaranteed_dropsets=>set({guaranteed_dropsets})} label="Garantierte Dropsets" emptyAll={false}/>
        <p>Manuelle Auswahl, etwa für bekannte Bosssets. Setzt alle enthaltenen Einzelchancen auf 100 % und hat Vorrang vor Chancenmultiplikatoren. Die Garantie gilt beim Auslösen des Sets; Bosszuordnungen und vorgelagerte Ereignisse werden nicht automatisch erkannt.</p>
      </section>
      <section className="mod-card mod-trust"><div className="mod-card-title"><Sparkles size={22}/><div><small>B3 · TRUST</small><h2>Vertrauen</h2></div><span>{info.trust_rows} Friendly-Datensätze</span></div>
        <p>Multipliziert positive Zuwächse. Negative Werte bleiben erhalten. Die Wirkung im Spiel wurde noch nicht bestätigt.</p>
        <label className="mod-number">Bonusmultiplikator<NumberInput label="Trustmultiplikator" value={request.trust_multiplier} max={1000} change={trust_multiplier=>set({trust_multiplier})}/></label>
      </section>
      {info.advanced&&<AdvancedMods session={session} info={info.advanced} items={info.items} value={request.advanced} change={advanced=>set({advanced})}/>}
    </fieldset>
    <div className="mod-actions"><button type="submit" className="primary" disabled={busy}>{busy?<LoaderCircle size={16} className="spin"/>:<ArrowRight size={16}/>}Vorschau berechnen</button><button type="button" className="secondary" disabled={busy} onClick={()=>{reset();setResetId(n=>n+1);setPreview(null);setRehearsal(null);setExported('');setError('')}}>Zurücksetzen</button><span role="status">{invalidInput?'Ungültige Eingabe – markierte Zahlenfelder korrigieren.':dirty?'Einstellungen geändert – Vorschau neu berechnen.':preview?`${count(preview.changes.length)} Feldänderungen vorbereitet.`:'Noch keine Änderungen berechnet.'}</span></div></form>
    {preview&&<div className={dirty?'mod-preview mod-stale':'mod-preview'}>
      <section className="mod-gates"><LockKeyhole size={21}/><div><h2>Tabellenplan vorbereitet</h2><ul>{preview.gates.reasons.map(reason=><li key={reason}>{reason}</li>)}</ul><p>Prozessstatus bei Vorschauerstellung: {preview.gates.game_running===null?'unbekannt':preview.gates.game_running?'Spiel läuft':'Spiel beendet'}.</p></div><span>Zum Schreiben oben die Live-Dateivorschau verwenden.</span></section>
      <div className="mod-result-heading"><h2>Dein Änderungsplan <span>{count(preview.changes.length)}</span></h2><div><button className="secondary" disabled={!enabled} onClick={()=>void output('rehearse')}><FlaskConical size={15}/>Probe an Projektkopie</button><button className="secondary" disabled={!enabled} onClick={()=>void output('export')}>Vorschau exportieren</button></div></div>
      {rehearsal&&<><div className="mod-success" role="status"><Check size={18}/><div><strong>{rehearsal.cycles} Apply-/Restore-Zyklen geprüft.</strong>{rehearsal.reapply_passed&&rehearsal.recovery_passed&&<p>Wiederanwendung und Wiederherstellung nach Abbruch geprüft.</p>}{rehearsal.launch_guard_held&&rehearsal.update_refusal_passed&&<p>Startschutz, Backupschutz und Update-Sperre an Testdateien geprüft.</p>}<p>{rehearsal.scope}</p><code>{rehearsal.directory}</code></div></div>
      <details className="mod-rehearsal"><summary>Dateiänderungen der Projektprobe · {rehearsal.transitions.length} Schritte</summary><p>Diese Zugriffe wurden ausschließlich in der angezeigten Projektkopie ausgeführt.</p>{rehearsal.transitions.map(step=><section className="mod-files" key={step.name}><h3>{step.name}</h3>{step.files.map(file=><div key={file.path}><strong>{file.path}</strong><span>{file.action==='create'?'Neu':file.action==='remove'?'Entfernen':'Ersetzen'}</span><code>Vorher: {file.before_sha256??'nicht vorhanden'}<br/>Nachher: {file.after_sha256??'nicht vorhanden'}</code></div>)}</section>)}</details></>}
      {exported&&<div className="mod-success" role="status"><Check size={18}/><div><strong>Vorschau exportiert; nichts angewendet.</strong><code>{exported}</code></div></div>}
      <details className="mod-files" open><summary>Geplanter Dateizugriff · {preview.files.length} Dateien</summary>{preview.files.length===0?<p>Keine Dateien erforderlich.</p>:preview.files.map(file=><div key={file.path}><strong>{file.path}</strong><span>{file.action==='create'?'Neu':'Ersetzen'} · {count(file.bytes)} Bytes</span><code>Vorher: {file.before_sha256??'nicht vorhanden'}<br/>Nachher: {file.after_sha256}</code></div>)}</details>
      <label className="mod-filter">Änderungen durchsuchen<input aria-label="Änderungen durchsuchen" value={filter} onChange={e=>setFilter(e.target.value)} placeholder="Name, ID oder Feld …"/></label>
      <div className="mod-diff"><table><thead><tr><th>Datensatz</th><th>Feld</th><th>Vorher</th><th>Nachher</th></tr></thead><tbody>{changes.slice(0,150).map((change,i)=><tr key={`${change.module}-${change.key}-${change.field}-${i}`}><td><strong>{change.name}</strong><small>{change.table} · {change.key}</small></td><td>{change.field}</td><td>{change.before}</td><td>{change.after}</td></tr>)}</tbody></table></div><p className="mod-footnote">{count(Math.min(changes.length,150))} von {count(changes.length)} Treffern angezeigt. Der Export enthält alle Änderungen.</p>
      <details className="mod-coverage"><summary>Abdeckung & Herkunft</summary><ul>{preview.warnings.map(w=><li key={w}>{w}</li>)}</ul><p>{preview.credits}</p><code>Plan: {preview.plan_id}<br/>Snapshot: {preview.fingerprint}</code></details>
    </div>}
    </>}
  </div>
}
function NumberInput({label,value,max,disabled,change}:{label:string;value:number;max:number;disabled?:boolean;change:(n:number)=>void}) {
  const [raw,setRaw]=useState(String(value));
  useEffect(()=>setRaw(String(value)),[value]);
  return <input type="number" aria-label={label} required min={0} max={max} step={1} disabled={disabled} value={raw} onChange={e=>{setRaw(e.target.value);const n=Number(e.target.value);if(e.target.value!==''&&Number.isInteger(n)&&n>=0&&n<=max)change(n)}}/>;
}
function Scope({choices,selected,onChange,label,emptyAll=true}:{choices:ModChoice[];selected:number[];onChange:(v:number[])=>void;label:string;emptyAll?:boolean}) {
  const [search,setSearch]=useState('');
  const filtered=choices.filter(v=>`${v.key} ${v.name}`.toLowerCase().includes(search.toLowerCase()));
  return <details className="mod-scope"><summary>{selected.length?`${selected.length} ${label} ausgewählt`:emptyAll?`Alle freigegebenen ${label}`:`${label}: keine Auswahl`}</summary>
    <input aria-label={`${label} filtern`} placeholder="Name oder ID suchen" value={search} onChange={e=>setSearch(e.target.value)}/>
    <div className="mod-options">{filtered.slice(0,80).map(c=><label key={c.key}><input type="checkbox" checked={selected.includes(c.key)} onChange={e=>onChange(e.target.checked?[...selected,c.key].sort((a,b)=>a-b):selected.filter(k=>k!==c.key))}/><span>{c.name}<small>{c.key}{c.entries>0?` · ${c.entries} Positionen`:''}</small></span></label>)}</div><small>{Math.min(filtered.length,80)} von {count(filtered.length)} Treffern. {emptyAll?'Ohne Auswahl gilt die globale Einstellung für alle freigegebenen Datensätze.':'Ohne Auswahl wird hier nichts geändert.'}</small>{(label==='Zusatzartikel'||label.startsWith('Artikel für Händler '))&&<button type="button" onClick={()=>onChange(choices.map(c=>c.key).sort((a,b)=>a-b))}>Alle {count(choices.length)} Artikel auswählen</button>}{selected.length>0&&<button type="button" onClick={()=>onChange([])}>Auswahl leeren ({selected.length})</button>}
  </details>
}
function Overrides({choices,values,change,label,defaultValue,max}:{choices:ModChoice[];values:Record<number,number>;change:(v:Record<number,number>)=>void;label:string;defaultValue:number;max:number}) {
  const [key,setKey]=useState(''),[value,setValue]=useState(defaultValue);
  const found=choices.find(c=>String(c.key)===key);
  return <details className="mod-scope"><summary>Einzelausnahmen ({Object.keys(values).length})</summary><p>Eine Ausnahme hat Vorrang vor der globalen Auswahl und deren Wert.</p><div className="mod-override-form"><input aria-label={`${label} Ausnahme-ID`} placeholder="Datensatz-ID" value={key} onChange={e=>setKey(e.target.value)}/><NumberInput label={`${label} Ausnahmewert`} value={value} max={max} change={setValue}/><button type="button" className="secondary" disabled={!found} onClick={()=>{if(found){change({...values,[found.key]:value});setKey('')}}}>Setzen</button></div>{key&&<small>{found?.name??'Keine freigegebene ID gefunden.'}</small>}{Object.entries(values).map(([id,v])=><div className="mod-override" key={id}><span>{choices.find(c=>c.key===Number(id))?.name??id} · {id}</span><strong>{v}</strong><button type="button" aria-label={`${label} Ausnahme ${id} entfernen`} onClick={()=>{const next={...values};delete next[Number(id)];change(next)}}><X size={14}/></button></div>)}</details>
}

function VendorDetails({info,values,change,globalItems}:{info:ModInfo;values:Record<number,VendorOptions>;change:(v:Record<number,VendorOptions>)=>void;globalItems:number[]}) {
  const [key,setKey]=useState('');
  const found=info.vendors.find(v=>String(v.key)===key);
  function edit(id:number,update:Partial<VendorOptions>){change({...values,[id]:{...values[id],...update}})}
  return <details className="mod-scope"><summary>Händlerdetails ({Object.keys(values).length})</summary>
    <p>Eigene Artikelsets und Refresh-Ausnahmen gelten auch außerhalb der globalen Händlerauswahl. Eine leere eigene Artikelauswahl unterdrückt Zusatzartikel bei diesem Händler.</p>
    <div className="mod-override-form"><input aria-label="Händlerdetails ID" placeholder="Händler-ID" value={key} onChange={e=>setKey(e.target.value)}/><button type="button" disabled={!found||values[found.key]!==undefined} onClick={()=>{if(found){change({...values,[found.key]:{items:null,daily_refresh:null}});setKey('')}}}>Händler hinzufügen</button></div>
    {key&&<small>{found?.name??'Keine freigegebene Händler-ID gefunden.'}</small>}
    {Object.entries(values).map(([id,options])=>{const n=Number(id);return <section className="mod-vendor-detail" key={id}>
      <strong>{info.vendors.find(v=>v.key===n)?.name} · {id}</strong>
      <label className="mod-switch"><input type="checkbox" aria-label={`Eigene Artikel für Händler ${id}`} disabled={!info.append_vendors.includes(n)} checked={options.items!==null} onChange={e=>edit(n,{items:e.target.checked?[...globalItems]:null})}/>Eigene Artikelauswahl</label>
      {options.items!==null&&<Scope choices={info.items} selected={options.items} onChange={items=>edit(n,{items})} label={`Artikel für Händler ${id}`} emptyAll={false}/>}
      <label className="mod-number">Refresh<select aria-label={`Refresh für Händler ${id}`} value={options.daily_refresh===null?'inherit':options.daily_refresh?'daily':'original'} onChange={e=>edit(n,{daily_refresh:e.target.value==='inherit'?null:e.target.value==='daily'})}>
        <option value="inherit">Globale Einstellung</option><option value="daily" disabled={!info.daily_vendors.includes(n)}>Täglich</option><option value="original">Originalintervall behalten</option>
      </select></label>
      <button type="button" aria-label={`Händlerdetails ${id} entfernen`} onClick={()=>{const next={...values};delete next[n];change(next)}}>Ausnahme entfernen</button>
    </section>})}
  </details>
}
