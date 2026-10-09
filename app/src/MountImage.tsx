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
        portraits.set(key,api.mountIcon(session,mountKey).catch(()=>({data_url:null,source:null,reason:'Tierporträt derzeit nicht verfügbar.'})));
      }
      portraits.get(key)!.then(value=>{if(active)setIcon(value);});
    };
    if(large||!node||typeof IntersectionObserver==='undefined') {if(large||node)load();return()=>{active=false;};}
    const observer=new IntersectionObserver(entries=>{if(entries.some(e=>e.isIntersecting)){observer.disconnect();load();}});
    observer.observe(node);return()=>{active=false;observer.disconnect();};
  },[session,mountKey,large,node]);
  const content=icon?.data_url?<img src={icon.data_url} alt="" draggable={false}/>:<PawPrint size={large?88:40} strokeWidth={1.2}/>;
  const caption=icon?.data_url?(icon.caption??'Originales Tierporträt aus dem Spielarchiv.'):'Allgemeines Tiersymbol. Für dieses Tier ist kein eindeutiges Porträt zugeordnet.';
  return <>{large?<div className="mount-hero"><button ref={setNode} className="mount-portrait zoom-image" type="button" aria-label="Reittierbild vergrößern" onClick={()=>setZoom(true)}>{content}</button><small>{icon?.data_url?(icon.caption?'Beispielbild der Tierart':'Tierporträt'):'Allgemeines Tiersymbol'} · Anklicken zum Vergrößern</small>{icon?.caption&&<small>{icon.caption}</small>}</div>:<span ref={setNode} className="mount-icon" title={icon?.caption??icon?.reason??icon?.source??'Tierporträt wird geladen'}>{content}</span>}
    {zoom&&<ImagePreview title={label} src={icon?.data_url??undefined} fallback={<PawPrint size={300} strokeWidth={1}/>} caption={caption} onClose={()=>setZoom(false)}/>}
  </>;
}
