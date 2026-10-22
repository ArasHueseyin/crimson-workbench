import {ModAvailability} from './ModAvailability';
import {useEffect,useRef,useState} from 'react';
import {Bird,Camera,CloudSun,Download,Gauge,Hourglass,LoaderCircle,MapPin,Moon,Package,RefreshCw,ShieldCheck,TimerOff,Trash2,Zap} from 'lucide-react';
import {api,asError} from './api';
import type {ConsumableSettings,NativeModSnapshot,NativeModId,PlayerJumpSettings,PlayerSpeedSettings,RuntimeOptionId,RuntimeOptionSettings} from './marketplace-types';
import {MarketplaceConsumableSelection} from './MarketplaceConsumableSelection';
import {MarketplaceWorldControls} from './MarketplaceWorldControls';
import {MarketplacePhotoControls} from './MarketplacePhotoControls';
import {MarketplaceTeleportControls} from './MarketplaceTeleportControls';
import {matchesMarketplaceMod} from './marketplace-search';
import {MarketplaceStartupControls,RuntimeNumericInput} from './MarketplaceStartupControls';
import {Dialog} from './Dialog';
import {emptyMarketplaceBrowseFilter,matchesMarketplaceBrowse,type MarketplaceBrowseFilter,type MarketplaceBrowseSnapshot} from './marketplace-browse';
import {discardStartupDraft,readStartupDrafts,reconcileStartupDraft,updateStartupDraft} from './startup-draft-store';
import './runtime-ui.css';

