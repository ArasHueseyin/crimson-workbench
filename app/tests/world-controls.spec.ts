import {expect,test,type Page} from '@playwright/test';
import type {WorldRequest,WorldSnapshot} from '../src/world-types';

type FixtureState={snapshot:WorldSnapshot;calls:{command:string;args:Record<string,unknown>}[];mode:'normal'|'lost'|'queued';offline:boolean};
declare global {interface Window {worldFixture:FixtureState} var worldFixture:FixtureState}
async function fixture(page:Page,enabled=true){
 await page.addInitScript(()=>{
  const root=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;worldFixture:FixtureState};root.isTauri=true;
  const initial:WorldSnapshot={pid:123,state:'ready',id:'9007199254740993',session:'18364758544493064720',generation:'9007199254740995',monotonic_ms:'4294967296000',reason:0,flags:3,identity:12,world:34,stage:56,clock_reason:0,weather_reason:0,time:{day:7,hour:12,minute:34,second:56,millisecond:789,weekday:0,rate:12,anchor:'9007199254740997'},weather:{precipitation:0,cloudiness:0,humidity:0.5,wind_speed:10,height_fog:1,phase:0},time_receipt:'9007199254740992',weather_receipt:'9007199254740991',time_result:'published',time_result_reason:0,weather_result:'published',weather_result_reason:0};
  const saved=localStorage.getItem('world-fixture.native');
  const f:FixtureState={snapshot:saved?JSON.parse(saved):initial,calls:[],mode:'normal',offline:false};root.worldFixture=f;
  root.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>={})=>{
   f.calls.push({command,args});if(f.offline)throw{message:'Owned fixture disconnected.'};
   if(command==='world_status')return structuredClone(f.snapshot);
   if(command!=='world_change')throw Error(`Unexpected command ${command}`);
   const r=args.request as WorldRequest,s=f.snapshot;s.id=r.id;s.state='published';s.reason=0;
   if(r.operation.endsWith('weather')){s.weather_receipt=r.id;s.weather_result='published';s.weather_result_reason=0}
   else{s.time_receipt=r.id;s.time_result='published';s.time_result_reason=0}
   if(r.operation==='choose_time')s.time={...s.time,...r.time!};
   if(r.operation==='advance_time'){
    const hours=s.time.hour+r.hours!,days=Math.floor(hours/24);
    s.time={...s.time,day:s.time.day+days,hour:hours%24,weekday:(s.time.weekday+days)%7};
   }
   if(r.operation==='freeze_time'){s.flags|=4;s.time.rate=0}
   if(r.operation==='release_time'){s.flags&=~4;s.time.rate=12;s.state='released';s.time_result='released'}
   if(r.operation==='choose_weather')s.weather=structuredClone(r.weather!);
   if(r.operation==='freeze_weather')s.flags=r.enabled?s.flags|8:s.flags&~8;
   if(r.operation==='release_weather'){s.flags&=~8;s.state='released';s.weather_result='released'}
   if(f.mode!=='normal'){s.state='queued';if(r.operation.endsWith('weather'))s.weather_result='queued';else s.time_result='queued'}
   localStorage.setItem('world-fixture.native',JSON.stringify(s));
   if(f.mode==='lost')throw{message:'Owned fixture lost the response.'};
   return structuredClone(s);
  }};
 });
 await page.goto(`/tests/world-controls-fixture.html?enabled=${enabled}`);
 await expect(page.getByLabel('Live time and weather controls')).toBeVisible();
}
test('live edits are explicit and time and weather freeze independently',async({page})=>{
 await fixture(page);await expect(page.getByRole('button',{name:'Set time',exact:true})).toBeEnabled();
 expect(await page.evaluate(()=>worldFixture.calls.some(c=>c.command==='world_change'))).toBe(false);
 await page.getByLabel('Choose time of day').fill('18:45');
 await page.getByRole('button',{name:'Set time',exact:true}).click();
 await expect(page.locator('.world-feedback').getByRole('status')).toHaveText('Time updated.');
 await page.getByRole('button',{name:'Freeze time',exact:true}).click();
 await expect(page.getByRole('button',{name:'Resume time',exact:true})).toBeEnabled();
 await expect(page.getByRole('button',{name:'Freeze weather',exact:true})).toBeEnabled();
 await page.getByLabel('Precipitation type').selectOption('1');
 await page.getByLabel('Precipitation intensity').fill('0.75');
 await page.getByLabel('Cloudiness',{exact:true}).fill('0.6');
 await page.getByLabel('Wind strength',{exact:true}).fill('25');
 let calls=await page.evaluate(()=>worldFixture.calls.filter(c=>c.command==='world_change'));
 expect(calls).toHaveLength(2); // Editing controls never sends a command.
 await page.getByRole('button',{name:'Apply weather',exact:true}).click();
 await expect(page.locator('.world-feedback').getByRole('status')).toHaveText('Weather updated.');
 await page.getByRole('button',{name:'Freeze weather',exact:true}).click();
 await expect(page.getByRole('button',{name:'Unfreeze weather',exact:true})).toBeEnabled();
 await expect(page.getByRole('button',{name:'Resume time',exact:true})).toBeEnabled();
 await page.getByRole('button',{name:'Unfreeze weather',exact:true}).click();
 await page.getByRole('button',{name:'Resume time',exact:true}).click();
 await expect(page.getByRole('button',{name:'Freeze time',exact:true})).toBeEnabled();
 calls=await page.evaluate(()=>worldFixture.calls.filter(c=>c.command==='world_change'));
 const requests=calls.map(c=>c.args.request as WorldRequest);
 expect(requests.map(r=>r.operation)).toEqual(['choose_time','freeze_time','choose_weather','freeze_weather','freeze_weather','release_time']);
 expect(requests[0]).toMatchObject({id:'9007199254740994',session:'18364758544493064720',generation:'9007199254740995',time:{hour:18,minute:45,second:0,millisecond:0},weather:null,enabled:null});
 expect(requests[2].weather).toMatchObject({phase:1,precipitation:0.75,cloudiness:0.6,wind_speed:25});
 expect(new Set(requests.map(r=>r.id)).size).toBe(requests.length);
 await page.screenshot({path:'../.local/marketplace-world-controls-preview.png'});
});
test('lost response and reload query the original request without sending it again',async({page})=>{
 await fixture(page);await expect(page.getByRole('button',{name:'Freeze time',exact:true})).toBeEnabled();
 await page.evaluate(()=>{worldFixture.mode='lost'});
 await page.getByRole('button',{name:'Freeze time',exact:true}).click();
 await expect(page.getByRole('alert')).toHaveText('Owned fixture lost the response.');
 expect(await page.evaluate(()=>worldFixture.calls.filter(c=>c.command==='world_change').length)).toBe(1);
 const original=await page.evaluate(()=>worldFixture.snapshot.time_receipt);
 await page.reload();await expect(page.getByRole('button',{name:'Set time',exact:true})).toBeDisabled();
 await expect(page.locator('.world-feedback').getByRole('status')).toContainText('will not be sent again');
 expect(await page.evaluate(()=>worldFixture.calls.filter(c=>c.command==='world_change').length)).toBe(0);
 await page.evaluate(()=>{const s=worldFixture.snapshot;s.time_result='published';s.state='released';s.id='0'});
 await page.getByRole('button',{name:'Check live status',exact:true}).click();
 await expect(page.locator('.world-feedback').getByRole('status')).toHaveText('Time updated.');
 expect(await page.evaluate(()=>worldFixture.snapshot.time_receipt)).toBe(original);
 expect(await page.evaluate(()=>worldFixture.calls.filter(c=>c.command==='world_change').length)).toBe(0);
});

