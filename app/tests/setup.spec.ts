import { expect, test, type Page } from '@playwright/test';

async function setup(page: Page, damaged = false) {
  await page.addInitScript(({ damaged }) => {
    const root = window as unknown as { isTauri: boolean; __TAURI_INTERNALS__: unknown; setupOpened?: Record<string, unknown> };
    root.isTauri = true;
    const defaults = { version: 1, game_dir: null, save_dir: null, language: 'ger' };
    function bootstrap() {
      const settings = JSON.parse(localStorage.getItem('setup-test-preferences') ?? JSON.stringify(defaults));
      return { project: 'C:/Users/Friend/AppData/Local/CrimsonWorkbench', settings, settings_warning: damaged && !localStorage.getItem('setup-test-preferences') ? 'Gespeicherte Einstellungen konnten nicht geladen werden. Bitte die Pfade erneut auswählen und Einstellungen speichern.' : null,
        discovery: { installations: settings.game_dir ? [{ path: settings.game_dir, platform: 'explicit', build_id: 'synthetic' }] : damaged && !localStorage.getItem('setup-test-preferences') ? [{ path: 'C:/Detected Game', platform: 'steam', build_id: 'synthetic' }] : [], configured_game: settings.game_dir, save_directories: ['C:/Friend Saves'] }, languages: [{ language: 'ger' }, { language: 'eng' }] };
    }
    root.__TAURI_INTERNALS__ = { invoke: async (command: string, args: Record<string, unknown> = {}) => {
      if (command === 'bootstrap') return bootstrap();
      if (command === 'save_settings') {
        const value = args.settings as typeof defaults;
        if (String(value.game_dir).includes('missing')) throw { code: 'settings_error', message: 'Installationsordner ist nicht erreichbar.' };
        localStorage.setItem('setup-test-preferences', JSON.stringify(value));
        return bootstrap();
      }
      if (command === 'open_catalog') {
        root.setupOpened = args;
        return { session: 1, game_path: args.game, info: { item_count: 0, types: [], categories: [], tiers: [], stats: [], groups: [], exe_version: '1.0.0.2976', index: { path: 'C:/Users/Friend/AppData/Local/CrimsonWorkbench/.local/index/items.sqlite', language: args.language, fingerprint: 'synthetic' } } };
      }
      if (command === 'search_items') return { items: [], total: 0, offset: 0 };
      throw new Error(`Unexpected command: ${command}`);
    } };
  }, { damaged });
  await page.goto('/');
}

test('first run saves independent game and save roots and restores them and language on restart', async ({ page }) => {
  await setup(page);
  await expect(page.getByLabel('Installationsordner', { exact: true })).toHaveValue('');
  await expect(page.getByText('C:/Users/Friend/AppData/Local/CrimsonWorkbench', { exact: true })).toBeVisible();
  await page.getByLabel('Installationsordner', { exact: true }).fill('C:/Friend Game');
  await page.getByLabel('Spielstandordner', { exact: true }).fill('D:/Own Saves');
  await page.getByRole('button', { name: 'Einstellungen speichern', exact: true }).click();
  await expect(page.getByRole('status')).toContainText('Einstellungen gespeichert');
  await expect(page.locator('.workbench')).toHaveAttribute('data-ready', 'true');
  await page.getByLabel('Itemsprache').selectOption('eng');
  await expect.poll(() => page.evaluate(() => (window as unknown as { setupOpened?: { language: string } }).setupOpened?.language)).toBe('eng');
  await page.reload();
  await expect(page.locator('.workbench')).toHaveAttribute('data-ready', 'true');
  await expect(page.getByLabel('Itemsprache')).toHaveValue('eng');
  await page.getByRole('button', { name: 'Datenquellen', exact: true }).click();
  await expect(page.getByLabel('Installationsordner', { exact: true })).toHaveValue('C:/Friend Game');
  await expect(page.getByLabel('Spielstandordner', { exact: true })).toHaveValue('D:/Own Saves');
});

test('invalid paths leave persisted preferences unchanged and show actionable error', async ({ page }) => {
  await setup(page);
  await page.getByLabel('Installationsordner', { exact: true }).fill('C:/missing');
  await page.getByRole('button', { name: 'Einstellungen speichern', exact: true }).click();
  await expect(page.getByRole('alert')).toContainText('Installationsordner ist nicht erreichbar');
  expect(await page.evaluate(() => localStorage.getItem('setup-test-preferences'))).toBeNull();
  await expect(page.getByRole('button', { name: 'Einstellungen speichern', exact: true })).toBeEnabled();
});

test('damaged settings can be replaced from first-run UI', async ({ page }) => {
  await setup(page, true);
  await expect(page.getByRole('alert')).toContainText('Gespeicherte Einstellungen konnten nicht geladen werden');
  await expect(page.locator('.workbench')).toHaveAttribute('data-ready', 'false');
  await page.getByRole('button', { name: 'Einstellungen speichern', exact: true }).click();
  await expect(page.getByRole('status')).toContainText('Einstellungen gespeichert');
  await expect(page.getByRole('alert')).toHaveCount(0);
});
