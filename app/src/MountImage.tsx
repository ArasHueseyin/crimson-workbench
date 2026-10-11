import {useEffect,useState} from 'react';
import {PawPrint} from 'lucide-react';
import {api} from './api';
import type {ItemIcon} from './types';
import {ImagePreview} from './ImagePreview';
const portraits=new Map<string,Promise<ItemIcon>>();
export function MountImage({session,mountKey,label,large=false}:{session:number;mountKey:number;label:string;large?:boolean}) {
  const [node,setNode]=useState<HTMLElement|null>(null),[icon,setIcon]=useState<ItemIcon|null>(null),[zoom,setZoom]=useState(false);
  useEffect(()=>{
    let active=true;setIcon(null);setZoom(false);
    const load=()=>{
      const key=`${session}:${mountKey}`;
      if(!portraits.has(key)){
        if(portraits.size>=128)portraits.clear();
        portraits.set(key,api.mountIcon(session,mountKey).catch(()=>({data_url:null,source:null,reason:'Animal portrait currently unavailable.'})));
      }
      portraits.get(key)!.then(value=>{if(active)setIcon(value);});
    };
    if(large||!node||typeof IntersectionObserver==='undefined') {if(large||node)load();return()=>{active=false;};}
    const observer=new IntersectionObserver(entries=>{if(entries.some(e=>e.isIntersecting)){observer.disconnect();load();}});
    observer.observe(node);return()=>{active=false;observer.disconnect();};
  },[session,mountKey,large,node]);
  const content=icon?.data_url?<img src={icon.data_url} alt="" draggable={false}/>:<PawPrint size={large?88:40} strokeWidth={1.2}/>;
  const caption=icon?.data_url?(icon.caption??'Original animal portrait from the game archive.'):'Generic animal icon. No unique portrait is assigned to this animal.';
  return <>{large?<div className="mount-hero"><button ref={setNode} className="mount-portrait zoom-image" type="button" aria-label="Enlarge mount image" onClick={()=>setZoom(true)}>{content}</button><small>{icon?.data_url?(icon.caption?'Example image of species':'Animal portrait'):'Generic animal icon'} · Click to enlarge</small>{icon?.caption&&<small>{icon.caption}</small>}</div>:<span ref={setNode} className="mount-icon" title={icon?.caption??icon?.reason??icon?.source??'Loading animal portrait'}>{content}</span>}
    {zoom&&<ImagePreview title={label} src={icon?.data_url??undefined} fallback={<PawPrint size={300} strokeWidth={1}/>} caption={caption} onClose={()=>setZoom(false)}/>}
  </>;
}
