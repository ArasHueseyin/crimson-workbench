// Connect only to the separately launched, hidden Workbench test process.
// Launch it with --background-test and a private WEBVIEW2 debugging port.
import { chromium } from '@playwright/test';
import { mkdir, writeFile } from 'node:fs/promises';
import { resolve } from 'node:path';
const output = resolve(process.env.CD_SMOKE_OUTPUT ?? '../.local/phase2-native');
await mkdir(output, { recursive: true });
let browser;
for (let i = 0; i < 30; i++) {
  try { browser = await chromium.connectOverCDP('http://127.0.0.1:9225', { timeout: 1500 }); break; }
  catch { await new Promise(r => setTimeout(r, 500)); }
}
if (!browser) throw new Error('Hidden Workbench WebView2 not reachable');
const page = browser.contexts().flatMap(c => c.pages()).find(p => !p.url().startsWith('devtools:'));
if (!page) throw new Error('No Workbench webview');
const errors = [];
page.on('pageerror', e => errors.push(e.message));
await page.locator('.workbench[data-ready="true"]').waitFor({ timeout: 90000 });
await page.getByRole('button', { name: /Itemdatenbank/ }).click();
await page.getByRole('textbox', { name: 'Items suchen' }).fill('Stumpfpfeil');
await page.locator('[data-item-key="2200"]').waitFor({ timeout: 60000 });
await page.locator('[data-item-key="2200"]').click();
await page.getByRole('heading', { name: 'Stumpfpfeil', exact: true }).waitFor({ timeout: 60000 });
await page.locator('.detail-hero img').waitFor({ timeout: 60000 });
const decoded = await page.locator('.detail-hero img').evaluate(img => img.complete && img.naturalWidth > 0);
if (!decoded) throw new Error('Real icon did not decode');
await page.getByRole('tab', { name: /Alle Felder/ }).click();
await page.getByRole('textbox', { name: 'Rohfelder durchsuchen' }).fill('max_stack_count');
await page.locator('.raw-field').first().waitFor();
const field = await page.locator('.raw-field').first().innerText();
if (!field.includes('max_stack_count')) throw new Error('Real field view missing');
await page.getByRole('tab', { name: 'Übersicht' }).click();
await page.getByRole('button', { name: 'Item als JSON speichern' }).click();
await page.getByRole('status').waitFor({ timeout: 60000 });
const exported = await page.getByRole('status').innerText();
if (!exported.includes('exports')) throw new Error('Unexpected export location');
await page.getByRole('textbox', { name: 'Items suchen' }).fill('Arrow');
await page.waitForTimeout(2500);
await page.screenshot({ path: resolve(output, 'item-database.png') });
await page.getByRole('button', { name: 'Datenquellen', exact: true }).click();
await page.getByRole('heading', { name: 'Lokaler Suchindex' }).waitFor();
await page.screenshot({ path: resolve(output, 'data-sources.png') });
await page.getByRole('button', { name: /Itemdatenbank/ }).click();
await page.getByLabel('Itemsprache', { exact: true }).selectOption('eng');
await page.locator('.workbench[data-ready="true"]').waitFor({ timeout: 90000 });
await page.getByRole('textbox', { name: 'Items suchen' }).fill('Pyeonjeon');
await page.locator('[data-item-key="2200"]').waitFor({ timeout: 60000 });
await page.locator('[data-item-key="2200"]').click();
await page.locator('.detail-hero h2').waitFor({ timeout: 60000 });
const english = await page.locator('.detail-hero h2').innerText();
if (english === 'Stumpfpfeil') throw new Error('Language change did not refresh item');
if (errors.length) throw new Error(errors.join('\n'));
await writeFile(resolve(output, 'result.json'), JSON.stringify({ native: true, item: 2200, german: 'Stumpfpfeil', english, realIconDecoded: decoded, field, exported, pageErrors: errors, screenshot: 'item-database.png' }, null, 2));
console.log(JSON.stringify({ native: true, item: 2200, english, realIconDecoded: decoded, pageErrors: errors, output }));
await browser.close();
