import {useEffect,useRef,useState} from 'react';
import {Clock3,CloudSun,LoaderCircle,RefreshCw} from 'lucide-react';
import {api,asError} from './api';
import {clearWorldPending,readWorldHistory,saveWorldPending,validWeather,worldOutcome,worldReason,worldRequest,type PendingWorld} from './world-controls';
import type {WeatherChoice,WorldChoice,WorldSnapshot} from './world-types';

const emptyWeather:WeatherChoice={precipitation:0,cloudiness:0,humidity:0,wind_speed:0,height_fog:0,phase:0};
const clamp=(value:number,min:number,max:number)=>Math.min(max,Math.max(min,value));
const displayTime=(time:WorldSnapshot['time'])=>`${String(time.hour).padStart(2,'0')}:${String(time.minute).padStart(2,'0')}`;
const weatherFields=[
 {key:'precipitation',label:'Precipitation intensity',min:0,max:1,step:0.05,percent:true,decimals:0},
 {key:'cloudiness',label:'Cloudiness',min:-1,max:1,step:0.05,percent:false,decimals:2},
 {key:'humidity',label:'Humidity',min:0,max:1,step:0.05,percent:true,decimals:0},
 {key:'wind_speed',label:'Wind strength',min:0,max:60,step:1,percent:false,decimals:1},
 {key:'height_fog',label:'Height fog',min:0,max:10,step:0.1,percent:false,decimals:1},
] as const;

