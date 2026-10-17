import type {SaveChoice} from './mount-types';

export function saveIdentity(save:SaveChoice):string {
  const parts=save.id.split(/[\\/]/).filter(Boolean);
  return parts.length>1?`${save.label} · ${parts.slice(1).join('/')} · Account ${parts[0]}`:save.label;
}
export function saveDate(modified:number):string {
  return Number.isFinite(modified)&&modified>0?new Date(modified*1000).toLocaleString(undefined,{dateStyle:'medium',timeStyle:'short'}):'Date unavailable';
}
export function newestSave(saves:SaveChoice[]):string|null {
  return saves.reduce<SaveChoice|null>((latest,save)=>!latest||save.modified>latest.modified?save:latest,null)?.id??null;
}
