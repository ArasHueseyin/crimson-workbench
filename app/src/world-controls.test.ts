import {describe,expect,it} from 'vitest';
import {clearWorldPending,nativeInteger,readWorldHistory,saveWorldPending,worldOutcome,worldRequest} from './world-controls';
import type {WorldSnapshot} from './world-types';

function snapshot():WorldSnapshot {
 return{pid:123,state:'ready',id:'9007199254740993',session:'18364758544493064720',generation:'9007199254740995',monotonic_ms:'4294967296000',reason:0,flags:3,identity:12,world:34,stage:56,clock_reason:0,weather_reason:0,time:{day:7,hour:12,minute:34,second:56,millisecond:789,weekday:0,rate:12,anchor:'9007199254740997'},weather:{precipitation:0,cloudiness:0,humidity:0.5,wind_speed:10,height_fog:1,phase:0},time_receipt:'9007199254740992',weather_receipt:'9007199254740991',time_result:'published',time_result_reason:0,weather_result:'published',weather_result_reason:0};
}
class MemoryStorage implements Storage {
 private values=new Map<string,string>();
 get length(){return this.values.size}
 clear(){this.values.clear()}
 getItem(key:string){return this.values.get(key)??null}
 key(i:number){return [...this.values.keys()][i]??null}
 removeItem(key:string){this.values.delete(key)}
 setItem(key:string,value:string){this.values.set(key,String(value))}
}
describe('live command identity and recovery',()=>{
 it('keeps all 64-bit identities exact and allocates above every receipt',()=>{
  const s=snapshot();s.weather_receipt='9007199254740999';
  const r=worldRequest(s,{operation:'choose_time',time:{hour:23,minute:59,second:0,millisecond:0}});
  expect(r.id).toBe('9007199254741000');expect(r.session).toBe(s.session);expect(r.generation).toBe(s.generation);
  expect(r.not_after_ms).toBe('4294967304000');expect(JSON.parse(JSON.stringify(r))).toEqual(r);
  expect(worldRequest(s,{operation:'freeze_weather',enabled:true},'9007199254741009').id).toBe('9007199254741010');
  for(const bad of ['','00','01','-1','+1','1.0','1e2',' 1','１','18446744073709551616'])expect(()=>nativeInteger(bad)).toThrow();
  s.id='18446744073709551615';expect(()=>worldRequest(s,{operation:'release_time'})).toThrow();
 });
 it('only permits the requested available lane and validates controls before transmission',()=>{
  const s=snapshot();s.flags=1;
  expect(()=>worldRequest(s,{operation:'freeze_weather',enabled:true})).toThrow();
  expect(()=>worldRequest(s,{operation:'freeze_time',enabled:true})).not.toThrow();
  s.flags|=16;expect(()=>worldRequest(s,{operation:'freeze_time',enabled:true})).toThrow();
  s.flags=3;
  expect(()=>worldRequest(s,{operation:'choose_time',time:{hour:24,minute:0,second:0,millisecond:0}})).toThrow();
  expect(()=>worldRequest(s,{operation:'choose_weather',weather:{...s.weather,wind_speed:61}})).toThrow();
  expect(()=>worldRequest(s,{operation:'choose_weather',weather:{...s.weather,precipitation:NaN}})).toThrow();
  for(const hours of [0,-1,1.5,10001,Infinity,NaN])expect(()=>worldRequest(s,{operation:'advance_time',hours})).toThrow();
  const future=worldRequest(s,{operation:'advance_time',hours:48});
  expect(future).toMatchObject({operation:'advance_time',hours:48,time:null,weather:null,enabled:null});
  const futurePending=saveWorldPending(new MemoryStorage(),future);
  s.time_receipt=future.id;s.time_result='published';
  expect(worldOutcome(s,futurePending)).toEqual({kind:'done',message:'Time advanced.'});
  s.state='queued';expect(()=>worldRequest(s,{operation:'release_weather'})).toThrow();
 });
 it('persists an unresolved command as data and never constructs an automatic retry',()=>{
  const storage=new MemoryStorage(),s=snapshot(),r=worldRequest(s,{operation:'freeze_time',enabled:true});
  expect(readWorldHistory(storage)).toEqual({pending:null,counter:'0'});
  const pending=saveWorldPending(storage,r);expect(readWorldHistory(storage)).toEqual({pending,counter:r.id});
  expect(pending).not.toHaveProperty('time');expect(pending).not.toHaveProperty('enabled');
  clearWorldPending(storage);expect(readWorldHistory(storage)).toEqual({pending:null,counter:r.id});
  storage.setItem('crimson-workbench.world.pending.v1','{}');expect(()=>readWorldHistory(storage)).toThrow();
 });
 it('uses each lane receipt, even if unsolicited weather status overwrites the global code',()=>{
  const storage=new MemoryStorage(),s=snapshot(),r=worldRequest(s,{operation:'freeze_time',enabled:true});
  const p=saveWorldPending(storage,r);
  s.id=r.id;s.state='queued';s.time_receipt=r.id;s.time_result='queued';
  expect(worldOutcome(s,p).kind).toBe('waiting');
  s.time_result='published';s.id='0';s.state='released'; // unrelated automatic weather release
  expect(worldOutcome(s,p)).toEqual({kind:'done',message:'Time updated.'});
  s.time_result='uncertain';s.state='published';s.weather_receipt=r.id;
  expect(worldOutcome(s,p).kind).toBe('uncertain');
  s.time_result='queued';s.id=r.id;s.state='rejected';s.reason=6;
  expect(worldOutcome(s,p,true)).toEqual({kind:'done',message:'Another live command is still waiting for the game.'});
  expect(worldOutcome(s,p,false).kind).toBe('waiting');
 });
 it('never calls an old receipt success after a process or world change',()=>{
  const s=snapshot(),r=worldRequest(s,{operation:'release_time'}),p=saveWorldPending(new MemoryStorage(),r);
  s.time_receipt=r.id;s.time_result='released';s.id=r.id;
  expect(worldOutcome(s,p).kind).toBe('done');
  s.generation='9007199254741000';expect(worldOutcome(s,p).kind).toBe('uncertain');
  s.generation=p.generation;s.pid++;expect(worldOutcome(s,p).kind).toBe('uncertain');
  s.pid=p.pid;s.session='999';expect(worldOutcome(s,p).kind).toBe('uncertain');
 });
});
