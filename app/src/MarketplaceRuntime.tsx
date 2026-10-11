import {useEffect,useRef,useState} from 'react';
import {Bird,Download,LoaderCircle,Moon,RefreshCw,TimerOff,Trash2} from 'lucide-react';
import {api,asError} from './api';
import type {NativeModSnapshot,NativeModId} from './marketplace-types';

const mods=[
 {id:'sleep_cooldown',title:'No sleep cooldown',tag:'SLEEP',icon:TimerOff,description:'Sleep or wait again without a cooldown. Works with beds and campfires while keeping the game’s other validity checks.',detail:'No extra delay · sleep durations unchanged'},
 {id:'sleep_duration',title:'30-hour sleep',tag:'TIME',icon:Moon,description:'Advance the game clock by 30 hours when you choose the 12-hour sleep option. The in-game label still says 12 hours; other sleep durations stay unchanged.',detail:'Choose 12 hours in the game → sleep 30 hours'},
 {id:'blackstar',title:'Blackstar anywhere',tag:'DRAGON',icon:Bird,description:'Use Blackstar with regional, city and altitude restrictions lifted, boosted stamina regeneration, a 1-second summon cooldown and a 1,000-minute riding duration.',detail:'Quest, indoor and final placement checks can still block summoning'},
] as const;

export function MarketplaceRuntime({session,supported}:{session:number;supported:boolean}) {
 const [snapshot,setSnapshot]=useState<NativeModSnapshot|null>(null),[busy,setBusy]=useState(false),[error,setError]=useState(''),[message,setMessage]=useState('');
 const mounted=useRef(true),lock=useRef(false),revision=useRef(0);
 useEffect(()=>{mounted.current=true;const generation=++revision.current;setSnapshot(null);setError('');setMessage('');
  void api.marketplaceModStatus(session).then(s=>{if(mounted.current&&revision.current===generation)setSnapshot(s)}).catch(e=>{if(mounted.current&&revision.current===generation)setError(asError(e).message)});
  return()=>{mounted.current=false;revision.current++};
 },[session]);
 async function work(action:()=>Promise<NativeModSnapshot>,done:string|((s:NativeModSnapshot)=>string)='') {
  if(lock.current)return;lock.current=true;const generation=++revision.current;setBusy(true);setError('');setMessage('');
  try {const next=await action();if(mounted.current&&revision.current===generation){setSnapshot(next);setMessage(typeof done==='string'?done:done(next))}}
  catch(e){if(mounted.current&&revision.current===generation)setError(asError(e).message)}
  finally{lock.current=false;if(mounted.current)setBusy(false)}
 }
 async function change(id:NativeModId,enabled:boolean){await work(()=>api.marketplaceModChange(session,id,enabled),next=>next.mods.find(s=>s.id===id)?.state==='installed'?'Mod installed. Start the game to load it.':enabled?'Recovery completed. Check the mod status before installing again.':'Mod removed. It will stay disabled at the next game start.')}
 return <>
  {mods.map(mod=>{const status=snapshot?.mods.find(s=>s.id===mod.id),Icon=mod.icon,installed=status?.state==='installed'||status?.state==='update_available';
   return <article className={`market-card runtime-card ${installed?'selected':''}`} key={mod.id}>
    <div className="market-card-top"><div className={`market-icon ${mod.id}`}><Icon size={28}/></div><span className="market-free">FREE</span></div>
    <small>{mod.tag} · RUNTIME MOD</small><h2>{mod.title}</h2><p>{mod.description}</p><div className="market-detail">{mod.detail}</div>
    <div className="mod-actions">
     {status?.state!=='installed'&&<button className="primary" disabled={busy||!supported||!status?.can_install} onClick={()=>void change(mod.id,true)}>{busy?<LoaderCircle size={16} className="spin"/>:<Download size={16}/>} {status?.state==='unmanaged'?'Import & update mod':status?.state==='update_available'?'Update mod':status?.state==='pending'?'Recover operation':'Install mod'}</button>}
     {installed&&<button className="secondary" disabled={busy||!status?.can_remove} onClick={()=>void change(mod.id,false)}><Trash2 size={16}/> Remove mod</button>}
    </div>
    <span className="market-hint">{status?.message??'Checking installed modules…'}</span>
   </article>
  })}
  <div className="market-runtime-status">
   <p>Close the game to install or remove runtime mods. Windows may ask for administrator permission. These three cards are independent of the combined archive plan.</p>
   <button className="secondary" disabled={busy} onClick={()=>void work(()=>api.marketplaceModStatus(session))}><RefreshCw size={14}/> Refresh runtime status</button>
   {snapshot?.game_running&&<p role="status">Crimson Desert is running. Close it, then refresh to enable mod changes.</p>}
   {message&&<p role="status">{message}</p>}{error&&<p role="alert" className="craft-error">{error}</p>}
  </div>
 </>;
}
