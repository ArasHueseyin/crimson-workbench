import {useEffect,useRef,useState} from 'react';
import {Footprints,Gauge,ListChecks,LoaderCircle,Package,ShieldCheck,Sparkles,ToggleLeft,ToggleRight,Zap} from 'lucide-react';
import {api,asError} from './api';
import {useCatalog,useMods} from './store';
import {LiveApply} from './LiveApply';
import {emptySelection,type MarketplaceSelection} from './marketplace-types';
import type {Catalog} from './types';
import type {ModPreview} from './mod-types';
import type {SkillPreview,SkillReceipt,SkillRequest} from './marketplace-types';
import './mods.css';
import './marketplace.css';

const mods=[
 {id:'larger_lion',title:'Larger lion',tag:'APPEARANCE',icon:Footprints,description:'Make the shared lion model 30% larger. Includes the circus lion and other characters using this model; lionesses keep their original size.',detail:'Model scale: 0.90 → 1.17'},
 {id:'faster_mounts',title:'Faster mounts',tag:'MOVEMENT',icon:Gauge,description:'Ride the circus lion and Rokade with a 30% higher movement speed stat. Covers all five Rokade stages.',detail:'Movement speed stat: ×1.30'},
 {id:'no_mount_stamina',title:'No mount stamina cost',tag:'STAMINA',icon:Zap,description:'Remove stamina consumption and increase stamina regeneration for the circus lion and Rokade. Player stamina stays unchanged.',detail:'Mount stamina cost: 0 · regeneration boosted'},
] as const;

