import {Dialog} from './Dialog';
import {SaveSelector,SaveTarget} from './SaveSelector';
import {useEffect,useRef,useState} from 'react';
import {Sparkles} from 'lucide-react';
import {api,asError} from './api';
import type {Catalog} from './types';
import type {AbilityChoice,SkillPreview,SkillReceipt,SkillRequest} from './marketplace-types';

const owners=['Kliff','Oongka','Damiane'];

export function MarketplaceSkills({catalog,close}:{catalog:Catalog;close:()=>void}) {
 const [preview,setPreview]=useState<SkillPreview|null>(null);
 const [choices,setChoices]=useState<AbilityChoice[]>([]),[slots,setSlots]=useState<SkillPreview['saves']>([]);
 const [save,setSave]=useState(''),[selection,setSelection]=useState<number[]|null>(null);
 const [query,setQuery]=useState(''),[owner,setOwner]=useState('all');
 const [checked,setChecked]=useState(false),[busy,setBusy]=useState(false),[error,setError]=useState('');
 const [receipt,setReceipt]=useState<SkillReceipt|null>(null);
 const storage=`crimson-skill-unlock-pending:${catalog.game_path}`;
 const [pending,setPending]=useState<SkillRequest|null>(()=>{try{return JSON.parse(localStorage.getItem(storage)??'null') as SkillRequest|null}catch{return null}});
 const latest=useRef(0),lock=useRef(false),mounted=useRef(true);
 useEffect(()=>{mounted.current=true;return()=>{mounted.current=false;latest.current++}},[]);

 async function load(id:string|null,selected:number[]|null) {
  const generation=++latest.current;
  setChecked(false);setError('');setPreview(null);
  if(selected?.length===0){setBusy(false);return}
  setBusy(true);
  try {
   const next=await api.skillUnlockPreview(catalog.session,id,selected);
   if(mounted.current&&generation===latest.current){setPreview(next);setChoices(next.abilities??[]);setSlots(next.saves)}
  }catch(e){if(mounted.current&&generation===latest.current)setError(asError(e).message)}
  finally{if(mounted.current&&generation===latest.current)setBusy(false)}
 }
 useEffect(()=>{void load(null,null)},[catalog.session]);
 function select(next:number[]|null){setSelection(next);setReceipt(null);void load(save||null,next)}
 function toggle(key:number,enabled:boolean){
  const keys=new Set(selection??choices.map(a=>a.key));
  if(enabled)keys.add(key);else keys.delete(key);
  select([...keys].sort((a,b)=>a-b));
 }
 async function apply(request:SkillRequest) {
  if(lock.current)return;lock.current=true;setBusy(true);setError('');
  try {
   const result=await api.skillUnlockApply(catalog.session,request);
   localStorage.removeItem(storage);
   if(mounted.current){setPending(null);setReceipt(result);setSave(request.save);setSelection(request.abilities??null);await load(request.save,request.abilities??null)}
  }catch(e){if(mounted.current)setError(asError(e).message)}
  finally{lock.current=false;if(mounted.current)setBusy(false)}
 }
 function unlock() {
  if(!preview?.selected_save||!preview.save_sha256||!preview.lobby_sha256||!preview.plan_id||!checked)return;
  const request:SkillRequest={save_root:preview.save_root,id:crypto.randomUUID().replaceAll('-',''),save:preview.selected_save,save_sha256:preview.save_sha256,lobby_sha256:preview.lobby_sha256,plan_id:preview.plan_id,abilities:selection};
  try{localStorage.setItem(storage,JSON.stringify(request));setPending(request);void apply(request)}catch(e){setError(asError(e).message)}
 }
 const summary=preview?.summary;
 const selectedCount=selection?.length??choices.length;
 const all=selection===null||selection.length===choices.length;
 const filtered=choices.filter(a=>(owner==='all'||a.owner===Number(owner))&&`${a.name} ${a.key}`.toLowerCase().includes(query.toLowerCase()));
 return <Dialog title="Unlock skills" eyebrow="SAVE ACTION" onClose={close} closeDisabled={busy}>
  <p>Select specific abilities or unlock all three character trees. Only selected abilities change. Available skill points, inventory and other save progress are preserved.</p>
  <SaveSelector saves={slots} value={save||null} ariaLabel="Skill unlock save" disabled={busy||!!pending} onChange={value=>{setSave(value);setReceipt(null);void load(value||null,selection)}}/>
  {preview&&<p>Save folder: <code>{preview.save_root}</code></p>}
  {preview&&!slots.length&&<p>No complete saves found. Select your save folder under Data sources.</p>}
  <div className="skill-selection-actions"><button className="secondary" disabled={busy||!!pending||!choices.length} onClick={()=>select(null)}>Select all skills</button><button className="secondary" disabled={busy||!!pending||!choices.length} onClick={()=>select([])}>Clear selection</button><span role="status">{selectedCount} of {choices.length} abilities selected</span></div>
  <div className="skill-selection-filters"><label>Search abilities<input aria-label="Search abilities" placeholder="Name or ability ID" value={query} onChange={e=>setQuery(e.target.value)}/></label><label>Character<select aria-label="Filter abilities by character" value={owner} onChange={e=>setOwner(e.target.value)}><option value="all">All characters</option>{owners.map((name,i)=><option key={name} value={i}>{name}</option>)}</select></label></div>
  <div className="skill-selection-list" aria-label="Abilities">{filtered.map(a=><label key={a.key} className="skill-selection-row"><input type="checkbox" aria-label={`Unlock ${a.name} (${a.key})`} checked={selection===null||selection.includes(a.key)} disabled={busy||!!pending} onChange={e=>toggle(a.key,e.target.checked)}/><span><strong>{a.name}</strong><small>{owners[a.owner]} · ID {a.key} · Maximum level {a.maximum}</small></span></label>)}{choices.length>0&&!filtered.length&&<p>No matching abilities.</p>}</div>
  {selectedCount===0&&<p>Select at least one ability to prepare a preview.</p>}
  {summary&&<div className="skills-summary"><div><strong>{summary.new_abilities}</strong><span>New abilities</span></div><div><strong>{summary.upgraded_abilities}</strong><span>Upgraded abilities</span></div><div><strong>{summary.already_maxed}</strong><span>Already at maximum</span></div></div>}
  {summary&&<p>Available points kept: Kliff {summary.available_points[0]}, Oongka {summary.available_points[1]}, Damiane {summary.available_points[2]}. Missing upgrade costs are recorded as gifted points so refunds remain consistent.</p>}
  {preview?.game_running&&<p role="alert" className="craft-error">Save and fully close Crimson Desert before applying this action.</p>}
  {busy&&<p role="status">Checking the save and preparing the transaction …</p>}
  {error&&<p role="alert" className="craft-error">{error}</p>}
  {pending&&<div className="recovery-review"><p>A saved unlock request for {pending.save} needs a result check. Retrying keeps its original ability selection and cannot grant points twice.</p><button className="secondary" disabled={busy} onClick={()=>void apply(pending)}>Check / recover pending unlock</button><button className="secondary" disabled={busy} onClick={()=>{localStorage.removeItem(storage);setPending(null);void load(save||null,selection)}}>Reload selection</button></div>}
  {receipt&&<div className="recovery-success" role="status"><p>{receipt.message}</p><strong>Backup folder</strong><code>{receipt.backup}</code></div>}
  <p className="fine-print">This changes save progression. Restoring an old save later also discards progress made since that backup.</p>
  <SaveTarget saves={slots} value={preview?.selected_save??null}/>
  <label className="mod-switch"><input type="checkbox" aria-label="Confirm skill save edit" checked={checked} disabled={busy||!!pending||!summary||preview?.game_running} onChange={e=>setChecked(e.target.checked)}/>I want to unlock the selected abilities in this save and understand that this changes progression.</label>
  <button className="primary" disabled={busy||!!pending||!checked||!summary||summary.already_maxed===summary.total||preview?.game_running} onClick={unlock}><Sparkles size={16}/>{all?'Back up & unlock all skills':'Back up & unlock selected skills'}</button>
 </Dialog>;
}
