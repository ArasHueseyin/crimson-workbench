import {expect,test,type Page} from '@playwright/test';
import type {PhotoRequest,PhotoSnapshot} from '../src/photo-types';
type PhotoFixture={snapshot:PhotoSnapshot;calls:{command:string;args:Record<string,unknown>}[];mode:'normal'|'lost'|'queued';offline:boolean};
declare global {interface Window {photoFixture:PhotoFixture} var photoFixture:PhotoFixture}
async function fixture(page:Page,enabled=true){
 await page.addInitScript(()=>{
  const root=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;photoFixture:PhotoFixture};root.isTauri=true;
  const initial:PhotoSnapshot={pid:123,state:'ready',id:'9007199254740993',session:'18364758544493064720',epoch:'9007199254740995',observed_ms:'4294967296000',reason:0,native_photo:false,owned:false,hud_hidden:false,scene_frozen:false};
  const saved=localStorage.getItem('photo-fixture.native');
  const f:PhotoFixture={snapshot:saved?JSON.parse(saved):initial,calls:[],mode:'normal',offline:false};root.photoFixture=f;
  root.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>={})=>{
   f.calls.push({command,args});if(f.offline)throw{message:'Owned photo fixture disconnected.'};
   if(command==='photo_status')return structuredClone(f.snapshot);
   if(command!=='photo_change')throw Error(`Unexpected command ${command}`);
   const r=args.request as PhotoRequest,s=f.snapshot;s.id=r.id;s.reason=0;
   if(f.mode==='queued')s.state='native_queued';
   else if(r.operation==='enter'){s.state='active';s.native_photo=s.owned=s.scene_frozen=true;s.hud_hidden=r.hide_on_enter!}
   else if(r.operation==='leave'){s.state='left';s.native_photo=s.owned=s.scene_frozen=s.hud_hidden=false}
   else{s.state=r.operation==='hide_hud'?'hud_hidden':'hud_visible';s.hud_hidden=r.operation==='hide_hud'}
   localStorage.setItem('photo-fixture.native',JSON.stringify(s));
   if(f.mode==='lost')throw{message:'Owned photo fixture lost the response.'};
   return structuredClone(s);
  }};
 });
 await page.goto(`/tests/photo-controls-fixture.html?enabled=${enabled}`);
 await expect(page.getByLabel('Live photo controls',{exact:true})).toBeVisible();
}
test('photo actions are explicit and native scene pause is reported separately',async({page})=>{
 await fixture(page);const enter=page.getByRole('button',{name:'Enter photo mode',exact:true});await expect(enter).toBeEnabled();
 expect(await page.evaluate(()=>photoFixture.calls.filter(c=>c.command==='photo_change'))).toHaveLength(0);
 await page.getByLabel('Hide HUD on entering photo mode').uncheck();await enter.click();
 await expect(page.getByRole('status')).toHaveText('Photo mode active. Move the camera with the game’s photo controls.');
 await expect(page.locator('.photo-status')).toContainText('Scene frozen');
 await page.getByRole('button',{name:'Hide HUD',exact:true}).click();await expect(page.getByRole('status')).toHaveText('HUD hidden.');
 await page.getByRole('button',{name:'Show HUD',exact:true}).click();await expect(page.getByRole('status')).toHaveText('HUD restored.');
 await page.screenshot({path:'../.local/marketplace-photo-controls-preview.png'});
 await page.getByRole('button',{name:'Leave photo mode',exact:true}).click();await expect(page.getByRole('status')).toHaveText('Photo mode closed.');
 const writes=await page.evaluate(()=>photoFixture.calls.filter(c=>c.command==='photo_change').map(c=>c.args.request as PhotoRequest));
 expect(writes.map(r=>r.operation)).toEqual(['enter','hide_hud','show_hud','leave']);
 expect(writes[0]).toMatchObject({id:'9007199254740994',session:'18364758544493064720',epoch:'9007199254740995',not_after_ms:'4294967298500',hide_on_enter:false});
 expect(writes.slice(1).every(r=>r.hide_on_enter===null)).toBe(true);
 expect(new Set(writes.map(r=>r.id)).size).toBe(4);
});
test('a lost photo response is queried after reload without replay',async({page})=>{
 await fixture(page);await expect(page.getByRole('button',{name:'Enter photo mode',exact:true})).toBeEnabled();
 await page.evaluate(()=>{photoFixture.mode='lost'});await page.getByRole('button',{name:'Enter photo mode',exact:true}).click();
 await expect(page.getByRole('alert')).toHaveText('Owned photo fixture lost the response.');
 expect(await page.evaluate(()=>photoFixture.calls.filter(c=>c.command==='photo_change'))).toHaveLength(1);
 await page.reload();await expect(page.locator('.photo-status')).toContainText('Workbench photo session');
 await expect(page.getByRole('button',{name:'Leave photo mode',exact:true})).toBeEnabled();
 expect(await page.evaluate(()=>photoFixture.calls.filter(c=>c.command==='photo_change'))).toHaveLength(0);
 expect(await page.evaluate(()=>localStorage.getItem('crimson-workbench.photo.pending.v1'))).toBeNull();
});
test('queued entry never claims scene freeze and an unavailable callback times out without retry',async({page})=>{
 await fixture(page);await expect(page.getByRole('button',{name:'Enter photo mode',exact:true})).toBeEnabled();
 await page.evaluate(()=>{photoFixture.mode='queued'});await page.getByRole('button',{name:'Enter photo mode',exact:true}).click();
 await expect(page.getByRole('status')).toContainText('Waiting for the game.');
 await expect(page.getByRole('button',{name:'Leave photo mode',exact:true})).toBeDisabled();
 await page.evaluate(()=>{photoFixture.snapshot.state='waiting'});
 await expect(page.getByRole('button',{name:'I checked the result in game',exact:true})).toBeVisible({timeout:9000});
 expect(await page.evaluate(()=>photoFixture.calls.filter(c=>c.command==='photo_change'))).toHaveLength(1);
 await page.getByRole('button',{name:'I checked the result in game',exact:true}).click();
 expect(await page.evaluate(()=>photoFixture.calls.filter(c=>c.command==='photo_change'))).toHaveLength(1);
});
test('foreign photo sessions cannot be controlled or adopted',async({page})=>{
 await fixture(page);await page.evaluate(()=>Object.assign(photoFixture.snapshot,{native_photo:true,owned:false,scene_frozen:true}));
 await expect(page.locator('.photo-status')).toContainText('Photo mode opened in game');
 for(const name of ['Enter photo mode','Hide HUD','Leave photo mode'])await expect(page.getByRole('button',{name,exact:true})).toBeDisabled();
 expect(await page.evaluate(()=>photoFixture.calls.filter(c=>c.command==='photo_change'))).toHaveLength(0);
});
test('disabled setup does not read or change native photo state',async({page})=>{
 await fixture(page,false);await expect(page.getByRole('button',{name:'Enter photo mode',exact:true})).toBeDisabled();
 expect(await page.evaluate(()=>photoFixture.calls)).toHaveLength(0);
});
