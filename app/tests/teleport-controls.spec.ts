import {expect,test,type Page} from '@playwright/test';
import type {TeleportRequest,TeleportSnapshot} from '../src/teleport-types';
type TeleportFixture={snapshot:TeleportSnapshot;calls:{command:string;args:Record<string,unknown>}[];mode:'normal'|'lost'|'queued';changeOnRead:boolean;contextOnRead:boolean};
declare global {interface Window {teleportFixture:TeleportFixture} var teleportFixture:TeleportFixture}
async function fixture(page:Page,enabled=true){
 await page.addInitScript(()=>{
  const root=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;teleportFixture:TeleportFixture};root.isTauri=true;
  const current={field_definition:1,stage:17,position:{x:10,y:20,z:30},yaw:0.5};
  const initial:TeleportSnapshot={pid:123,state:'ready',id:'9007199254740993',session:'18364758544493064720',epoch:'9007199254740995',observed_ms:'4294967296000',reason:0,features:7,identity:123,world:456,native_reason:0,native_error:0,storage_status:1,bookmarks_revision:'9007199254740997',current,bookmarks:Array.from({length:16},(_,i)=>({occupied:i===0,name:i===0?'Camp café':'',position:i===0?structuredClone(current):null})),marker_status:0,markers_observed_ms:'4294967296000',markers:[{id:'0',source:'map_pin',position:{x:100,y:0,z:200},layer:'mainland',shape:1,color:2},{id:'0',source:'waypoint',position:{x:300,y:0,z:400},layer:'mainland',shape:3,color:4},{id:'9007199254740999',source:'map_pin',position:{x:500,y:1501,z:600},layer:'alternate',shape:1,color:2}]};
  const saved=localStorage.getItem('teleport-fixture.native');
  const f:TeleportFixture={snapshot:saved?JSON.parse(saved):initial,calls:[],mode:'normal',changeOnRead:false,contextOnRead:false};root.teleportFixture=f;
  root.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>={})=>{
   f.calls.push({command,args});const s=f.snapshot;
   if(command==='teleport_status'){
    if(f.contextOnRead){f.contextOnRead=false;s.epoch=(BigInt(s.epoch)+1n).toString();s.identity++}
    if(f.changeOnRead){f.changeOnRead=false;s.bookmarks[0].name='Changed camp';s.bookmarks[0].position!.position.x+=100}
    return structuredClone(s);
   }
   if(command!=='teleport_change')throw Error(`Unexpected command ${command}`);
   const r=args.request as TeleportRequest;s.id=r.id;s.reason=0;
   if(r.operation==='save_current'){s.bookmarks[r.slot]={occupied:true,name:r.name,position:structuredClone(s.current)};s.bookmarks_revision=(BigInt(s.bookmarks_revision)+1n).toString();s.state='captured'}
   else if(r.operation==='remove_saved'){s.bookmarks[r.slot]={occupied:false,name:'',position:null};s.bookmarks_revision=(BigInt(s.bookmarks_revision)+1n).toString();s.state='removed'}
   else{
    if(r.operation==='travel_saved')s.current=structuredClone(s.bookmarks[r.slot].position);
    else s.current={...s.current!,position:{...s.markers.find(m=>m.id===r.marker&&m.source===r.marker_source)!.position,y:42}};
    s.state=f.mode==='queued'?'native_queued':'position_observed';
   }
   localStorage.setItem('teleport-fixture.native',JSON.stringify(s));
   if(f.mode==='lost')throw{message:'Owned teleport fixture lost the response.'};
   return structuredClone(s);
  }};
 });
 await page.goto(`/tests/teleport-controls-fixture.html?enabled=${enabled}`);
 await expect(page.getByLabel('Live teleport controls',{exact:true})).toBeVisible();
}
test('saved-position capture, replacement consent, travel and removal are explicit',async({page})=>{
 await fixture(page);await expect(page.getByLabel('Saved destination')).toHaveValue('0');
 expect(await page.evaluate(()=>teleportFixture.calls.filter(c=>c.command==='teleport_change'))).toHaveLength(0);
 await page.getByLabel('Position name',{exact:true}).fill('New camp');await expect(page.getByLabel('Save slot',{exact:true})).toHaveValue('1');
 await page.getByRole('button',{name:'Save current position',exact:true}).click();await expect(page.getByRole('status')).toHaveText('Position saved.');
 await expect(page.getByRole('button',{name:'Save current position',exact:true})).toBeDisabled();
 await page.getByLabel('Replace the saved position in this slot').check();await expect(page.getByRole('button',{name:'Save current position',exact:true})).toBeEnabled();
 await page.getByLabel('Saved destination').selectOption('0');await page.getByRole('button',{name:'Teleport to saved position',exact:true}).click();
 await expect(page.getByRole('status')).toContainText('observed at the destination');
 await page.getByLabel('Saved destination').selectOption('1');await page.getByRole('button',{name:'Remove saved position',exact:true}).click();await expect(page.getByRole('status')).toHaveText('Saved position removed.');
 const writes=await page.evaluate(()=>teleportFixture.calls.filter(c=>c.command==='teleport_change').map(c=>c.args.request as TeleportRequest));
 expect(writes.map(r=>r.operation)).toEqual(['save_current','travel_saved','remove_saved']);
 expect(writes[0]).toMatchObject({slot:1,name:'New camp',id:'9007199254740994',session:'18364758544493064720',epoch:'9007199254740995',not_after_ms:'4294967298500'});
 expect(writes.every(r=>!('position' in r))).toBe(true);
});
test('marker ID zero retains its distinct source and unsupported layers cannot travel',async({page})=>{
 await fixture(page);await expect(page.getByLabel('Map marker')).toBeEnabled();
 await page.getByLabel('Map marker').selectOption('map_pin:0');await page.getByRole('button',{name:'Teleport to map marker',exact:true}).click();
 await expect(page.getByRole('status')).toContainText('observed at the destination');
 await page.getByLabel('Map marker').selectOption('waypoint:0');await page.getByRole('button',{name:'Teleport to map marker',exact:true}).click();
 await page.getByLabel('Map marker').selectOption('map_pin:9007199254740999');await expect(page.getByRole('button',{name:'Teleport to map marker',exact:true})).toBeDisabled();
 const writes=await page.evaluate(()=>teleportFixture.calls.filter(c=>c.command==='teleport_change').map(c=>c.args.request as TeleportRequest));
 expect(writes).toHaveLength(2);expect(writes[0]).toMatchObject({operation:'travel_marker',marker:'0',marker_source:'map_pin'});expect(writes[1]).toMatchObject({marker:'0',marker_source:'waypoint'});
 await page.screenshot({path:'../.local/marketplace-teleport-controls-preview.png',fullPage:true});
});
test('lost responses and reloads inspect the original receipt without replay',async({page})=>{
 await fixture(page);await expect(page.getByRole('button',{name:'Teleport to saved position',exact:true})).toBeEnabled();
 await page.evaluate(()=>{teleportFixture.mode='lost'});await page.getByRole('button',{name:'Teleport to saved position',exact:true}).click();
 await expect(page.getByRole('alert')).toHaveText('Owned teleport fixture lost the response.');
 await page.reload();await expect(page.getByRole('button',{name:'Teleport to saved position',exact:true})).toBeEnabled();
 expect(await page.evaluate(()=>teleportFixture.calls.filter(c=>c.command==='teleport_change'))).toHaveLength(0);
 expect(await page.evaluate(()=>localStorage.getItem('crimson-workbench.teleport.pending.v1'))).toBeNull();
});
test('queue acceptance stays pending until arrival and an old receipt never resends',async({page})=>{
 await fixture(page);await expect(page.getByRole('button',{name:'Teleport to saved position',exact:true})).toBeEnabled();
 await page.evaluate(()=>{teleportFixture.mode='queued'});await page.getByRole('button',{name:'Teleport to saved position',exact:true}).click();
 await expect(page.getByRole('status')).toContainText('not an arrival confirmation');await expect(page.getByRole('button',{name:'Teleport to saved position',exact:true})).toBeDisabled();
 await page.evaluate(()=>{const key='crimson-workbench.teleport.pending.v1',p=JSON.parse(localStorage.getItem(key)!);p.recorded_at=Date.now()-70001;localStorage.setItem(key,JSON.stringify(p))});
 await page.reload();await expect(page.getByRole('button',{name:'I checked the result in game',exact:true})).toBeVisible();
 expect(await page.evaluate(()=>teleportFixture.calls.filter(c=>c.command==='teleport_change'))).toHaveLength(0);
});
test('a destination changed during preflight is reviewed before any command',async({page})=>{
 await fixture(page);await expect(page.getByRole('button',{name:'Teleport to saved position',exact:true})).toBeEnabled();
 await page.getByRole('button',{name:'Teleport to saved position',exact:true}).evaluate(button=>{teleportFixture.changeOnRead=true;(button as HTMLButtonElement).click()});
 await expect(page.getByRole('alert')).toHaveText('This saved position changed. Review it before continuing.');
 expect(await page.evaluate(()=>teleportFixture.calls.filter(c=>c.command==='teleport_change'))).toHaveLength(0);
});
test('failed bookmark storage is visible independently of successful capture',async({page})=>{
 await fixture(page);await expect(page.getByLabel('Position name',{exact:true})).toBeEnabled();
 await page.evaluate(()=>{teleportFixture.snapshot.storage_status=2});await page.getByLabel('Position name',{exact:true}).fill('Temporary camp');
 await page.getByRole('button',{name:'Save current position',exact:true}).click();await expect(page.getByRole('status')).toContainText('may be lost');
 await expect(page.getByRole('alert')).toContainText('Bookmark storage failed');
});
test('replacement consent expires when a poll observes different saved-position contents',async({page})=>{
 await fixture(page);await expect(page.getByLabel('Save slot',{exact:true})).toBeEnabled();
 await page.getByLabel('Position name',{exact:true}).fill('Replacement');await page.getByLabel('Save slot',{exact:true}).selectOption('0');
 const consent=page.getByLabel('Replace the saved position in this slot');await consent.check();await expect(page.getByRole('button',{name:'Save current position',exact:true})).toBeEnabled();
 await page.evaluate(()=>{teleportFixture.snapshot.bookmarks[0].name='Changed camp'});await page.getByRole('button',{name:'Refresh teleport status',exact:true}).click();
 await expect(page.getByLabel('Save slot',{exact:true})).toContainText('Changed camp');await expect(consent).not.toBeChecked();
 await expect(page.getByRole('button',{name:'Save current position',exact:true})).toBeDisabled();
 expect(await page.evaluate(()=>teleportFixture.calls.filter(c=>c.command==='teleport_change'))).toHaveLength(0);
});
test('preflight context changes cannot apply an old click to another player or field',async({page})=>{
 await fixture(page);await expect(page.getByRole('button',{name:'Teleport to saved position',exact:true})).toBeEnabled();
 await page.getByRole('button',{name:'Teleport to saved position',exact:true}).evaluate(button=>{teleportFixture.contextOnRead=true;(button as HTMLButtonElement).click()});
 await expect(page.getByRole('alert')).toHaveText('The game, player or field changed. Review the current context before continuing.');
 expect(await page.evaluate(()=>teleportFixture.calls.filter(c=>c.command==='teleport_change'))).toHaveLength(0);
});
test('closed setup performs no native reads or writes',async({page})=>{
 await fixture(page,false);await expect(page.getByRole('button',{name:'Teleport to saved position',exact:true})).toBeDisabled();
 expect(await page.evaluate(()=>teleportFixture.calls)).toHaveLength(0);
});
