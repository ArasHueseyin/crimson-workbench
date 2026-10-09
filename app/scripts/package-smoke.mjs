// Exercise only a separately launched packaged Workbench with private paths.
import { chromium, expect } from '@playwright/test';
import { mkdir, readFile, writeFile } from 'node:fs/promises';
import { resolve } from 'node:path';

const endpoint = process.env.CD_PACKAGE_CDP;
const output = resolve(process.env.CD_SMOKE_OUTPUT ?? '../.local/package-smoke');
const project = process.env.CD_PACKAGE_PROJECT;
if (!endpoint?.match(/^http:\/\/127\.0\.0\.1:\d+$/) || !project || !process.env.CD_GAME_DIR)
  throw new Error('Package smoke requires a private CDP endpoint, project and missing game path');
await mkdir(output, { recursive: true });
let browser;
for (let attempt = 0; attempt < 60; attempt++) {
  try { browser = await chromium.connectOverCDP(endpoint, { timeout: 1000 }); break; }
  catch { await new Promise(resolve => setTimeout(resolve, 500)); }
}
if (!browser) throw new Error('Packaged WebView2 did not expose its private debug endpoint');
try {
  const page = browser.contexts().flatMap(context => context.pages())
    .find(candidate => !candidate.url().startsWith('devtools:'));
  if (!page) throw new Error('Packaged Workbench webview missing');
  const errors = [];
  page.on('pageerror', error => errors.push(error.message));
  await page.waitForURL(/^(https?:\/\/tauri\.localhost|tauri:\/\/localhost)(\/|$)/, { timeout: 60000 });
  // Reload once after attaching so startup JavaScript exceptions are captured too.
  await page.reload({ waitUntil: 'domcontentloaded' });
  await expect(page.locator('.workbench')).toBeVisible({ timeout: 60000 });
  if (!/^(https?:\/\/tauri\.localhost|tauri:\/\/localhost)(\/|$)/.test(page.url()))
    throw new Error(`Unexpected frontend origin: ${page.url()}`);
  const scripts = await page.locator('script[src]').evaluateAll(nodes => nodes.map(node => node.getAttribute('src')));
  if (!scripts.some(src => /^\/assets\/[^/]+\.js$/.test(src ?? '')) || scripts.some(src => src?.includes('@vite')))
    throw new Error('Workbench did not load the bundled production frontend');
  await page.getByRole('button', { name: 'Datenquellen', exact: true }).click();
  await expect(page.getByRole('heading', { name: /^Datenquellen/ })).toBeVisible();
  await expect(page.getByLabel('Installationsordner', { exact: true })).toBeVisible();
  await expect(page.getByLabel('Spielstandordner', { exact: true })).toBeVisible();
  await expect(page.getByRole('button', { name: 'Einstellungen speichern', exact: true })).toBeEnabled();
  await expect(page.locator('.workbench')).toHaveAttribute('data-ready', 'false');
  await expect(page.getByLabel('Itemsprache', { exact: true })).toHaveValue('eng');
  // A genuine settings write is safe: only null paths in our private project.
  await page.getByLabel('Installationsordner', { exact: true }).fill('');
  await page.getByLabel('Spielstandordner', { exact: true }).fill('');
  await page.getByRole('button', { name: 'Einstellungen speichern', exact: true }).click();
  await expect(page.getByText(/^Einstellungen gespeichert\./)).toBeVisible({ timeout: 15000 });
  const settings = JSON.parse(await readFile(resolve(project, 'settings.json'), 'utf8'));
  if (settings.version !== 1 || settings.game_dir != null || settings.save_dir != null || settings.language !== 'eng')
    throw new Error('Packaged settings persistence did not preserve the expected isolated values');
  await page.screenshot({ path: resolve(output, 'packaged-data-sources.png'), fullPage: true });
  if (errors.length) throw new Error(errors.join('\n'));
  const result = { packaged: true, origin: page.url(), scripts, noGameLoaded: true,
    settingsPersisted: true, language: settings.language, pageErrors: errors };
  await writeFile(resolve(output, 'result.json'), JSON.stringify(result, null, 2) + '\n');
  console.log(JSON.stringify(result));
} finally {
  await browser.close();
}
