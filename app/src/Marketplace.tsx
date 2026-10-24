import {ModAvailability} from './ModAvailability';
import {useEffect,useRef,useState} from 'react';
import {BookOpen,Check,Fish,Footprints,Gauge,ListChecks,LoaderCircle,Map,Package,ShieldCheck,Sparkles,Swords,ToggleLeft,ToggleRight,Zap} from 'lucide-react';
import {api,asError} from './api';
import {useCatalog,useMods} from './store';
import {LiveApply} from './LiveApply';
import {MarketplaceRuntime,runtimeModCount,runtimeModMatchCount} from './MarketplaceRuntime';
import {matchesMarketplaceMod} from './marketplace-search';
import {emptySelection,type MarketplaceSelection} from './marketplace-types';
import type {Catalog} from './types';
import type {ModPreview} from './mod-types';
import {MarketplaceSkills} from './MarketplaceSkills';
import {MarketplaceStacks} from './MarketplaceStacks';
import {MarketplaceEffects} from './MarketplaceEffects';
import {MarketplaceContributions} from './MarketplaceContributions';
import {MarketplaceKnowledge} from './MarketplaceKnowledge';
import {MarketplaceMap} from './MarketplaceMap';
import {MarketplaceBrowseControls} from './MarketplaceBrowseControls';
import {emptyMarketplaceBrowseFilter,matchesMarketplaceBrowse,type MarketplaceBrowseFilter,type MarketplaceBrowseSnapshot} from './marketplace-browse';
import './mods.css';
import './marketplace.css';
import './marketplace-browse.css';

export const marketplaceMods=[
 {id:'larger_lion',title:'Larger lion',tag:'APPEARANCE',icon:Footprints,description:'Make the shared lion model 30% larger. Includes the circus lion and other characters using this model; lionesses keep their original size.',detail:'Model scale: 0.90 → 1.17'},
 {id:'faster_mounts',title:'Faster mounts',tag:'MOVEMENT',icon:Gauge,description:'Ride the circus lion and Rokade with a 30% higher movement speed stat. Covers all five Rokade stages.',detail:'Movement speed stat: ×1.30'},
 {id:'no_mount_stamina',title:'No mount stamina cost',tag:'STAMINA',icon:Zap,description:'Remove stamina consumption and increase stamina regeneration for the circus lion and Rokade. Player stamina stays unchanged.',detail:'Mount stamina cost: 0 · regeneration boosted'},
 {id:'inventory_999',title:'999 inventory slots',tag:'INVENTORY',icon:Package,description:'Expand player inventory capacity to 999 slots using the verified player inventory definition.',detail:'Player inventory: 999 slots'},
 {id:'no_durability_loss',title:'No durability loss',tag:'EQUIPMENT',icon:ShieldCheck,description:'Prevent ordinary equipment wear for weapons, armor and tools. Existing lost durability is not repaired.',detail:'Wear disabled · finite durability uses the engine sentinel'},
 {id:'automatic_fishing',title:'Automatic fishing',tag:'FISHING',icon:Fish,description:'Give every supported fishing rod the Crab rod’s automatic reeling effect. Equip a rod and start fishing normally; it reels in fish after a bite. Cast again yourself for the next fish.',detail:'Native auto-reel · all four fishing rods'},
 {id:'free_upgrade_materials',title:'Free equipment upgrades',tag:'CRAFTING',icon:ShieldCheck,description:'Remove material and refinement-token costs from all supported equipment upgrade recipes. Use your existing equipment and follow the original refinement, tool and unlock requirements.',detail:'17,069 upgrade recipes · existing equipment still required'},
 {id:'free_crafting_currency',title:'No Clockwork Insect crafting fee',tag:'CRAFTING',icon:Package,description:'Remove the 300 Clockwork Insects charged by the Kuku drone backpack recipe. Blueprint requirements and results stay unchanged. Material costs are controlled separately.',detail:'Currency component · combine with Free crafting materials'},
 {id:'larger_rokade',title:'Larger Rokade',tag:'APPEARANCE',icon:Footprints,description:'Make Rokade 30% larger with a dedicated appearance. Other horses keep their original size. Combine with Faster mounts for the requested speed setting.',detail:'Rokade only · model scale: 1.13 → 1.469'},
] as const;
const mods=marketplaceMods;

