import { expect, test, type Page } from '@playwright/test';
async function fixture(page: Page, live: { ready?: boolean; defer?: boolean; lostReply?: boolean; stack10?:boolean; knowledge?:boolean; arrows?:boolean; npcAmmo?:boolean } = {}) {
  await page.addInitScript(live => {
    const root = window as unknown as { isTauri: boolean; __TAURI_INTERNALS__: unknown; spawnCalls: { command: string; args: Record<string, unknown> }[] };
    root.isTauri = true;
    root.spawnCalls = [];
    const items = Array.from({ length: 450 }, (_, n) => ({ key: 1000 + n, name: live.arrows&&n<3?['Blitz-Pfeile','Feuerpfeile','Goldbarren'][n]:n === 0 ? 'Dunkler Zweihänder' : `Material ${n}`, internal_key: `Synthetic_${n}`, description: n === 0 ? 'Eine dunkle Testwaffe. <b>Nur Testdaten.</b>' : 'Material für Herstellung.', item_type: 1, category: 3, tier: 3, max_stack: live.stack10?'10':'1000', stat_keys: [7], knowledge_keys:live.knowledge&&n===0?[501]:[], icon_key: 1 }));
    if(live.npcAmmo) Object.assign(items[0],{key:1001315,name:'Blitzpfeil',internal_key:'Lightning_Arrow',use_restriction:'Monstermunition: Diese Variante ist für NPCs vorgesehen. Verwende Pfeil (ID 50001) oder Explosionspfeil (ID 1001321).'});
    root.__TAURI_INTERNALS__ = { invoke: async (command: string, args: Record<string, unknown> = {}) => {
      if (command.startsWith('spawn_')) {
        root.spawnCalls.push({ command, args });
        if (!live.ready) throw { message: 'Live-Modul nicht verbunden.' };
        const request = args.request as { id: string; key: number; quantity: number } | null;
        if (command === 'spawn_grant' && live.lostReply) throw { message: 'Keine vollständige Antwort.' };
        const state = command === 'spawn_cancel' ? 'cancelled' : !request ? 'ready' : command === 'spawn_grant' || live.defer ? 'queued' : 'applied';
        return { state, pid: 123, epoch: 456, id: request?.id ?? '', key: request?.key ?? 0, quantity: request?.quantity ?? 0, message: state === 'applied' ? 'Itemmenge bestätigt. Jetzt normal speichern.' : state === 'queued' ? 'Anfrage wartet auf Inventarereignis.' : state === 'cancelled' ? 'Anfrage abgebrochen.' : 'Live-Spawner verbunden.' };
      }
      if (command === 'bootstrap') return { project: 'C:/Test', discovery: { installations: [{ path: 'C:/Synthetic', platform: 'steam' }], configured_game: 'C:/Synthetic' }, languages: [{ language: 'ger' }] };
      if (command === 'knowledge_snapshot') return {saves:[{id:'123/slot2',label:'Slot 2',modified:1700000000},{id:'123/slot1',label:'Slot 1',modified:1600000000}],selected_save:args.save??'123/slot2',modified:1700000000,items:live.knowledge?[{key:1000,total:1,learned:args.save==='123/slot1'?0:1,unknown:0,state:args.save==='123/slot1'?'missing':'learned'}]:[],message:'Wissen aus dem gespeicherten Spielstand.'};
      if (command === 'open_catalog') return { session: 1, game_path: 'C:/Synthetic', info: { item_count: 450, types: [], categories: [], tiers: [], stats: [], index: {}, groups: [{ key: 1, name: 'Ausrüstung', internal_key: 'equipment', order: 1, items: [1000] }, { key: 2, name: 'Materialien', internal_key: 'materials', order: 3, items: items.slice(1).map(i => i.key) }, { key: 3, name: 'Zweihandwaffen', internal_key: 'twohand', order: 1500, items: [1000] }, ...(live.knowledge?[{key:4,name:'Dokumente',internal_key:'ItemGroup_Category_Document',order:4,items:[1001]}]:[])] } };
      if (command === 'search_items') {
        const q = args.query as { text: string; regex?:boolean; group: number | null; offset: number };
        if (q.text === 'langsam') await new Promise(r => setTimeout(r, 700));
        let matcher:RegExp|undefined;
        if(q.regex){try{matcher=new RegExp(q.text,'i');}catch{throw {message:'Ungültiges Regexmuster.'};}}
        const list = items.filter(i => (q.group === 1 || q.group === 3 ? i.key === 1000 : q.group === 2 ? i.key !== 1000 : true) && (!q.text || (matcher?matcher.test(i.name):`${i.name} ${i.key}`.toLowerCase().includes(q.text.toLowerCase()))));
        return { items: list.slice(q.offset, q.offset + 200), total: list.length, offset: q.offset };
      }
      if (command === 'item_icon') return { data_url: 'data:image/svg+xml,%3Csvg xmlns="http://www.w3.org/2000/svg" width="40" height="40"%3E%3Crect width="40" height="40" fill="%23aabb88"/%3E%3C/svg%3E' };
      if (command === 'mod_info') return { advanced: { statuses: [{ key: 7, name: 'Attack', entries: 1 }] } };
      if (command === 'advanced_item') return { item: { key: args.key, max_endurance: 100, enchant_levels: [0, 1], stats: [{ enchant_level: 0, stat: 7, list: 'static', value: '25' }, { enchant_level: 1, stat: 7, list: 'static', value: '35' }] } };
      if (command === 'craft_item') return { recipes: [], used_in: [] };
      throw { message: `Unexpected fixture command ${command}` };
    } };
  }, live);
  await page.goto('/');
  await expect(page.locator('.workbench')).toHaveAttribute('data-ready', 'true');
  await page.getByRole('button', { name: 'Item-Auswahl', exact: true }).click();
}
test('icons, descriptions, categories, exact ID and refinement values', async ({ page }) => {
  await fixture(page);
  await page.getByRole('button', { name: 'Ausrüstung', exact: true }).click();
  await expect(page.locator('.result-caption')).toContainText('1 Gegenstände');
  await page.getByRole('option', { name: /Dunkler Zweihänder/ }).click();
  await expect(page.locator('.picker-hero img')).toBeVisible();
  await expect(page.locator('.picker-description')).toHaveText('Eine dunkle Testwaffe. Nur Testdaten.');
  await expect(page.locator('.picker-values')).toContainText(['25']);
  await page.getByLabel('Werte für Verfeinerungsstufe').selectOption('1');
  await expect(page.locator('.picker-values')).toContainText(['35']);
  await page.getByLabel('Gewünschte Itemmenge').fill('500');
  await expect(page.getByRole('button', { name: 'Ins Inventar geben' })).toBeDisabled();
  await page.getByLabel('Item-Unterkategorie').selectOption('3');
  await page.getByLabel('Item-Auswahl durchsuchen').fill('1000');
  await expect(page.getByRole('option', { name: /Dunkler Zweihänder/ })).toBeVisible();
  await page.getByLabel('Item-Auswahl durchsuchen').fill('Material');
  await expect(page.getByRole('heading', { name: 'Keine passenden Gegenstände' })).toBeVisible();
});
test('part-word search applies to all items and regex mode validates, recovers and changes the IPC query',async({page})=>{
  await fixture(page,{arrows:true});const search=page.getByLabel('Item-Auswahl durchsuchen');
  await search.fill('pfeil');await expect(page.locator('.picker-row')).toHaveCount(2);
  await expect(page.locator('.picker-row')).toContainText(['Blitz-Pfeile','Feuerpfeile']);
  await page.getByLabel('Regex-Suche',{exact:true}).check();await search.fill('^(Blitz|Gold)');
  await expect(page.locator('.picker-row')).toHaveCount(2);await expect(page.locator('.picker-row')).toContainText(['Blitz-Pfeile','Goldbarren']);
  await search.fill('[');await expect(page.getByRole('alert')).toContainText('Ungültiges Regexmuster');
  await search.fill('^Feuer');await expect(page.locator('.picker-row')).toHaveCount(1);await expect(page.locator('.picker-row')).toContainText('Feuerpfeile');
  await page.getByLabel('Regex-Suche',{exact:true}).uncheck();await search.fill('goldbarren');await expect(page.locator('.picker-row')).toHaveCount(1);
});
test('larger item icon opens a modal image preview and Escape returns to the same selection',async({page})=>{
  await fixture(page);await page.getByRole('option',{name:/Dunkler Zweihänder/}).click();
  const trigger=page.getByRole('button',{name:'Itembild vergrößern'});await expect(trigger).toBeVisible();
  await trigger.click();const dialog=page.getByRole('dialog',{name:'Bildvorschau: Dunkler Zweihänder'});await expect(dialog).toBeVisible();
  expect((await dialog.locator('img').boundingBox())!.width).toBeGreaterThan(300);
  const imageBox=(await dialog.locator('img').boundingBox())!,stageBox=(await dialog.locator('.image-preview-stage').boundingBox())!;
  expect(imageBox.y+imageBox.height).toBeLessThanOrEqual(stageBox.y+stageBox.height+1);
  await page.screenshot({path:'../.local/mount-template-fix-20261004/item-image-preview.png'});
  await page.keyboard.press('Escape');await expect(dialog).toHaveCount(0);await expect(page.locator('.picker-hero')).toContainText('Dunkler Zweihänder');
  expect(await page.evaluate(()=>(window as unknown as {spawnCalls:{command:string}[]}).spawnCalls.filter(c=>c.command==='spawn_grant').length)).toBe(0);
});
test('virtualization, pages, stale results and compact layout', async ({ page }) => {
  await page.setViewportSize({ width: 1024, height: 768 });
  await fixture(page);
  await expect(page.locator('.picker-pages')).toContainText('1–200 von 450');
  expect(await page.locator('.picker-row').count()).toBeLessThan(30);
  await page.locator('.picker-pages').getByRole('button', { name: 'Weiter' }).click();
  await expect(page.locator('.picker-pages')).toContainText('201–400 von 450');
  const input = page.getByLabel('Item-Auswahl durchsuchen');
  await input.fill('langsam'); await page.waitForTimeout(320); await input.fill('Dunkler');
  await expect(page.getByRole('option', { name: /Dunkler Zweihänder/ })).toBeVisible();
  await page.waitForTimeout(850);
  await expect(page.getByRole('option', { name: /Dunkler Zweihänder/ })).toBeVisible();
  expect(await page.evaluate(() => document.documentElement.scrollWidth > innerWidth)).toBe(false);
  await page.screenshot({ path: '../.local/live-items-20261003/picker-1024.png' });
});
test('NPC-only arrow warns before giving and does not submit a native grant',async({page})=>{
  await fixture(page,{ready:true,npcAmmo:true});await page.getByRole('option',{name:/Blitzpfeil/}).click();
  await expect(page.getByRole('note')).toContainText('Monstermunition');await page.getByLabel('Gewünschte Itemmenge').fill('100');
  await expect(page.getByRole('button',{name:'Ins Inventar geben'})).toBeDisabled();
  expect(await page.evaluate(()=>(window as unknown as {spawnCalls:{command:string}[]}).spawnCalls.filter(c=>c.command==='spawn_grant').length)).toBe(0);
  await page.getByRole('option',{name:/Material 1 /}).click();await expect(page.getByRole('button',{name:'Ins Inventar geben'})).toBeEnabled();
});

