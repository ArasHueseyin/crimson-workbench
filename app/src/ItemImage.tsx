import { useEffect, useState } from 'react';
import { Box } from 'lucide-react';
import { api } from './api';
import type { ItemIcon } from './types';
import { ImagePreview } from './ImagePreview';
const icons = new Map<string, Promise<ItemIcon>>();
export function ItemImage({ session, itemKey, large = false, label }: { session: number; itemKey: number; large?: boolean; label?:string }) {
  const [icon, setIcon] = useState<ItemIcon | null>(null);
  const [zoom,setZoom]=useState(false);
  useEffect(() => {
    let active = true; setIcon(null);setZoom(false);
    const id = `${session}:${itemKey}`;
    if (!icons.has(id)) {
      if (icons.size > 384) icons.clear();
      icons.set(id, api.icon(session, itemKey).catch(() => ({ data_url: null, source: null, reason: 'Icon derzeit nicht verfügbar' })));
    }
    icons.get(id)!.then(value => { if (active) setIcon(value); });
    return () => { active = false; };
  }, [session, itemKey]);
  const content=icon?.data_url?<img src={icon.data_url} alt="" draggable={false}/>:<Box size={large?54:24} strokeWidth={1.2}/>;
  return <>{large&&icon?.data_url?<button type="button" className="item-image large zoom-image" aria-label="Itembild vergrößern" title="Bild vergrößern" onClick={()=>setZoom(true)}>{content}</button>:<span className={`item-image ${large?'large':''}`} title={icon?.reason??icon?.source??'Icon wird geladen'}>{content}</span>}
    {zoom&&icon?.data_url&&<ImagePreview src={icon.data_url} title={label??`Item ${itemKey}`} caption="Originales Spielicon, vergrößert dargestellt." onClose={()=>setZoom(false)}/>}
  </>;
}
