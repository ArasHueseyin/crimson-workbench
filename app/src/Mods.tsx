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
    <div className="page-heading"><div><div className="eyebrow">PLAN · CHECK · PREPARE</div><h1>Mod workshop<span>.</span></h1><p>Shops, drop chances, quantities and trust. Preview every change.</p></div><span className="mod-preview-badge"><FlaskConical size={15}/>Preview version</span></div>
    <div className="mod-assurance"><ShieldCheck size={20}/><p><strong>Previews and rehearsals stay in the workspace.</strong> Installation changes require the separate live apply workflow, validation and a closed game.</p></div>
    {error&&<div role="alert" className="craft-error">{error}</div>}
    <ForeignFiles session={session} changed={()=>setConsentEpoch(v=>v+1)}/>
    <InstallationCheck key={`inventory-${consentEpoch}`} session={session}/>
    <Baseline session={session}/>
    <LiveApply key={`live-${consentEpoch}`} session={session} settings={preview&&!dirty?preview.request:null}/>
    <RecoveryCenter session={session} latestDirectory={rehearsal?.directory}/>
    {!info?(!error&&<p role="status">Checking mod schemas …</p>):<>
    <form ref={form} onChange={updateValidity} onSubmit={e=>{e.preventDefault();void calculate()}}><fieldset key={resetId} className="mod-settings" disabled={busy}>
      <section className="mod-card"><div className="mod-card-title"><Package size={22}/><div><small>B1 · SHOPS</small><h2>Shop stock</h2></div><span>{count(info.stock_rows)} Entries</span></div>
        <p>Existing stock entries from {count(info.vendors.length)} vendors. {info.opaque_stores} other vendors remain unchanged.</p>
        <label className="mod-switch"><input type="checkbox" checked={request.shop_stock!==null} onChange={e=>set({shop_stock:e.target.checked?999:null})}/>Change shop stock</label>
        <label className="mod-number">Stock per entry<NumberInput label="Shop stock" value={request.shop_stock??999} max={1000000} disabled={request.shop_stock===null} change={shop_stock=>set({shop_stock})}/></label>
        <Scope choices={info.vendors} selected={request.vendors} onChange={vendors=>set({vendors})} label="Vendors"/>
        <Overrides choices={info.vendors} values={request.vendor_overrides} change={vendor_overrides=>set({vendor_overrides})} label="Vendors" defaultValue={999} max={1000000}/>
        <label className="mod-switch"><input type="checkbox" checked={request.daily_refresh} onChange={e=>set({daily_refresh:e.target.checked})}/>Daily shop refresh</label>
        <p>3- and 7-day intervals become daily. {count(info.daily_vendors.length)} vendors have a verified interval; one-time stock stays one-time.</p>
        <Scope choices={info.items} selected={request.shop_items} onChange={shop_items=>set({shop_items})} label="Additional items" emptyAll={false}/>
        <p>Adds extra items while preserving existing offers. {count(info.append_vendors.length)} vendors support additions. New entries receive the configured stock (otherwise 999) and copy price factors from a normal vendor offer. To add all items, choose individual vendors: at most 100,000 vendor/item combinations per preview. Effects and large inventories have not been tested in game.</p>
        <VendorDetails info={info} values={request.vendor_options} change={vendor_options=>set({vendor_options})} globalItems={request.shop_items}/>
      </section>
      <section className="mod-card"><div className="mod-card-title"><Archive size={22}/><div><small>B2 · DROPS</small><h2>Drop quantities</h2></div><span>{count(info.dropsets.length)} Drop sets</span></div>
        <p>Multiplies minimum and maximum by a whole number. Also includes recipe and quest rewards.</p>
        <label className="mod-number">Quantity multiplier<NumberInput label="Drop quantity multiplier" value={request.quantity_multiplier} max={1000} change={quantity_multiplier=>set({quantity_multiplier})}/></label>
        <Scope choices={info.dropsets} selected={request.dropsets} onChange={dropsets=>set({dropsets})} label="Drop sets"/>
        <Overrides choices={info.dropsets} values={request.quantity_overrides} change={quantity_overrides=>set({quantity_overrides})} label="Drop sets" defaultValue={2} max={1000}/>
        <label className="mod-number">Chance multiplier<NumberInput label="Drop chance multiplier" value={request.chance_multiplier} max={1000} change={chance_multiplier=>set({chance_multiplier})}/></label>
        <p>{count(info.chance_dropsets.length)} unconditional drop sets with independent chances. The shared drop set selection also applies here; each chance is capped at 100%. Weighted or limited selections are preserved.</p>
        <Overrides choices={info.chance_dropsets} values={request.chance_overrides} change={chance_overrides=>set({chance_overrides})} label="Drop chances" defaultValue={2} max={1000}/>
        <Scope choices={info.guarantee_dropsets} selected={request.guaranteed_dropsets} onChange={guaranteed_dropsets=>set({guaranteed_dropsets})} label="Guaranteed drop sets" emptyAll={false}/>
        <p>Manual selection, for example known boss sets. Sets every contained independent chance to 100% and overrides chance multipliers. The guarantee applies when the set is triggered; boss assignments and preceding events are not detected automatically.</p>
      </section>
      <section className="mod-card mod-trust"><div className="mod-card-title"><Sparkles size={22}/><div><small>B3 · TRUST</small><h2>Trust</h2></div><span>{info.trust_rows} Friendly records</span></div>
        <p>Multiplies positive gains. Negative values are preserved. The in-game effect has not been confirmed yet.</p>
        <label className="mod-number">Bonus multiplier<NumberInput label="Trust multiplier" value={request.trust_multiplier} max={1000} change={trust_multiplier=>set({trust_multiplier})}/></label>
      </section>
      {info.advanced&&<AdvancedMods session={session} info={info.advanced} items={info.items} value={request.advanced} change={advanced=>set({advanced})}/>}
    </fieldset>
    <div className="mod-actions"><button type="submit" className="primary" disabled={busy}>{busy?<LoaderCircle size={16} className="spin"/>:<ArrowRight size={16}/>}Calculate preview</button><button type="button" className="secondary" disabled={busy} onClick={()=>{reset();setResetId(n=>n+1);setPreview(null);setRehearsal(null);setExported('');setError('')}}>Reset</button><span role="status">{invalidInput?'Invalid input — correct the highlighted number fields.':dirty?'Settings changed — recalculate the preview.':preview?`${count(preview.changes.length)} field changes prepared.`:'No changes calculated yet.'}</span></div></form>
    {preview&&<div className={dirty?'mod-preview mod-stale':'mod-preview'}>
      <section className="mod-gates"><LockKeyhole size={21}/><div><h2>Table plan prepared</h2><ul>{preview.gates.reasons.map(reason=><li key={reason}>{reason}</li>)}</ul><p>Process status when preview was created: {preview.gates.game_running===null?'unknown':preview.gates.game_running?'Game running':'Game closed'}.</p></div><span>Use the live file preview above to write changes.</span></section>
      <div className="mod-result-heading"><h2>Your change plan <span>{count(preview.changes.length)}</span></h2><div><button className="secondary" disabled={!enabled} onClick={()=>void output('rehearse')}><FlaskConical size={15}/>Rehearse on workspace copy</button><button className="secondary" disabled={!enabled} onClick={()=>void output('export')}>Export preview</button></div></div>
      {rehearsal&&<><div className="mod-success" role="status"><Check size={18}/><div><strong>{rehearsal.cycles} apply/restore cycles checked.</strong>{rehearsal.reapply_passed&&rehearsal.recovery_passed&&<p>Reapplication and recovery after interruption checked.</p>}{rehearsal.launch_guard_held&&rehearsal.update_refusal_passed&&<p>Launch protection, backup protection and update blocking checked with test files.</p>}<p>{rehearsal.scope}</p><code>{rehearsal.directory}</code></div></div>
      <details className="mod-rehearsal"><summary>Workspace rehearsal file changes · {rehearsal.transitions.length} steps</summary><p>These operations ran only in the workspace copy shown here.</p>{rehearsal.transitions.map(step=><section className="mod-files" key={step.name}><h3>{step.name}</h3>{step.files.map(file=><div key={file.path}><strong>{file.path}</strong><span>{file.action==='create'?'New':file.action==='remove'?'Remove':'Replace'}</span><code>Before: {file.before_sha256??'absent'}<br/>After: {file.after_sha256??'absent'}</code></div>)}</section>)}</details></>}
      {exported&&<div className="mod-success" role="status"><Check size={18}/><div><strong>Preview exported; nothing applied.</strong><code>{exported}</code></div></div>}
      <details className="mod-files" open><summary>Planned file access · {preview.files.length} files</summary>{preview.files.length===0?<p>No files required.</p>:preview.files.map(file=><div key={file.path}><strong>{file.path}</strong><span>{file.action==='create'?'New':'Replace'} · {count(file.bytes)} Bytes</span><code>Before: {file.before_sha256??'absent'}<br/>After: {file.after_sha256}</code></div>)}</details>
      <label className="mod-filter">Search changes<input aria-label="Search changes" value={filter} onChange={e=>setFilter(e.target.value)} placeholder="Name, ID or field …"/></label>
      <div className="mod-diff"><table><thead><tr><th>Record</th><th>Field</th><th>Before</th><th>After</th></tr></thead><tbody>{changes.slice(0,150).map((change,i)=><tr key={`${change.module}-${change.key}-${change.field}-${i}`}><td><strong>{change.name}</strong><small>{change.table} · {change.key}</small></td><td>{change.field}</td><td>{change.before}</td><td>{change.after}</td></tr>)}</tbody></table></div><p className="mod-footnote">{count(Math.min(changes.length,150))} of {count(changes.length)} matches shown. The export includes all changes.</p>
      <details className="mod-coverage"><summary>Coverage & source</summary><ul>{preview.warnings.map(w=><li key={w}>{w}</li>)}</ul><p>{preview.credits}</p><code>Plan: {preview.plan_id}<br/>Snapshot: {preview.fingerprint}</code></details>
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
  return <details className="mod-scope"><summary>{selected.length?`${selected.length} ${label} selected`:emptyAll?`All supported ${label}`:`${label}: no selection`}</summary>
    <input aria-label={`${label} filter`} placeholder="Search name or ID" value={search} onChange={e=>setSearch(e.target.value)}/>
    <div className="mod-options">{filtered.slice(0,80).map(c=><label key={c.key}><input type="checkbox" checked={selected.includes(c.key)} onChange={e=>onChange(e.target.checked?[...selected,c.key].sort((a,b)=>a-b):selected.filter(k=>k!==c.key))}/><span>{c.name}<small>{c.key}{c.entries>0?` · ${c.entries} Entries`:''}</small></span></label>)}</div><small>{Math.min(filtered.length,80)} of {count(filtered.length)} matches. {emptyAll?'With no selection, the global setting applies to all supported records.':'With no selection, nothing changes here.'}</small>{(label==='Additional items'||label.startsWith('Items for vendors '))&&<button type="button" onClick={()=>onChange(choices.map(c=>c.key).sort((a,b)=>a-b))}>All {count(choices.length)} Select items</button>}{selected.length>0&&<button type="button" onClick={()=>onChange([])}>Clear selection ({selected.length})</button>}
  </details>
}
function Overrides({choices,values,change,label,defaultValue,max}:{choices:ModChoice[];values:Record<number,number>;change:(v:Record<number,number>)=>void;label:string;defaultValue:number;max:number}) {
  const [key,setKey]=useState(''),[value,setValue]=useState(defaultValue);
  const found=choices.find(c=>String(c.key)===key);
  return <details className="mod-scope"><summary>Individual overrides ({Object.keys(values).length})</summary><p>An override takes precedence over the global selection and value.</p><div className="mod-override-form"><input aria-label={`${label} Override ID`} placeholder="Record ID" value={key} onChange={e=>setKey(e.target.value)}/><NumberInput label={`${label} Override value`} value={value} max={max} change={setValue}/><button type="button" className="secondary" disabled={!found} onClick={()=>{if(found){change({...values,[found.key]:value});setKey('')}}}>Set</button></div>{key&&<small>{found?.name??'No supported ID found.'}</small>}{Object.entries(values).map(([id,v])=><div className="mod-override" key={id}><span>{choices.find(c=>c.key===Number(id))?.name??id} · {id}</span><strong>{v}</strong><button type="button" aria-label={`${label} Override ${id} remove`} onClick={()=>{const next={...values};delete next[Number(id)];change(next)}}><X size={14}/></button></div>)}</details>
}

function VendorDetails({info,values,change,globalItems}:{info:ModInfo;values:Record<number,VendorOptions>;change:(v:Record<number,VendorOptions>)=>void;globalItems:number[]}) {
  const [key,setKey]=useState('');
  const found=info.vendors.find(v=>String(v.key)===key);
  function edit(id:number,update:Partial<VendorOptions>){change({...values,[id]:{...values[id],...update}})}
  return <details className="mod-scope"><summary>Vendor details ({Object.keys(values).length})</summary>
    <p>Custom item sets and refresh overrides also apply outside the global vendor selection. An empty custom item selection suppresses additional items for that vendor.</p>
    <div className="mod-override-form"><input aria-label="Vendor details ID" placeholder="Vendor ID" value={key} onChange={e=>setKey(e.target.value)}/><button type="button" disabled={!found||values[found.key]!==undefined} onClick={()=>{if(found){change({...values,[found.key]:{items:null,daily_refresh:null}});setKey('')}}}>Add vendor</button></div>
    {key&&<small>{found?.name??'No supported vendor ID found.'}</small>}
    {Object.entries(values).map(([id,options])=>{const n=Number(id);return <section className="mod-vendor-detail" key={id}>
      <strong>{info.vendors.find(v=>v.key===n)?.name} · {id}</strong>
      <label className="mod-switch"><input type="checkbox" aria-label={`Custom items for vendor ${id}`} disabled={!info.append_vendors.includes(n)} checked={options.items!==null} onChange={e=>edit(n,{items:e.target.checked?[...globalItems]:null})}/>Custom item selection</label>
      {options.items!==null&&<Scope choices={info.items} selected={options.items} onChange={items=>edit(n,{items})} label={`Items for vendors ${id}`} emptyAll={false}/>}
      <label className="mod-number">Refresh<select aria-label={`Refresh for vendor ${id}`} value={options.daily_refresh===null?'inherit':options.daily_refresh?'daily':'original'} onChange={e=>edit(n,{daily_refresh:e.target.value==='inherit'?null:e.target.value==='daily'})}>
        <option value="inherit">Global setting</option><option value="daily" disabled={!info.daily_vendors.includes(n)}>Daily</option><option value="original">Keep original interval</option>
      </select></label>
      <button type="button" aria-label={`Vendor details ${id} remove`} onClick={()=>{const next={...values};delete next[n];change(next)}}>Remove override</button>
    </section>})}
  </details>
}