test('reset weather releases only the weather override and keeps the time freeze',async({page})=>{
 await fixture(page);await expect(page.getByRole('button',{name:'Freeze time',exact:true})).toBeEnabled();
 await page.getByRole('button',{name:'Freeze time',exact:true}).click();
 await expect(page.getByRole('button',{name:'Resume time',exact:true})).toBeEnabled();
 await page.getByRole('button',{name:'Apply weather',exact:true}).click();
 await page.getByRole('button',{name:'Freeze weather',exact:true}).click();
 await expect(page.getByRole('button',{name:'Unfreeze weather',exact:true})).toBeEnabled();
 await page.getByRole('button',{name:'Reset weather',exact:true}).click();
 await expect(page.locator('.world-feedback').getByRole('status')).toHaveText('Previous weather control restored.');
 await expect(page.getByRole('button',{name:'Freeze weather',exact:true})).toBeEnabled();
 await expect(page.getByRole('button',{name:'Resume time',exact:true})).toBeEnabled();
 const requests=await page.evaluate(()=>worldFixture.calls.filter(c=>c.command==='world_change').map(c=>c.args.request as WorldRequest));
 expect(requests.map(r=>r.operation)).toEqual(['freeze_time','choose_weather','freeze_weather','release_weather']);
 expect(requests.at(-1)).toMatchObject({time:null,weather:null,enabled:null});
 expect(await page.evaluate(()=>worldFixture.snapshot.flags&4)).toBe(4);
 expect(await page.evaluate(()=>worldFixture.snapshot.time.rate)).toBe(0);
});
test('advance time moves forward across days and preserves both active freezes',async({page})=>{
 await fixture(page);await expect(page.getByRole('button',{name:'Freeze time',exact:true})).toBeEnabled();
 await page.getByLabel('Choose time of day').fill('23:45');
 await page.getByRole('button',{name:'Set time',exact:true}).click();
 await page.getByRole('button',{name:'Freeze time',exact:true}).click();
 await page.getByLabel('Precipitation intensity').fill('0.75');
 await page.getByRole('button',{name:'Apply weather',exact:true}).click();
 await page.getByRole('button',{name:'Freeze weather',exact:true}).click();
 const before=await page.evaluate(()=>structuredClone(worldFixture.snapshot));
 const count=await page.evaluate(()=>worldFixture.calls.filter(call=>call.command==='world_change').length);
 const input=page.getByLabel('Hours to advance'),advance=page.getByRole('button',{name:'Advance time',exact:true});
 for(const invalid of ['0','-1','1.5','10001']){await input.fill(invalid);await expect(advance).toBeDisabled()}
 await input.fill('48');await expect(advance).toBeEnabled();
 expect(await page.evaluate(()=>worldFixture.calls.filter(call=>call.command==='world_change').length)).toBe(count);
 await advance.click();await expect(page.locator('.world-feedback').getByRole('status')).toHaveText('Time advanced.');
 const after=await page.evaluate(()=>structuredClone(worldFixture.snapshot));
 expect(after.time).toMatchObject({day:before.time.day+2,hour:23,minute:45,rate:0});
 expect(after.flags&12).toBe(12);expect(after.weather).toEqual(before.weather);
 await expect(page.locator('.world-current').first()).toContainText(`Day ${before.time.day+2}`);
 const request=await page.evaluate(()=>worldFixture.calls.filter(call=>call.command==='world_change').at(-1)!.args.request as WorldRequest);
 expect(request).toMatchObject({operation:'advance_time',hours:48,time:null,weather:null,enabled:null});
 await page.screenshot({path:'../.local/marketplace-advance-time-preview.png'});
});

