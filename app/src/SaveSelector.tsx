import type {SaveChoice} from './mount-types';
import {newestSave,saveDate,saveIdentity} from './save-selection';

interface Props {
  saves:SaveChoice[];value:string|null;onChange:(value:string)=>void;label?:string;ariaLabel:string;disabled?:boolean;
}
export function SaveSelector({saves,value,onChange,label='Save slot',ariaLabel,disabled=false}:Props) {
  const newest=newestSave(saves);
  return <label className="save-selector source-field">{label}<select aria-label={ariaLabel} value={value??''} disabled={disabled||!saves.length} onChange={event=>onChange(event.target.value)}>
    <option value="">{saves.length?'Choose a save':'No complete saves found'}</option>
    {saves.map(save=><option key={save.id} value={save.id}>{saveIdentity(save)} · {saveDate(save.modified)}{save.id===newest?' · newest':''}</option>)}
  </select></label>;
}
export function SaveTarget({saves,value,label='Changes apply to'}:{saves:SaveChoice[];value:string|null;label?:string}) {
  const selected=saves.find(save=>save.id===value);
  return <p className="save-target"><strong>{label}</strong><span>{selected?saveIdentity(selected):value??'Choose a save first'}</span>{selected&&<time dateTime={Number.isFinite(selected.modified)&&selected.modified>0?new Date(selected.modified*1000).toISOString():undefined}>{saveDate(selected.modified)}</time>}</p>;
}