const extraModText={
 knowledge:'Unlock journal knowledge people places creatures recipes quest information missing entries levels save action',
 skills:'Skill unlocks progression save action individual abilities unlock all 140 abilities Kliff Oongka Damiane available points',
 item_stacks:'Item stack limits inventory maximum stack size choose item quantities',
 map:'Reveal world map exploration save action map fog quest knowledge discovered locations fast travel',
 contributions:'Contribution points progression save action faction contribution balances experience levels currencies',
 effects:'Native weapon effects equipment game mod Electro-Mecha Longsword lightning attack Flame Spear fire upgrades sockets native abilities',
};
const extraModTags:Record<keyof typeof extraModText,string>={knowledge:'PROGRESSION',skills:'PROGRESSION',item_stacks:'INVENTORY',map:'EXPLORATION',contributions:'PROGRESSION',effects:'EQUIPMENT'};

export function Marketplace({catalog}:{catalog:Catalog}) {
 const request=useMods(s=>s.request),set=useMods(s=>s.set),setView=useCatalog(s=>s.setView);
 const selected={...emptySelection,...request.marketplace};
 const [preview,setPreview]=useState<ModPreview|null>(null),[busy,setBusy]=useState(false),[error,setError]=useState(''),[exported,setExported]=useState(''),[skills,setSkills]=useState(false),[effects,setEffects]=useState(false);
 const [contributions,setContributions]=useState(false);
 const [map,setMap]=useState(false),[knowledge,setKnowledge]=useState(false);
 const [query,setQuery]=useState('');
 const [browseFilter,setBrowseFilter]=useState<MarketplaceBrowseFilter>({...emptyMarketplaceBrowseFilter});
 const [runtimeSnapshot,setRuntimeSnapshot]=useState<MarketplaceBrowseSnapshot>(null);
 const browseAnchor=useRef<HTMLDivElement>(null);
 const key=JSON.stringify(request),latest=useRef(key);latest.current=key;
 const lock=useRef(false),mounted=useRef(true);
 useEffect(()=>{mounted.current=true;return()=>{mounted.current=false}},[]);
 useEffect(()=>{setPreview(null);setError('');setExported('')},[key]);
 async function work(action:()=>Promise<void>){if(lock.current)return;lock.current=true;setBusy(true);setError('');try{await action()}catch(e){if(mounted.current)setError(asError(e).message)}finally{lock.current=false;if(mounted.current)setBusy(false)}}
 function toggle(id:Exclude<keyof MarketplaceSelection,'item_stacks'|'weapon_effects'>){set({marketplace:{...selected,[id]:!selected[id]}})}
 async function calculate(){const original=latest.current;const p=await api.modPreview(catalog.session,request);if(mounted.current&&latest.current===original)setPreview(p)}
 function revealBrowse(){
  const anchor=browseAnchor.current,container=anchor?.parentElement;
  if(!anchor||!container)return;
  const top=container.scrollTop+anchor.getBoundingClientRect().top-container.getBoundingClientRect().top;
  if(container.scrollTop>top)container.scrollTo({top});
 }
 function changeQuery(value:string){revealBrowse();setQuery(value)}
 function changeBrowseFilter(value:MarketplaceBrowseFilter){revealBrowse();setBrowseFilter(value)}
 const supported=catalog.info.exe_version==='1.0.0.2976';
 const n=Object.entries(selected).filter(([key,value])=>['item_stacks','weapon_effects'].includes(key)?Object.keys(value).length>0:value===true).length;
 const matches=(id:keyof typeof extraModText)=>matchesMarketplaceMod(query,id,extraModText[id])&&matchesMarketplaceBrowse(browseFilter,{id,tag:extraModTags[id]});
 const matchesCard=(mod:typeof mods[number])=>matchesMarketplaceMod(query,mod.id,mod.title,mod.tag,mod.description,mod.detail)&&matchesMarketplaceBrowse(browseFilter,mod);
 const total=mods.length+Object.keys(extraModText).length+runtimeModCount;
 const shown=mods.filter(matchesCard).length+(Object.keys(extraModText) as (keyof typeof extraModText)[]).filter(matches).length+runtimeModMatchCount(query,browseFilter,runtimeSnapshot);
 return <div className="marketplace market-browsable">
  <div className="page-heading"><div><div className="eyebrow">CURATED · LOCAL</div><h1>Mod marketplace<span>.</span></h1><p>Your mods, ready to use. No account, payment or extra downloads.</p></div><span className="market-count"><Package size={18}/>{total} mods</span></div>
  <div className="market-intro"><ShieldCheck size={22}/><p>Built for game version <strong>1.0.0.2976</strong>. Each card shows when its changes can be applied. You can browse and prepare plans while playing.</p></div>
  <div className="market-browse-anchor" ref={browseAnchor} aria-hidden="true"/>
  <MarketplaceBrowseControls query={query} changeQuery={changeQuery} filter={browseFilter} changeFilter={changeBrowseFilter} total={total} shown={shown} snapshot={runtimeSnapshot}/>
  {!supported&&<p role="alert" className="craft-error">These recipes require game version 1.0.0.2976. Your connected version is {catalog.info.exe_version??'unknown'}.</p>}
  <div className="market-grid">{mods.map(mod=>{const Icon=mod.icon;return <article hidden={!matchesCard(mod)} className={`market-card ${selected[mod.id]?'selected':''}`} key={mod.id}>
   <div className="market-card-top"><div className={`market-icon ${mod.id}`}><Icon size={28}/></div><span className={`market-draft-badge ${selected[mod.id]?'selected':''}`}>{selected[mod.id]&&<Check size={12}/>} {selected[mod.id]?'Draft selected':'Not selected for draft'}</span></div>
   <small>{mod.tag} · GAME MOD</small><h2>{mod.title}</h2><ModAvailability mode="closed"/><p>{mod.description}</p><div className="market-detail">{mod.detail}</div>
   <button className={`secondary market-toggle ${selected[mod.id]?'selected':''}`} disabled={busy||!supported} aria-pressed={selected[mod.id]} onClick={()=>toggle(mod.id)}>{selected[mod.id]?<ToggleRight size={24}/>:<ToggleLeft size={24}/>} {selected[mod.id]?'Deselect mod':'Select mod'}</button>
   <span className="market-hint">{selected[mod.id]?'Selected in draft · not an installation status':'Combine with the other game mods'}</span>
  </article>})}
   <article hidden={!matches('knowledge')} className="market-card knowledge-card"><div className="market-card-top"><div className="market-icon all-skills"><BookOpen size={28}/></div></div><small>PROGRESSION · SAVE ACTION</small><h2>Unlock journal knowledge</h2><ModAvailability mode="closed"/><p>Complete 3,939 supported journal entries. Add missing knowledge and levels for your selected save, keeping existing entries and skill points.</p><div className="market-detail">People, places, creatures, recipes and quest information</div><button className="primary" disabled={!supported} onClick={()=>setKnowledge(true)}>Choose save & review <ListChecks size={16}/></button><span className="market-hint">Includes story spoilers · backup before changes</span></article>
   <article hidden={!matches('skills')} className="market-card skills-card"><div className="market-card-top"><div className="market-icon all-skills"><Sparkles size={28}/></div></div><small>PROGRESSION · SAVE ACTION</small><h2>Skill unlocks</h2><ModAvailability mode="closed"/><p>Choose individual abilities or unlock all 140 abilities across Kliff, Oongka and Damiane. Keep your available points, inventory and other save progress.</p><div className="market-detail">One selected save · backup before changes</div><button className="primary" disabled={!supported} onClick={()=>setSkills(true)}>Choose save & review <ListChecks size={16}/></button><span className="market-hint">A save edit; it cannot be toggled off like a game mod</span></article>
   <MarketplaceStacks catalog={catalog} supported={supported} hidden={!matches('item_stacks')}/>
   <article hidden={!matches('map')} className="market-card"><div className="market-card-top"><div className="market-icon"><Map size={28}/></div></div><small>EXPLORATION · SAVE ACTION</small><h2>Reveal world map</h2><ModAvailability mode="closed"/><p>Remove world map fog in a selected save. Preserve quest progress, knowledge, discovered locations and fast-travel unlocks.</p><div className="market-detail">Map fog only · selected save · automatic backup</div><button className="secondary" disabled={!supported} onClick={()=>setMap(true)}>Choose save & reveal map</button><span className="market-hint">Save action · close the game before applying</span></article>
   <article hidden={!matches('contributions')} className="market-card"><div className="market-card-top"><div className="market-icon"><ListChecks size={28}/></div></div><small>PROGRESSION · SAVE ACTION</small><h2>Contribution points</h2><ModAvailability mode="closed"/><p>Increase selected faction contribution balances to your chosen amounts. Preserve faction experience, levels, other currencies and inventory items.</p><div className="market-detail">Six faction balances · selected save · automatic backup</div><button className="secondary" disabled={!supported} onClick={()=>setContributions(true)}>Choose save & contribution targets</button><span className="market-hint">Existing wallet entries only · close the game before applying</span></article>
   <article hidden={!matches('effects')} className="market-card"><div className="market-card-top"><div className="market-icon"><Swords size={28}/></div><span className={`market-draft-badge ${Object.keys(selected.weapon_effects).length?'selected':''}`}>{Object.keys(selected.weapon_effects).length>0&&<Check size={12}/>} {Object.keys(selected.weapon_effects).length?'Draft selected':'Not selected for draft'}</span></div><small>EQUIPMENT · GAME MOD</small><h2>Native weapon effects</h2><ModAvailability mode="closed"/><p>Transfer the Electro-Mecha Longsword’s lightning attack to compatible weapons, or use the Flame Spear’s fire effect. Preserve upgrade stats and sockets. Choose how to handle existing native abilities.</p><div className="market-detail">Native abilities · reversible in your combined plan</div><button className="secondary" disabled={!supported} onClick={()=>setEffects(true)}>Choose weapons & effects</button><span className="market-hint">Combat and visual testing of transfers is pending</span></article>
   <MarketplaceRuntime session={catalog.session} supported={supported} query={query} browseFilter={browseFilter} onBrowseSnapshot={setRuntimeSnapshot}/>
  </div>
  {shown===0&&<div className="market-search-empty" role="status"><h2>No mods match your search</h2><p>Try a different name, effect, category or status filter.</p><button className="secondary" onClick={()=>{revealBrowse();setQuery('');setBrowseFilter({...emptyMarketplaceBrowseFilter})}}>Show all mods</button></div>}
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
  {knowledge&&<MarketplaceKnowledge catalog={catalog} close={()=>setKnowledge(false)}/>}
  {skills&&<MarketplaceSkills catalog={catalog} close={()=>setSkills(false)}/>}
  {effects&&<MarketplaceEffects catalog={catalog} close={()=>setEffects(false)}/>}
  {contributions&&<MarketplaceContributions catalog={catalog} close={()=>setContributions(false)}/>}
  {map&&<MarketplaceMap catalog={catalog} close={()=>setMap(false)}/>}
 </div>
}
