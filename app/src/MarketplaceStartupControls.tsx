import {useEffect,useRef,useState} from 'react';
import type {RuntimeOptionId,RuntimeOptionSettings} from './marketplace-types';

const labels={force_palm_height:'Force Palm height',axiom_reach:'Axiom reach',player_resources:'Player resources',glide_speed:'Gliding speed'};
export function RuntimeNumericInput({label,value,min,max,unit,disabled,change,valid=()=>true,onValidity}:{label:string;value:number;min:number;max:number;unit:'%'|'×';disabled:boolean;change:(value:number)=>void;valid?:(value:number)=>boolean;onValidity?:(valid:boolean)=>void}) {
 const divisor=unit==='×'?100:1,[text,setText]=useState(String(value/divisor)),[error,setError]=useState(false),focused=useRef(false);
 useEffect(()=>{if(!focused.current){setText(String(value/divisor));setError(false);onValidity?.(true)}},[value,divisor]);
 function edit(raw:string) {
  setText(raw);const parsed=Number(raw)*divisor;
  const okay=raw.trim()!==''&&Math.abs(parsed-Math.round(parsed))<0.000001&&parsed>=min&&parsed<=max&&valid(Math.round(parsed));
  setError(!okay);onValidity?.(okay);if(okay)change(Math.round(parsed));
 }
 return <span className="runtime-precision"><label>{label}<input type="number" aria-label={label} aria-invalid={error||undefined} min={min/divisor} max={max/divisor} step={1/divisor} value={text} disabled={disabled} onFocus={()=>{focused.current=true}} onBlur={()=>{focused.current=false;if(error){setText(String(value/divisor));setError(false);onValidity?.(true)}}} onChange={e=>edit(e.target.value)}/>{unit}</label>{error&&<span role="alert">{unit==='×'&&min===100&&max===500?'Choose 1× or a value from 2× to 5×.':`Choose ${min/divisor}–${max/divisor}${unit}.`} Invalid input is not applied.</span>}</span>;
}
export function MarketplaceStartupControls({id,settings,value,disabled,change,save,discard,onValidityChange,unavailable=false}:{id:RuntimeOptionId;settings:RuntimeOptionSettings|null;value:number;disabled:boolean;change:(value:number)=>void;save:()=>void;discard?:()=>void;onValidityChange?:(valid:boolean)=>void;unavailable?:boolean}) {
 const [numericValid,setNumericValid]=useState(true);
 const dirty=!!settings&&(settings.invalid||settings.value!==value);
 const resources=id==='player_resources',normal=value===100,axiom=id==='axiom_reach';
 const minimum=id==='axiom_reach'?200:100,maximum=id==='force_palm_height'?10000:500,step=1;
 const inputId=`runtime-option-${id}`;
 const unit=id==='glide_speed'?'%':'×',presets=id==='force_palm_height'?[100,200,500,1000,2500,5000,10000]:axiom?[100,200,300,400,500]:[100,150,200,300,500];
 return <div className="market-jump-settings">
  {resources?<>
   <label className="mod-switch"><input aria-label="Unlimited player stamina" type="checkbox" disabled={disabled} checked={(value&1)!==0} onChange={e=>change(e.target.checked?value|1:value&~1)}/>Unlimited player stamina</label>
   <label className="mod-switch"><input aria-label="Unlimited player spirit" type="checkbox" disabled={disabled} checked={(value&2)!==0} onChange={e=>change(e.target.checked?value|2:value&~2)}/>Unlimited player spirit</label>
   <span className="market-hint">Choose either resource or both. You still need enough spirit for the game’s normal skill eligibility checks.</span>
  </>:<>
   <label htmlFor={inputId}>{labels[id]} <output htmlFor={inputId}>{id==='glide_speed'?`${value}% · `:''}{(value/100).toLocaleString('en-US',{maximumFractionDigits:2})}×{normal?' · Normal':''}</output></label>
   {axiom&&<div className="runtime-normal-mode" role="group" aria-label="Axiom reach mode">
    <label><input type="radio" name={inputId+'-mode'} checked={normal} disabled={disabled} onChange={()=>change(100)}/>Normal reach · 1×</label>
    <label><input type="radio" name={inputId+'-mode'} checked={!normal} disabled={disabled} onChange={()=>change(200)}/>Extended reach · 2×–5×</label>
   </div>}
   <input id={inputId} aria-label={`${labels[id]} multiplier`} aria-valuetext={axiom&&normal?'2 times enhanced reach; Normal 1 times is selected. Moving this slider enables extended reach.':`${value/100} times${normal?', Normal':''}`} type="range" min={minimum} max={maximum} step={step} value={Math.max(minimum,value)} disabled={disabled} onChange={e=>change(Number(e.target.value))}/>
   {axiom&&normal&&<span className="market-hint">Normal 1× is selected. The extended-range slider starts at 2×; moving it enables extended reach.</span>}
   <RuntimeNumericInput label={`${labels[id]} exact value`} value={value} min={100} max={maximum} unit={unit} disabled={disabled} valid={v=>!axiom||v===100||v>=200} onValidity={okay=>{setNumericValid(okay);onValidityChange?.(okay)}} change={change}/>
   <div className="runtime-presets" role="group" aria-label={`${labels[id]} presets`}>{presets.map(preset=><button type="button" className="secondary" key={preset} disabled={disabled} aria-pressed={value===preset} onClick={()=>change(preset)}>{preset===100?'Normal':unit==='%'?`${preset}%`:`${preset/100}×`}</button>)}</div>
  </>}
  <div className="mod-actions">
   <button className="secondary" disabled={disabled||!dirty||!numericValid} onClick={save}>Save {labels[id].toLowerCase()}</button>
   {id==='axiom_reach'&&normal?<button className="secondary" disabled={disabled} onClick={()=>change(200)}>Use 2× reach</button>:!resources&&<button className="secondary" disabled={disabled||normal} onClick={()=>change(100)}>Reset to normal</button>}
   {dirty&&discard&&<button className="secondary draft-discard" disabled={disabled} onClick={discard}>Discard changes</button>}
  </div>
  <span className="market-hint">{settings?.message??(unavailable?'Startup settings unavailable. Your draft is retained; refresh runtime status to retry.':'Loading startup settings…')}{dirty?' Unsaved settings. Kept on this device until saved or discarded.':''}</span>
 </div>;
}
