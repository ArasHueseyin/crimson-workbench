import {Dialog} from './Dialog';
import {SaveSelector,SaveTarget} from './SaveSelector';
import {useEffect,useRef,useState} from 'react';
import {api,asError} from './api';
import type {Catalog} from './types';
import type {ContributionEdit,ContributionPreview,ContributionReceipt,ContributionRequest} from './contribution-types';

const maximum=9000000000000000000n;
export function MarketplaceContributions({catalog,close}:{catalog:Catalog;close:()=>void}) {
 const [snapshot,setSnapshot]=useState<ContributionPreview|null>(null),[save,setSave]=useState('');
 const [targets,setTargets]=useState<Record<number,string>>({}),[checked,setChecked]=useState(false);
 const [busy,setBusy]=useState(false),[error,setError]=useState(''),[receipt,setReceipt]=useState<ContributionReceipt|null>(null);
 const storage=`crimson-contributions-pending:${catalog.game_path}`;
 const [pending,setPending]=useState<ContributionRequest|null>(()=>{try{return JSON.parse(localStorage.getItem(storage)??'null') as ContributionRequest|null}catch{return null}});
 const mounted=useRef(true),generation=useRef(0),lock=useRef(false);
 useEffect(()=>{mounted.current=true;return()=>{mounted.current=false;generation.current++}},[]);
 async function load(id:string|null,edits:ContributionEdit[]=[]) {
  const current=++generation.current;setBusy(true);setChecked(false);setError('');
  // Never keep a previous plan actionable while a new request is in flight.
  setSnapshot(s=>s?{...s,plan:null}:s);
  try{const next=await api.contributionPreview(catalog.session,id,edits);if(mounted.current&&current===generation.current)setSnapshot(next)}
  catch(e){if(mounted.current&&current===generation.current)setError(asError(e).message)}
  finally{if(mounted.current&&current===generation.current)setBusy(false)}
 }
 useEffect(()=>{void load(null)},[catalog.session]);
 function change(key:number,value:string){setTargets(t=>({...t,[key]:value}));setChecked(false);setReceipt(null);setError('');setSnapshot(s=>s?{...s,plan:null}:s)}
 const edits=Object.entries(targets).filter(([,value])=>value!=='').map(([key,target])=>({key:Number(key),target})).sort((a,b)=>a.key-b.key);
 const valid=edits.length>0&&edits.every(edit=>{
  if(!/^[1-9][0-9]{0,18}$/.test(edit.target))return false;
  const current=snapshot?.balances.find(b=>b.key===edit.key)?.amount;
  return current!=null&&BigInt(edit.target)<=maximum&&BigInt(edit.target)>=BigInt(current);
 });
 async function apply(request:ContributionRequest) {
  if(lock.current)return;lock.current=true;setBusy(true);setError('');setChecked(false);
  try{const result=await api.contributionApply(catalog.session,request);localStorage.removeItem(storage);if(mounted.current){setPending(null);setReceipt(result);setTargets({});setSave(request.save);await load(request.save)}}
  catch(e){if(mounted.current)setError(asError(e).message)}
  finally{lock.current=false;if(mounted.current)setBusy(false)}
 }
 function confirm() {
  const plan=snapshot?.plan;if(!plan||!snapshot.selected_save||!checked)return;
  const request:ContributionRequest={save_root:snapshot.save_root,id:crypto.randomUUID().replaceAll('-',''),save:snapshot.selected_save,save_sha256:plan.save_sha256,lobby_sha256:plan.lobby_sha256,plan_id:plan.plan_id,edits};
  try{localStorage.setItem(storage,JSON.stringify(request));setPending(request);void apply(request)}catch(e){setError(asError(e).message)}
 }
 const plan=snapshot?.plan;
 const changed=plan?.changes.some(c=>c.before!==c.after)??false;
 return <Dialog title="Contribution points" eyebrow="SAVE ACTION" onClose={close} closeDisabled={busy}>
  <p>Increase existing faction contribution balances in one save. Faction experience, levels, other currencies and inventory items are preserved.</p>
  <SaveSelector saves={snapshot?.saves??[]} value={save||null} ariaLabel="Contribution save" disabled={busy||!!pending} onChange={value=>{setSave(value);setTargets({});setReceipt(null);void load(value||null)}}/>
  {snapshot&&<p>Save folder: <code>{snapshot.save_root}</code></p>}
  {snapshot&&!snapshot.saves.length&&<p>No complete saves found. Select your save folder under Data sources.</p>}
  <div className="contribution-balances">{snapshot?.balances.map(balance=><label key={balance.key} className="contribution-row"><span><strong>{balance.name}</strong><small>Current: {balance.amount??'No existing entry in this save'}</small></span><input aria-label={`${balance.name} contribution target`} inputMode="numeric" placeholder="Leave unchanged" value={targets[balance.key]??''} disabled={busy||!!pending||balance.amount==null} onChange={e=>change(balance.key,e.target.value)}/></label>)}</div>
  {(snapshot?.balances.length??0)>0&&<p className="fine-print">Enter a target amount from the current balance up to 9,000,000,000,000,000,000. Leave other fields empty. Missing wallet entries cannot be created.</p>}
  {edits.length>0&&!valid&&<p role="alert" className="craft-error">Use whole numbers at least as large as the current balance, within the supported limit.</p>}
  <button className="secondary" disabled={busy||!!pending||!save||!valid} onClick={()=>void load(save,edits)}>Review contribution changes</button>
  {plan&&<div className="recovery-review"><strong>Changes for {snapshot?.selected_save}</strong><div className="market-changes">{plan.changes.map(c=><div key={c.key}><strong>{c.name}</strong><code>{c.before} → {c.after}</code></div>)}</div>{!changed&&<p>These balances already match their targets.</p>}</div>}
  {snapshot?.game_running&&<p role="alert" className="craft-error">Save and fully close Crimson Desert before applying this action.</p>}
  {busy&&<p role="status">Checking the save and preparing the transaction …</p>}
  {error&&<p role="alert" className="craft-error">{error}</p>}
  {pending&&<div className="recovery-review"><p>A saved contribution request for {pending.save} needs a result check. Retrying keeps the original targets and cannot increase points twice.</p><button className="secondary" disabled={busy} onClick={()=>void apply(pending)}>Check / recover pending contribution edit</button><button className="secondary" disabled={busy} onClick={()=>{localStorage.removeItem(storage);setPending(null);setTargets({});void load(save||null)}}>Discard saved request & reload balances</button></div>}
  {receipt&&<div className="recovery-success" role="status"><p>{receipt.message}</p><strong>Backup folder</strong><code>{receipt.backup}</code></div>}
  <SaveTarget saves={snapshot?.saves??[]} value={snapshot?.selected_save??null}/>
  <label className="mod-switch"><input type="checkbox" aria-label="Confirm contribution save edit" checked={checked} disabled={busy||!!pending||!plan||!changed||snapshot?.game_running} onChange={e=>setChecked(e.target.checked)}/>I want to apply these contribution targets to this save.</label>
  <button className="primary" disabled={busy||!!pending||!checked||!plan||!changed||snapshot?.game_running} onClick={confirm}>Back up & update contribution points</button>
 </Dialog>;
}
