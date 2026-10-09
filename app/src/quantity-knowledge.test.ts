import {expect,it} from 'vitest';
import {grantQuantity,maximumNewStacks} from './quantity';
import {knowledgeLabel} from './knowledge-types';
it('quantity accepts empty editing and pasted digits without admitting fractions or exponents',()=>{
  for(const raw of ['','0','-1','1.5','1e2','10001','abc'])expect(grantQuantity(raw)).toBeNull();
  expect(grantQuantity('100')).toBe(100);expect(grantQuantity(' 100 ')).toBe(100);expect(maximumNewStacks(100,'10')).toBe(10);expect(maximumNewStacks(101,'10')).toBe(11);expect(maximumNewStacks(100,'18446744073709551615')).toBe(1);
});
it('knowledge reports partial and unknown separately from learned',()=>{
  expect(knowledgeLabel({key:1,total:2,learned:1,unknown:0,state:'partial'})).toContain('(1/2)');expect(knowledgeLabel()).toBe('Wissen: unbekannt');
});
