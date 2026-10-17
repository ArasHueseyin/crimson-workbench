import {describe,expect,it} from 'vitest';
import {discardStartupDraft,parseStartupDrafts,readStartupDrafts,reconcileStartupDraft,startupDraftStorageKey,updateStartupDraft,validStartupDraft} from './startup-draft-store';
function storage(){const values=new Map<string,string>();return {getItem:(key:string)=>values.get(key)??null,setItem:(key:string,value:string)=>{values.set(key,value)},removeItem:(key:string)=>{values.delete(key)}}}
describe('startup choices remain local drafts until confirmed saved',()=>{
 it('keeps independent drafts through refresh and clears only confirmed or discarded choices',()=>{
  const disk=storage();updateStartupDraft('player_jump',175,100,disk);updateStartupDraft('glide_speed',275,100,disk);
  expect(reconcileStartupDraft('player_jump',100,disk)).toBe(175);
  expect(reconcileStartupDraft('glide_speed',100,disk)).toBe(275);
  expect(reconcileStartupDraft('player_jump',175,disk)).toBe(175);
  expect(readStartupDrafts(disk)).toEqual({glide_speed:275});
  discardStartupDraft('glide_speed',disk);expect(disk.getItem(startupDraftStorageKey)).toBeNull();
 });
 it('sorts and preserves consumable selection without mutating caller data',()=>{
  const disk=storage(),keys=[300,100];updateStartupDraft('preserve_consumables',keys,[],disk);
  expect(keys).toEqual([300,100]);expect(reconcileStartupDraft('preserve_consumables',[],disk)).toEqual([100,300]);
  updateStartupDraft('player_resources',0,1,disk);expect(readStartupDrafts(disk).player_resources).toBe(0);
  updateStartupDraft('preserve_consumables',[100,300],[100,300],disk);
  expect(readStartupDrafts(disk)).toEqual({player_resources:0});
 });
 it('rejects bad schemas, out-of-range values, invalid Axiom gap and unknown keys',()=>{
  expect(parseStartupDrafts('bad json')).toEqual({});expect(parseStartupDrafts('{"version":2,"values":{"glide_speed":300}}')).toEqual({});
  expect(parseStartupDrafts(JSON.stringify({version:1,values:{player_jump:301,player_speed:99,glide_speed:501,axiom_reach:150,player_resources:4,force_palm_height:10001,preserve_consumables:[1,1],unknown:123}}))).toEqual({});
  for(const value of [100,200,233,500])expect(validStartupDraft('axiom_reach',value)).toBe(true);
  for(const value of [99,101,199,501,NaN,Infinity])expect(validStartupDraft('axiom_reach',value)).toBe(false);
 });
 it('retains a failed-save draft and does not publish invalid edits',()=>{
  const disk=storage();updateStartupDraft('axiom_reach',270,100,disk);updateStartupDraft('axiom_reach',150,100,disk);
  expect(reconcileStartupDraft('axiom_reach',100,disk)).toBe(270);
  updateStartupDraft('player_jump',100,100,disk);expect(readStartupDrafts(disk)).toEqual({axiom_reach:270});
 });
 it('keeps memory authoritative after quota failure and synchronizes when storage recovers',()=>{
  let reject=true,raw:string|null=null;
  const disk={getItem:()=>raw,setItem:(_key:string,value:string)=>{if(reject)throw Error('quota');raw=value},removeItem:()=>{if(reject)throw Error('denied');raw=null}};
  updateStartupDraft('player_jump',175,100,disk);expect(readStartupDrafts(disk)).toEqual({player_jump:175});
  updateStartupDraft('glide_speed',275,100,disk);expect(readStartupDrafts(disk)).toEqual({player_jump:175,glide_speed:275});
  reject=false;updateStartupDraft('player_jump',200,100,disk);
  expect(parseStartupDrafts(raw)).toEqual({player_jump:200,glide_speed:275});
  reject=true;discardStartupDraft('player_jump',disk);discardStartupDraft('glide_speed',disk);
  expect(readStartupDrafts(disk)).toEqual({});
  reject=false;updateStartupDraft('player_resources',3,1,disk);expect(parseStartupDrafts(raw)).toEqual({player_resources:3});
 });
});
