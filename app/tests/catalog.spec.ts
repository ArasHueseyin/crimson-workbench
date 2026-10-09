import { expect, test, type Page } from '@playwright/test';
async function fixture(page: Page, mode: 'normal' | 'unknown' | 'empty' = 'normal') {
  await page.addInitScript(({ mode }) => {
    // Synthetic fixtures are tests only, never a production browser fallback.
    const root = window as unknown as { isTauri: boolean; __TAURI_INTERNALS__: unknown };
    root.isTauri = true;
    let session = 1;
    const items = Array.from({ length: 1000 }, (_, i) => ({ key: 2200 + i, name: i === 0 ? 'Test-Stumpfpfeil' : `Testgegenstand ${String(i).padStart(4, '0')}`, internal_key: `Synthetic_Item_${i}`, description: 'Synthetische Testbeschreibung.', item_type: i % 2, category: i % 3, tier: i % 4, max_stack: i === 0 ? '18446744073709551615' : String(i % 2 ? 1 : 99), stat_keys: [1234], icon_key: null }));
    root.__TAURI_INTERNALS__ = { invoke: async (command: string, args: Record<string, unknown> = {}) => {
      if (command === 'bootstrap') return { project: 'C:/Testprojekt', discovery: { installations: [{ path: 'C:/Synthetic Game', platform: 'steam', build_id: 'synthetic' }], configured_game: 'C:/Synthetic Game', save_directories: [] }, languages: [{ language: 'ger' }, { language: 'eng' }] };
      if (command === 'open_catalog') {
        session++; if (mode === 'unknown') throw { code: 'unsupported_build', message: 'Synthetic hash mismatch' };
        return { session, game_path: args.game, info: { item_count: 1000, types: [{ value: 0, count: 500 }, { value: 1, count: 500 }], categories: [{ value: 0, count: 334 }, { value: 1, count: 333 }, { value: 2, count: 333 }], tiers: [{ value: 0, count: 250 }, { value: 1, count: 250 }], stats: [{ value: 1234, count: 1000 }], exe_version: 'synthetic', index: { path: 'C:/Testprojekt/.local/test.sqlite', rebuilt: true, items: 1000, language: args.language, fingerprint: 'synthetic' } } };
      }
      if (command === 'search_items') {
        const q = args.query as { text: string; item_type: number | null; category: number | null; tier: number | null; stackable: boolean; offset: number; sort: string; descending: boolean };
        if (q.text === 'langsam') await new Promise(resolve => setTimeout(resolve, 700));
        const all = mode === 'empty' ? [] : items.filter(i => i.name.toLowerCase().includes(q.text.toLowerCase()) && (q.item_type === null || i.item_type === q.item_type) && (q.category === null || i.category === q.category) && (q.tier === null || i.tier === q.tier) && (!q.stackable || i.max_stack !== '1'));
        if (q.descending) all.reverse();
        return { items: all.slice(q.offset, q.offset + 200), total: all.length, offset: q.offset };
      }
      if (command === 'craft_item') return { recipes: [], used_in: [] };
      if (command === 'item_icon') return { data_url: null, source: null, reason: 'Synthetic fixture without game assets' };
      if (command === 'item_detail') {
        const item = items.find(i => i.key === args.key)!;
        return { name: item.name, description: item.description, language: 'ger', name_resolved: true, record: { key: item.key, string_key: item.internal_key, item_type: item.item_type, item_tier: item.tier, category_info: item.category, max_stack_count: item.max_stack, icon_path: null, stat_keys: [1234], inventory_info_list: [], offset: 0, length: 400 }, fields: Array.from({ length: 120 }, (_, i) => ({ path: i % 2 ? `unk_field_${i}` : `field_${i}`, start: i * 4, end: (i + 1) * 4, type_name: 'u32', raw_hex: '01000000', value: i, interpretation: i % 2 ? 'unknown' : 'upstream_named' })), item_references: [{ path: 'packed_item_info', key: 2201, name: 'Testgegenstand 0001', available: true }] };
      }
      if (command === 'export_item') return `C:/Testprojekt/exports/item-${args.key}.json`;
      throw new Error(`Unexpected command ${command}`);
    } };
  }, { mode });
  await page.goto('/');
}
test('search, exact numbers, fields, references and export form one flow', async ({ page }) => {
  await fixture(page);
  await expect(page.locator('.workbench')).toHaveAttribute('data-ready', 'true');
  await page.getByRole('textbox', { name: 'Items suchen' }).fill('Stumpfpfeil');
  await expect(page.getByRole('option', { name: /Test-Stumpfpfeil/ })).toBeVisible();
  await page.getByRole('option', { name: /Test-Stumpfpfeil/ }).click();
  await expect(page.getByRole('heading', { name: 'Test-Stumpfpfeil' })).toBeVisible();
  await expect(page.locator('.values-grid')).toContainText('18.446.744.073.709.551.615');
  await page.getByRole('tab', { name: /Alle Felder/ }).click();
  await page.getByLabel('Nur unbekannte Felder').check();
  await page.getByRole('textbox', { name: 'Rohfelder durchsuchen' }).fill('unk_field_3');
  await expect(page.locator('.raw-field').first()).toContainText('unk_field_3');
  await page.getByRole('tab', { name: 'Verknüpfungen' }).click();
  await page.getByRole('button', { name: /packed_item_info/ }).click();
  await expect(page.getByRole('heading', { name: 'Testgegenstand 0001' })).toBeVisible();
  await page.getByRole('button', { name: 'Item als JSON speichern' }).click();
  await expect(page.getByRole('status')).toContainText('exports/item-2201.json');
});
test('virtualized list, pagination and intersecting filters', async ({ page }) => {
  await fixture(page);
  await expect(page.locator('.result-caption')).toContainText('1.000');
  expect(await page.locator('.item-row').count()).toBeLessThan(40);
  await page.locator('.table-scroll').evaluate(el => { el.scrollTop = el.scrollHeight; });
  await expect(page.locator('.table-footer')).toContainText('400 von 1.000');
  await page.getByLabel('Typ', { exact: true }).selectOption('0');
  await page.getByLabel('Kategorie', { exact: true }).selectOption('1');
  await expect(page.locator('.result-caption')).toContainText('166');
  await page.getByRole('button', { name: /Mehr Filter/ }).click();
  await page.getByLabel(/Stapelbar/).check();
  await expect(page.locator('.result-caption')).toContainText('166');
  await page.getByRole('button', { name: 'Alle Filter zurücksetzen' }).click();
  await expect(page.locator('.result-caption')).toContainText('1.000');
});
test('late searches cannot replace more recent results', async ({ page }) => {
  await fixture(page);
  const input = page.getByRole('textbox', { name: 'Items suchen' });
  await input.fill('langsam');
  await page.waitForTimeout(320);
  await input.fill('Stumpfpfeil');
  await expect(page.locator('.result-caption')).toContainText('1 Gegenstände');
  await page.waitForTimeout(850);
  await expect(page.getByRole('option', { name: /Test-Stumpfpfeil/ })).toBeVisible();
});
test('unknown build blocks browsing and shows recovery controls', async ({ page }) => {
  await fixture(page, 'unknown');
  await expect(page.getByRole('alert')).toContainText('Dieser Build ist noch nicht unterstützt');
  await expect(page.getByRole('listbox', { name: 'Gegenstände' })).toHaveCount(0);
  await page.getByRole('button', { name: 'Datenquelle prüfen' }).click();
  await expect(page.getByRole('textbox', { name: 'Installationsordner' })).toHaveValue('C:/Synthetic Game');
});
test('empty results and 1024px desktop layout remain usable', async ({ page }) => {
  await page.setViewportSize({ width: 1024, height: 768 });
  await fixture(page, 'empty');
  await expect(page.getByRole('heading', { name: 'Keine passenden Gegenstände' })).toBeVisible();
  const overflow = await page.evaluate(() => document.documentElement.scrollWidth > window.innerWidth);
  expect(overflow).toBe(false);
  await expect(page.getByRole('button', { name: 'Suche zurücksetzen' })).toBeVisible();
});
test('browser without desktop bridge never fabricates game data', async ({ page }) => {
  await page.goto('/');
  await expect(page.getByRole('heading', { name: 'Die Desktop-App wird benötigt.' })).toBeVisible();
  await expect(page.locator('.item-row')).toHaveCount(0);
});