const mods=[
 {id:'region_return_30min',title:'More time beyond restricted regions',tag:'EXPLORATION',icon:Hourglass,description:'Extend restricted-region return warnings from 30 seconds to 30 minutes. Install once with the game closed. Existing mods are preserved; quest timers and other world boundaries keep their own rules.',detail:'Independent installation · return countdown: 30 seconds → 30 minutes'},
 {id:'glide_speed',title:'Gliding speed',tag:'MOVEMENT',icon:Bird,description:'Increase boosted gliding speed up to 500% (5×) using the game’s glide boost action. Opening the wings and ordinary gliding remain unchanged. Native ability and resource requirements still apply.',detail:'100%–500% boost speed · ordinary wing glide unchanged'},
 {id:'teleport',title:'Teleport',tag:'EXPLORATION',icon:MapPin,description:'Save custom positions and return to them, or travel to mainland map pins and waypoints. The game checks native ground and placement before marker travel. Unloaded or unsuitable marker destinations are refused.',detail:'Live controls · 16 saved positions · current field and native movement rules'},
 {id:'photo_mode',title:'Extended photo mode',tag:'CAMERA',icon:Camera,description:'Open the normal movable photo camera while playing, hide or restore its HUD and freeze the scene through the game’s native photo mode. Move the camera using the game’s normal photo controls and limits.',detail:'Live controls · native photo camera · HUD visibility · scene pause'},
 {id:'world_controls',title:'Live time & weather',tag:'WORLD',icon:CloudSun,description:'Set the time of day or move forward by hours, including later days. Adjust rain, snow, clouds, humidity, wind and fog while playing. Freeze the clock and weather independently, or reset weather to restore the game’s previous weather control.',detail:'Live controls · set or advance time · independent time and weather reset'},
 {id:'auto_loot',title:'Auto-loot nearby items',tag:'LOOT',icon:Package,description:'Collect eligible free ground items and normal corpse loot within the normal interaction radius, including outside the camera view. Retains native ownership, crime, animation and inventory-capacity checks. Purchases, stealing and scripted interactions are excluded.',detail:'Free ground & corpse loot · normal interaction rules · no inventory grants'},
 {id:'mount_dismount',title:'Stay mounted on longer falls',tag:'MOUNTS',icon:Bird,description:'Keep riding the circus lion or Rokade until five times the normal automatic fall-height threshold. Manual dismounts, combat throws and other native removal rules still apply.',detail:'5× automatic fall threshold · separate from jump height and fall damage'},
 {id:'force_palm',title:'Unlimited Force Palm jumps',tag:'MOVEMENT',icon:Bird,description:'Repeat the aerial Force Palm chain beyond its normal three-jump limit. The ability must be learned, and its normal spirit costs and movement checks still apply.',detail:'Remove the three-use limit · normal spirit costs · player only'},
 {id:'force_palm_height',title:'Force Palm jump height',tag:'MOVEMENT',icon:Bird,description:'Choose a height multiplier for the aerial Force Palm jump, from normal height up to 100×. The ability must be learned; jump repetition and spirit protection remain separate choices.',detail:'1×–100× height · independent of normal player jump height'},
 {id:'axiom_reach',title:'Axiom armlet reach',tag:'ABILITIES',icon:Zap,description:'Extend the reach of Axiom Force with a multiplier from 2× to 5×, or reset it to normal. Native target eligibility, object weight and ability requirements still apply.',detail:'2×–5× reach · reset to normal 1×'},
 {id:'free_crafting_materials',title:'Free crafting materials',tag:'CRAFTING',icon:Package,description:'Craft from 1,507 supported recipes without material requirements. Recipe unlocks, tools and results keep their normal rules. The separate Clockwork Insect fee is still required unless you remove it with its own mod.',detail:'Free recipes: up to 40 per batch · separate currency fees retained'},
 {id:'mount_protection',title:'Protect active mount',tag:'MOUNTS',icon:ShieldCheck,description:'Protect the mount you are riding and your selected summoned mount, including while you are on foot. Healing and player health are unchanged. Scripted deaths and despawns keep their normal behavior.',detail:'Combat HP protection · inactive registered mounts stay unchanged'},
 {id:'preserve_consumables',title:'Preserve selected consumables',tag:'INVENTORY',icon:Package,description:'Choose arrows, stat potions, food and Holy Water to keep after normal use. Native effects, cooldowns and other item actions keep their normal rules. Unselected items are consumed normally.',detail:'561 selectable items · arrows, potions, food & healing'},
 {id:'player_speed',title:'Player movement speed',tag:'MOVEMENT',icon:Gauge,description:'Adjust normal ground walking, running and sprinting for the character you control, from 100% to 300%. Mount speed and airborne movement are separate.',detail:'Horizontal ground movement · original gravity, time step and collision checks'},
 {id:'player_jump',title:'Player jump height',tag:'MOVEMENT',icon:Bird,description:'Set native jump height for the character you control, from normal height up to 3×. Mount jumps and aerial-use limits are separate settings.',detail:'Player only · original gravity and jump eligibility preserved'},
 {id:'mount_jump',title:'Higher lion & Rokade jumps',tag:'MOUNTS',icon:Bird,description:'Jump 40% higher with the circus lion and Rokade. Changes their native jump-height input while keeping the original gravity and jump checks.',detail:'+40% jump height · forced dismount threshold is a separate mod'},
 {id:'player_stamina',title:'Unlimited player stamina & spirit',tag:'RESOURCES',icon:Zap,description:'Protect stamina, spirit or both for the character you control. Sprint, climb, swim and use learned abilities without spending the selected resource. Native skill eligibility still requires enough spirit. Mount resources are separate.',detail:'Independent stamina and spirit switches · no health or maximum-stat changes'},
 {id:'no_fall',title:'No fall damage',tag:'PROTECTION',icon:ShieldCheck,description:'Prevent fall damage for Kliff, Oongka, Damiane and the verified rideable character definitions, including the lion and Rokade.',detail:'Fall damage protection · ordinary combat damage stays unchanged'},
 {id:'sleep_cooldown',title:'No sleep cooldown',tag:'SLEEP',icon:TimerOff,description:'Sleep or wait again without a cooldown. Works with beds and campfires while keeping the game’s other validity checks.',detail:'No extra delay · sleep durations unchanged'},
 {id:'sleep_duration',title:'30-hour sleep',tag:'TIME',icon:Moon,description:'Advance the game clock by 30 hours when you choose the 12-hour sleep option. The in-game label still says 12 hours; other sleep durations stay unchanged.',detail:'Choose 12 hours in the game → sleep 30 hours'},
 {id:'blackstar',title:'Blackstar anywhere',tag:'DRAGON',icon:Bird,description:'Use Blackstar with regional, city and altitude restrictions lifted, boosted stamina regeneration, a 1-second summon cooldown and a 1,000-minute riding duration.',detail:'Quest, indoor and final placement checks can still block summoning'},
] as const;
export const runtimeModCount=mods.length;
const liveMod=(id:string)=>['world_controls','photo_mode','teleport'].includes(id);
export const runtimeModMatchCount=(query:string,filter:MarketplaceBrowseFilter=emptyMarketplaceBrowseFilter,snapshot:MarketplaceBrowseSnapshot=null)=>mods.filter(mod=>matchesMarketplaceMod(query,mod.id,mod.title,mod.tag,mod.description,mod.detail)&&matchesMarketplaceBrowse(filter,{...mod,live:liveMod(mod.id)},snapshot)).length;

