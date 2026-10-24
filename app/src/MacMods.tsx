import {useEffect,useState} from 'react';
import {Box,LoaderCircle,PawPrint,RefreshCw,Search,ShieldCheck} from 'lucide-react';
import {api,asError} from './api';
import {marketplaceMods} from './Marketplace';
import {runtimeMods} from './MarketplaceRuntime';
import {useCatalog} from './store';
import type {SpawnSnapshot} from './spawn-types';
import type {Catalog} from './types';
import './mac-mods.css';

const unavailable=[...marketplaceMods.map(m=>m.title),...runtimeMods.map(m=>m.title),
  'Unlock journal knowledge','Skill unlocks','Item stack limits','Reveal world map',
  'Contribution points','Native weapon effects','Shop stock','Daily shop refresh',
  'Drop quantities','Drop chances','Trust'];
const capabilities=[{name:'Live item spawning',view:'spawn',read:api.spawnStatus},
  {name:'Live mount registration',view:'mounts',read:api.mountLiveStatus}] as const;
type Status={snapshot:SpawnSnapshot|null;error:string};

export function MacMods({catalog}:{catalog:Catalog}) {
  const setView=useCatalog(s=>s.setView);
  const [status,setStatus]=useState<Status[]>([]),[busy,setBusy]=useState(true);
  const [reload,setReload]=useState(0),[query,setQuery]=useState('');
  useEffect(()=>{
    let active=true;setBusy(true);setStatus([]);
    Promise.allSettled(capabilities.map(c=>c.read(catalog.session))).then(results=>{
      if(!active)return;
      setStatus(results.map(r=>r.status==='fulfilled'?{snapshot:r.value,error:''}:{snapshot:null,error:asError(r.reason).message}));
      setBusy(false);
    });
    return()=>{active=false};
  },[catalog.session,reload]);
  const shown=unavailable.filter(name=>name.toLocaleLowerCase().includes(query.trim().toLocaleLowerCase()));
  return <div className="mods-page mac-mods">
    <div className="page-heading"><div><div className="eyebrow">NATIVE MACOS</div><h1>Mac mods<span>.</span></h1><p>Your current Workbench module and tools available for this Mac installation.</p></div><button className="secondary" disabled={busy} onClick={()=>setReload(n=>n+1)}>{busy?<LoaderCircle size={16} className="spin"/>:<RefreshCw size={16}/>}Refresh mod status</button></div>
    <section className="mac-mod-section" aria-label="Current mods" aria-busy={busy}>
      <h2>Current mods</h2><p>Items and mounts share the Workbench live module, loaded when you launch from either tool. This checks its connection to the running game; it does not scan third-party mods or confirm a permanent installation.</p>
      <ul className="mac-mod-status">{capabilities.map((c,i)=>{
        const value=status[i],snapshot=value?.snapshot;
        const label=busy?'Checking …':value?.error?'Could not check':snapshot?.state==='ready'?'Ready':snapshot&&snapshot.pid>0&&snapshot.state!=='offline'?'Connected · not ready':'Not connected';
        return <li key={c.view}><div><strong>{c.name}</strong><span className={`mac-mod-state ${label==='Ready'?'ready':''}`}>{label}</span></div><p>{value?.error||snapshot?.message||'Checking the native module …'}</p></li>;
      })}</ul>
      <p className="muted">Not connected means the live module was not detected. The game may be closed or running without Workbench. Open a tool for launch instructions.</p>
    </section>
    <section className="mac-mod-section" aria-label="Available on macOS"><h2>Available on macOS</h2>
      <ul className="mac-tool-list">
        <li><Box size={22}/><div><h3>Live item spawning</h3><p>Add selected items while playing. Requires the verified game build and a loaded character.</p></div><button className="secondary" onClick={()=>setView('spawn')}>Open Item picker</button></li>
        <li><PawPrint size={22}/><div><h3>Live mount registration</h3><p>Register one previously unowned mount while playing. Save normally afterwards.</p></div><button className="secondary" onClick={()=>setView('mounts')}>Open Mounts</button></li>
        <li><ShieldCheck size={22}/><div><h3>Offline mount registration</h3><p>Register a mount in a selected save with automatic backups. Save and fully close the game first.</p></div><button className="secondary" onClick={()=>setView('mounts')}>Choose saved stable</button></li>
      </ul>
    </section>
    <section className="mac-mod-section" aria-label="Unavailable on macOS"><h2>Unavailable on macOS</h2><p>These Windows features have not been ported to the native Mac game. Their presence in the catalog does not mean they are installed or compatible.</p>
      <label className="input-wrap"><Search size={18}/><input type="search" aria-label="Search Mac mods" placeholder="Search unavailable mods …" value={query} onChange={e=>setQuery(e.target.value)}/></label>
      <ul className="mac-unavailable">{shown.map(name=><li key={name}><span>{name}</span><small>Mac port unavailable</small></li>)}</ul>
      {!shown.length&&<p role="status">No unavailable mods match this search.</p>}
    </section>
  </div>;
}
