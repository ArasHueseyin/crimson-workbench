import {useEffect,useId,useRef,type ReactNode,type KeyboardEvent} from 'react';
import {createPortal} from 'react-dom';
import './runtime-ui.css';

/** Native top-layer dialog: browser focus containment, Escape and opener return. */
export function Dialog({title,eyebrow,onClose,children,className='',closeDisabled=false}:{title:string;eyebrow?:string;onClose:()=>void;children:ReactNode;className?:string;closeDisabled?:boolean}) {
 const element=useRef<HTMLDialogElement>(null),close=useRef(onClose),blocked=useRef(closeDisabled),titleId=useId();
 close.current=onClose;blocked.current=closeDisabled;
 useEffect(()=>{
  const dialog=element.current,opener=document.activeElement;
  if(!dialog)return;
  dialog.showModal();
  return()=>{
   dialog.close();
   if(opener instanceof HTMLElement&&opener.isConnected)opener.focus({preventScroll:true});
  };
 },[]);
 function containTab(event:KeyboardEvent<HTMLDialogElement>){
  if(event.key!=='Tab')return;
  const controls=Array.from(event.currentTarget.querySelectorAll<HTMLElement>('button,input,select,textarea,a[href],[tabindex]')).filter(node=>node.tabIndex>=0&&!node.matches(':disabled')&&node.getClientRects().length>0);
  const first=controls[0],last=controls[controls.length-1];
  if(!first){event.preventDefault();event.currentTarget.focus();return}
  if(event.shiftKey&&document.activeElement===first){event.preventDefault();last.focus()}
  else if(!event.shiftKey&&document.activeElement===last){event.preventDefault();first.focus()}
 }
 return createPortal(<dialog ref={element} className={`market-modal workbench-dialog ${className}`} aria-labelledby={titleId} aria-modal="true" onKeyDown={containTab} onCancel={event=>{event.preventDefault();if(!blocked.current)close.current()}}>
  <div className="market-modal-heading"><div>{eyebrow&&<small>{eyebrow}</small>}<h2 id={titleId}>{title}</h2></div><button type="button" className="secondary" disabled={closeDisabled} onClick={()=>close.current()}>Close</button></div>
  {children}
 </dialog>,document.body);
}