export function Marketplace({catalog}:{catalog:Catalog}) {
 const request=useMods(s=>s.request),set=useMods(s=>s.set),setView=useCatalog(s=>s.setView);
 const selected=request.marketplace??emptySelection;
 const [preview,setPreview]=useState<ModPreview|null>(null),[busy,setBusy]=useState(false),[error,setError]=useState(''),[exported,setExported]=useState(''),[skills,setSkills]=useState(false);
 const key=JSON.stringify(request),latest=useRef(key);latest.current=key;
 const lock=useRef(false),mounted=useRef(true);
 useEffect(()=>{mounted.current=true;return()=>{mounted.current=false}},[]);
 useEffect(()=>{setPreview(null);setError('');setExported('')},[key]);
 async function work(action:()=>Promise<void>){if(lock.current)return;lock.current=true;setBusy(true);setError('');try{await action()}catch(e){if(mounted.current)setError(asError(e).message)}finally{lock.current=false;if(mounted.current)setBusy(false)}}
 function toggle(id:keyof MarketplaceSelection){set({marketplace:{...selected,[id]:!selected[id]}})}
 async function calculate(){const original=latest.current;const p=await api.modPreview(catalog.session,request);if(mounted.current&&latest.current===original)setPreview(p)}
 const supported=catalog.info.exe_version==='1.0.0.2976';
 const n=Object.values(selected).filter(Boolean).length;
 return <div className="marketplace">
  <div className="page-heading"><div><div className="eyebrow">FREE · CURATED · LOCAL</div><h1>Mod marketplace<span>.</span></h1><p>Your mods, ready to use. No account, payment or extra downloads.</p></div><span className="market-count"><Package size={18}/>4 free mods</span></div>
  <div className="market-intro"><ShieldCheck size={22}/><p>Built for game version <strong>1.0.0.2976</strong>. Recipes use your own game files. Select game mods below, review the combined plan, then use <strong>Live apply & restore</strong> with the game closed.</p></div>
  {!supported&&<p role="alert" className="craft-error">These recipes require game version 1.0.0.2976. Your connected version is {catalog.info.exe_version??'unknown'}.</p>}
  <div className="market-grid">{mods.map(mod=>{const Icon=mod.icon;return <article className={`market-card ${selected[mod.id]?'selected':''}`} key={mod.id}>
   <div className="market-card-top"><div className={`market-icon ${mod.id}`}><Icon size={28}/></div><span className="market-free">FREE</span></div>
   <small>{mod.tag} · GAME MOD</small><h2>{mod.title}</h2><p>{mod.description}</p><div className="market-detail">{mod.detail}</div>
   <button className={`secondary market-toggle ${selected[mod.id]?'selected':''}`} disabled={busy||!supported} aria-pressed={selected[mod.id]} onClick={()=>toggle(mod.id)}>{selected[mod.id]?<ToggleRight size={24}/>:<ToggleLeft size={24}/>} {selected[mod.id]?'Deselect mod':'Select mod'}</button>
   <span className="market-hint">{selected[mod.id]?'Selected in draft · not an installation status':'Combine with the other game mods'}</span>
  </article>})}
   <article className="market-card skills-card"><div className="market-card-top"><div className="market-icon all-skills"><Sparkles size={28}/></div><span className="market-free">FREE</span></div><small>PROGRESSION · SAVE ACTION</small><h2>All skills</h2><p>Max out all 140 abilities in the Kliff, Oongka and Damiane skill trees. Keep your available points, inventory and other save progress.</p><div className="market-detail">One selected save · backup before changes</div><button className="primary" disabled={!supported} onClick={()=>setSkills(true)}>Choose save & review <ListChecks size={16}/></button><span className="market-hint">A save edit; it cannot be toggled off like a game mod</span></article>
  </div>
  <section className="market-plan"><div><small>YOUR COMBINED PLAN</small><h2>{n} marketplace {n===1?'mod':'mods'} selected</h2><p>Workshop settings are kept in the same plan. Applying a new plan replaces the previous Workbench overlay. To remove one mod, remove it from the plan and apply again. Restore removes the complete Workbench overlay. Unmanaged mods remain untouched.</p></div><div className="mod-actions"><button className="primary" disabled={busy||!supported} onClick={()=>void work(calculate)}>{busy?<LoaderCircle size={16} className="spin"/>:<Package size={16}/>}Review combined plan</button><button className="secondary" onClick={()=>setView('mods')}>Open mod workshop</button></div>
   {error&&<p role="alert" className="craft-error">{error}</p>}
   {preview&&<div className="recovery-review"><strong>{preview.changes.length} changes · {preview.files.length} planned files</strong>{preview.changes.length===0&&<p>This plan contains no changes. Use Check live restore below to remove an active Workbench overlay.</p>}
    {!preview.gates.can_apply&&<p>Live writes require the installation checks below. A preview alone does not authorize applying this plan.</p>}
    {preview.warnings.filter(w=>w.includes('unmanaged changes')).map(w=><p role="alert" key={w}>{w}</p>)}
    <details open><summary>Changes to review</summary><div className="market-changes">{preview.changes.map((c,i)=><div key={i}><strong>{c.name}</strong><span>{c.field}</span><code>{c.before} → {c.after}</code></div>)}</div></details>
    <button className="secondary" disabled={busy||!preview.files.length} onClick={()=>void work(async()=>{const path=await api.modExport(catalog.session,preview.request,preview.plan_id);if(mounted.current)setExported(path)})}>Export local package</button>
    {exported&&<p role="status">Package saved to <code>{exported}</code></p>}
   </div>}
  </section>
  <LiveApply session={catalog.session} settings={preview?.files.length?preview.request:null}/>
  {skills&&<SkillUnlock catalog={catalog} close={()=>setSkills(false)}/>}
 </div>
}

