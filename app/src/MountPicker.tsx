import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { Search, RefreshCw, PawPrint, LoaderCircle } from 'lucide-react';
import { api, asError } from './api';
import { plainText } from './format';
import { filterMounts, type MountSnapshot, type MountRequest } from './mount-types';
import type { Catalog } from './types';
import {MountImage} from './MountImage';

export function MountPicker({catalog}:{catalog:Catalog}) {
  const storage=`crimson-mount-request-v1:${catalog.game_path}`;
  const [data,setData]=useState<MountSnapshot|null>(null),[busy,setBusy]=useState(false),[giving,setGiving]=useState(false);
  const [text,setText]=useState(''),[family,setFamily]=useState(''),[available,setAvailable]=useState(false),[selected,setSelected]=useState<number|null>(null);
  const [regex,setRegex]=useState(false),[matchKeys,setMatchKeys]=useState<Set<number>|null>(null),[searchBusy,setSearchBusy]=useState(false),[searchError,setSearchError]=useState('');
  const [error,setError]=useState(''),[receipt,setReceipt]=useState(''),[backup,setBackup]=useState('');
  const [pending,setPending]=useState<MountRequest|null>(()=>{try{return JSON.parse(localStorage.getItem(storage)??'null');}catch{return null;}});
  const latch=useRef(false),generation=useRef(0);
  const refresh=useCallback(async(save:string|null=null)=>{
    const id=++generation.current;setBusy(true);setError('');
    try {const value=await api.mountCatalog(catalog.session,save);if(id===generation.current)setData(value);}
    catch(e){if(id===generation.current)setError(asError(e).message);}
    finally{if(id===generation.current)setBusy(false);}
  },[catalog.session]);
  useEffect(()=>{void refresh();return()=>{generation.current++;};},[refresh]);
  useEffect(()=>{
    let active=true;setMatchKeys(null);setSearchError('');
    if(!regex||!text.trim()){setSearchBusy(false);return;}
    setSearchBusy(true);
    const timer=setTimeout(()=>{api.mountSearch(catalog.session,text,true).then(keys=>{if(active)setMatchKeys(new Set(keys));}).catch(e=>{if(active)setSearchError(asError(e).message);}).finally(()=>{if(active)setSearchBusy(false);});},240);
    return()=>{active=false;clearTimeout(timer);};
  },[catalog.session,text,regex]);
  const families=useMemo(()=>Array.from(new Set(data?.mounts.map(m=>m.family)??[])).sort(),[data]);
  const rows=useMemo(()=>{
    const filtered=filterMounts(data?.mounts??[],regex?'':text,family,available);
    return regex&&text.trim()?filtered.filter(m=>matchKeys?.has(m.key)):filtered;
  },[data,text,family,available,regex,matchKeys]);
  const availableCount=data?.mounts.filter(m=>m.supported).length??0;
  const ownedCount=data?.mounts.filter(m=>m.owned>0).length??0;
  const familyCount=(f:string)=>data?.mounts.filter(m=>m.family===f&&m.supported).length??0;
  const mount=data?.mounts.find(m=>m.key===selected);
  async function add(checkPending=false) {
    if(latch.current||!data||data.game_running||(!checkPending&&(!mount?.supported||!data.selected_save)))return;
    const request=checkPending?pending:{id:crypto.randomUUID().replaceAll('-',''),save:data.selected_save!,save_sha256:data.save_sha256,lobby_sha256:data.lobby_sha256,key:mount!.key};
    if(!request)return;latch.current=true;setGiving(true);setError('');setReceipt('');
    try {
      // Store before sending. A lost reply is checked with this SAME request;
      // the backend journal never grants a second animal for the same ID.
      localStorage.setItem(storage,JSON.stringify(request));setPending(request);
      const result=await api.mountRegister(catalog.session,request);setReceipt(result.message);setBackup(result.backup);
      localStorage.removeItem(storage);setPending(null);await refresh(request.save);
    } catch(e){setError(asError(e).message);}
    finally{latch.current=false;setGiving(false);}
  }
  return <section className="item-picker">
    <div className="page-heading"><div><div className="eyebrow">REITTIERE AUSWÄHLEN</div><h1>Reittiere<span>.</span></h1><p>Tiere suchen und dauerhaft im Stall registrieren.</p></div><PawPrint size={28}/></div>
    <div className="mount-save-bar"><label>Spielstand<select aria-label="Spielstand für Reittiere" disabled={busy||giving} value={data?.selected_save??''} onChange={e=>void refresh(e.target.value)}>{data?.saves.map(s=><option key={s.id} value={s.id}>{s.label} · {new Date(s.modified*1000).toLocaleString('de-AT')}{s.id===data.saves[0]?.id?' · zuletzt gespeichert':''}</option>)}</select></label><button className="secondary" disabled={busy||giving} onClick={()=>void refresh(data?.selected_save??null)}><RefreshCw size={15}/>Neu einlesen</button></div>
    <div className="mount-status" role="status"><p>{data?.message??'Reittiere und Spielstände werden eingelesen …'}</p>{data&&!busy&&<><small>{availableCount} hinzufügbar · {ownedCount} Tierarten bereits registriert · {data.mounts.length-availableCount-ownedCount} derzeit nicht unterstützt</small>{data.mounts.some(m=>m.registration==='base')&&<p>Basiseinträge ermöglichen zusätzliche Tierarten ohne passende Stallvorlage. Herbeirufen und Reiten sind für diese Varianten noch im Spiel zu prüfen.</p>}{availableCount===0&&<p>Dieser Spielstand enthält keine geeignete Stallvorlage. Prüfe oben Slot und Konto.{data.saves[0]&&data.selected_save!==data.saves[0].id&&<button className="secondary" disabled={giving} onClick={()=>void refresh(data.saves[0].id)}>Neuesten Spielstand auswählen</button>}</p>}</>}</div>
    {pending&&<div className="mount-pending"><p>Eine Anfrage ist noch offen. Prüfe das Ergebnis mit derselben Anfrage, bevor du ein weiteres Tier hinzufügst.</p><button className="secondary" disabled={busy||giving||!data||data.game_running} onClick={()=>void add(true)}>Anfrage prüfen / fortsetzen</button><button className="secondary" disabled={giving} onClick={()=>{localStorage.removeItem(storage);setPending(null);setError('');}}>Anfrage verwerfen</button><small>Bei unklarem Ergebnis zuerst neu einlesen und den Stall prüfen.</small></div>}
    {error&&<p className="error-message" role="alert">{error}</p>}
    {receipt&&<div className="mount-success" role="status"><p>{receipt}</p><small>Sicherung: {backup}</small></div>}
    <div className="picker-layout"><div className="picker-main">
      <label className="input-wrap main-search"><Search size={18}/><input aria-label="Reittiere durchsuchen" maxLength={512} placeholder={regex?'Suchmuster, z. B. Bär|Löwe':'Name, Tierart oder ID …'} value={text} onChange={e=>setText(e.target.value)}/></label>
      <label className="checkbox-label search-mode"><input type="checkbox" checked={regex} onChange={e=>setRegex(e.target.checked)}/>Regex-Suche</label>
      {searchError&&<p className="error-message" role="alert">{searchError}</p>}
      <div className="picker-categories"><button aria-pressed={!family} onClick={()=>setFamily('')}>Alle Tierarten</button>{families.map(f=><button key={f} aria-label={f} aria-pressed={family===f} onClick={()=>setFamily(f)}>{f} <small>({familyCount(f)})</small></button>)}</div>
      <label className="checkbox-label"><input type="checkbox" checked={available} onChange={e=>setAvailable(e.target.checked)}/>Nur hinzufügbare Reittiere ({availableCount})</label>
      <p className="mount-count">{searchBusy?'Suchmuster wird geprüft …':`${rows.length} Reittiere`}</p>
      <div className="mount-list" aria-label="Reittierliste">{busy?<div className="picker-empty"><LoaderCircle className="spin"/>Wird eingelesen …</div>:!rows.length?<p className="picker-empty">{available?'Keine hinzufügbaren Reittiere für diesen Filter. Andere Tierart oder Spielstand auswählen.':'Keine Reittiere für diesen Suchbegriff.'}</p>:rows.map(m=><button key={m.key} className={`mount-row ${selected===m.key?'selected':''}`} onClick={()=>setSelected(m.key)}><MountImage session={catalog.session} mountKey={m.key} label={plainText(m.name)}/><span><strong>{plainText(m.name)}</strong><small>{m.family} · ID {m.key}</small><small>{m.owned?`${m.owned}× registriert`:m.registration==='base'?'Basiseintrag · Spieltest offen':m.supported?'Kann hinzugefügt werden':`Keine Vorlage für ${m.family}`}</small></span></button>)}</div>
    </div><aside className="picker-detail">{mount?<><MountImage session={catalog.session} mountKey={mount.key} label={plainText(mount.name)} large/><div className="eyebrow">ID {mount.key}</div><h2>{plainText(mount.name)}</h2><p>{mount.family}</p><h3>Beschreibung</h3><p>{mount.description}</p><p>{mount.reason}</p>{mount.registration==='base'?<p>Es wird ein neuer Basiseintrag für dieses Tier erstellt. Ausrüstung, Lebenspunkte und Levelwerte eines anderen Tiers werden nicht kopiert. Der Eintrag ist für die Initialisierung durch das Spiel vorbereitet; seine Funktion im Spiel ist noch unbestätigt. Eine Sicherung wird automatisch angelegt.</p>:<p>Es wird ein Tier hinzugefügt. Die Werte und Reitausrüstung der vorhandenen Vorlage werden übernommen; ihr eigener Name wird nicht kopiert.</p>}<p>Speichere und schließe das Spiel, lies den Spielstand neu ein und klicke auf „Im Stall registrieren“. Lade danach genau diesen Spielstand.</p><button className="primary" disabled={busy||giving||!!pending||data?.game_running||!mount.supported} onClick={()=>void add()}>{giving?<LoaderCircle className="spin" size={16}/>:<PawPrint size={16}/>}Im Stall registrieren</button></>:<div className="picker-empty"><PawPrint size={36}/><p>Wähle links ein Reittier aus.</p></div>}</aside></div>
  </section>;
}
