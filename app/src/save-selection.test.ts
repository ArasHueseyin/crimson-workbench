import {describe,expect,it} from 'vitest';
import {newestSave,saveDate,saveIdentity} from './save-selection';

describe('save selection identity',()=>{
  it('distinguishes equal slot names in different accounts and handles either path separator',()=>{
    expect(saveIdentity({id:'account-a/slot0',label:'Manual save 0',modified:100})).toBe('Manual save 0 · slot0 · Account account-a');
    expect(saveIdentity({id:'account-b\\slot0',label:'Manual save 0',modified:100})).toBe('Manual save 0 · slot0 · Account account-b');
  });
  it('finds the newest save independently of API ordering',()=>{
    expect(newestSave([{id:'old',label:'Old',modified:1},{id:'new',label:'New',modified:2}])).toBe('new');
    expect(newestSave([])).toBeNull();
  });
  it('shows local dates and makes missing timestamps explicit',()=>{
    expect(saveDate(100)).toBe(new Date(100000).toLocaleString(undefined,{dateStyle:'medium',timeStyle:'short'}));
    expect(saveDate(0)).toBe('Date unavailable');
    expect(saveDate(NaN)).toBe('Date unavailable');
  });
});