function SkillUnlock({catalog,close}:{catalog:Catalog;close:()=>void}) {
 const [preview,setPreview]=useState<SkillPreview|null>(null),[save,setSave]=useState(''),[checked,setChecked]=useState(false),[busy,setBusy]=useState(false),[error,setError]=useState(''),[receipt,setReceipt]=useState<SkillReceipt|null>(null);
 const storage=`crimson-skill-unlock-pending:${catalog.game_path}`;
 const [pending,setPending]=useState<SkillRequest|null>(()=>{try{return JSON.parse(localStorage.getItem(storage)??'null') as SkillRequest|null}catch{return null}});
 const latest=useRef(0),lock=useRef(false),mounted=useRef(true);
 useEffect(()=>{mounted.current=true;return()=>{mounted.current=false;latest.current++}},[]);
 async function load(id:string|null){const generation=++latest.current;setChecked(false);setError('');setPreview(null);setBusy(true);try{const p=await api.skillUnlockPreview(catalog.session,id);if(mounted.current&&generation===latest.current)setPreview(p)}catch(e){if(mounted.current&&generation===latest.current)setError(asError(e).message)}finally{if(mounted.current&&generation===latest.current)setBusy(false)}}
 useEffect(()=>{void load(null)},[]);
 async function apply(request:SkillRequest){if(lock.current)return;lock.current=true;setBusy(true);setError('');try{const result=await api.skillUnlockApply(catalog.session,request);localStorage.removeItem(storage);if(mounted.current){setPending(null);setReceipt(result);setChecked(false);await load(request.save)}}catch(e){if(mounted.current)setError(asError(e).message)}finally{lock.current=false;if(mounted.current)setBusy(false)}}
 function unlock(){if(!preview?.selected_save||!preview.save_sha256||!preview.lobby_sha256||!preview.plan_id||!checked)return;const request:SkillRequest={save_root:preview.save_root,id:crypto.randomUUID().replaceAll('-',''),save:preview.selected_save,save_sha256:preview.save_sha256,lobby_sha256:preview.lobby_sha256,plan_id:preview.plan_id};try{localStorage.setItem(storage,JSON.stringify(request));setPending(request);void apply(request)}catch(e){setError(asError(e).message)}}
 const summary=preview?.summary;
 return <div className="market-modal-backdrop"><section className="market-modal" role="dialog" aria-modal="true" aria-labelledby="skills-title"><div className="market-modal-heading"><div><small>FREE SAVE ACTION</small><h2 id="skills-title">Unlock all skills</h2></div><button className="secondary" disabled={busy} onClick={close}>Close</button></div>
  <p>Choose the exact slot you will load in game. Workbench creates a backup and grants the missing levels for all three character trees. Available skill points and inventory remain unchanged.</p>
  {preview&&<p>Save folder: <code>{preview.save_root}</code></p>}
  <label className="source-field">Save slot<select aria-label="Skill unlock save" value={save} disabled={busy||!!pending} onChange={e=>{setSave(e.target.value);setReceipt(null);void load(e.target.value||null)}}><option value="">Choose a save</option>{preview?.saves.map(s=><option key={s.id} value={s.id}>{s.label}</option>)}</select></label>
  {preview&&!preview.saves.length&&<p>No complete saves found. Select your save folder under Data sources.</p>}
  {summary&&<div className="skills-summary"><div><strong>{summary.new_abilities}</strong><span>New abilities</span></div><div><strong>{summary.upgraded_abilities}</strong><span>Upgraded abilities</span></div><div><strong>{summary.already_maxed}</strong><span>Already at maximum</span></div></div>}
  {summary&&<p>Available points kept: Kliff {summary.available_points[0]}, Oongka {summary.available_points[1]}, Damiane {summary.available_points[2]}. Missing upgrade costs are recorded as gifted points so refunds remain consistent.</p>}
  {preview?.game_running&&<p role="alert" className="craft-error">Save and fully close Crimson Desert before applying this action.</p>}
  {busy&&<p role="status">Checking the save and preparing the transaction …</p>}
  {error&&<p role="alert" className="craft-error">{error}</p>}
  {pending&&<div className="recovery-review"><p>A saved unlock request needs a result check. Retry uses the same request ID and cannot grant points twice.</p><button className="secondary" disabled={busy} onClick={()=>void apply(pending)}>Check / recover pending unlock</button><button className="secondary" disabled={busy} onClick={()=>{localStorage.removeItem(storage);setPending(null);void load(save||null)}}>Reload selection</button></div>}
  {receipt&&<div className="recovery-success" role="status"><p>{receipt.message}</p><strong>Backup folder</strong><code>{receipt.backup}</code></div>}
  <p className="fine-print">This changes save progression. There is no automatic off switch: restoring an old save later would also lose progress made since that backup.</p>
  <label className="mod-switch"><input type="checkbox" aria-label="Confirm skill save edit" checked={checked} disabled={busy||!!pending||!summary||preview?.game_running} onChange={e=>setChecked(e.target.checked)}/>I want to edit this save and understand that this is a permanent progression change.</label>
  <button className="primary" disabled={busy||!!pending||!checked||!summary||summary.already_maxed===summary.total||preview?.game_running} onClick={unlock}><Sparkles size={16}/>Back up & unlock all skills</button>
 </section></div>
}
