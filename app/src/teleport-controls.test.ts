import {describe,it,expect} from 'vitest';
import {clearTeleportPending,saveTeleportPending,teleportHistory,teleportOutcome,teleportRequest,validTeleportName} from './teleport-controls';
import type {TeleportSnapshot} from './teleport-types';
function snapshot():TeleportSnapshot {
 const current={field_definition:1,stage:17,position:{x:10,y:20,z:30},yaw:0.5};
 return{pid:123,state:'ready',id:'9007199254740993',session:'18364758544493064720',epoch:'9007199254740995',observed_ms:'4294967296000',reason:0,features:7,identity:123,world:456,native_reason:0,native_error:0,storage_status:1,bookmarks_revision:'9007199254740997',current,bookmarks:Array.from({length:16},(_,i)=>({occupied:i===0,name:i===0?'Camp café':'',position:i===0?structuredClone(current):null})),marker_status:0,markers_observed_ms:'4294967296000',markers:[{id:'0',source:'map_pin',position:{x:100,y:0,z:200},layer:'mainland',shape:1,color:2},{id:'0',source:'waypoint',position:{x:300,y:0,z:400},layer:'mainland',shape:3,color:4}]};
}
function storage():Storage {
 const data=new Map<string,string>();return{get length(){return data.size},clear(){data.clear()},getItem:k=>data.get(k)??null,key:n=>[...data.keys()][n]??null,removeItem:k=>{data.delete(k)},setItem:(k,v)=>{data.set(k,v)}};
}
describe('explicit teleport commands',()=>{
 it('retains full-width IDs and allows marker zero in both native sources',()=>{
  const s=snapshot();
  const r=teleportRequest(s,{operation:'save_current',slot:15,name:'Camp café'},'9007199254740999');
  expect(r).toEqual({operation:'save_current',slot:15,name:'Camp café',pid:123,id:'9007199254741000',session:s.session,epoch:s.epoch,not_after_ms:'4294967298500'});
  for(const marker_source of ['map_pin','waypoint'] as const){
   const m=teleportRequest(s,{operation:'travel_marker',marker:'0',marker_source});
   expect(m).toMatchObject({marker:'0',marker_source});expect(m).not.toHaveProperty('position');expect(m).not.toHaveProperty('slot');
  }
  expect(()=>teleportRequest({...s,id:'18446744073709551615'},{operation:'remove_saved',slot:0})).toThrow();
  expect(()=>teleportRequest(s,{operation:'travel_marker',marker:'18446744073709551615',marker_source:'map_pin'})).toThrow();
 });
 it('checks UTF-8 names, slots, capabilities, fields and fresh marker observations',()=>{
  expect(validTeleportName('é'.repeat(23)+'a')).toBe(true);expect(validTeleportName('é'.repeat(24))).toBe(false);
  for(const name of ['', 'A\nB','A\u0085B','A\0B','\ud800'])expect(validTeleportName(name)).toBe(false);
  const s=snapshot();expect(()=>teleportRequest(s,{operation:'travel_saved',slot:1})).toThrow('empty');
  expect(()=>teleportRequest(s,{operation:'save_current',slot:16,name:'Home'})).toThrow();
  s.bookmarks[0].position!.stage=18;expect(()=>teleportRequest(s,{operation:'travel_saved',slot:0})).toThrow('another field');
  s.markers_observed_ms='4294967292999';expect(()=>teleportRequest(s,{operation:'travel_marker',marker:'0',marker_source:'map_pin'})).toThrow();
  s.markers_observed_ms=s.observed_ms;s.markers[0].layer='alternate';expect(()=>teleportRequest(s,{operation:'travel_marker',marker:'0',marker_source:'map_pin'})).toThrow();
  for(const state of ['waiting','faulted','pending','native_queued'] as const)expect(()=>teleportRequest({...snapshot(),state},{operation:'remove_saved',slot:0})).toThrow();
 });
 it('writes pointer-free history before transmission and keeps the counter after clearing',()=>{
  const st=storage(),r=teleportRequest(snapshot(),{operation:'travel_marker',marker:'0',marker_source:'waypoint'});
  const p=saveTeleportPending(st,r),history=teleportHistory(st);expect(history).toEqual({counter:r.id,pending:p});
  expect(Object.keys(p).sort()).toEqual(['pid','id','session','epoch','not_after_ms','operation','recorded_at'].sort());
  clearTeleportPending(st);expect(teleportHistory(st)).toEqual({counter:r.id,pending:null});
  const broken=storage();broken.setItem('crimson-workbench.teleport.pending.v1','{}');expect(()=>teleportHistory(broken)).toThrow();
  const denied=storage();denied.setItem=()=>{throw Error('denied')};expect(()=>saveTeleportPending(denied,r)).toThrow('denied');
 });
 it('distinguishes queue acceptance, later observed arrival and lost context without replay',()=>{
  const s=snapshot(),p=saveTeleportPending(storage(),teleportRequest(s,{operation:'travel_saved',slot:0}));
  s.id=p.id;s.state='native_queued';expect(teleportOutcome(s,p).kind).toBe('waiting');
  s.state='position_observed';expect(teleportOutcome(s,p)).toMatchObject({kind:'done',message:expect.stringContaining('observed')});
  for(const changed of [{pid:999},{session:'99'},{epoch:'99'}])expect(teleportOutcome({...s,...changed},p).kind).toBe('uncertain');
  expect(teleportOutcome({...s,id:(BigInt(p.id)+1n).toString()},p).kind).toBe('uncertain');
  expect(teleportOutcome({...s,state:'native_queued'},p,false,p.recorded_at+70001).kind).toBe('uncertain');
 });
 it('reports persistence failure separately from successful capture/removal',()=>{
  const s=snapshot(),st=storage();
  const captured=saveTeleportPending(st,teleportRequest(s,{operation:'save_current',slot:1,name:'Camp'}));
  expect(teleportOutcome({...s,id:captured.id,state:'captured',storage_status:2},captured)).toMatchObject({kind:'done',message:expect.stringContaining('may be lost')});
  const removed=saveTeleportPending(st,teleportRequest(s,{operation:'remove_saved',slot:0}));
  expect(teleportOutcome({...s,id:removed.id,state:'removed',storage_status:2},removed)).toMatchObject({kind:'done',message:expect.stringContaining('may return')});
 });
 it('finishes an authenticated immediate refusal but retains native uncertainty',()=>{
  const s=snapshot(),p=saveTeleportPending(storage(),teleportRequest(s,{operation:'travel_saved',slot:0}));
  expect(teleportOutcome({...s,state:'rejected',reason:5},p,true).kind).toBe('done');
  expect(teleportOutcome({...s,state:'uncertain',id:p.id,reason:12},p).kind).toBe('uncertain');
  expect(teleportOutcome({...s,state:'faulted',id:p.id,reason:10},p).kind).toBe('uncertain');
 });
});
