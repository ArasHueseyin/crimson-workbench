import type {SaveChoice} from './mount-types';
export interface KnowledgeStatus {key:number;total:number;learned:number;unknown:number;state:'learned'|'missing'|'partial'|'unknown'}
export interface KnowledgeSnapshot {saves:SaveChoice[];selected_save:string|null;modified:number|null;items:KnowledgeStatus[];message:string}
export function knowledgeLabel(status?:KnowledgeStatus):string {
  if(!status||status.state==='unknown')return 'Knowledge: unknown';
  if(status.state==='learned')return 'Knowledge already acquired';
  if(status.state==='missing')return 'Knowledge not yet acquired';
  return `Knowledge partially acquired (${status.learned}/${status.total})`;
}
