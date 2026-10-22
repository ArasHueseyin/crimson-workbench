import {useEffect,useRef,useState} from 'react';
import {BookOpen} from 'lucide-react';
import {Dialog} from './Dialog';
import {SaveSelector,SaveTarget} from './SaveSelector';
import {api,asError} from './api';
import type {Catalog} from './types';
import type {KnowledgePreview,KnowledgeReceipt,KnowledgeRequest} from './marketplace-types';

export function MarketplaceKnowledge({catalog,close}:{catalog:Catalog;close:()=>void}) {
 const [snapshot,setSnapshot]=useState<KnowledgePreview|null>(null),[save,setSave]=useState('');
 const [checked,setChecked]=useState(false),[busy,setBusy]=useState(false),[error,setError]=useState('');
 const [receipt,setReceipt]=useState<KnowledgeReceipt|null>(null);
 const storage=`crimson-knowledge-unlock-pending:${catalog.game_path}`;
 const [pending,setPending]=useState<KnowledgeRequest|null>(()=>{try{return JSON.parse(localStorage.getItem(storage)??'null') as KnowledgeRequest|null}catch{return null}});
 const mounted=useRef(true),generation=useRef(0),lock=useRef(false);
 useEffect(()=>{mounted.current=true;return()=>{mounted.current=false;generation.current++}},[]);
 async function load(id:string|null) {
  const current=++generation.current;setBusy(true);setChecked(false);setError('');setSnapshot(s=>s?{...s,plan:null}:s);
  try{const next=await api.knowledgeUnlockPreview(catalog.session,id);if(mounted.current&&current===generation.current)setSnapshot(next)}
  catch(e){if(mounted.current&&current===generation.current)setError(asError(e).message)}
  finally{if(mounted.current&&current===generation.current)setBusy(false)}
 }
 useEffect(()=>{void load(null)},[catalog.session]);
 async function apply(request:KnowledgeRequest) {
  if(lock.current)return;lock.current=true;setBusy(true);setError('');setChecked(false);
  try{const result=await api.knowledgeUnlockApply(catalog.session,request);localStorage.removeItem(storage);if(mounted.current){setPending(null);setReceipt(result);setSave(request.save);await load(request.save)}}
  catch(e){if(mounted.current)setError(asError(e).message)}
  finally{lock.current=false;if(mounted.current)setBusy(false)}
 }
 function confirm() {
  const plan=snapshot?.plan;if(!plan||!snapshot.selected_save||!checked)return;
  const request:KnowledgeRequest={save_root:snapshot.save_root,id:crypto.randomUUID().replaceAll('-',''),save:snapshot.selected_save,save_sha256:plan.save_sha256,lobby_sha256:plan.lobby_sha256,plan_id:plan.plan_id};
  try{localStorage.setItem(storage,JSON.stringify(request));setPending(request);void apply(request)}catch(e){setError(asError(e).message)}
 }
 const summary=snapshot?.plan?.summary,changed=!!summary&&summary.already_complete<summary.total;
 return <Dialog title="Unlock journal knowledge" eyebrow="SAVE ACTION" onClose={close} closeDisabled={busy}>
  <p>Complete the 3,939 supported journal entries, including people, places, creatures, recipes and quest information. Each save is checked separately: only missing entries and levels are added. Existing knowledge and skill points are kept.</p>
  <p>This reveals story and quest information. Hidden internal knowledge and skill unlocks are excluded.</p>
  <SaveSelector saves={snapshot?.saves??[]} value={save||null} ariaLabel="Knowledge unlock save" disabled={busy||!!pending} onChange={value=>{setSave(value);setReceipt(null);void load(value||null)}}/>
  {snapshot&&!snapshot.saves.length&&<p>No complete saves found. Select your save folder under Data sources.</p>}
  {summary&&<div className="skills-summary"><div><strong>{summary.new_entries}</strong><span>New entries</span></div><div><strong>{summary.upgraded_entries}</strong><span>Missing knowledge levels</span></div><div><strong>{summary.already_complete}</strong><span>Already complete</span></div></div>}
  {summary&&!changed&&<p>Journal knowledge is already complete in this save.</p>}
  {snapshot?.game_running&&<p role="alert" className="craft-error">Save and fully close Crimson Desert before applying this action.</p>}
  {busy&&<p role="status">Checking the save and preparing the knowledge preview …</p>}
  {error&&<p role="alert" className="craft-error">{error}</p>}
  {pending&&<div className="recovery-review"><p>A saved knowledge unlock request for {pending.save} needs a result check.</p><button className="secondary" disabled={busy} onClick={()=>void apply(pending)}>Check / recover pending knowledge unlock</button><button className="secondary" disabled={busy} onClick={()=>{localStorage.removeItem(storage);setPending(null);void load(save||null)}}>Discard saved request & reload</button></div>}
  {receipt&&<div className="recovery-success" role="status"><p>{receipt.message}</p><strong>Backup folder</strong><code>{receipt.backup}</code></div>}
  <p className="fine-print">A backup is created before changes. Restoring it later also discards progress made since that backup.</p>
  <SaveTarget saves={snapshot?.saves??[]} value={snapshot?.selected_save??null}/>
  <label className="mod-switch"><input type="checkbox" aria-label="Confirm knowledge save edit" checked={checked} disabled={busy||!!pending||!changed||snapshot?.game_running} onChange={e=>setChecked(e.target.checked)}/>I want to unlock journal knowledge in this save, including story and quest information.</label>
  <button className="primary" disabled={busy||!!pending||!checked||!changed||snapshot?.game_running} onClick={confirm}><BookOpen size={16}/>Back up & unlock knowledge</button>
 </Dialog>;
}
