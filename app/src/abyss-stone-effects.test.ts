import {describe,it,expect} from 'vitest';
import {stoneBonuses,stoneEffects,stoneDescription,type StoneDetail} from './abyss-stone-effects';
const stone=(stats:StoneDetail['stats']=[],buffs:StoneDetail['buffs']=[]):StoneDetail=>({key:1002787,name:'Test',description:'',category:2501,equip_type:0,stack_size:'50',stack_risk:false,max_endurance:65535,repair_entries:0,enchant_levels:[0],can_copy_enchants:false,stats,buffs});
describe('socket stone effects',()=>{
  it('converts attack/defence thousandths and hides placeholder zeros',()=>{
    expect(stoneBonuses(stone([{enchant_level:0,list:'static',stat:1000002,value:'3000'},{enchant_level:0,list:'static',stat:1000003,value:'0'}]))).toEqual([{label:'Angriff',value:'+3'}]);
    expect(stoneBonuses(stone([{enchant_level:0,list:'static',stat:1000003,value:'6000'}]))).toEqual([{label:'Verteidigung',value:'+6'}]);
  });
  it('keeps speed/critical bonuses in levels, without inventing percentages',()=>{
    expect(stoneBonuses(stone([{enchant_level:0,list:'per_level',stat:1000010,value:'3'},{enchant_level:0,list:'per_level',stat:1000007,value:'1'}]))).toEqual([{label:'Angriffsgeschwindigkeit',value:'+3 Stufen'},{label:'Kritische Trefferchance',value:'+1 Stufe'}]);
  });
  it('does not mix enchant levels or silently interpret unknown units',()=>{
    const detail=stone([{enchant_level:0,list:'static',stat:42,value:'1500'},{enchant_level:10,list:'static',stat:1000002,value:'99000'}]);
    expect(stoneBonuses(detail)).toEqual([{label:'Stat-ID 42 (static)',value:'+1.500 Rohwert'}]);
  });
  it('explains known buff effects and strength tiers',()=>{
    expect(stoneEffects({description:''},stone([],[{enchant_level:0,buff:1000008,level:3}]))).toEqual(['Regeneriert Gesundheit · Effektstufe 3']);
  });
  it('uses localized ability descriptions and preserves paragraph breaks as plain text',()=>{
    const item={description:'Allgemeiner Text.<br/><br/>Flamme: <b>Feuerschaden</b>.'};
    expect(stoneDescription(item)).toBe('Allgemeiner Text.\n\nFlamme: Feuerschaden.');
    expect(stoneEffects(item,stone([],[{enchant_level:0,buff:1000120,level:1},{enchant_level:0,buff:1000120,level:1}]))).toEqual(['Flamme: Feuerschaden.']);
  });
  it('makes missing effect semantics explicit rather than treating a buff as a number',()=>{
    expect(stoneEffects({description:''},stone([],[{enchant_level:0,buff:42,level:2}]))).toEqual(['Spezialeffekt · Effektstufe 2 (Buff-ID 42)']);
    expect(stoneBonuses(undefined)).toEqual([]);expect(stoneEffects({description:''},undefined)).toEqual([]);
  });
});
