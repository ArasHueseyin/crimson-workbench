import {useEffect,useRef} from 'react';
import {Search,X} from 'lucide-react';
import {emptyMarketplaceBrowseFilter,marketplaceBrowseCategories,type MarketplaceBrowseFilter,type MarketplaceBrowseSnapshot,type MarketplaceBrowseStatus} from './marketplace-browse';

const statuses:{id:MarketplaceBrowseStatus;label:string}[]=[
 {id:'all',label:'All mods'}, {id:'installed',label:'Installed'},
 {id:'updates',label:'Updates'}, {id:'live',label:'Live controls'},
];

export function MarketplaceBrowseControls({query,changeQuery,filter,changeFilter,total,shown,snapshot}:{
 query:string;changeQuery:(value:string)=>void;filter:MarketplaceBrowseFilter;changeFilter:(value:MarketplaceBrowseFilter)=>void;
 total:number;shown:number;snapshot:MarketplaceBrowseSnapshot;
}) {
 const search=useRef<HTMLInputElement>(null);
 useEffect(()=>{
  const listener=(event:KeyboardEvent)=>{if((event.ctrlKey||event.metaKey)&&event.key==='k'&&!document.querySelector('dialog[open]')){event.preventDefault();search.current?.focus()}};
  window.addEventListener('keydown',listener);return()=>window.removeEventListener('keydown',listener);
 },[]);
 const installed=snapshot?.mods.filter(mod=>mod.state==='installed'||mod.state==='update_available').length;
 const updates=snapshot?.mods.filter(mod=>mod.state==='update_available').length;
 const filtered=filter.category!=='all'||filter.status!=='all';
 return <div className="market-browse">
  <div className="market-search" role="search" aria-label="Search marketplace mods">
   <label><Search size={18} aria-hidden="true"/><input ref={search} aria-label="Search mods" placeholder="Search mods by name, effect or category" value={query} onChange={event=>changeQuery(event.target.value)}/><kbd>Ctrl K</kbd></label>
   {query&&<button className="secondary" aria-label="Clear mod search" onClick={()=>changeQuery('')}><X size={16}/>Clear</button>}
   <span role="status" aria-live="polite">{query.trim()||filtered?`${shown} of ${total} mods`:`${total} mods available`}</span>
  </div>
  <div className="market-browse-filters">
   <div className="market-status-filters" role="group" aria-label="Mod status filter">{statuses.map(status=><button key={status.id} type="button" aria-pressed={filter.status===status.id} onClick={()=>changeFilter({...filter,status:status.id})}>{status.label}{status.id==='installed'&&installed!==undefined&&<span>{installed}</span>}{status.id==='updates'&&updates!==undefined&&<span>{updates}</span>}</button>)}</div>
   <label className="market-category-filter">Category<select aria-label="Mod category" value={filter.category} onChange={event=>changeFilter({...filter,category:event.target.value})}><option value="all">All categories</option>{marketplaceBrowseCategories.map(([id,label])=><option key={id} value={id}>{label}</option>)}</select></label>
   {filtered&&<button className="market-reset-filters" type="button" onClick={()=>changeFilter({...emptyMarketplaceBrowseFilter})}>Reset filters</button>}
  </div>
  {(filter.status==='installed'||filter.status==='updates')&&<p className="market-browse-note">{snapshot?'Installed and update filters show verified runtime modules. Game mods in your combined plan have a separate apply workflow.':'Runtime installation status is unavailable. Refresh runtime status below to check installed modules.'}</p>}
 </div>;
}
