import {expect,test,type Page} from '@playwright/test';

async function fixture(page:Page,{running=false,version='1.0.0.2976',stale=false}={}) {
 await page.addInitScript(({running,version,stale})=>{
  const root=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;marketCalls:{command:string;args:Record<string,unknown>}[]};root.isTauri=true;root.marketCalls=[];
  let unlocked=false;
  const summary=()=>({total:140,new_abilities:unlocked?0:40,upgraded_abilities:unlocked?0:20,already_maxed:unlocked?140:80,gifted_points:unlocked?[0,0,0]:[20,15,10],available_points:[3,2,1]});
  root.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>={})=>{
   root.marketCalls.push({command,args});
   if(command==='bootstrap')return{project:'C:/Synthetic',discovery:{installations:[{path:'C:/Synthetic Game',platform:'steam'}],configured_game:'C:/Synthetic Game'},languages:[{language:'eng'}]};
   if(command==='open_catalog')return{session:1,game_path:args.game,info:{item_count:0,types:[],categories:[],tiers:[],stats:[],index:{language:'eng',path:'synthetic',fingerprint:'synthetic'},exe_version:version}};
   if(command==='search_items')return{items:[],total:0,offset:0};
   if(command==='mod_preview')return{request:args.request,plan_id:'reviewed-plan',fingerprint:'synthetic',changes:[{module:'Marketplace',table:'appearance',key:1,name:'Shared lion model',field:'Scale',before:'0.90',after:'1.17'}],files:[{path:'0041/0.paz',action:'create',before_sha256:null,after_sha256:'sha',bytes:123}],warnings:[],gates:{game_running:running,can_apply:false,reasons:[]},credits:'Synthetic'};
   if(command==='mod_export')return'C:/Synthetic/exports/market-package';
   if(command==='skill_unlock_preview')return{save_root:'C:/Synthetic Saves',saves:[{id:'123/slot0',label:'Manual save 0',modified:100}],selected_save:args.save??null,save_sha256:args.save?'a'.repeat(64):null,lobby_sha256:args.save?'b'.repeat(64):null,plan_id:args.save?'c'.repeat(64):null,summary:args.save?summary():null,game_running:running};
   if(command==='skill_unlock_apply'){
    if(stale)throw{message:'Save changed since preview. Reload first.',code:'read_error'};
    unlocked=true;return{request:args.request,summary:summary(),backup:'C:/Synthetic/.local/skill-unlocks/backup',message:'All 140 abilities unlocked. Available points preserved.'};
   }
   throw Error(`Unexpected command: ${command}`);
  }};
 },{running,version,stale});
 await page.goto('/');await expect(page.locator('.workbench')).toHaveAttribute('data-ready','true');await page.getByRole('button',{name:'Mod marketplace',exact:true}).click();
}
test('four English free mods, combined selection and reviewed export without automatic writes',async({page})=>{
 await fixture(page);await expect(page.locator('.market-card')).toHaveCount(4);await expect(page.locator('.market-free')).toHaveCount(4);
 await page.getByRole('button',{name:'Add to plan',exact:true}).first().click();await page.getByRole('button',{name:'Add to plan',exact:true}).first().click();
 await page.getByRole('button',{name:'Review combined plan'}).click();await expect(page.locator('.market-changes')).toContainText('0.90 → 1.17');
 await page.getByRole('button',{name:'Export local package'}).click();await expect(page.getByRole('status')).toContainText('exports/market-package');
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 const request=calls.find(c=>c.command==='mod_preview')!.args.request as {marketplace:unknown};
 expect(request.marketplace).toEqual({larger_lion:true,faster_mounts:true,no_mount_stamina:false});
 expect(calls.find(c=>c.command==='mod_export')!.args.planId).toBe('reviewed-plan');
 expect(calls.some(c=>['live_start','skill_unlock_apply'].includes(c.command))).toBe(false);
 await page.getByRole('button',{name:'Remove from plan'}).first().click();await expect(page.getByRole('button',{name:'Export local package'})).toHaveCount(0);
 await page.screenshot({path:'../.local/marketplace-preview.png',fullPage:true});
});
test('skills require a selected reviewed save and explicit confirmation; completed action cannot grant again',async({page})=>{
 await fixture(page);await page.getByRole('button',{name:'Choose save & review'}).click();
 const apply=page.getByRole('button',{name:'Back up & unlock all skills'});await expect(apply).toBeDisabled();
 await page.getByLabel('Skill unlock save').selectOption('123/slot0');await expect(page.locator('.skills-summary')).toContainText('40');await expect(apply).toBeDisabled();
 await page.getByLabel('Confirm skill save edit').check();await expect(apply).toBeEnabled();await apply.click();
 await expect(page.getByRole('status')).toContainText('All 140 abilities unlocked');await expect(page.locator('.skills-summary')).toContainText('140');await expect(apply).toBeDisabled();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 const requests=calls.filter(c=>c.command==='skill_unlock_apply');expect(requests).toHaveLength(1);
 expect(requests[0].args.request).toMatchObject({save_root:'C:/Synthetic Saves',save:'123/slot0',save_sha256:'a'.repeat(64),lobby_sha256:'b'.repeat(64),plan_id:'c'.repeat(64)});
});
test('running game blocks skill editing and unsupported build disables every recipe',async({page})=>{
 await fixture(page,{running:true});await page.getByRole('button',{name:'Choose save & review'}).click();await page.getByLabel('Skill unlock save').selectOption('123/slot0');
 await expect(page.getByLabel('Confirm skill save edit')).toBeDisabled();await expect(page.getByRole('button',{name:'Back up & unlock all skills'})).toBeDisabled();
 await page.getByRole('button',{name:'Close',exact:true}).click();
 await page.reload();await page.getByRole('button',{name:'Mod marketplace',exact:true}).click();
});
test('unknown version gives a clear explanation and no enabled install actions',async({page})=>{
 await fixture(page,{version:'1.0.0.9999'});await expect(page.getByRole('alert')).toContainText('require game version 1.0.0.2976');
 for (const button of await page.getByRole('button',{name:'Add to plan',exact:true}).all())await expect(button).toBeDisabled();
 await expect(page.getByRole('button',{name:'Choose save & review'})).toBeDisabled();
});
test('stale save is refused and retry keeps the same request ID',async({page})=>{
 await fixture(page,{stale:true});await page.getByRole('button',{name:'Choose save & review'}).click();await page.getByLabel('Skill unlock save').selectOption('123/slot0');
 await page.getByLabel('Confirm skill save edit').check();await page.getByRole('button',{name:'Back up & unlock all skills'}).click();await expect(page.getByRole('alert')).toContainText('Save changed since preview');
 await page.getByRole('button',{name:'Check / recover pending unlock'}).click();
 const requests=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='skill_unlock_apply'));
 expect(requests).toHaveLength(2);expect(requests[0].args.request).toEqual(requests[1].args.request);
});
