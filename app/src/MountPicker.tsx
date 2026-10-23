import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { Search, RefreshCw, PawPrint, LoaderCircle } from 'lucide-react';
import { api, asError } from './api';
import { plainText } from './format';
import { filterMounts, type MountSnapshot, type MountRequest } from './mount-types';
import type { Catalog } from './types';
import {MountImage} from './MountImage';
import {SaveSelector,SaveTarget} from './SaveSelector';

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
  const readOnly=data?.registration_available===false;
  const availableCount=data?.mounts.filter(m=>m.supported).length??0;
  const ownedCount=data?.mounts.filter(m=>m.owned>0).length??0;
  const familyCount=(f:string)=>data?.mounts.filter(m=>m.family===f&&m.supported).length??0;
  const mount=data?.mounts.find(m=>m.key===selected);
  async function add(checkPending=false) {
    if(latch.current||!data||readOnly||data.game_running||(!checkPending&&(!mount?.supported||!data.selected_save)))return;
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
    <div className="page-heading"><div><div className="eyebrow">CHOOSE MOUNTS</div><h1>Mounts<span>.</span></h1><p>{readOnly?'Browse mounts and inspect ownership in your saved stable.':'Search animals and register them permanently in your stable.'}</p></div><PawPrint size={28}/></div>
    <div className="mount-save-bar"><SaveSelector saves={data?.saves??[]} value={data?.selected_save??null} disabled={busy||giving} ariaLabel="Save for mounts" onChange={value=>void refresh(value)}/><button className="secondary" disabled={busy||giving} onClick={()=>void refresh(data?.selected_save??null)}><RefreshCw size={15}/>Reload</button></div>
    <div className="mount-status" role="status"><p>{data?.message??'Reading mounts and saves …'}</p>{data&&!busy&&<><small>{availableCount} available to add · {ownedCount} species already registered · {data.mounts.length-availableCount-ownedCount} currently unsupported</small>{!readOnly&&data.mounts.some(m=>m.registration==='base')&&<p>Base entries allow additional species without a matching stable template. Summoning and riding these variants still require in-game testing.</p>}{!readOnly&&availableCount===0&&<p>This save contains no suitable stable template. Check the slot and account above.{data.saves[0]&&data.selected_save!==data.saves[0].id&&<button className="secondary" disabled={giving} onClick={()=>void refresh(data.saves[0].id)}>Select newest save</button>}</p>}</>}</div>
    {pending&&<div className="mount-pending"><p>A request is still pending. Check the result using that same request before adding another animal.</p><button className="secondary" disabled={busy||giving||!data||readOnly||data.game_running} onClick={()=>void add(true)}>Check / continue request</button><button className="secondary" disabled={giving} onClick={()=>{localStorage.removeItem(storage);setPending(null);setError('');}}>Discard request</button><small>If the result is unclear, reload and check the stable first.</small></div>}
    {error&&<p className="error-message" role="alert">{error}</p>}
    {receipt&&<div className="mount-success" role="status"><p>{receipt}</p><small>Backup: {backup}</small></div>}
    <div className="picker-layout"><div className="picker-main">
      <label className="input-wrap main-search"><Search size={18}/><input aria-label="Search mounts" maxLength={512} placeholder={regex?'Pattern, e.g. bear|lion':'Name, species or ID …'} value={text} onChange={e=>setText(e.target.value)}/></label>
      <label className="checkbox-label search-mode"><input type="checkbox" checked={regex} onChange={e=>setRegex(e.target.checked)}/>Regex search</label>
      {searchError&&<p className="error-message" role="alert">{searchError}</p>}
      <div className="picker-categories"><button aria-pressed={!family} onClick={()=>setFamily('')}>All species</button>{families.map(f=><button key={f} aria-label={f} aria-pressed={family===f} onClick={()=>setFamily(f)}>{f} <small>({familyCount(f)})</small></button>)}</div>
      <label className="checkbox-label"><input type="checkbox" checked={available} onChange={e=>setAvailable(e.target.checked)}/>Only mounts available to add ({availableCount})</label>
      <p className="mount-count">{searchBusy?'Checking search pattern …':`${rows.length} Mounts`}</p>
      <div className="mount-list" aria-label="Mount list">{busy?<div className="picker-empty"><LoaderCircle className="spin"/>Reading …</div>:!rows.length?<p className="picker-empty">{available?'No mounts available to add with this filter. Choose another species or save.':'No mounts match this search.'}</p>:rows.map(m=><button key={m.key} className={`mount-row ${selected===m.key?'selected':''}`} onClick={()=>setSelected(m.key)}><MountImage session={catalog.session} mountKey={m.key} label={plainText(m.name)}/><span><strong>{plainText(m.name)}</strong><small>{m.family} · ID {m.key}</small><small>{m.owned?`${m.owned}× registered`:readOnly?'Registration unavailable':m.registration==='base'?'Base entry · Game test pending':m.supported?'Can be added':`No template for ${m.family}`}</small></span></button>)}</div>
    </div><aside className="picker-detail">{mount?<><MountImage session={catalog.session} mountKey={mount.key} label={plainText(mount.name)} large/><div className="eyebrow">ID {mount.key}</div><h2>{plainText(mount.name)}</h2><p>{mount.family}</p><h3>Description</h3><p>{mount.description}</p><p>{mount.reason}</p>{!readOnly&&(mount.registration==='base'?<p>Creates a new base entry for this animal. Equipment, health and levels from another animal are not copied. The entry is prepared for initialization by the game; its in-game behavior is not yet confirmed. A backup is created automatically.</p>:<p>Adds one animal. Values and riding equipment are copied from the existing template; its custom name is not copied.</p>)}{!readOnly&&<p>Save and close the game, reload the save here, then click “Register in stable”. Keep the game closed until registration finishes. Afterwards, load that exact save in game.</p>}<SaveTarget saves={data?.saves??[]} value={data?.selected_save??null}/><button className="primary" disabled={busy||giving||readOnly||!!pending||!data?.selected_save||data?.game_running||!mount.supported} onClick={()=>void add()}>{giving?<LoaderCircle className="spin" size={16}/>:<PawPrint size={16}/>}Register in stable</button></>:<div className="picker-empty"><PawPrint size={36}/><p>Choose a mount on the left.</p></div>}</aside></div>
  </section>;
}
