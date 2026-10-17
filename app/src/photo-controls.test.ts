import {afterEach,describe,expect,it,vi} from 'vitest';
import {clearPhotoPending,photoHistory,photoOutcome,photoReason,photoRequest,savePhotoPending,type PendingPhoto} from './photo-controls';
import type {PhotoChoice,PhotoSnapshot} from './photo-types';

const pendingKey='crimson-workbench.photo.pending.v1',counterKey='crimson-workbench.photo.counter.v1';
function snapshot(overrides:Partial<PhotoSnapshot>={}):PhotoSnapshot {
 return{pid:123,state:'ready',id:'9007199254740993',session:'18364758544493064720',epoch:'9007199254740995',observed_ms:'4294967296000',reason:0,native_photo:false,owned:false,hud_hidden:false,scene_frozen:false,...overrides};
}
class MemoryStorage implements Storage {
 values=new Map<string,string>();writes=0;removes=0;
 get length(){return this.values.size}
 clear(){this.values.clear()}
 getItem(key:string){return this.values.get(key)??null}
 key(index:number){return [...this.values.keys()][index]??null}
 removeItem(key:string){this.removes++;this.values.delete(key)}
 setItem(key:string,value:string){this.writes++;this.values.set(key,String(value))}
}
function pending(operation:PendingPhoto['operation']='enter'):PendingPhoto {
 return{pid:123,id:'9007199254740994',session:'18364758544493064720',epoch:'9007199254740995',not_after_ms:'4294967298500',operation,recorded_at:1_800_000_000_000};
}
function receipt(p:PendingPhoto,overrides:Partial<PhotoSnapshot>={}):PhotoSnapshot {
 return snapshot({pid:p.pid,id:p.id,session:p.session,epoch:p.epoch,...overrides});
}
afterEach(()=>{vi.restoreAllMocks();vi.unstubAllGlobals()});

describe('photo request identity and ownership',()=>{
 it('preserves full u64 identities and allocates beyond native and persisted counters',()=>{
  const s=snapshot(),r=photoRequest(s,{operation:'enter',hide_on_enter:true});
  expect(r).toEqual({pid:123,id:'9007199254740994',session:s.session,epoch:s.epoch,not_after_ms:'4294967298500',operation:'enter',hide_on_enter:true});
  expect(JSON.parse(JSON.stringify(r))).toEqual(r);
  expect(photoRequest(s,{operation:'enter',hide_on_enter:false},'9007199254741009').id).toBe('9007199254741010');
  expect(photoRequest({...s,id:'18446744073709551614'},{operation:'enter',hide_on_enter:false}).id).toBe('18446744073709551615');
  expect(()=>photoRequest({...s,id:'18446744073709551615'},{operation:'enter',hide_on_enter:true})).toThrow();
  expect(()=>photoRequest(s,{operation:'enter',hide_on_enter:true},'18446744073709551615')).toThrow();
 });
 it('checks canonical identifiers, active context and the exact 2500ms deadline boundary',()=>{
  const choice:PhotoChoice={operation:'enter',hide_on_enter:true};
  for(const field of ['id','session','epoch','observed_ms'] as const){
   for(const bad of ['','00','01','-1','+1','1.0','1e2',' 1','１','18446744073709551616'])expect(()=>photoRequest(snapshot({[field]:bad}),choice)).toThrow();
  }
  for(const pid of [0,-1,0.5,NaN,Infinity])expect(()=>photoRequest(snapshot({pid}),choice)).toThrow();
  for(const field of ['session','epoch'] as const)expect(()=>photoRequest(snapshot({[field]:'0'}),choice)).toThrow();
  expect(()=>photoRequest(snapshot({state:'waiting'}),choice)).toThrow();
  expect(()=>photoRequest(snapshot(),choice,'01')).toThrow();
  const s=snapshot({observed_ms:'18446744073709549115'});
  expect(photoRequest(s,choice).not_after_ms).toBe('18446744073709551615');
  expect(()=>photoRequest({...s,observed_ms:'18446744073709549116'},choice)).toThrow();
 });
 it('uses enter-only HUD payloads and refuses ownership of a foreign photo session',()=>{
  for(const hide_on_enter of [false,true])expect(photoRequest(snapshot(),{operation:'enter',hide_on_enter}).hide_on_enter).toBe(hide_on_enter);
  for(const owned of [false,true])expect(()=>photoRequest(snapshot({native_photo:true,owned}),{operation:'enter',hide_on_enter:true})).toThrow();
  for(const operation of ['hide_hud','show_hud','leave'] as const){
   for(const [owned,native_photo] of [[false,false],[false,true],[true,false]])expect(()=>photoRequest(snapshot({owned,native_photo}),{operation})).toThrow();
   expect(photoRequest(snapshot({owned:true,native_photo:true}),{operation})).toMatchObject({operation,hide_on_enter:null});
  }
  for(const state of ['pending','native_queued'] as const){
   expect(()=>photoRequest(snapshot({state}),{operation:'enter',hide_on_enter:true})).toThrow();
   expect(()=>photoRequest(snapshot({state,owned:true,native_photo:true}),{operation:'leave'})).toThrow();
  }
 });
});