type Update={native?:NativeModSnapshot;nativeFailed?:boolean;failedSettings?:NativeModId[];jump?:PlayerJumpSettings;speed?:PlayerSpeedSettings;consumables?:ConsumableSettings;options?:RuntimeOptionSettings[];errors?:string[]};
type Feedback={kind:'busy'|'success'|'error';text:string};
const optionForMod:Partial<Record<NativeModId,RuntimeOptionId>>={force_palm_height:'force_palm_height',axiom_reach:'axiom_reach',glide_speed:'glide_speed',player_stamina:'player_resources'};
const emptyOptions:Record<RuntimeOptionId,RuntimeOptionSettings|null>={force_palm_height:null,axiom_reach:null,glide_speed:null,player_resources:null};
const defaultOptions:Record<RuntimeOptionId,number>={force_palm_height:100,axiom_reach:100,glide_speed:100,player_resources:1};
const defaultValidity:Record<RuntimeOptionId,boolean>={force_palm_height:true,axiom_reach:true,glide_speed:true,player_resources:true};

export function MarketplaceRuntime({session,supported,query='',browseFilter=emptyMarketplaceBrowseFilter,onBrowseSnapshot}:{session:number;supported:boolean;query?:string;browseFilter?:MarketplaceBrowseFilter;onBrowseSnapshot?:(snapshot:MarketplaceBrowseSnapshot)=>void}) {
 const [worldOpen,setWorldOpen]=useState(false);
 const [photoOpen,setPhotoOpen]=useState(false);
 const [teleportOpen,setTeleportOpen]=useState(false);
 const [snapshot,setSnapshot]=useState<NativeModSnapshot|null>(null),[jump,setJump]=useState<PlayerJumpSettings|null>(null),[percent,setPercent]=useState(100);
 const [speed,setSpeed]=useState<PlayerSpeedSettings|null>(null),[speedPercent,setSpeedPercent]=useState(100);
 const [consumables,setConsumables]=useState<ConsumableSettings|null>(null),[protectedItems,setProtectedItems]=useState<number[]>([]);
 const [busy,setBusy]=useState(false),[error,setError]=useState(''),[message,setMessage]=useState('');
 const [gameRunning,setGameRunning]=useState<boolean|null>(null),[gameError,setGameError]=useState('');
 const [busyMod,setBusyMod]=useState<NativeModId|null>(null),[feedback,setFeedback]=useState<Partial<Record<NativeModId,Feedback>>>({});
 const [jumpNumericValid,setJumpNumericValid]=useState(true),[speedNumericValid,setSpeedNumericValid]=useState(true);
 const [options,setOptions]=useState(emptyOptions),[optionValues,setOptionValues]=useState(defaultOptions);
 const [optionValidity,setOptionValidity]=useState(defaultValidity);
 const [readFailures,setReadFailures]=useState<Set<NativeModId>>(()=>new Set());
 const mounted=useRef(true),lock=useRef(false),revision=useRef(0);
 async function read():Promise<Update> {
  const results=await Promise.allSettled([api.marketplaceModStatus(session),api.marketplacePlayerJump(session),api.marketplacePlayerSpeed(session),api.marketplaceConsumables(session),api.marketplaceRuntimeSetting(session,'force_palm_height'),api.marketplaceRuntimeSetting(session,'axiom_reach'),api.marketplaceRuntimeSetting(session,'player_resources'),api.marketplaceRuntimeSetting(session,'glide_speed')]);
  const [native,jump,speed,consumables]=results;
  const labels=['Runtime status','Jump settings','Movement settings','Consumable settings','Force Palm height settings','Axiom reach settings','Resource settings','Gliding speed settings'];
  const settingsIds:(NativeModId|null)[]=[null,'player_jump','player_speed','preserve_consumables','force_palm_height','axiom_reach','player_stamina','glide_speed'];
  return {
   ...(native.status==='fulfilled'?{native:native.value}:{}),
   nativeFailed:native.status==='rejected',
   failedSettings:results.flatMap((result,i)=>result.status==='rejected'&&settingsIds[i]?[settingsIds[i]!]:[]),
   ...(jump.status==='fulfilled'?{jump:jump.value}:{}),
   ...(speed.status==='fulfilled'?{speed:speed.value}:{}),
   ...(consumables.status==='fulfilled'?{consumables:consumables.value}:{}),
   options:results.slice(4).flatMap(result=>result.status==='fulfilled'?[result.value as RuntimeOptionSettings]:[]),
   errors:results.flatMap((result,i)=>result.status==='rejected'?[`${labels[i]}: ${asError(result.reason).message}`]:[]),
  };
 }
 function accept(next:Update) {
  if(next.errors)setError(next.errors.join(' '));
  if(next.native){setSnapshot(next.native);onBrowseSnapshot?.(next.native)}
  if(next.nativeFailed){setSnapshot(null);onBrowseSnapshot?.(null)}
  // A failed refresh removes that resource's saved-state authority, while its
  // editable value remains in component state and the persistent draft store.
  for(const id of next.failedSettings??[]) {
   if(id==='player_jump')setJump(null);else if(id==='player_speed')setSpeed(null);else if(id==='preserve_consumables')setConsumables(null);
   else {const optionId=optionForMod[id];if(optionId)setOptions(previous=>({...previous,[optionId]:null}))}
  }
  if(next.jump){setJump(next.jump);setPercent(reconcileStartupDraft('player_jump',next.jump.height_percent))}
  if(next.speed){setSpeed(next.speed);setSpeedPercent(reconcileStartupDraft('player_speed',next.speed.speed_percent))}
  if(next.consumables){setConsumables(next.consumables);setProtectedItems(reconcileStartupDraft('preserve_consumables',next.consumables.item_keys))}
  if(next.options?.length){setOptions(previous=>({...previous,...Object.fromEntries(next.options!.map(setting=>[setting.id,setting]))}));setOptionValues(previous=>({...previous,...Object.fromEntries(next.options!.map(setting=>[setting.id,reconcileStartupDraft(setting.id,setting.value)]))}))}
  setReadFailures(previous=>{
   const unknown=new Set(previous);
   if(next.jump)unknown.delete('player_jump');if(next.speed)unknown.delete('player_speed');if(next.consumables)unknown.delete('preserve_consumables');
   for(const setting of next.options??[])unknown.delete(setting.id==='player_resources'?'player_stamina':setting.id);
   for(const id of next.failedSettings??[])unknown.add(id);
   return unknown;
  });
 }
 useEffect(()=>{
  mounted.current=true;const generation=++revision.current;
  const drafts=readStartupDrafts();
  setSnapshot(null);onBrowseSnapshot?.(null);setJump(null);setPercent(drafts.player_jump??100);setSpeed(null);setSpeedPercent(drafts.player_speed??100);setError('');setMessage('');setFeedback({});
  setConsumables(null);setProtectedItems(drafts.preserve_consumables??[]);
  setOptions(emptyOptions);setOptionValues({...defaultOptions,...Object.fromEntries(Object.keys(defaultOptions).map(id=>[id,drafts[id as RuntimeOptionId]??defaultOptions[id as RuntimeOptionId]]))});
  setOptionValidity(defaultValidity);setJumpNumericValid(true);setSpeedNumericValid(true);
  setReadFailures(new Set());
  setWorldOpen(false);
  setPhotoOpen(false);
  setTeleportOpen(false);
  void read().then(next=>{if(mounted.current&&revision.current===generation)accept(next)}).catch(e=>{if(mounted.current&&revision.current===generation)setError(asError(e).message)});
  return()=>{mounted.current=false;revision.current++};
 },[session]);
 useEffect(()=>{
  let cancelled=false,previous:boolean|null=null,timer:ReturnType<typeof setTimeout>|undefined;
  setGameRunning(null);setGameError('');
  async function poll(){
   try{
    if(lock.current)return;
    let running:boolean;
    try{running=await api.marketplaceGameRunning(session)}
    catch(e){if(!cancelled){setGameRunning(null);setGameError(asError(e).message)}return}
    if(cancelled)return;
    setGameRunning(running);setGameError('');
    if(previous!==running){
     previous=running;const generation=revision.current;
     try{const native=await api.marketplaceModStatus(session);if(!cancelled&&!lock.current&&generation===revision.current){setSnapshot(native);onBrowseSnapshot?.(native)}}
     catch(e){if(!cancelled&&!lock.current&&generation===revision.current){setSnapshot(null);onBrowseSnapshot?.(null);setError(`Runtime status: ${asError(e).message}`)}}
    }
   }finally{if(!cancelled)timer=setTimeout(()=>void poll(),2000)}
  }
  void poll();
  return()=>{cancelled=true;if(timer)clearTimeout(timer)};
 },[session]);
 async function work(action:()=>Promise<Update>,done:string|((s:Update)=>string)='',mod:NativeModId|null=null,label='Saving settings…') {
  if(lock.current)return;lock.current=true;const generation=++revision.current;setBusy(true);setBusyMod(mod);setError('');setMessage('');
  if(mod)setFeedback(previous=>({...previous,[mod]:{kind:'busy',text:label}}));
  try {const next=await action();if(mounted.current&&revision.current===generation){accept(next);const text=typeof done==='string'?done:done(next);if(mod)setFeedback(previous=>({...previous,[mod]:{kind:'success',text}}));else setMessage(text)}}
  catch(e){if(mounted.current&&revision.current===generation){const text=asError(e).message;if(mod)setFeedback(previous=>({...previous,[mod]:{kind:'error',text}}));else setError(text)}}
  finally{lock.current=false;if(mounted.current){setBusy(false);setBusyMod(null)}}
 }
 async function change(id:NativeModId,enabled:boolean){
  await work(async()=>({native:await api.marketplaceModChange(session,id,enabled)}),next=>next.native?.mods.find(s=>s.id===id)?.state==='installed'?'Mod installed. Start the game to load it.':enabled?'Recovery completed. Check the mod status before installing again.':'Mod removed. It will stay disabled at the next game start.',id,enabled?'Installing mod…':'Removing mod…');
 }
 function editJump(value:number){setPercent(value);updateStartupDraft('player_jump',value,jump?.height_percent)}
 function editSpeed(value:number){setSpeedPercent(value);updateStartupDraft('player_speed',value,speed?.speed_percent)}
 function editConsumables(value:number[]){setProtectedItems(value);updateStartupDraft('preserve_consumables',value,consumables?.item_keys)}
 function editOption(id:RuntimeOptionId,value:number){setOptionValues(previous=>({...previous,[id]:value}));updateStartupDraft(id,value,options[id]?.value)}
 const dirty=jump!==null&&percent!==jump.height_percent;
 const settingsLocked=busy||!supported||!jump||gameRunning!==false;
 const speedDirty=speed!==null&&speedPercent!==speed.speed_percent;
 const speedLocked=busy||!supported||!speed||gameRunning!==false;
 const consumablesDirty=!!consumables&&JSON.stringify(protectedItems)!==JSON.stringify(consumables.item_keys);
 const consumablesLocked=busy||!supported||!consumables||gameRunning!==false;
 const optionDirty=(id:RuntimeOptionId)=>!!options[id]&&(options[id]!.invalid||options[id]!.value!==optionValues[id]);
 return <>
  {mods.map(mod=>{const status=snapshot?.mods.find(s=>s.id===mod.id),Icon=mod.icon,installed=status?.state==='installed'||status?.state==='update_available',optionId=optionForMod[mod.id];
   const unsaved=optionId?!!options[optionId]&&options[optionId]!.value!==optionValues[optionId]:mod.id==='player_jump'?dirty:mod.id==='player_speed'?speedDirty:mod.id==='preserve_consumables'?consumablesDirty:false;
   const currentFeedback=feedback[mod.id],working=busy&&busyMod===mod.id;
   const statusLabel=!status?error?'Status unavailable':'Checking status':status.state==='installed'?gameRunning===false?'Installed · start game':gameRunning===true?'Installed · load unverified':'Installed · game status unknown':({not_installed:'Not installed',update_available:'Update available',unmanaged:'Unmanaged installation',conflict:'Installation conflict',pending:'Recovery needed'} as const)[status.state];
   return <article hidden={!matchesMarketplaceMod(query,mod.id,mod.title,mod.tag,mod.description,mod.detail)||!matchesMarketplaceBrowse(browseFilter,{...mod,live:liveMod(mod.id)},snapshot)} aria-busy={working||undefined} className={`market-card runtime-card ${installed?'selected':''}`} key={mod.id}>
    <div className="market-card-top"><div className={`market-icon ${mod.id}`}><Icon size={28}/></div></div>
    <small>{mod.tag} · {mod.id==='region_return_30min'?'GAME MOD':'RUNTIME MOD'}</small><h2>{mod.title}</h2><ModAvailability mode={mod.id==='region_return_30min'?'closed':liveMod(mod.id)?'live':'runtime'}/><p>{mod.description}</p><div className="market-detail">{mod.detail}</div>
    <div className="runtime-badges"><span className={`runtime-state ${status?.state??'unknown'}`}>{statusLabel}</span>{unsaved&&<span className="runtime-state unsaved">Unsaved settings</span>}{readFailures.has(mod.id)&&<span className="runtime-state conflict">Settings unavailable · draft retained</span>}{optionId&&options[optionId]?.invalid&&<span className="runtime-state conflict">Invalid saved setting</span>}</div>
    {mod.id==='world_controls'&&<button className="secondary" disabled={busy||!supported||status?.state!=='installed'||gameRunning!==true} onClick={()=>setWorldOpen(true)}><CloudSun size={16}/> Open live controls</button>}
    {mod.id==='photo_mode'&&<button className="secondary" disabled={busy||!supported||status?.state!=='installed'||gameRunning!==true} onClick={()=>setPhotoOpen(true)}><Camera size={16}/> Open photo controls</button>}
    {mod.id==='teleport'&&<button className="secondary" disabled={busy||!supported||status?.state!=='installed'||gameRunning!==true} onClick={()=>setTeleportOpen(true)}><MapPin size={16}/> Open teleport controls</button>}
    {optionId&&<MarketplaceStartupControls id={optionId} settings={options[optionId]} unavailable={readFailures.has(mod.id)} value={optionValues[optionId]} disabled={busy||!supported||gameRunning!==false||!options[optionId]} onValidityChange={okay=>setOptionValidity(previous=>({...previous,[optionId]:okay}))} change={value=>editOption(optionId,value)} discard={()=>{discardStartupDraft(optionId);setOptionValues(previous=>({...previous,[optionId]:options[optionId]!.value}))}} save={()=>void work(async()=>({options:[await api.marketplaceRuntimeSetting(session,optionId,optionValues[optionId])]}),'Startup settings saved. Install or update this mod, then start the game to use them.',mod.id)}/>}
    {mod.id==='preserve_consumables'&&<MarketplaceConsumableSelection settings={consumables} unavailable={readFailures.has(mod.id)} draft={protectedItems} disabled={consumablesLocked} change={editConsumables} discard={()=>{discardStartupDraft('preserve_consumables');setProtectedItems(consumables!.item_keys)}} save={()=>void work(async()=>({consumables:await api.marketplaceConsumables(session,protectedItems)}),'Protected items saved. Install the consumable mod if needed, then start the game to use this selection.',mod.id)}/>}
    {mod.id==='player_speed'&&<div className="market-jump-settings">
     <label htmlFor="player-movement-speed">Movement speed <output htmlFor="player-movement-speed">{speedPercent}%{speedPercent===100?' · Normal':` · +${speedPercent-100}%`}</output></label>
     <input id="player-movement-speed" type="range" min={100} max={300} step={1} value={speedPercent} disabled={speedLocked} onChange={e=>editSpeed(Number(e.target.value))}/>
     <RuntimeNumericInput label="Movement speed exact value" value={speedPercent} min={100} max={300} unit="%" disabled={speedLocked} change={editSpeed} onValidity={setSpeedNumericValid}/>
     <div className="runtime-presets" role="group" aria-label="Movement speed presets">{[100,150,200,300].map(value=><button type="button" className="secondary" key={value} disabled={speedLocked} aria-pressed={speedPercent===value} onClick={()=>editSpeed(value)}>{value}%</button>)}</div>
     <div className="mod-actions">
      <button className="secondary" disabled={speedLocked||!speedDirty||!speedNumericValid} onClick={()=>void work(async()=>({speed:await api.marketplacePlayerSpeed(session,speedPercent)}),'Movement speed saved. Install the player speed mod if needed, then start the game to use this setting.',mod.id)}>Save movement speed</button>
      <button className="secondary" disabled={speedLocked||speedPercent===100} onClick={()=>editSpeed(100)}>Reset to normal</button>
      {speedDirty&&<button className="secondary draft-discard" disabled={speedLocked} onClick={()=>{discardStartupDraft('player_speed');setSpeedPercent(speed!.speed_percent)}}>Discard changes</button>}
     </div>
     <span className="market-hint">{speed?.message??(readFailures.has(mod.id)?'Movement settings unavailable. Refresh runtime status to retry.':'Loading movement settings…')}{speedDirty?' Unsaved setting. Save before installing or starting the game.':''}</span>
    </div>}
    {mod.id==='player_jump'&&<div className="market-jump-settings">
     <label htmlFor="player-jump-height">Jump height <output htmlFor="player-jump-height">{percent}%{percent===100?' · Normal':` · +${percent-100}%`}</output></label>
     <input id="player-jump-height" type="range" min={100} max={300} step={1} value={percent} disabled={settingsLocked} onChange={e=>editJump(Number(e.target.value))}/>
     <RuntimeNumericInput label="Jump height exact value" value={percent} min={100} max={300} unit="%" disabled={settingsLocked} change={editJump} onValidity={setJumpNumericValid}/>
     <div className="runtime-presets" role="group" aria-label="Jump height presets">{[100,140,200,300].map(value=><button type="button" className="secondary" key={value} disabled={settingsLocked} aria-pressed={percent===value} onClick={()=>editJump(value)}>{value}%</button>)}</div>
     <div className="mod-actions">
      <button className="secondary" disabled={settingsLocked||!dirty||!jumpNumericValid} onClick={()=>void work(async()=>({jump:await api.marketplacePlayerJump(session,percent)}),'Jump height saved. Install the player jump mod if needed, then start the game to use this setting.',mod.id)}>Save jump height</button>
      <button className="secondary" disabled={settingsLocked||percent===100} onClick={()=>editJump(100)}>Reset to normal</button>
      {dirty&&<button className="secondary draft-discard" disabled={settingsLocked} onClick={()=>{discardStartupDraft('player_jump');setPercent(jump!.height_percent)}}>Discard changes</button>}
     </div>
     <span className="market-hint">{jump?.message??(readFailures.has(mod.id)?'Jump settings unavailable. Refresh runtime status to retry.':'Loading jump settings…')}{dirty?' Unsaved setting. Save before installing or starting the game.':''}</span>
    </div>}
    <div className="mod-actions">
     {status?.state!=='installed'&&<button className="primary" disabled={busy||!supported||gameRunning!==false||!status?.can_install||(optionId&&(!options[optionId]||!optionValidity[optionId]||optionDirty(optionId)||(optionId==='player_resources'&&optionValues[optionId]===0)))||(mod.id==='player_jump'&&(!jump||dirty||!jumpNumericValid))||(mod.id==='player_speed'&&(!speed||speedDirty||!speedNumericValid))||(mod.id==='preserve_consumables'&&(!consumables||consumablesDirty||!protectedItems.length))} onClick={()=>void change(mod.id,true)}>{working?<LoaderCircle size={16} className="spin"/>:<Download size={16}/>} {status?.state==='unmanaged'?'Import & update mod':status?.state==='update_available'?'Update mod':status?.state==='pending'?'Recover operation':'Install mod'}</button>}
     {(installed||status?.can_remove)&&<button className="secondary" disabled={busy||gameRunning!==false||!status?.can_remove} onClick={()=>void change(mod.id,false)}>{working?<LoaderCircle size={16} className="spin"/>:<Trash2 size={16}/>} Remove mod</button>}
    </div>
    {currentFeedback?.text&&<p role={currentFeedback.kind==='error'?'alert':'status'} className={`runtime-feedback ${currentFeedback.kind}`}>{currentFeedback.kind==='busy'&&<LoaderCircle size={14} className="spin" aria-hidden="true"/>} {currentFeedback.text}</p>}
    <span className="market-hint">{status?.message??(error?'Runtime status unavailable. Refresh runtime status to retry.':'Checking installed modules…')}</span>
   </article>
  })}
  <div className="market-runtime-status">
   <p>Close the game to install or remove runtime mods and save startup settings. Cards labelled “Adjust while playing” provide live controls after setup. Windows may ask for administrator permission. Runtime cards install separately from the combined archive plan.</p>
   <p>“Installed” confirms the files on disk, not that a running game has loaded them. Live controls check their own connection. Unsaved startup choices stay on this device until you save or discard them.</p>
   <button className="secondary" disabled={busy} onClick={()=>void work(read)}><RefreshCw size={14}/> Refresh runtime status</button>
   {(gameRunning===true)&&<p role="status">Crimson Desert is running. Runtime mods keep working; installation, removal and startup settings are locked. Open installed live controls from their card.</p>}
   {gameError&&<p role="alert" className="craft-error">Game status unavailable: {gameError}</p>}
   {message&&<p role="status">{message}</p>}{error&&<p role="alert" className="craft-error">{error}</p>}
  </div>
  {worldOpen&&<Dialog title="Live time & weather" eyebrow="LIVE WORLD CONTROLS" onClose={()=>setWorldOpen(false)}>
   <p>Changes apply to the current world while the game is running. Closing this panel keeps your settings active. Reset weather restores the previous weather control; Resume time releases a frozen clock.</p>
   <MarketplaceWorldControls session={session} enabled={supported&&gameRunning===true&&!!snapshot?.mods.some(mod=>mod.id==='world_controls'&&mod.state==='installed')}/>
  </Dialog>}
  {photoOpen&&<Dialog title="Extended photo mode" eyebrow="LIVE PHOTO CONTROLS" onClose={()=>setPhotoOpen(false)}>
   <MarketplacePhotoControls session={session} enabled={supported&&gameRunning===true&&!!snapshot?.mods.some(mod=>mod.id==='photo_mode'&&mod.state==='installed')}/>
  </Dialog>}
  {teleportOpen&&<Dialog title="Teleport" eyebrow="LIVE TELEPORT CONTROLS" onClose={()=>setTeleportOpen(false)}>
   <MarketplaceTeleportControls session={session} enabled={supported&&gameRunning===true&&!!snapshot?.mods.some(mod=>mod.id==='teleport'&&mod.state==='installed')}/>
  </Dialog>}
 </>;
}
