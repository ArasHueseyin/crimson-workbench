import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import { realpathSync } from 'node:fs';
export default defineConfig({
  // Windows Documents may be a junction to another drive. Keep Rollup and Vite
  // on the same physical root when assigning output asset names.
  root: realpathSync(process.cwd()),
  cacheDir: '../.local/vite-cache',
  plugins: [react()], clearScreen: false,
  server: { host: '127.0.0.1', port: 1420, strictPort: true, watch: { ignored: ['**/src-tauri/**'] } },
  build: { target: 'es2022' },
});
