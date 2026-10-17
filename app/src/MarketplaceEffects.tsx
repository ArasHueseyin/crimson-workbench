import {useEffect,useState} from 'react';
import {Plus,Trash2} from 'lucide-react';
import {api,asError} from './api';
import {useMods} from './store';
import {emptySelection,type WeaponEffectSource} from './marketplace-types';
import type {Catalog} from './types';
import {Dialog} from './Dialog';

export function MarketplaceEffects({catalog,close}:{catalog:Catalog;close:()=>void}) {
 const request=useMods(s=>s.request),set=useMods(s=>s.set);
 const selected={...emptySelection,...request.marketplace};
 const [sources,setSources]=useState<WeaponEffectSource[]>([]),[source,setSource]=useState('');
 const [target,setTarget]=useState(''),[query,setQuery]=useState(''),[replace,setReplace]=useState(false);
 const [busy,setBusy]=useState(true),[error,setError]=useState('');
 useEffect(()=>{
  let current=true;setBusy(true);setError('');setSources([]);setSource('');setTarget('');setReplace(false);
  void api.marketplaceWeaponEffects(catalog.session).then(values=>{
   if(current){setSources(values);setSource(String(values[0]?.source??''))}
  }).catch(e=>{if(current)setError(asError(e).message)}).finally(()=>{if(current)setBusy(false)});
  return()=>{current=false};
 },[catalog.session]);
 const effect=sources.find(s=>String(s.source)===source);
 const weapon=effect?.targets.find(t=>String(t.key)===target);
 const already=weapon?.existing.length===1&&weapon.existing[0].skill===effect?.skill&&weapon.existing[0].level===effect?.level;
 const needsReplacement=!!weapon?.existing.length&&!already;
 const results=effect?.targets.filter(t=>`${t.name} ${t.key}`.toLowerCase().includes(query.trim().toLowerCase()))??[];
 const names=new Map(sources.flatMap(s=>s.targets.map(t=>[t.key,t.name] as const)));
 function remove(key:number) {
  const current={...emptySelection,...useMods.getState().request.marketplace};
  const transfers={...current.weapon_effects};delete transfers[key];
  set({marketplace:{...current,weapon_effects:transfers}});
 }
 function add() {
  if(!effect||!weapon||already||(needsReplacement&&!replace))return;
  const current={...emptySelection,...useMods.getState().request.marketplace};
  set({marketplace:{...current,weapon_effects:{...current.weapon_effects,[weapon.key]:{source:effect.source,replace_existing:needsReplacement&&replace}}}});
 }
 return <Dialog title="Native weapon effects" eyebrow="EQUIPMENT · GAME MOD" onClose={close}>
  <p>Transfer a source weapon’s native attack ability, including its original triggers and effects. This changes all copies of the selected weapon definition. Source weapons, upgrade stats and sockets are preserved. Close the game before applying the combined plan.</p>
  <p>Lightning supports the observed greatsword, spear and greathammer classes. Fire supports the observed spear class. Other weapon classes and effects are not offered yet. Combat and visual testing of transfers is still pending.</p>
  {busy&&<p role="status">Reading native weapon effects…</p>}
  {error&&<p role="alert" className="craft-error">{error}</p>}
  <label className="source-field">Source weapon & effect<select aria-label="Weapon effect source" disabled={busy||!sources.length} value={source} onChange={e=>{setSource(e.target.value);setTarget('');setQuery('');setReplace(false)}}>{sources.map(s=><option key={s.source} value={s.source}>{s.effect_name} · {s.source_name}</option>)}</select></label>
  <label className="market-stack-field">Find a compatible target<input aria-label="Find weapon effect target" placeholder="Weapon name or ID" disabled={busy||!effect} value={query} onChange={e=>{setQuery(e.target.value);setTarget('');setReplace(false)}}/></label>
  {effect&&<div className="market-stack-results" aria-label="Compatible weapon effect targets">{results.map(t=><button className="secondary" key={t.key} aria-pressed={target===String(t.key)} onClick={()=>{setTarget(String(t.key));setReplace(false)}}>{t.name} · {t.key}</button>)}</div>}
  {effect&&!results.length&&<p>No compatible weapons match your search.</p>}
  {weapon&&<div className="recovery-review"><strong>{weapon.name}</strong><p>Existing native abilities: {weapon.existing.length?weapon.existing.map(p=>`skill ${p.skill} (level ${p.level})`).join(', '):'none'}. Upgrade stats and socket data stay unchanged.</p>
   {already&&<p role="status">This weapon already has the selected native effect.</p>}
   {needsReplacement&&<label className="mod-switch"><input type="checkbox" aria-label="Replace existing native weapon abilities" checked={replace} onChange={e=>setReplace(e.target.checked)}/>Replace all existing native weapon abilities listed above with {effect?.effect_name}.</label>}
  </div>}
  <button className="primary" disabled={busy||!weapon||already||(needsReplacement&&!replace)} onClick={add}><Plus size={16}/>Add effect transfer to plan</button>
  <div className="market-stack-overrides">{Object.entries(selected.weapon_effects).map(([key,edit])=><div key={key}><span>{names.get(Number(key))??`Weapon ${key}`} · {sources.find(s=>s.source===edit.source)?.effect_name??`Source ${edit.source}`}</span><button className="secondary" aria-label={`Remove weapon effect transfer ${key}`} onClick={()=>remove(Number(key))}><Trash2 size={14}/></button></div>)}</div>
  <p>To undo one transfer, remove it and apply the reviewed plan again. The weapon then uses its original native abilities. Restore removes the complete Workbench overlay.</p>
 </Dialog>;
}