test('live grant validates quantity, submits once and waits for receipt', async ({ page }) => {
  await fixture(page, { ready: true });
  await page.getByRole('option', { name: /Dunkler Zweihänder/ }).click();
  const button = page.getByRole('button', { name: 'Ins Inventar geben' });
  await expect(button).toBeEnabled();
  for (const invalid of ['0', '-1', '1.5', '10001']) {
    await page.getByLabel('Gewünschte Itemmenge').fill(invalid); await expect(button).toBeDisabled();
  }
  await page.getByLabel('Gewünschte Itemmenge').fill('20'); await button.dblclick();
  await expect(page.locator('.picker-runtime-note')).toContainText('Anfrage wartet'); await expect(button).toBeDisabled();
  await expect(page.locator('.picker-runtime-note')).toContainText('Itemmenge bestätigt', { timeout: 6000 });
  const calls = await page.evaluate(() => (window as unknown as { spawnCalls: { command: string; args: { request?: { quantity: number; key: number } } }[] }).spawnCalls.filter(c => c.command === 'spawn_grant'));
  expect(calls).toHaveLength(1); expect(calls[0].args.request).toMatchObject({ quantity: 20, key: 1000 });
});
test('pending request can be cancelled before it executes', async ({ page }) => {
  await fixture(page, { ready: true, defer: true });
  await page.getByRole('option', { name: /Dunkler Zweihänder/ }).click();
  await page.getByRole('button', { name: 'Ins Inventar geben' }).click();
  await page.getByRole('button', { name: 'Anfrage abbrechen' }).click();
  await expect(page.locator('.picker-runtime-note')).toContainText('Anfrage abgebrochen');
});
test('lost reply and page reload query the stored ID without re-granting', async ({ page }) => {
  await fixture(page, { ready: true, lostReply: true });
  await page.getByRole('option', { name: /Dunkler Zweihänder/ }).click();
  await page.getByRole('button', { name: 'Ins Inventar geben' }).click();
  await expect(page.locator('.picker-runtime-note')).toContainText('Keine vollständige Antwort');
  const record = await page.evaluate(() => localStorage.getItem('crimson-live-item-request-v1:c:/synthetic'));
  expect(record).toBeTruthy(); await page.reload();
  await expect(page.locator('.workbench')).toHaveAttribute('data-ready', 'true');
  await page.getByRole('button', { name: 'Item-Auswahl', exact: true }).click();
  await expect(page.locator('.picker-runtime-note')).toContainText('Itemmenge bestätigt');
  expect(await page.evaluate(() => (window as unknown as { spawnCalls: { command: string }[] }).spawnCalls.filter(c => c.command === 'spawn_grant').length)).toBe(0);
});
test('quantity can be cleared, typed and pasted; 100 items with stack10 show ten stacks',async({page})=>{
  await fixture(page,{ready:true,stack10:true});await page.getByRole('option',{name:/Dunkler Zweihänder/}).click();
  const input=page.getByLabel('Gewünschte Itemmenge'),button=page.getByRole('button',{name:'Ins Inventar geben'});
  await input.click();await input.press('Backspace');await expect(input).toHaveValue('');await expect(button).toBeDisabled();
  await input.pressSequentially('100');await expect(input).toHaveValue('100');await expect(button).toBeEnabled();
  await expect(page.locator('.picker-detail')).toContainText('höchstens 10 neue Stapel');
  await input.fill(' 200 ');await expect(button).toBeEnabled();await input.fill('1e2');await expect(button).toBeDisabled();
});
test('saved knowledge badges update for the chosen save without changing any game state',async({page})=>{
  await fixture(page,{knowledge:true});await page.getByRole('option',{name:/Dunkler Zweihänder/}).click();
  await expect(page.locator('.knowledge-badge')).toContainText('Wissen bereits erlangt');
  await page.getByLabel('Spielstand für Wissensanzeige').selectOption('123/slot1');await expect(page.locator('.knowledge-badge')).toContainText('Wissen noch nicht erlangt');
  expect(await page.evaluate(()=>(window as unknown as {spawnCalls:{command:string}[]}).spawnCalls.filter(c=>c.command==='spawn_grant').length)).toBe(0);
  await page.screenshot({path:'../.local/live-items-bag-mounts-20261003/knowledge-quantity-picker.png'});
  await page.getByLabel('Item-Auswahl durchsuchen').fill('1001');await page.getByRole('option',{name:/Material 1/}).click();
  await expect(page.locator('.knowledge-badge')).toContainText('Wissen: unbekannt');
  await expect(page.locator('.knowledge-badge')).toContainText('Keine eindeutige Wissensbelohnung');
});
