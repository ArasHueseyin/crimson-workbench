import type {SaveChoice} from './mount-types';
export interface KnowledgeStatus {key:number;total:number;learned:number;unknown:number;state:'learned'|'missing'|'partial'|'unknown'}
export interface KnowledgeSnapshot {saves:SaveChoice[];selected_save:string|null;modified:number|null;items:KnowledgeStatus[];message:string}
export function knowledgeLabel(status?:KnowledgeStatus):string {
  if(!status||status.state==='unknown')return 'Wissen: unbekannt';
  if(status.state==='learned')return 'Wissen bereits erlangt';
  if(status.state==='missing')return 'Wissen noch nicht erlangt';
  return `Wissen teilweise erlangt (${status.learned}/${status.total})`;
}
