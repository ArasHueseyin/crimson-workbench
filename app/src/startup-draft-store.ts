import type {RuntimeOptionId} from './marketplace-types';

export type StartupDraftId=RuntimeOptionId|'player_jump'|'player_speed'|'preserve_consumables';
export type StartupDraftValues=Record<Exclude<StartupDraftId,'preserve_consumables'>,number>&{preserve_consumables:number[]};
export type StartupDrafts=Partial<StartupDraftValues>;
export const startupDraftStorageKey='crimson-workbench.startup-drafts.v1';
type Storage=Pick<globalThis.Storage,'getItem'|'setItem'|'removeItem'>;
let fallback:StartupDrafts={};
// Quota failures can leave readable but stale storage. Keep this storage
// instance's unpublished draft authoritative until a later write succeeds.
const unpublished=new WeakMap<Storage,StartupDrafts>();
function browserStorage():Storage|undefined {try{return typeof window==='undefined'?undefined:window.localStorage}catch{return undefined}}
export function validStartupDraft(id:string,value:unknown):boolean {
 if(id==='preserve_consumables')return Array.isArray(value)&&value.length<=10000&&value.every(v=>Number.isSafeInteger(v)&&v>0&&v<=0xffffffff)&&new Set(value).size===value.length;
 if(typeof value!=='number'||!Number.isInteger(value))return false;
 if(id==='player_resources')return value>=0&&value<=3;
 if(id==='force_palm_height')return value>=100&&value<=10000;
 if(id==='axiom_reach')return value===100||value>=200&&value<=500;
 if(id==='glide_speed')return value>=100&&value<=500;
 return (id==='player_jump'||id==='player_speed')&&value>=100&&value<=300;
}
export function parseStartupDrafts(raw:string|null):StartupDrafts {
 try {
  const parsed=JSON.parse(raw??'null');
  if(parsed?.version!==1||!parsed.values||typeof parsed.values!=='object'||Array.isArray(parsed.values))return {};
  return Object.fromEntries(Object.entries(parsed.values).filter(([key,value])=>validStartupDraft(key,value)));
 }catch{return {}}
}
export function readStartupDrafts(storage:Storage|undefined=browserStorage()):StartupDrafts {
 if(storage&&unpublished.has(storage))return {...unpublished.get(storage)};
 try{return storage?parseStartupDrafts(storage.getItem(startupDraftStorageKey)):{...fallback}}catch{return {...fallback}}
}
function writeStartupDrafts(values:StartupDrafts,storage:Storage|undefined) {
 fallback={...values};
 try {
  if(Object.keys(values).length)storage?.setItem(startupDraftStorageKey,JSON.stringify({version:1,values}));else storage?.removeItem(startupDraftStorageKey);
  if(storage)unpublished.delete(storage);
 }catch{if(storage)unpublished.set(storage,{...values})}
}
export function sameStartupValue(a:unknown,b:unknown):boolean {return JSON.stringify(a)===JSON.stringify(b)}
export function updateStartupDraft<K extends StartupDraftId>(id:K,value:StartupDraftValues[K],saved?:StartupDraftValues[K],storage:Storage|undefined=browserStorage()) {
 if(!validStartupDraft(id,value))return;
 const values=readStartupDrafts(storage);
 if(sameStartupValue(value,saved))delete values[id];else Object.assign(values,{[id]:Array.isArray(value)?[...value].sort((a,b)=>a-b):value});
 writeStartupDrafts(values,storage);
}
export function discardStartupDraft(id:StartupDraftId,storage:Storage|undefined=browserStorage()) {
 const values=readStartupDrafts(storage);delete values[id];writeStartupDrafts(values,storage);
}
/** A status refresh keeps a draft. A confirmed matching save clears it. */
export function reconcileStartupDraft<K extends StartupDraftId>(id:K,saved:StartupDraftValues[K],storage:Storage|undefined=browserStorage()):StartupDraftValues[K] {
 const value=readStartupDrafts(storage)[id];
 if(value===undefined)return saved;
 if(sameStartupValue(value,saved)){discardStartupDraft(id,storage);return saved}
 return value as StartupDraftValues[K];
}
