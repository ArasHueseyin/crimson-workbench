import { expect, test, type Page } from '@playwright/test';

async function setup(page: Page, damaged = false, readOnly = false) {
  await page.addInitScript(({ damaged, readOnly }) => {
    const root = window as unknown as { isTauri: boolean; __TAURI_INTERNALS__: unknown; setupOpened?: Record<string, unknown>; setupCalls: string[] };
    root.isTauri = true;
    root.setupCalls = []; let launched = false;
    const defaults = { version: 1, game_dir: null, save_dir: null, language: 'auto' };
    function bootstrap() {
      const settings = JSON.parse(localStorage.getItem('setup-test-preferences') ?? JSON.stringify(defaults));
      return { project: 'C:/Users/Friend/AppData/Local/CrimsonWorkbench', settings, settings_warning: damaged && !localStorage.getItem('setup-test-preferences') ? 'Saved settings could not be loaded. Select the paths again and save settings.' : null,
        discovery: { installations: settings.game_dir ? [{ path: settings.game_dir, platform: 'explicit', build_id: 'synthetic' }] : damaged && !localStorage.getItem('setup-test-preferences') ? [{ path: 'C:/Detected Game', platform: 'steam', build_id: 'synthetic' }] : [], configured_game: settings.game_dir, save_directories: ['C:/Friend Saves'] }, languages: [{ language: 'ger' }, { language: 'eng' }] };
    }
    root.__TAURI_INTERNALS__ = { invoke: async (command: string, args: Record<string, unknown> = {}) => {
      root.setupCalls.push(command);
      if (command === 'spawn_launch') { launched = true; return {pid:42, backup_path:'/Synthetic verified backup'}; }
      if (command === 'spawn_status' && launched) return {state:'ready',pid:42,epoch:7,id:'',key:0,quantity:0,message:'Live item module connected.'};
      if (command === 'bootstrap') return bootstrap();
      if (command === 'save_settings') {
        const value = args.settings as typeof defaults;
        if (String(value.game_dir).includes('missing')) throw { code: 'settings_error', message: 'Installation folder is unreachable.' };
        localStorage.setItem('setup-test-preferences', JSON.stringify(value));
        return bootstrap();
      }
      if (command === 'open_catalog') {
        root.setupOpened = args;
        const language = args.language === 'auto' ? 'ger' : args.language;
        return { session: 1, read_only: readOnly, game_path: args.game, item_language: { language, source: args.language === 'auto' ? 'steam' : 'manual' }, info: { item_count: 0, types: [], categories: [], tiers: [], stats: [], groups: [], exe_version: '1.0.0.2976', index: { path: 'C:/Users/Friend/AppData/Local/CrimsonWorkbench/.local/index/items.sqlite', language, fingerprint: 'synthetic' } } };
      }
      if (command === 'search_items') return { items: [], total: 0, offset: 0 };
      throw new Error(`Unexpected command: ${command}`);
    } };
  }, { damaged, readOnly });
  await page.goto('/');
}

test('first run saves independent game and save roots and restores them and language on restart', async ({ page }) => {
  await setup(page);
  await expect(page.getByLabel('Installation folder', { exact: true })).toHaveValue('');
  await expect(page.getByText('C:/Users/Friend/AppData/Local/CrimsonWorkbench', { exact: true })).toBeVisible();
  await page.getByLabel('Installation folder', { exact: true }).fill('C:/Friend Game');
  await page.getByLabel('Save folder', { exact: true }).fill('D:/Own Saves');
  await page.getByRole('button', { name: 'Save & connect', exact: true }).click();
  await expect(page.getByRole('status')).toContainText('Settings saved');
  await expect(page.locator('.workbench')).toHaveAttribute('data-ready', 'true');
  await expect(page.getByLabel('Item language')).toHaveValue('auto');
  await expect(page.getByLabel('Item language').locator('option:checked')).toHaveText('Automatic · Deutsch');
  await expect(page.getByText('Detected from Steam', { exact: true })).toBeVisible();
  await expect(page.getByRole('button', { name: 'Save & connect', exact: true })).toBeVisible();
  await page.reload();
  await expect(page.getByLabel('Item language')).toHaveValue('auto');
  await expect.poll(() => page.evaluate(() => (window as unknown as { setupOpened?: { language: string } }).setupOpened?.language)).toBe('auto');
  await page.getByLabel('Item language').selectOption('eng');
  await expect(page.getByLabel('Item language').locator('option[value=auto]')).toHaveText('Automatic (game language)');
  await expect.poll(() => page.evaluate(() => (window as unknown as { setupOpened?: { language: string } }).setupOpened?.language)).toBe('eng');
  await page.reload();
  await expect(page.locator('.workbench')).toHaveAttribute('data-ready', 'true');
  await expect(page.getByLabel('Item language')).toHaveValue('eng');
  await page.getByRole('button', { name: 'Data sources', exact: true }).click();
  await expect(page.getByLabel('Installation folder', { exact: true })).toHaveValue('C:/Friend Game');
  await expect(page.getByLabel('Save folder', { exact: true })).toHaveValue('D:/Own Saves');
});

test('invalid paths leave persisted preferences unchanged and show actionable error', async ({ page }) => {
  await setup(page);
  await page.getByLabel('Installation folder', { exact: true }).fill('C:/missing');
  await page.getByRole('button', { name: 'Save & connect', exact: true }).click();
  await expect(page.getByRole('alert')).toContainText('Installation folder is unreachable');
  expect(await page.evaluate(() => localStorage.getItem('setup-test-preferences'))).toBeNull();
  await expect(page.getByRole('button', { name: 'Save & connect', exact: true })).toBeEnabled();
});

test('damaged settings can be replaced from first-run UI', async ({ page }) => {
  await setup(page, true);
  await expect(page.getByRole('alert')).toContainText('Saved settings could not be loaded');
  await expect(page.locator('.workbench')).toHaveAttribute('data-ready', 'false');
  await page.getByRole('button', { name: 'Save & connect', exact: true }).click();
  await expect(page.getByRole('status')).toContainText('Settings saved');
  await expect(page.getByRole('alert')).toHaveCount(0);
});

 test('Mac catalog offers explicit backed-up item launch and explains remaining limits', async ({ page }) => {
  await setup(page, false, true);
  await page.getByLabel('Installation folder', { exact: true }).fill('/Games/CrimsonDesert_Steam.app');
  await page.getByRole('button', { name: 'Save & connect', exact: true }).click();
  await expect(page.locator('.workbench')).toHaveAttribute('data-ready', 'true');
  await expect(page.getByText('Mac preview · Experimental item spawning.', { exact: true })).toBeVisible();
  await expect(page.locator('.source-assurance[role=status]')).toContainText('Game file edits and other runtime mods remain unavailable.');
  await page.getByRole('button', {name:'Item picker',exact:true}).click();
  const launch = page.getByRole('button', {name:'Launch game with item spawning'});
  await expect(launch).toBeEnabled();
  expect(await page.evaluate(() => (window as unknown as {setupCalls:string[]}).setupCalls.filter(c=>c==='spawn_launch'||c==='spawn_grant'))).toEqual([]);
  await launch.click();
  await expect(page.getByText('Verified save backup: /Synthetic verified backup')).toBeVisible();
  await expect(launch).toBeDisabled();
  expect(await page.evaluate(() => (window as unknown as {setupCalls:string[]}).setupCalls.filter(c=>c==='spawn_launch'||c==='spawn_grant'))).toEqual(['spawn_launch']);
  await page.screenshot({path:'../.local/macos-items/mac-launch-picker.png'});
});
