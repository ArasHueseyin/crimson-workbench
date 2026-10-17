import {useCallback,useEffect,useRef,useState} from 'react';
import {Gem,RefreshCw,Search,LoaderCircle} from 'lucide-react';
import {api,asError} from './api';
import {emptyQuery} from './store';
import {plainText} from './format';
import {ItemImage} from './ItemImage';
import type {Catalog,Item} from './types';
import type {ExtraSnapshot,ExtraCandidates,ExtraCandidate} from './extra-socket-types';
import {stoneBonuses,stoneEffects,stoneDescription,type StoneDetail} from './abyss-stone-effects';
import {SaveSelector,SaveTarget} from './SaveSelector';

function StoneValues({item,detail}:{item:Pick<Item,'description'>;detail:StoneDetail|undefined}){
  if(!detail)return <p className="muted">No values available for this stone.</p>;
  const bonuses=stoneBonuses(detail),effects=stoneEffects(item,detail);
  return <div className="stone-values"><h4>Base bonuses &amp; effects</h4>
    {!!bonuses.length&&<dl>{bonuses.map((b,i)=><div key={i}><dt>{b.label}</dt><dd>{b.value}</dd></div>)}</dl>}
    {!!effects.length&&<ul>{effects.map(e=><li key={e}>{e}</li>)}</ul>}
    {!bonuses.length&&!effects.length&&<p className="muted">No additional bonuses in the item data.</p>}
  </div>;
}

