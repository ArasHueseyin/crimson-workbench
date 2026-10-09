import {test,expect,type Page} from '@playwright/test';
type Harness={calls:{command:string;args:Record<string,unknown>}[];approved:boolean;empty:boolean;issue:string|null;fail:boolean;defer:boolean;release:(()=>void)|null};
async function fixture(page:Page){
 await page.addInitScript(()=>{
  const w=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;foreign:Harness};w.isTauri=true;w.foreign={calls:[],approved:false,empty:false,issue:null,fail:false,defer:false,release:null};
  w.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>={})=>{
   const h=w.foreign;h.calls.push({command,args});
   if(command==='bootstrap')return{project:'C:/Synthetic',discovery:{installations:[{path:'C:/Synthetic Game',platform:'steam'}],configured_game:'C:/Synthetic Game'},languages:[{language:'ger'}]};
   if(command==='open_catalog')return{session:1,game_path:args.game,info:{item_count:0,types:[],categories:[],tiers:[],stats:[],index:{language:'ger',path:'synthetic',fingerprint:'synthetic'},exe_version:'synthetic'}};
   if(command==='search_items')return{items:[],total:0,offset:0};
   if(command==='installation_audit_status')return null;
   if(command==='mod_info')return{vendors:[],dropsets:[],store_rows:0,stock_rows:0,opaque_stores:0,opaque_dropsets:0,quantity_excluded:0,trust_rows:0,limitations:[],items:[],append_vendors:[],daily_vendors:[],chance_dropsets:[],guarantee_dropsets:[]};
   if(command==='foreign_preview'){
    if(h.defer)await new Promise<void>(resolve=>{h.release=resolve});
    return{approval_id:h.approved?'approval-token':null,approved_files:h.approved?2:0,issue:h.issue,review:h.issue?null:{review_id:'review-token',game_path:'C:/Synthetic Game',files:h.empty?[]:[{path:'dxgi.dll',bytes:20,sha256:'a'.repeat(64)},{path:'0041/0.paz',bytes:40,sha256:'b'.repeat(64)}],directories:h.empty?[]:['0041'],total_bytes:h.empty?'0':'60',previous_approval:h.approved?'approval-token':null,game_running:true}};
   }
   if(command==='foreign_confirm'){if(h.fail)throw{message:'Fremddateivorschau veraltet; nichts bestätigt'};h.approved=true;return{approval_id:'approval-token',files:2,directories:1,action:'preserve_foreign'}}
   if(command==='foreign_revoke'){h.approved=false;return{approval_id:'revoked-token',files:0,directories:0,action:'revoke_foreign'}}
   throw Error(`Unexpected ${command}`);
  }};
 });
 await page.goto('/');await expect(page.locator('.workbench')).toHaveAttribute('data-ready','true');await page.getByRole('button',{name:'Modwerkstatt',exact:true}).click();return page.getByRole('region',{name:'Fremde Zusatzdateien',exact:true});
}
test('foreign consent is opt-in, exact and project-only even while the game runs',async({page})=>{
 const panel=await fixture(page);
 expect(await page.evaluate(()=>(window as unknown as {foreign:Harness}).foreign.calls.some(c=>c.command.startsWith('foreign_')))).toBe(false);
 await panel.getByRole('button',{name:'Zusatzdateien prüfen',exact:true}).click();await expect(panel).toContainText('dxgi.dll');await expect(panel).toContainText('0041/0.paz');
 const button=panel.getByRole('button',{name:'Geprüfte Zusatzdateien bestätigen'});await expect(button).toBeDisabled();
 await panel.getByLabel('Zusatzdateien ausdrücklich beibehalten').check();await button.click();await expect(panel).toContainText('Spieldateien unverändert');
 const calls=await page.evaluate(()=>(window as unknown as {foreign:Harness}).foreign.calls);
 expect(calls.filter(c=>c.command==='foreign_confirm').map(c=>c.args)).toEqual([{session:1,reviewId:'review-token',preserve:true}]);
 expect(calls.some(c=>['live_start','installation_audit_start'].includes(c.command))).toBe(false);
});
test('revocation remains available when a foreign registry blocks new consent',async({page})=>{
 const panel=await fixture(page);await page.evaluate(()=>{const h=(window as unknown as {foreign:Harness}).foreign;h.approved=true;h.issue='Fremde Registry; Steam-Dateiprüfung abschließen'});
 await panel.getByRole('button',{name:'Zusatzdateien prüfen',exact:true}).click();await expect(panel.getByRole('alert')).toContainText('Fremde Registry');await expect(panel.getByRole('button',{name:'Geprüfte Zusatzdateien bestätigen'})).toHaveCount(0);
 await panel.getByRole('button',{name:'Bestätigung zurücknehmen'}).click();await expect(panel).toContainText('Zusatzdateien bleiben erhalten');
 expect(await page.evaluate(()=>(window as unknown as {foreign:Harness}).foreign.calls.filter(c=>c.command==='foreign_revoke').map(c=>c.args))).toEqual([{session:1,approvalId:'approval-token'}]);
});
test('stale confirmation is cleared without retry and needs a new checkbox',async({page})=>{
 const panel=await fixture(page);await page.evaluate(()=>{(window as unknown as {foreign:Harness}).foreign.fail=true});await panel.getByRole('button',{name:'Zusatzdateien prüfen',exact:true}).click();await panel.getByLabel('Zusatzdateien ausdrücklich beibehalten').check();await panel.getByRole('button',{name:'Geprüfte Zusatzdateien bestätigen'}).click();
 await expect(panel.getByRole('alert')).toContainText('veraltet');await expect(panel.getByRole('button',{name:'Geprüfte Zusatzdateien bestätigen'})).toHaveCount(0);
 await panel.getByRole('button',{name:'Zusatzdateien prüfen',exact:true}).click();await expect(panel.getByLabel('Zusatzdateien ausdrücklich beibehalten')).not.toBeChecked();
 expect(await page.evaluate(()=>(window as unknown as {foreign:Harness}).foreign.calls.filter(c=>c.command==='foreign_confirm'))).toHaveLength(1);
});
test('empty inventory creates no consent and file hashes fit at 1024px',async({page})=>{
 const panel=await fixture(page);await page.setViewportSize({width:1024,height:768});await panel.getByRole('button',{name:'Zusatzdateien prüfen',exact:true}).click();
 expect(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth)).toBe(false);
 await page.evaluate(()=>{(window as unknown as {foreign:Harness}).foreign.empty=true});await panel.getByRole('button',{name:'Zusatzdateien prüfen',exact:true}).click();await expect(panel).toContainText('Keine Bestätigung erforderlich');await expect(panel.getByRole('button',{name:'Geprüfte Zusatzdateien bestätigen'})).toHaveCount(0);
});
test('late preview after leaving the view cannot carry consent into a fresh view',async({page})=>{
 const panel=await fixture(page);await page.evaluate(()=>{(window as unknown as {foreign:Harness}).foreign.defer=true});await panel.getByRole('button',{name:'Zusatzdateien prüfen',exact:true}).click();
 await page.getByRole('button',{name:/Itemdatenbank/}).click();await page.evaluate(()=>{(window as unknown as {foreign:Harness}).foreign.release?.()});await page.getByRole('button',{name:'Modwerkstatt',exact:true}).click();await expect(panel.getByLabel('Zusatzdateien ausdrücklich beibehalten')).toHaveCount(0);
});
