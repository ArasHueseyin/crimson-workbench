import type {ModChoice} from './mod-types';
export type StatList='maximum'|'regeneration'|'static'|'per_level';
export interface StatEdit {enchant_level:number;list:StatList;stat:number;value:string}
export interface BuffEdit {enchant_level:number;buff:number;level:number}
export interface EnchantCopy {source_level:number;target_level:number}
export interface SkillValue {path:string;offset:number;bytes:number;kind:string;value:string;editable:boolean}
export interface SkillBuff {matrix_level:number;index:number;type_id:number|null;name:string;opaque_layout:boolean;fields:SkillValue[]}
export interface AdvancedSkill {key:number;name:string;byte_len:number;matrix_start:number;matrix_end:number;level_counts:number[];buffs:SkillBuff[];record_fields:SkillValue[];raw_hex:string}
export interface ItemEdit {stack_size:number|null;free_repair:boolean;stats:StatEdit[];buffs:BuffEdit[];enchant_copies?:EnchantCopy[]}
export const emptyItemEdit:ItemEdit={stack_size:null,free_repair:false,stats:[],buffs:[]};
export interface AdvancedRequest {
 spawn_percent:number;patrol_reset_percent?:number;reoccupation_delay_percent?:number;mount_cooldown:number|null;mount_duration:number|null;town_running:boolean;dragon_no_cooldown:boolean;dragon_regions:boolean;dragon_duration:number|null;
 stack_size:number|null;stack_categories:number[];experimental_stacks:boolean;no_wear:boolean;free_repair:boolean;
 cost_percent:Record<string,number>;skill_cooldown_percent:number;skill_cooldowns:Record<number,number>;skill_buffs?:Record<number,Record<string,string>>;fields:Record<string,string>;items:Record<number,ItemEdit>;
}
export const emptyAdvanced:AdvancedRequest={spawn_percent:100,mount_cooldown:null,mount_duration:null,town_running:false,dragon_no_cooldown:false,dragon_regions:false,dragon_duration:null,stack_size:null,stack_categories:[],experimental_stacks:false,no_wear:false,free_repair:false,cost_percent:{},skill_cooldown_percent:100,skill_cooldowns:{},fields:{},items:{}};
export interface AdvancedField {name:string;value:string;min:string;max:string;rule:string}
export interface AdvancedRecord {table:string;key:number;name:string;module:string;display:string;description:string;category:string;fields:AdvancedField[]}
export interface AdvancedInfo {records:AdvancedRecord[];statuses:ModChoice[];categories:ModChoice[];repair_items:number;stack_safe_items:number;finite_endurance_items:number;limitations:string[]}
export interface AdvancedItem {item:{key:number;category:number;equip_type:number;stack_size:string;stack_risk:boolean;max_endurance:number;repair_entries:number;enchant_levels:number[];can_copy_enchants:boolean;stats:StatEdit[];buffs:BuffEdit[]};compatible_buffs:[number,number][]}
export function canonical(value:unknown):string {
 if(Array.isArray(value))return `[${value.map(canonical).join(',')}]`;
 if(value&&typeof value==='object')return `{${Object.entries(value).filter(([k,v])=>v!==undefined&&!(k==='enchant_copies'&&Array.isArray(v)&&v.length===0)).sort(([a],[b])=>a.localeCompare(b)).map(([k,v])=>`${JSON.stringify(k)}:${canonical(v)}`).join(',')}}`;
 return JSON.stringify(value);
}
export function validItemEdit(value:unknown):value is ItemEdit {
 if(!value||typeof value!=='object')return false;
 const e=value as ItemEdit;const integer=(n:unknown,min:number,max:number)=>typeof n==='number'&&Number.isSafeInteger(n)&&n>=min&&n<=max;
 if(e.enchant_copies!==undefined&&(!Array.isArray(e.enchant_copies)||e.enchant_copies.length>64||!e.enchant_copies.every(c=>c&&integer(c.source_level,0,65535)&&integer(c.target_level,0,65535)&&c.source_level!==c.target_level)||new Set(e.enchant_copies.map(c=>c.target_level)).size!==e.enchant_copies.length))return false;
 return (e.stack_size===null||integer(e.stack_size,1,1000000))&&typeof e.free_repair==='boolean'&&Array.isArray(e.stats)&&e.stats.length<=256&&e.stats.every(s=>s&&integer(s.enchant_level,0,65535)&&['maximum','regeneration','static','per_level'].includes(s.list)&&integer(s.stat,1,4294967295)&&typeof s.value==='string'&&/^-?\d+$/.test(s.value)&&integer(Number(s.value),s.list==='per_level'?-128:-1000000000,s.list==='per_level'?127:1000000000))&&Array.isArray(e.buffs)&&e.buffs.length<=64&&e.buffs.every(b=>b&&integer(b.enchant_level,0,65535)&&integer(b.buff,1,4294967295)&&integer(b.level,1,100));
}
