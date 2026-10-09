import { describe,it,expect } from 'vitest';
import { filterMounts,type Mount } from './mount-types';
const rows:Mount[]=[{key:30108,name:'Schwarzbär',internal:'Animal_Black_Bear',family:'Bären',vehicle:16979,owned:0,supported:true,reason:'',description:''},{key:31378,name:'Rokade',internal:'Riding_Rokade',family:'Pferde',vehicle:16960,owned:1,supported:false,reason:'',description:''}];
describe('mount selection',()=>{
  it('searches German names, internal keys and exact IDs',()=>{for(const q of ['SCHWARZBÄR','30108','black_bear'])expect(filterMounts(rows,q,'',false).map(m=>m.key)).toEqual([30108]);});
  it('combines species and availability without hiding owned entries by default',()=>{expect(filterMounts(rows,'','Pferde',false)).toHaveLength(1);expect(filterMounts(rows,'','Pferde',true)).toHaveLength(0);expect(filterMounts(rows,'','',true)).toHaveLength(1);});
});
