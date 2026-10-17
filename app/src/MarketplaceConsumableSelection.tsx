import {useState} from 'react';
import type {ConsumableSettings} from './marketplace-types';

export function MarketplaceConsumableSelection({settings,draft,disabled,change,save,discard,unavailable=false}:{settings:ConsumableSettings|null;draft:number[];disabled:boolean;change:(keys:number[])=>void;save:()=>void;discard?:()=>void;unavailable?:boolean}) {
 const [search,setSearch]=useState('');
 const dirty=!!settings&&JSON.stringify(draft)!==JSON.stringify(settings.item_keys);
 const choices=settings?.choices??[],query=search.trim().toLowerCase();
 const filtered=choices.filter(c=>`${c.name} ${c.key} ${c.kind}`.toLowerCase().includes(query));
 function toggle(key:number,enabled:boolean){change(enabled?[...draft,key].sort((a,b)=>a-b):draft.filter(k=>k!==key))}
 return <div className="market-consumable-settings">
  <label className="market-stack-field">Protected items<input aria-label="Find consumable to preserve" placeholder="Search food, arrows, potions or an item ID" value={search} disabled={disabled} onChange={e=>setSearch(e.target.value)}/></label>
  <div className="mod-actions"><button className="secondary" disabled={disabled||draft.length===choices.length} onClick={()=>change(choices.map(c=>c.key).sort((a,b)=>a-b))}>Select all supported items</button><button className="secondary" disabled={disabled||!draft.length} onClick={()=>change([])}>Clear protected items</button></div>
  <div className="market-consumable-list">{filtered.map(item=><label className="market-consumable-row" key={item.key}><input type="checkbox" aria-label={`Preserve ${item.name} (${item.key})`} checked={draft.includes(item.key)} disabled={disabled} onChange={e=>toggle(item.key,e.target.checked)}/><span><strong>{item.name}</strong><small>{item.kind} · {item.key}</small></span></label>)}{settings&&!filtered.length&&<p>No supported items match this search.</p>}</div>
  <span className="market-hint">{settings?`${draft.length} of ${choices.length} supported item types selected.`:`${draft.length} item choices retained. The supported item list is unavailable until settings are read.`} You must already own enough for normal eating, healing, potion or arrow use. Feeding, releasing or summoning creatures, and item conversions keep their normal costs.</span>
  <div className="mod-actions"><button className="secondary" disabled={disabled||!dirty} onClick={save}>Save protected items</button>{dirty&&discard&&<button className="secondary draft-discard" disabled={disabled} onClick={discard}>Discard changes</button>}</div>
  <span className="market-hint">{settings?.message??(unavailable?'Consumable settings unavailable. Your selection is retained; refresh runtime status to retry.':'Loading consumable settings…')}{dirty?' Unsaved selection. Kept on this device until saved or discarded.':''}</span>
 </div>;
}
