import {readFileSync} from 'node:fs';
import {describe,expect,it} from 'vitest';

const source=(path:string)=>readFileSync(new URL(path,import.meta.url),'utf8');
describe('packaged desktop command access',()=>{
 it('registers every desktop handler and grants it to the local main window',()=>{
  const handlers=[...source('../src-tauri/src/lib.rs').matchAll(/#\[tauri::command\]\s*(?:async\s+)?fn\s+(\w+)/g)].map(match=>match[1]);
  const manifest=source('../src-tauri/build.rs');
  const capability=JSON.parse(source('../src-tauri/capabilities/main-read-only.json')) as {local:boolean;windows:string[];permissions:string[]};
  expect(capability.local).toBe(true);
  expect(capability.windows).toEqual(['main']);
  expect(handlers.filter(command=>!manifest.includes(`"${command}"`)), 'Handlers missing from the Tauri command manifest').toEqual([]);
  expect(handlers.filter(command=>!capability.permissions.includes(`allow-${command.replaceAll('_','-')}`)), 'Commands blocked by the main window capability').toEqual([]);
 });
});