test('disabled setup, native unavailable states and blocked history prevent live mutations',async({page})=>{
 await fixture(page,false);await expect(page.getByRole('button',{name:'Set time',exact:true})).toBeDisabled();
 expect(await page.evaluate(()=>worldFixture.calls)).toHaveLength(0);
 await page.goto('/tests/world-controls-fixture.html');
 await expect(page.getByRole('button',{name:'Set time',exact:true})).toBeEnabled();
 await page.evaluate(()=>{worldFixture.snapshot.flags=2;worldFixture.snapshot.reason=8});
 await page.getByRole('button',{name:'Check live status',exact:true}).click();
 await expect(page.getByRole('button',{name:'Set time',exact:true})).toBeDisabled();
 await expect(page.getByRole('button',{name:'Apply weather',exact:true})).toBeEnabled();
 await page.evaluate(()=>{localStorage.setItem('crimson-workbench.world.pending.v1','invalid')});
 await page.reload();await expect(page.getByRole('alert')).toContainText('history could not be read');
 await expect(page.getByRole('button',{name:'Apply weather',exact:true})).toBeDisabled();
 expect(await page.evaluate(()=>worldFixture.calls.some(c=>c.command==='world_change'))).toBe(false);
});

test('world change keeps an unresolved command blocked until its result is acknowledged',async({page})=>{
 await fixture(page);await expect(page.getByRole('button',{name:'Set time',exact:true})).toBeEnabled();
 await page.evaluate(()=>{worldFixture.mode='queued'});
 await page.getByRole('button',{name:'Set time',exact:true}).click();
 await expect(page.locator('.world-feedback').getByRole('status')).toContainText('Waiting for the game');
 await page.evaluate(()=>{worldFixture.snapshot.generation='9007199254741001';worldFixture.snapshot.id='0';worldFixture.snapshot.state='ready';worldFixture.mode='normal'});
 await page.getByRole('button',{name:'Check live status',exact:true}).click();
 await expect(page.locator('.world-feedback').getByRole('status')).toContainText('world or character changed');
 await expect(page.getByRole('button',{name:'Apply weather',exact:true})).toBeDisabled();
 expect(await page.evaluate(()=>worldFixture.calls.filter(c=>c.command==='world_change').length)).toBe(1);
 await page.getByRole('button',{name:'I checked the result in game',exact:true}).click();
 await expect(page.getByRole('button',{name:'Apply weather',exact:true})).toBeEnabled();
 await page.getByRole('button',{name:'Apply weather',exact:true}).click();
 const requests=await page.evaluate(()=>worldFixture.calls.filter(c=>c.command==='world_change').map(c=>c.args.request as WorldRequest));
 expect(requests).toHaveLength(2);expect(requests[1].generation).toBe('9007199254741001');
 expect(BigInt(requests[1].id)).toBeGreaterThan(BigInt(requests[0].id));
});
