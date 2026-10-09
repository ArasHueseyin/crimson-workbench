import { describe, expect, it } from 'vitest';
import { filterFields, integer, plainText } from './format';
import { useCatalog } from './store';
import type { RawField } from './types';
describe('lossless display and field search', () => {
  it('formats u64 without floating point rounding', () => {
    expect(integer('18446744073709551615')).toBe('18.446.744.073.709.551.615');
    expect(integer('9007199254740993')).toBe('9.007.199.254.740.993');
  });
  it('displays game markup as plain text, never HTML', () => {
    expect(plainText('<PAColor>Öl</PAColor>\\nMaterial')).toBe('Öl\nMaterial');
    expect(plainText('<img src=x onerror="boom">Text')).toBe('Text');
  });
  it('intersects unknown status with path/value/offset searches', () => {
    const fields: RawField[] = [{ path: 'unk_flag', start: 32, end: 33, type_name: 'u8', raw_hex: '01', value: 1, interpretation: 'unknown' }, { path: 'key', start: 0, end: 4, type_name: 'ItemKey', raw_hex: '98080000', value: 2200, interpretation: 'upstream_named' }];
    expect(filterFields(fields, '0x00000020', true)).toEqual([fields[0]]);
    expect(filterFields(fields, '2200', true)).toEqual([]);
    expect(filterFields(fields, '2200', false)).toEqual([fields[1]]);
  });
});
it('resets filters and selection on a new catalog', () => {
  useCatalog.getState().setQuery({ text: 'Stumpfpfeil', category: 3, descending: true });
  useCatalog.getState().select(2200);
  useCatalog.getState().reset();
  expect(useCatalog.getState().query.text).toBe('');
  expect(useCatalog.getState().query.category).toBeNull();
  expect(useCatalog.getState().selected).toBeNull();
});
