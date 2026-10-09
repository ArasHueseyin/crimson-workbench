// Windows Documents can be a junction. Vite/esbuild must share a physical cwd.
import { realpathSync } from 'node:fs';
process.chdir(realpathSync(process.cwd()));
await import('../node_modules/vite/bin/vite.js');
