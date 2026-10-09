import type { RawField } from './types';
export const count = (value: number) => value.toLocaleString('de-DE');
export function integer(value: string | number): string {
  try { return BigInt(value).toLocaleString('de-DE'); } catch { return String(value); }
}
export function plainText(text: string): string { return text.replace(/<[^>]*>/g, '').replace(/\\n/g, '\n').trim(); }
export function fieldValue(value: unknown): string { return typeof value === 'string' ? value : JSON.stringify(value) ?? '—'; }
export const hex = (n: number) => `0x${n.toString(16).toUpperCase().padStart(8, '0')}`;
export function filterFields(fields: RawField[], text: string, onlyUnknown: boolean): RawField[] {
  const needle = text.trim().toLocaleLowerCase();
  return fields.filter(f => (!onlyUnknown || f.interpretation === 'unknown') && (!needle || `${f.path} ${f.type_name} ${fieldValue(f.value)} ${hex(f.start)}`.toLocaleLowerCase().includes(needle)));
}
export const languageNames: Record<string, string> = { ara: 'العربية', eng: 'English', fre: 'Français', ger: 'Deutsch', ita: 'Italiano', jpn: '日本語', kor: '한국어', pol: 'Polski', 'por-br': 'Português (BR)', rus: 'Русский', 'spa-es': 'Español (ES)', 'spa-mx': 'Español (MX)', tur: 'Türkçe', 'zho-cn': '简体中文', 'zho-tw': '繁體中文' };
