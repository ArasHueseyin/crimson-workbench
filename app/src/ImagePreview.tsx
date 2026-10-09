import {useEffect,useRef,type ReactNode} from 'react';
import {createPortal} from 'react-dom';
import {X} from 'lucide-react';
import './picker-enhancements.css';
export function ImagePreview({src,title,caption,fallback,onClose}:{src?:string;title:string;caption?:string;fallback?:ReactNode;onClose:()=>void}) {
  const dialog=useRef<HTMLDialogElement>(null);
  useEffect(()=>{const node=dialog.current;node?.showModal();return()=>{node?.close();};},[]);
  return createPortal(<dialog ref={dialog} className="image-preview" aria-label={`Bildvorschau: ${title}`} onCancel={e=>{e.preventDefault();onClose();}} onClick={e=>{if(e.target===e.currentTarget)onClose();}}>
    <div className="image-preview-content"><header><h2>{title}</h2><button type="button" className="icon-button" aria-label="Bildvorschau schließen" autoFocus onClick={onClose}><X size={24}/></button></header>
      <div className="image-preview-stage">{src?<img src={src} alt={title} draggable={false}/>:fallback}</div>{caption&&<p>{caption}</p>}<small>Esc oder × zum Schließen</small>
    </div>
  </dialog>,document.body);
}
