import { defineConfig } from '@playwright/test';

// Keep Windows CI on Edge; allow Chromium on machines without Edge.
const channel = process.env.PLAYWRIGHT_CHANNEL ?? (process.platform === 'win32' ? 'msedge' : 'chromium');

export default defineConfig({
  testDir: './tests',
  timeout: 30_000,
  workers: 1,
  fullyParallel: false,
  outputDir: '../.local/playwright-results',
  use: {
    baseURL: 'http://127.0.0.1:1420',
    browserName: 'chromium',
    channel,
    headless: true,
    viewport: { width: 1440, height: 920 },
    screenshot: 'only-on-failure',
  },
  webServer: {
    command: 'npm run dev',
    url: 'http://127.0.0.1:1420',
    reuseExistingServer: false,
    timeout: 30_000,
  },
});
