import {useEffect,useState} from 'react';
import {Box,Clock3,LoaderCircle,PawPrint,RefreshCw,Search,ShieldCheck} from 'lucide-react';
import {api,asError} from './api';
import {marketplaceMods} from './Marketplace';
import {runtimeMods} from './MarketplaceRuntime';
import {useCatalog} from './store';
import type {InstalledModSnapshot} from './mod-types';
import type {SpawnSnapshot} from './spawn-types';
import type {Catalog} from './types';
import {Dialog} from './Dialog';
import {MarketplaceWorldControls} from './MarketplaceWorldControls';
import './mac-mods.css';

const unavailable=[...marketplaceMods.map(m=>m.title),...runtimeMods.filter(m=>m.id!=='world_controls').map(m=>m.title),
  'Unlock journal knowledge','Skill unlocks','Item stack limits','Reveal world map',
  'Contribution points','Native weapon effects','Shop stock','Daily shop refresh',
  'Drop quantities','Drop chances','Trust'];
const capabilities=[{name:'Live item spawning',view:'spawn',read:api.spawnStatus},
  {name:'Live mount registration',view:'mounts',read:api.mountLiveStatus}] as const;
type Status={snapshot:SpawnSnapshot|null;error:string};

export function MacMods({catalog}:{catalog:Catalog}) {
  const setView=useCatalog(s=>s.setView);
  const [status,setStatus]=useState<Status[]>([]),[busy,setBusy]=useState(true);
  const [registry,setRegistry]=useState<{data:InstalledModSnapshot|null;error:string}>({data:null,error:''});
  const [reload,setReload]=useState(0),[query,setQuery]=useState(''),[clockOpen,setClockOpen]=useState(false);
  useEffect(()=>{
    let active=true;setBusy(true);setStatus([]);setRegistry({data:null,error:''});
    Promise.all([
      Promise.allSettled(capabilities.map(c=>c.read(catalog.session))),
      api.installedMods(catalog.session).then(data=>({data,error:''})).catch(e=>({data:null,error:asError(e).message})),
    ]).then(([results,installed])=>{
      if(!active)return;
      setStatus(results.map(r=>r.status==='fulfilled'?{snapshot:r.value,error:''}:{snapshot:null,error:asError(r.reason).message}));
      setRegistry(installed);setBusy(false);
    });
    return()=>{active=false};
  },[catalog.session,reload]);
  const shown=unavailable.filter(name=>name.toLocaleLowerCase().includes(query.trim().toLocaleLowerCase()));
  return <div className="mods-page mac-mods">
    <div className="page-heading"><div><div className="eyebrow">NATIVE MACOS</div><h1>Mac mods<span>.</span></h1><p>Your current Workbench module and tools available for this Mac installation.</p></div><button className="secondary" disabled={busy} onClick={()=>setReload(n=>n+1)}>{busy?<LoaderCircle size={16} className="spin"/>:<RefreshCw size={16}/>}Refresh mod status</button></div>
    <section className="mac-mod-section" aria-label="Current mods" aria-busy={busy}>
      <h2>Current mods</h2><p>Items and mounts share the Workbench live module. Its running connection is checked separately from third-party mods recorded by CDMods.</p>
      <ul className="mac-mod-status">{capabilities.map((c,i)=>{
        const value=status[i],snapshot=value?.snapshot;
        const label=busy?'Checking …':value?.error?'Could not check':snapshot?.state==='ready'?'Ready':snapshot&&snapshot.pid>0&&snapshot.state!=='offline'?'Connected · not ready':'Not connected';
        return <li key={c.view}><div><strong>{c.name}</strong><span className={`mac-mod-state ${label==='Ready'?'ready':''}`}>{label}</span></div><p>{value?.error||snapshot?.message||'Checking the native module …'}</p></li>;
      })}</ul>
      <p className="muted">Not connected means the live module was not detected. The game may be closed or running without Workbench. Open a tool for launch instructions.</p>
      <h3>Third-party mods · CDMods</h3>
      {busy?<p role="status">Reading CDMods registry …</p>:registry.error?<p role="alert">{registry.error}</p>:!registry.data?.registry_found?<p>No CDMods registry found for this installation. Mods from other managers are not scanned.</p>:registry.data.mods.length?<ul className="mac-mod-status" aria-label="CDMods registry entries">{registry.data.mods.map(m=><li key={m.id}><div><strong>{m.name}</strong><span className={`mac-mod-state ${m.applied?'ready':''}`}>{m.applied?'Applied · CDMods':m.enabled?'Enabled · not applied':'Disabled'}</span></div><p>{[m.version&&`Version ${m.version}`,m.author].filter(Boolean).join(' · ')}</p>{m.applied&&!m.enabled&&<p>Disabled in the manager; its previous application is still recorded.</p>}</li>)}</ul>:<p>The CDMods registry contains no mod entries.</p>}
      <p>Registry status does not verify the current in-game effect. Manage these mods in CDMods; Workbench only reads their status.</p>
    </section>
    <section className="mac-mod-section" aria-label="Available on macOS"><h2>Available on macOS</h2>
      <ul className="mac-tool-list">
        <li><Box size={22}/><div><h3>Live item spawning</h3><p>Add selected items while playing. Requires the verified game build and a loaded character.</p></div><button className="secondary" onClick={()=>setView('spawn')}>Open Item picker</button></li>
        <li><PawPrint size={22}/><div><h3>Live mount registration</h3><p>Register one previously unowned mount while playing. Save normally afterwards.</p></div><button className="secondary" onClick={()=>setView('mounts')}>Open Mounts</button></li>
        <li><Clock3 size={22}/><div><h3>Live time & weather</h3><p>Set game time and weather while playing. Resume time and Reset weather disable their Workbench overrides.</p></div><button className="secondary" onClick={()=>setClockOpen(true)}>Open live controls</button></li>
        <li><ShieldCheck size={22}/><div><h3>Offline mount registration</h3><p>Register a mount in a selected save with automatic backups. Save and fully close the game first.</p></div><button className="secondary" onClick={()=>setView('mounts')}>Choose saved stable</button></li>
      </ul>
    </section>
    <section className="mac-mod-section" aria-label="Unavailable on macOS"><h2>Workbench features unavailable on macOS</h2><p>These bundled Workbench features have not been ported. Separately installed Mac mods can provide similar effects; see Current mods above.</p>
      <label className="input-wrap"><Search size={18}/><input type="search" aria-label="Search Mac mods" placeholder="Search unavailable mods …" value={query} onChange={e=>setQuery(e.target.value)}/></label>
      <ul className="mac-unavailable">{shown.map(name=><li key={name}><span>{name}</span><small>Workbench Mac port unavailable</small></li>)}</ul>
      {!shown.length&&<p role="status">No unavailable mods match this search.</p>}
    </section>
    {clockOpen&&<Dialog title="Mac live time & weather" eyebrow="LIVE WORLD CONTROLS" onClose={()=>setClockOpen(false)}>
      <p>Launch the game through Workbench’s Item picker and load a save. Resume time disables a Workbench freeze. Closing this panel keeps the clock setting active.</p>
      <p>Weather controls require the game’s automatic weather mode. Teleport is still being ported.</p>
      <MarketplaceWorldControls session={catalog.session} enabled={true} platform="macos"/>
    </Dialog>}
  </div>;
}