/** Live controls only. Installation/removal is owned by the parent runtime card. */
export function MarketplaceWorldControls({session,enabled}:{session:number;enabled:boolean}) {
 const [snapshot,setSnapshot]=useState<WorldSnapshot|null>(null),[time,setTime]=useState('12:00'),[weather,setWeather]=useState<WeatherChoice>(emptyWeather);
 const [advanceHours,setAdvanceHours]=useState('1');
 const [busy,setBusy]=useState(false),[error,setError]=useState(''),[message,setMessage]=useState(''),[historyError,setHistoryError]=useState('');
 const [pending,setPending]=useState<PendingWorld|null>(null),[uncertain,setUncertain]=useState(false);
 const revision=useRef(0),lock=useRef(false),reading=useRef(false),pendingRef=useRef<PendingWorld|null>(null),counter=useRef('0'),initialized=useRef('');
 function finishPending(){
  try{clearWorldPending(localStorage);pendingRef.current=null;setPending(null);setUncertain(false)}
  catch{setHistoryError('The previous command could not be cleared from local history. Live changes are disabled.')}
 }
 function accept(next:WorldSnapshot,direct=false){
  setSnapshot(next);setError('');
  const context=`${next.pid}/${next.session}/${next.generation}`;
  if(initialized.current!==context&&(next.flags&3)){
   initialized.current=context;setTime(displayTime(next.time));
   const w=next.weather;
   setWeather({precipitation:clamp(w.precipitation,0,1),cloudiness:clamp(w.cloudiness,-1,1),humidity:clamp(w.humidity,0,1),wind_speed:clamp(w.wind_speed,0,60),height_fog:clamp(w.height_fog,0,10),phase:w.phase});
  }
  const p=pendingRef.current;
  if(p){const outcome=worldOutcome(next,p,direct);setMessage(outcome.message);setUncertain(outcome.kind==='uncertain');if(outcome.kind==='done')finishPending()}
 }
 async function refresh(token=revision.current){
  if(!enabled||reading.current||lock.current)return;
  reading.current=true;
  try{const next=await api.worldStatus(session);if(revision.current===token)accept(next)}
  catch(e){if(revision.current===token)setError(asError(e).message)}
  finally{reading.current=false}
 }
 useEffect(()=>{
  const token=++revision.current;initialized.current='';setSnapshot(null);setBusy(false);setError('');setHistoryError('');setMessage('');setUncertain(false);
  try{const saved=readWorldHistory(localStorage);counter.current=saved.counter;pendingRef.current=saved.pending;setPending(saved.pending);if(saved.pending)setMessage('Checking the previous live command. It will not be sent again.')}
  catch{setHistoryError('The previous live command history could not be read. Live changes are disabled.')}
  let timer:ReturnType<typeof setTimeout>|undefined;
  async function poll(){await refresh(token);if(revision.current===token&&enabled)timer=setTimeout(()=>void poll(),1500)}
  if(enabled)void poll();
  return()=>{revision.current++;if(timer)clearTimeout(timer)};
 },[session,enabled]);
 async function change(choice:WorldChoice){
  if(lock.current||pendingRef.current||!enabled||historyError)return;
  lock.current=true;setBusy(true);setError('');setMessage('');const token=revision.current;
  try{
   // Always preflight this click with current native context. Polling never
   // constructs or transmits a mutation, including after a remount or timeout.
   const current=await api.worldStatus(session);if(revision.current!==token)return;
   accept(current);const request=worldRequest(current,choice,counter.current);
   const saved=saveWorldPending(localStorage,request);counter.current=request.id;pendingRef.current=saved;setPending(saved);setUncertain(false);
   const next=await api.worldChange(session,request);if(revision.current===token)accept(next,true);
  }catch(e){if(revision.current===token){setError(asError(e).message);if(pendingRef.current){setUncertain(true);setMessage('The response was not confirmed. Use Check live status; this command will not be sent again.')}}}
  finally{lock.current=false;if(revision.current===token)setBusy(false)}
 }
 const locked=!enabled||busy||!!pending||!!historyError||!!error;
 const timeAvailable=!!snapshot&&(snapshot.flags&1)!==0&&(snapshot.flags&16)===0;
 const weatherAvailable=!!snapshot&&(snapshot.flags&2)!==0&&(snapshot.flags&32)===0;
 const timeFrozen=!!snapshot&&(snapshot.flags&4)!==0,weatherFrozen=!!snapshot&&(snapshot.flags&8)!==0;
 const validTime=/^(?:[01][0-9]|2[0-3]):[0-5][0-9]$/.test(time);
 const validHours=/^[0-9]+$/.test(advanceHours)&&Number.isSafeInteger(Number(advanceHours))&&Number(advanceHours)>=1&&Number(advanceHours)<=10000;
 function chooseTime(){if(!validTime)return;const [hour,minute]=time.split(':').map(Number);void change({operation:'choose_time',time:{hour,minute,second:0,millisecond:0}})}
 function setWeatherValue(key:keyof WeatherChoice,value:number){setWeather(previous=>({...previous,[key]:value}))}
 return <div className="market-world-controls" aria-label="Live time and weather controls">
  {!enabled&&<p className="world-notice">Install this mod with the game closed, then start the game and load a save to use the live controls.</p>}
  <fieldset disabled={locked||!timeAvailable}>
   <legend><Clock3 size={15}/> Time of day</legend>
   <div className="world-current">{timeAvailable&&snapshot?`Current ${displayTime(snapshot.time)} · Day ${snapshot.time.day} · ${timeFrozen?'Frozen':'Running'}`:snapshot?worldReason(snapshot.reason):'Waiting for the live clock…'}</div>
   <label className="world-time-input">Choose time<input aria-label="Choose time of day" type="time" step={60} value={time} onChange={e=>setTime(e.target.value)}/></label>
   <div className="mod-actions"><button className="secondary" disabled={!validTime} onClick={chooseTime}>Set time</button><button className="secondary" onClick={()=>void change(timeFrozen?{operation:'release_time'}:{operation:'freeze_time',enabled:true})}>{timeFrozen?'Resume time':'Freeze time'}</button></div>
   <span className="market-hint">Set the hour on the current game day. Freezing time stops the clock; characters and weather keep moving.</span>
   <label className="world-time-input">Hours to advance<input aria-label="Hours to advance" type="number" min={1} max={10000} step={1} value={advanceHours} onChange={e=>setAdvanceHours(e.target.value)}/></label>
   <button className="secondary" disabled={!validHours} onClick={()=>void change({operation:'advance_time',hours:Number(advanceHours)})}>Advance time</button>
   <span className="market-hint">Move forward by 1–10,000 game hours, including day changes. A frozen clock stays frozen; weather settings are kept.</span>
  </fieldset>
  <fieldset disabled={locked||!weatherAvailable}>
   <legend><CloudSun size={15}/> Weather</legend>
   <div className="world-current">{weatherAvailable?(weatherFrozen?'Weather frozen':'Live weather connected'):'Weather controls are waiting for the active world.'}</div>
   <label>Precipitation<select aria-label="Precipitation type" value={weather.phase} onChange={e=>setWeatherValue('phase',Number(e.target.value))}><option value={0}>Rain</option><option value={1}>Snow</option></select></label>
   {weatherFields.map(field=><div className="world-slider" key={field.key}><label htmlFor={`live-weather-${field.key}`}>{field.label}</label><output htmlFor={`live-weather-${field.key}`}>{field.percent?`${Math.round(weather[field.key]*100)}%`:weather[field.key].toFixed(field.decimals)}</output><input id={`live-weather-${field.key}`} type="range" min={field.min} max={field.max} step={field.step} value={weather[field.key]} onChange={e=>setWeatherValue(field.key,Number(e.target.value))}/></div>)}
   <div className="mod-actions"><button className="secondary" disabled={!validWeather(weather)} onClick={()=>void change({operation:'choose_weather',weather})}>Apply weather</button><button className="secondary" onClick={()=>void change({operation:'freeze_weather',enabled:!weatherFrozen})}>{weatherFrozen?'Unfreeze weather':'Freeze weather'}</button></div>
   <button className="secondary" title="Disable the Workbench weather override and restore the previous game weather control. Your time setting is kept." onClick={()=>void change({operation:'release_weather'})}>Reset weather</button>
   <span className="market-hint">Applied weather stays selected. Freeze also holds its climate values; wind and particle animations can still move. Reset weather disables this weather override and restores the previous game weather control. Your time setting is kept.</span>
  </fieldset>
  <div className="world-feedback">
   <button className="secondary" disabled={!enabled||busy} onClick={()=>void refresh()}>{busy?<LoaderCircle size={14} className="spin"/>:<RefreshCw size={14}/>} Check live status</button>
   {pending&&uncertain&&<button className="secondary" disabled={busy||!!historyError} onClick={()=>{finishPending();setMessage('Previous command dismissed. It will not be sent again.')}}>I checked the result in game</button>}
   {message&&<p role="status">{message}</p>}{error&&<p role="alert" className="craft-error">{error}</p>}{historyError&&<p role="alert" className="craft-error">{historyError}</p>}
  </div>
 </div>;
}
