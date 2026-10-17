import type {WorldChoice,WorldOperation,WorldRequest,WorldSnapshot} from './world-types';

const maxU64=18446744073709551615n;
const storageKey='crimson-workbench.world.pending.v1';
const counterKey='crimson-workbench.world.counter.v1';
const operations:WorldOperation[]=['choose_time','advance_time','freeze_time','release_time','choose_weather','freeze_weather','release_weather'];
export function nativeInteger(value:string):bigint {
 if(!/^(0|[1-9][0-9]{0,19})$/.test(value))throw Error('Invalid live control identifier.');
 const n=BigInt(value);if(n>maxU64)throw Error('Live control identifier is out of range.');return n;
}
export function validWeather(value:WorldSnapshot['weather']):boolean {
 return Object.values(value).every(Number.isFinite)&&value.precipitation>=0&&value.precipitation<=1&&value.cloudiness>=-1&&value.cloudiness<=1&&value.humidity>=0&&value.humidity<=1&&value.wind_speed>=0&&value.wind_speed<=60&&value.height_fog>=0&&value.height_fog<=10&&(value.phase===0||value.phase===1);
}
export function worldRequest(s:WorldSnapshot,choice:WorldChoice,lastId='0'):WorldRequest {
 const weather=choice.operation.endsWith('weather');
 if(!Number.isInteger(s.pid)||s.pid<=0||nativeInteger(s.session)===0n||nativeInteger(s.generation)===0n||!(s.flags&(weather?2:1))||s.flags&(weather?32:16))throw Error('These live controls are not available in the current game state.');
 if(s.state==='queued')throw Error('Another live command is still waiting for the game.');
 const highest=[s.id,s.time_receipt,s.weather_receipt,lastId].map(nativeInteger).reduce((a,b)=>a>b?a:b);
 const deadline=nativeInteger(s.monotonic_ms)+8000n;
 if(highest===maxU64||deadline>maxU64)throw Error('The live command counter is exhausted.');
 const request:WorldRequest={pid:s.pid,id:(highest+1n).toString(),session:s.session,generation:s.generation,not_after_ms:deadline.toString(),operation:choice.operation,time:null,weather:null,enabled:null,hours:null};
 if(choice.operation==='choose_time'){
  const t=choice.time;if(!Object.values(t).every(Number.isInteger)||t.hour<0||t.hour>=24||t.minute<0||t.minute>=60||t.second<0||t.second>=60||t.millisecond<0||t.millisecond>=1000)throw Error('Choose a valid time of day.');
  request.time=t;
 }else if(choice.operation==='advance_time'){
  if(!Number.isInteger(choice.hours)||choice.hours<1||choice.hours>10000)throw Error('Choose a whole number from 1 to 10,000 hours.');
  request.hours=choice.hours;
 }else if(choice.operation==='choose_weather'){
  if(!validWeather(choice.weather))throw Error('Choose valid weather values.');request.weather=choice.weather;
 }else if('enabled' in choice)request.enabled=choice.enabled;
 return request;
}
export type PendingWorld={pid:number;id:string;session:string;generation:string;not_after_ms:string;operation:WorldOperation};
function pending(value:unknown):value is PendingWorld {
 if(!value||typeof value!=='object')return false;
 const p=value as PendingWorld;
 return Number.isInteger(p.pid)&&p.pid>0&&operations.includes(p.operation)&&[p.id,p.session,p.generation,p.not_after_ms].every(s=>typeof s==='string'&&nativeInteger(s)>0n);
}
export function readWorldHistory(storage:Storage):{pending:PendingWorld|null;counter:string} {
 const raw=storage.getItem(storageKey),counter=storage.getItem(counterKey)??'0';nativeInteger(counter);
 if(raw===null)return{pending:null,counter};
 const saved:unknown=JSON.parse(raw);if(!pending(saved))throw Error('The previous live command could not be read.');
 return{pending:saved,counter};
}
export function saveWorldPending(storage:Storage,request:WorldRequest):PendingWorld {
 const p:PendingWorld={pid:request.pid,id:request.id,session:request.session,generation:request.generation,not_after_ms:request.not_after_ms,operation:request.operation};
 if(!pending(p))throw Error('Invalid live command.');
 // Store both before IPC. If either write fails, no command is transmitted.
 storage.setItem(counterKey,p.id);storage.setItem(storageKey,JSON.stringify(p));return p;
}
export function clearWorldPending(storage:Storage){storage.removeItem(storageKey)}
export type WorldOutcome={kind:'waiting'|'done'|'uncertain';message:string};
export function worldOutcome(s:WorldSnapshot,p:PendingWorld,direct=false):WorldOutcome {
 if(s.pid!==p.pid||s.session!==p.session)return{kind:'uncertain',message:'The game instance changed. The previous result cannot be confirmed; the command will not be sent again.'};
 if(direct&&s.id===p.id&&s.state==='rejected')return{kind:'done',message:worldReason(s.reason)};
 if(s.generation!==p.generation)return{kind:'uncertain',message:'The world or character changed. The previous result cannot be confirmed; the command will not be sent again.'};
 const weather=p.operation.endsWith('weather');
 const receipt=weather?s.weather_receipt:s.time_receipt,result=weather?s.weather_result:s.time_result,reason=weather?s.weather_result_reason:s.time_result_reason;
 if(receipt===p.id){
  if(result==='queued')return{kind:'waiting',message:'Waiting for the game. Return to your character briefly; this command will not be sent again.'};
  if(result==='published')return{kind:'done',message:weather?'Weather updated.':p.operation==='advance_time'?'Time advanced.':'Time updated.'};
  if(result==='released')return{kind:'done',message:weather?'Previous weather control restored.':'Time freeze released.'};
  if(result==='rejected')return{kind:'done',message:worldReason(reason)};
  if(result==='expired'||result==='world_changed')return{kind:'done',message:'The command expired or the world changed before completion. It will not be sent again.'};
  if(result==='uncertain'||result==='fault_latched')return{kind:'uncertain',message:'The game could not confirm this change. Check the result in the game; the command will not be sent again.'};
 }
 if(s.flags&(weather?32:16))return{kind:'uncertain',message:'The game could not confirm this change. Check the result in the game; the command will not be sent again.'};
 if(nativeInteger(s.id)>nativeInteger(p.id))return{kind:'uncertain',message:'Another command replaced the status. The previous result cannot be confirmed; the command will not be sent again.'};
 if(nativeInteger(s.monotonic_ms)>nativeInteger(p.not_after_ms))return{kind:'uncertain',message:'No result was confirmed before the command deadline. Check the game before making another change; this command will not be sent again.'};
 return{kind:'waiting',message:'Checking the previous live command. It will not be sent again.'};
}
export function worldReason(reason:number):string {
 return ['Ready.','The module protocol does not match this Workbench build.','The game instance changed. Refresh the live status.','The world or character changed. Refresh the live status.','The command expired. Refresh the live status.','This command was already seen and will not be sent again.','Another live command is still waiting for the game.','The loaded world is not ready for this change.','This scene fixes the time of day. Return to normal gameplay.','The game has stopped its own clock.','Another game system changed this setting.','The game could not confirm this change.','The game could not confirm this change.','The selected value is outside the supported range.','This control is not available in the current game state.','There is no Workbench freeze to release.'][reason]??'Live controls are not available.';
}
