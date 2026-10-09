import {describe,it,expect} from 'vitest';
import {canonical,validItemEdit,emptyItemEdit} from './advanced-types';
describe('advanced request identity and local templates',()=>{
 it('canonicalizes nested maps without reordering arrays or conflating values',()=>{
  expect(canonical({advanced:{fields:{b:'1',a:'2'}},unused:undefined})).toBe(canonical({advanced:{fields:{a:'2',b:'1'}}}));
  expect(canonical(['1','2'])).not.toBe(canonical(['2','1']));expect(canonical('1')).not.toBe(canonical(1));
 });
 it('rejects malformed or out of range templates before rendering',()=>{
  expect(validItemEdit(emptyItemEdit)).toBe(true);expect(validItemEdit({})).toBe(false);expect(validItemEdit({...emptyItemEdit,stats:[null]})).toBe(false);
  expect(validItemEdit({...emptyItemEdit,stats:[{enchant_level:0,list:'per_level',stat:1000000,value:'128'}]})).toBe(false);
  expect(validItemEdit({...emptyItemEdit,stats:[{enchant_level:0,list:'static',stat:1000000,value:'1000000001'}]})).toBe(false);
  expect(validItemEdit({...emptyItemEdit,buffs:[{enchant_level:0,buff:4,level:0}]})).toBe(false);
 });
 it('accepts old templates and validates new enchant targets',()=>{
  expect(canonical(emptyItemEdit)).toBe(canonical({...emptyItemEdit,enchant_copies:[]}));
  expect(validItemEdit({...emptyItemEdit,enchant_copies:[{source_level:0,target_level:5}]})).toBe(true);
  for(const enchant_copies of [[null],[{source_level:0,target_level:0}],[{source_level:0,target_level:65536}],[{source_level:0,target_level:2},{source_level:1,target_level:2}]])expect(validItemEdit({...emptyItemEdit,enchant_copies})).toBe(false);
 });
});
