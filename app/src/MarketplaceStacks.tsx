import {ModAvailability} from './ModAvailability';
import {useEffect,useRef,useState} from 'react';
import {Layers,Plus,Trash2} from 'lucide-react';
import {api,asError} from './api';
import {emptyQuery,useMods} from './store';
import {emptySelection} from './marketplace-types';
import type {Catalog,Item} from './types';
import {ItemImage} from './ItemImage';
import {languageNames} from './format';

export function MarketplaceStacks({catalog,supported,hidden=false}:{catalog:Catalog;supported:boolean;hidden?:boolean}) {
 const request=useMods(s=>s.request),set=useMods(s=>s.set);
 const selected={...emptySelection,...request.marketplace};
 const [query,setQuery]=useState(''),[results,setResults]=useState<Item[]>([]);
 const [item,setItem]=useState<Item|null>(null),[size,setSize]=useState('999');
 const [names,setNames]=useState<Record<number,string>>({}),[error,setError]=useState(''),[busy,setBusy]=useState(false);
 const revision=useRef(0);
 useEffect(()=>{
  const generation=++revision.current;
  setError('');setResults([]);
  if(!query.trim()){setBusy(false);return}
  setBusy(true);
  const timer=setTimeout(()=>{void api.search(catalog.session,{...emptyQuery,text:query}).then(page=>{
   if(revision.current===generation)setResults(page.items);
  }).catch(e=>{if(revision.current===generation)setError(asError(e).message)}).finally(()=>{if(revision.current===generation)setBusy(false)})},180);
  return()=>{clearTimeout(timer);revision.current++};
 },[query,catalog.session]);
 const valid=/^\d+$/.test(size)&&Number.isSafeInteger(Number(size))&&Number(size)>=1&&Number(size)<=1_000_000;
 function update(key:number,value:number|null) {
  const current={...emptySelection,...useMods.getState().request.marketplace};
  const overrides={...current.item_stacks};
  if(value===null)delete overrides[key];else overrides[key]=value;
  set({marketplace:{...current,item_stacks:overrides}});
 }
 const draftSelected=Object.keys(selected.item_stacks).length>0;
 return <article hidden={hidden} className={`market-card stack-card ${draftSelected?'selected':''}`}>
  <div className="market-card-top"><div className="market-icon"><Layers size={28}/></div><span className={`market-draft-badge ${draftSelected?'selected':''}`}>{draftSelected?'Draft selected':'Not selected for draft'}</span></div>
  <small>INVENTORY · GAME MOD</small><h2>Item stack limits</h2><ModAvailability mode="closed"/>
  <p>Choose an item and set its maximum stack size. Existing item quantities stay unchanged. The preview checks item compatibility before creating the plan.</p>
  <label className="market-stack-field">Find an item · {languageNames[catalog.info.index.language]??catalog.info.index.language}<input aria-label="Find item for stack limit" placeholder="Item name or ID" value={query} disabled={!supported} onChange={e=>{setQuery(e.target.value);setItem(null)}}/></label>
  {busy&&<small role="status">Searching items…</small>}
  {results.length>0&&<div className="market-stack-results" aria-label="Stack item search results">{results.map(candidate=><button className="secondary market-stack-item" key={candidate.key} aria-pressed={item?.key===candidate.key} onClick={()=>{setItem(candidate);setNames(n=>({...n,[candidate.key]:candidate.name}))}}><ItemImage session={catalog.session} itemKey={candidate.key}/><span>{candidate.name} · {candidate.key}</span></button>)}</div>}
  {query.trim()&&!busy&&!results.length&&!error&&<small>No matching items.</small>}
  {item&&<p className="market-stack-item market-stack-selected"><ItemImage session={catalog.session} itemKey={item.key}/><span>Selected: <strong>{item.name}</strong> · Current maximum: {item.max_stack}</span></p>}
  <label className="market-stack-field">Maximum stack size<input type="number" aria-label="Maximum stack size" min="1" max="1000000" step="1" value={size} disabled={!supported} onChange={e=>setSize(e.target.value)}/></label>
  {!valid&&<small role="alert">Enter a whole number from 1 to 1,000,000.</small>}
  <button className="secondary" disabled={!supported||!item||!valid} onClick={()=>{if(item&&valid)update(item.key,Number(size))}}><Plus size={16}/>Add stack override</button>
  <div className="market-stack-overrides">{Object.entries(selected.item_stacks).map(([key,value])=><div key={key}><span className="market-stack-item"><ItemImage session={catalog.session} itemKey={Number(key)}/><span>{names[Number(key)]??`Item ${key}`} · Maximum {value.toLocaleString('en-US')}</span></span><button className="secondary" aria-label={`Remove stack override ${key}`} onClick={()=>update(Number(key),null)}><Trash2 size={14}/></button></div>)}</div>
  {error&&<p role="alert" className="craft-error">{error}</p>}
  <span className="market-hint">Changes join your combined plan. Review the plan before applying.</span>
 </article>;
}