describe('photo persistence and explicit recovery',()=>{
 it('persists receipt metadata before IPC and clearing retains the high command counter',()=>{
  vi.spyOn(Date,'now').mockReturnValue(1_800_000_000_000);
  const storage=new MemoryStorage(),r=photoRequest(snapshot(),{operation:'enter',hide_on_enter:false});
  expect(photoHistory(storage)).toEqual({pending:null,counter:'0'});
  const p=savePhotoPending(storage,r);
  expect(p).toEqual(pending());expect(photoHistory(storage)).toEqual({pending:p,counter:r.id});
  expect(Object.keys(p).sort()).toEqual(['epoch','id','not_after_ms','operation','pid','recorded_at','session']);
  expect(p).not.toHaveProperty('hide_on_enter');
  expect(JSON.parse(storage.getItem(pendingKey)!)).toEqual(p);
  clearPhotoPending(storage);expect(photoHistory(storage)).toEqual({pending:null,counter:r.id});
  expect(photoRequest(snapshot(),{operation:'enter',hide_on_enter:true},photoHistory(storage).counter).id).toBe('9007199254740995');
 });
 it('rejects corrupted history and identifiers instead of treating them as an empty history',()=>{
  for(const raw of ['{','{}','null','[]']){
   const storage=new MemoryStorage();storage.setItem(pendingKey,raw);expect(()=>photoHistory(storage)).toThrow();
  }
  for(const changed of [{pid:0},{id:'0'},{session:'0'},{epoch:'0'},{not_after_ms:'0'},{id:'01'},{operation:'retry'},{recorded_at:0},{recorded_at:-1},{recorded_at:Number.MAX_SAFE_INTEGER+1}]){
   const storage=new MemoryStorage();storage.setItem(pendingKey,JSON.stringify({...pending(),...changed}));expect(()=>photoHistory(storage)).toThrow();
  }
  for(const counter of ['00','-1','18446744073709551616']){
   const storage=new MemoryStorage();storage.setItem(counterKey,counter);expect(()=>photoHistory(storage)).toThrow();
  }
 });
 it('propagates storage failures without silently discarding an unresolved command',()=>{
  const r=photoRequest(snapshot(),{operation:'enter',hide_on_enter:true});
  for(const failAt of [1,2]){
   const storage=new MemoryStorage();let attempts=0;
   vi.spyOn(storage,'setItem').mockImplementation((key,value)=>{
    if(++attempts===failAt)throw Error('Storage unavailable');storage.values.set(key,value);
   });
   expect(()=>savePhotoPending(storage,r)).toThrow('Storage unavailable');
   expect(attempts).toBe(failAt);expect(storage.getItem(pendingKey)).toBeNull();
   expect(storage.getItem(counterKey)).toBe(failAt===2?r.id:null);
  }
  const storage=new MemoryStorage();storage.setItem(pendingKey,JSON.stringify(pending()));
  vi.spyOn(storage,'getItem').mockImplementation(()=>{throw Error('Read denied')});
  expect(()=>photoHistory(storage)).toThrow('Read denied');
  vi.spyOn(storage,'removeItem').mockImplementation(()=>{throw Error('Clear denied')});
  expect(()=>clearPhotoPending(storage)).toThrow('Clear denied');expect(storage.values.has(pendingKey)).toBe(true);
 });
});