export function ExtraSockets({catalog}:{catalog:Catalog}){
  const [snapshot,setSnapshot]=useState<ExtraSnapshot|null>(null),[error,setError]=useState(''),[success,setSuccess]=useState('');
  const [gear,setGear]=useState(0),[slot,setSlot]=useState(0),[chosen,setChosen]=useState<Item|null>(null);
  const [text,setText]=useState(''),[regex,setRegex]=useState(false),[items,setItems]=useState<Item[]>([]),[total,setTotal]=useState(0),[searchError,setSearchError]=useState('');
  const [busy,setBusy]=useState(false),[loading,setLoading]=useState(false);const pending=useRef(false),generation=useRef(0),alive=useRef(true);
  const [owned,setOwned]=useState<ExtraCandidates|null>(null),[ownedBusy,setOwnedBusy]=useState(false),[ownedError,setOwnedError]=useState(''),[gearText,setGearText]=useState(''),[candidate,setCandidate]=useState<ExtraCandidate|null>(null);const ownedGeneration=useRef(0);
  const [details,setDetails]=useState<Record<number,StoneDetail>>({}),[detailsLoading,setDetailsLoading]=useState(true),[detailsError,setDetailsError]=useState(''),[detailsRetry,setDetailsRetry]=useState(0);
  useEffect(()=>{let current=true;setDetails({});setDetailsLoading(true);setDetailsError('');api.abyssStoneDetails(catalog.session).then(rows=>{if(current)setDetails(Object.fromEntries(rows.map(r=>[r.key,r])));}).catch(e=>{if(current)setDetailsError(asError(e).message);}).finally(()=>{if(current)setDetailsLoading(false);});return()=>{current=false;};},[catalog.session,detailsRetry]);
  const refreshOwned=useCallback(async(save:string|null=null)=>{const id=++ownedGeneration.current;setOwnedBusy(true);setOwnedError('');setCandidate(null);try{const value=await api.extraSocketCandidates(catalog.session,save);if(alive.current&&id===ownedGeneration.current)setOwned(value);}catch(e){if(alive.current&&id===ownedGeneration.current){setOwned(null);setOwnedError(asError(e).message);}}finally{if(alive.current&&id===ownedGeneration.current)setOwnedBusy(false);}},[catalog.session]);
  const refresh=useCallback(async()=>{if(pending.current)return;pending.current=true;setBusy(true);try{const s=await api.extraSockets(catalog.session);if(alive.current){setSnapshot(s);setError('');}}
    catch(e){if(alive.current)setError(asError(e).message);}finally{pending.current=false;if(alive.current)setBusy(false);}},[catalog.session]);
  useEffect(()=>{alive.current=true;void refresh();void refreshOwned();const timer=setInterval(()=>void refresh(),5000);return()=>{alive.current=false;ownedGeneration.current++;clearInterval(timer);};},[refresh,refreshOwned]);
  useEffect(()=>{const id=++generation.current;setLoading(true);setSearchError('');setItems([]);const timer=setTimeout(()=>{api.search(catalog.session,{...emptyQuery,item_type:74,category:2501,text,regex}).then(p=>{if(generation.current===id){setItems(p.items);setTotal(p.total);}}).catch(e=>{if(generation.current===id)setSearchError(asError(e).message);}).finally(()=>{if(generation.current===id)setLoading(false);});},text?240:0);return()=>{clearTimeout(timer);generation.current++;};},[catalog.session,text,regex]);
  const record=snapshot?.records[gear];const active=record?.gems[slot];
  const normalized=(value:string)=>plainText(value).toLocaleLowerCase().normalize('NFKD').replace(/\p{M}|[^\p{L}\p{N}]/gu,'');
  const visible=(owned?.items??[]).filter(i=>normalized(`${i.name} ${i.item_key} ${i.uid}`).includes(normalized(gearText)));
  async function add(){if(!candidate||!candidate.eligible||!snapshot||snapshot.game_running||!owned?.selected_save||!owned.save_revision||pending.current||ownedBusy)return;pending.current=true;setBusy(true);setSuccess('');setError('');
    try{const r=await api.addExtraSockets(catalog.session,{revision:snapshot.revision,save:owned.selected_save,save_revision:owned.save_revision,uid:candidate.uid});if(alive.current){setSnapshot(r.snapshot);setGear(r.snapshot.records.findIndex(x=>x.uid===candidate.uid));setSlot(0);setChosen(null);setSuccess(`${plainText(candidate.name)} now has ten empty extra sockets. Active after the next game launch. Backup: ${r.backup}`);void refreshOwned(owned.selected_save);}}
    catch(e){if(alive.current)setError(asError(e).message);}finally{pending.current=false;if(alive.current)setBusy(false);}}
  async function more(){if(loading||items.length>=total)return;const id=generation.current;setLoading(true);try{const p=await api.search(catalog.session,{...emptyQuery,item_type:74,category:2501,text,regex,offset:items.length});if(generation.current===id)setItems(v=>[...v,...p.items]);}catch(e){if(generation.current===id)setSearchError(asError(e).message);}finally{if(generation.current===id)setLoading(false);}}
  async function setGem(key:number){if(!record||!snapshot||snapshot.game_running||pending.current)return;pending.current=true;setBusy(true);setSuccess('');setError('');
    try{const r=await api.setExtraSocket(catalog.session,{revision:snapshot.revision,uid:record.uid,index:slot,gem_key:key});if(alive.current){setSnapshot(r.snapshot);setSuccess(`Extra sockets ${slot+1} ${key?'filled':'cleared'}. Active after the next game launch. Backup: ${r.backup}`);}}
    catch(e){if(alive.current)setError(asError(e).message);}finally{pending.current=false;if(alive.current)setBusy(false);}}
  return <section className="extra-sockets"><div className="page-heading"><div><div className="eyebrow">YOUR EQUIPMENT</div><h1>Extra sockets<span>.</span></h1><p>Ten additional slots per item. Existing sockets are preserved.</p></div><button className="secondary" disabled={busy} onClick={()=>void refresh()}><RefreshCw size={15} className={busy?'spin':''}/>Reload</button></div>
    <div className="extra-note">Manage additional slots here. The game interface shows at most five sockets. Changes load the next time the game starts.</div>
    {snapshot?.game_running&&<p className="extra-note" role="status">Crimson Desert is running. Save and close the game before changing sockets.</p>}
    {error&&<p role="alert" className="extra-note error-text">{error}</p>}{success&&<p role="status" className="extra-note">{success}</p>}
    {!snapshot&&busy&&<div className="empty-state"><LoaderCircle className="spin"/>Reading sockets …</div>}
    <details className="extra-add" open><summary>Add +10 sockets to another item</summary>
      <div className="extra-add-controls"><SaveSelector saves={owned?.saves??[]} value={owned?.selected_save??null} disabled={ownedBusy||busy} ariaLabel="Save for socket expansion" onChange={value=>void refreshOwned(value)}/><button className="secondary" disabled={ownedBusy||busy} onClick={()=>void refreshOwned(owned?.selected_save??null)}>Reload equipment</button></div>
      <label className="input-wrap"><Search size={17}/><input aria-label="Search owned equipment" placeholder="Name, item ID or instance ID …" value={gearText} maxLength={512} onChange={e=>setGearText(e.target.value)}/></label>
      <p className="muted">Choose a specific weapon, armor piece or accessory from your saved inventory. Equipment without open base sockets may also be supported. Each instance can receive ten additional slots once.</p>
      {ownedError&&<p role="alert">{ownedError}</p>}{ownedBusy?<p role="status"><LoaderCircle className="spin"/>Reading equipment …</p>:<div className="extra-owned" aria-label="Owned equipment">{visible.map(i=><button key={i.uid} className={candidate?.uid===i.uid?'selected':''} aria-pressed={candidate?.uid===i.uid} onClick={()=>setCandidate(i)}><ItemImage session={catalog.session} itemKey={i.item_key}/><span><strong>{plainText(i.name)}</strong><small>{i.location} · Instance {i.uid} · {i.baseline} existing sockets</small><small>{i.reason??`+10 available → ${i.baseline+10} total`}</small></span></button>)}{!visible.length&&<p>No matching equipment in the selected save.</p>}</div>}
      {candidate&&<div className="extra-add-selection"><ItemImage session={catalog.session} itemKey={candidate.item_key} large label={candidate.name}/><div><h3>{plainText(candidate.name)}</h3><p>Instance {candidate.uid} · {candidate.baseline} existing + 10 additional sockets. Existing stones and refinement are preserved.</p>{candidate.reason&&<p>{candidate.reason}</p>}<SaveTarget saves={owned?.saves??[]} value={owned?.selected_save??null}/><button className="primary" disabled={busy||ownedBusy||!candidate.eligible||!snapshot||snapshot.game_running} onClick={()=>void add()}>Add +10 sockets to this item</button></div></div>}
    </details>
    {snapshot&&<><div className="extra-gear" aria-label="Choose equipment">{snapshot.records.map((r,i)=><button key={r.uid} className={gear===i?'selected':''} onClick={()=>{setGear(i);setSlot(0);setChosen(null);setSuccess('');}}><ItemImage session={catalog.session} itemKey={r.item_key}/><span><strong>{plainText(r.name)}</strong><small>{r.baseline+r.additional} total sockets · {r.additional} additional</small></span></button>)}</div>
      {record&&<><h2>{plainText(record.name)}</h2><p className="muted">{record.gems.filter(g=>g.key!==0).length} of 10 extra sockets filled · {record.baseline} existing sockets</p>
      <div className="extra-grid" aria-label="Choose extra socket">{record.gems.map((g,i)=><button key={i} className={slot===i?'selected':''} aria-pressed={slot===i} aria-label={`Extra sockets ${i+1}${g.key?` · Item ${g.key}`:' · empty'}`} onClick={()=>{setSlot(i);setChosen(null);setSuccess('');}}>{g.key?<ItemImage session={catalog.session} itemKey={g.key}/>:<Gem size={28}/>}<span>Extra sockets {i+1}<small>{g.key?plainText(g.name??`Occupied · ID ${g.key}`):'Empty'}</small></span></button>)}</div>
      <div className="extra-editor"><div><h3>Choose a stone for extra socket {slot+1} select</h3><label className="input-wrap"><Search size={17}/><input aria-label="Search socket stones" value={text} placeholder="Name, partial word or ID …" maxLength={512} onChange={e=>setText(e.target.value)}/></label><label className="checkbox-label"><input type="checkbox" checked={regex} onChange={e=>setRegex(e.target.checked)}/>Regex search</label>
        {detailsLoading&&<p className="muted">Loading bonuses and effects …</p>}{detailsError&&<p role="alert">Could not load bonuses: {detailsError} <button className="secondary" onClick={()=>setDetailsRetry(v=>v+1)}>Reload bonuses</button></p>}
        {searchError&&<p role="alert">{searchError}</p>}<div className="extra-gems">{items.map(g=>{const preview=[...stoneBonuses(details[g.key]).map(b=>`${b.label} ${b.value}`),...stoneEffects(g,details[g.key])];return <button key={g.key} className={chosen?.key===g.key?'selected':''} onClick={()=>setChosen(g)}><ItemImage session={catalog.session} itemKey={g.key}/><span><strong>{plainText(g.name)}</strong>{preview.length?<small className="stone-preview">{preview.join(' · ')}</small>:<small>{detailsLoading?'Loading effects …':stoneDescription(g)||`ID ${g.key}`}</small>}</span></button>;})}{!items.length&&!loading&&<p>No matching socket stones.</p>}{loading&&<LoaderCircle className="spin"/>}</div>{items.length<total&&<button className="secondary" disabled={loading} onClick={()=>void more()}>Load more stones ({items.length}/{total})</button>}</div>
        <aside className="extra-detail">{chosen?<><ItemImage session={catalog.session} itemKey={chosen.key} large label={chosen.name}/><h3>{plainText(chosen.name)}</h3>{!detailsLoading&&<StoneValues item={chosen} detail={details[chosen.key]}/>}<p className="stone-description">{stoneDescription(chosen)||'No description available.'}</p><small>ID {chosen.key}</small><button className="primary" disabled={busy||snapshot.game_running} onClick={()=>void setGem(chosen.key)}>Insert this stone</button></>:<>{!!active?.key&&<><h3>Current: {plainText(active.name??`Stone ${active.key}`)}</h3>{!detailsLoading&&<StoneValues item={details[active.key]??{description:''}} detail={details[active.key]}/>}</>}<p>Choose a socket and a stone. Inserting creates the stone directly in the extra socket.</p></>}
          <button className="secondary" disabled={busy||snapshot.game_running||!active?.key} onClick={()=>void setGem(0)}>Clear extra socket</button><small>Clearing removes the stone. The previous configuration is backed up.</small></aside>
      </div></>}
      {snapshot.runtime_log&&<details className="extra-note"><summary>Recent mod messages</summary><pre>{snapshot.runtime_log}</pre></details>}
    </>}
  </section>;
}