describe('photo receipt reconciliation without automatic repetition',()=>{
 it('keeps asynchronous states pending until confirmation and exposes local timeout or clock reversal',()=>{
  const p=pending();
  for(const state of ['pending','native_queued','waiting'] as const){
   const s=receipt(p,{state,owned:true,native_photo:true,scene_frozen:true});
   expect(photoOutcome(s,p,false,p.recorded_at+5000).kind).toBe('waiting');
   expect(photoOutcome(s,p,false,p.recorded_at+5001).kind).toBe('uncertain');
   expect(photoOutcome(s,p,false,p.recorded_at-1).kind).toBe('uncertain');
   expect(photoOutcome(s,p,false,p.recorded_at).message).toContain('will not be sent again');
  }
 });
 it('requires the matching receipt and native result for each operation',()=>{
  const cases:[PendingPhoto['operation'],Partial<PhotoSnapshot>,Partial<PhotoSnapshot>][]=[
   ['enter',{state:'active',owned:true,native_photo:true,scene_frozen:true},{scene_frozen:false}],
   ['hide_hud',{state:'hud_hidden',owned:true,native_photo:true,hud_hidden:true},{hud_hidden:false}],
   ['show_hud',{state:'hud_visible',owned:true,native_photo:true,hud_hidden:false},{hud_hidden:true}],
   ['leave',{state:'left',owned:false,native_photo:false},{native_photo:true}],
  ];
  for(const [operation,good,bad] of cases){
   const p=pending(operation),s=receipt(p,good);
   expect(photoOutcome(s,p,false,p.recorded_at).kind).toBe('done');
   expect(photoOutcome({...s,...bad},p,false,p.recorded_at).kind).toBe('waiting');
   expect(photoOutcome({...s,id:'9007199254740993'},p,false,p.recorded_at).kind).toBe('waiting');
   for(const state of ['uncertain','rejected'] as const)expect(photoOutcome({...s,state,reason:12},p,false,p.recorded_at).kind).toBe('uncertain');
  }
 });
 it('does not confirm old results across PID, session or epoch changes',()=>{
  const p=pending(),s=receipt(p,{state:'active',owned:true,native_photo:true,scene_frozen:true});
  for(const changed of [{pid:124},{session:'18364758544493064721'},{epoch:'9007199254740996'}]){
   const outcome=photoOutcome({...s,...changed},p,false,p.recorded_at);
   expect(outcome.kind).toBe('uncertain');expect(outcome.message).toContain('will not be sent again');
  }
  expect(photoOutcome({...s,id:'9007199254740995'},p,false,p.recorded_at).kind).toBe('uncertain');
  const old=receipt(p,{id:'9007199254740993',observed_ms:p.not_after_ms});
  expect(photoOutcome(old,p,false,p.recorded_at).kind).toBe('waiting');
  expect(photoOutcome({...old,observed_ms:(BigInt(p.not_after_ms)+1n).toString()},p,false,p.recorded_at).kind).toBe('uncertain');
 });
 it('distinguishes a direct native rejection from a polled unresolved result, including zero-context busy replies',()=>{
  const p=pending(),rejected=receipt(p,{state:'rejected',reason:9});
  expect(photoOutcome(rejected,p,true,p.recorded_at)).toEqual({kind:'done',message:photoReason(9)});
  expect(photoOutcome(rejected,p,false,p.recorded_at).kind).toBe('uncertain');
  const busy=snapshot({id:'0',session:'0',epoch:'0',observed_ms:'0',state:'rejected',reason:5});
  expect(photoOutcome(busy,p,true,p.recorded_at)).toEqual({kind:'done',message:photoReason(5)});
  expect(photoOutcome(busy,p,false,p.recorded_at).kind).toBe('uncertain');
 });
 it('only reads persisted recovery data: repeated checks create no request, timer, network call or storage mutation',()=>{
  const storage=new MemoryStorage(),p=pending();storage.setItem(pendingKey,JSON.stringify(p));storage.setItem(counterKey,p.id);
  const saved=[...storage.values],writes=storage.writes;
  const fetch=vi.fn(),timeout=vi.spyOn(globalThis,'setTimeout'),interval=vi.spyOn(globalThis,'setInterval');vi.stubGlobal('fetch',fetch);
  const states:PhotoSnapshot[]=[receipt(p,{state:'pending'}),receipt(p,{state:'native_queued'}),receipt(p,{state:'uncertain',reason:12}),receipt(p,{state:'waiting',session:'999'})];
  for(let n=0;n<20;n++){
   const history=photoHistory(storage);expect(history.pending).toEqual(p);expect(history.counter).toBe(p.id);
   const outcome=photoOutcome(states[n%states.length],history.pending!,false,p.recorded_at+1000);
   expect(Object.keys(outcome).sort()).toEqual(['kind','message']);expect(outcome.kind).not.toBe('done');
  }
  expect(fetch).not.toHaveBeenCalled();expect(timeout).not.toHaveBeenCalled();expect(interval).not.toHaveBeenCalled();
  expect([...storage.values]).toEqual(saved);expect(storage.writes).toBe(writes);expect(storage.removes).toBe(0);
 });
});
