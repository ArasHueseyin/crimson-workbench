import {expect,test,type Page} from '@playwright/test';

async function fixture(page:Page,state='ready',registry='applied') {
  await page.addInitScript(({state,registry})=>{
    const root=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;calls:string[]};
    root.isTauri=true;root.calls=[];
    root.__TAURI_INTERNALS__={invoke:async(command:string)=>{
      root.calls.push(command);
      if(command==='bootstrap')return {project:'/Synthetic',discovery:{installations:[{path:'/Game.app',platform:'steam'}],configured_game:'/Game.app'},languages:[{language:'eng'}]};
      if(command==='open_catalog')return {session:1,game_path:'/Game.app',read_only:true,info:{item_count:0,types:[],categories:[],tiers:[],stats:[],groups:[],index:{language:'eng'}}};
      if(command==='installed_mods'){
        if(registry==='error')throw {message:'CDMods registry unreadable'};
        return {registry_found:registry!=='missing',mods:registry==='missing'?[]:[{id:5,name:'Infinite Stamina Steam Mac Byte Patch',version:'1.0.1',author:'Local custom mod',enabled:true,applied:true},{id:6,name:'Pending mod',version:null,author:null,enabled:true,applied:false},{id:7,name:'Disabled mod',version:null,author:null,enabled:false,applied:false}]};
      }
      if(command==='search_items')return {items:[],total:0,offset:0};
      if(command==='spawn_status'||command==='mount_live_status'){
        if(state==='error')throw {message:'Synthetic connection failure'};
        return {state,pid:state==='offline'?0:42,epoch:7,id:'',key:0,quantity:0,message:state==='offline'?'No live module connected.':'Verified native module connected.'};
      }
      throw {message:`Unexpected command ${command}`};
    }};
  },{state,registry});
  await page.goto('/');await expect(page.locator('.workbench')).toHaveAttribute('data-ready','true');
  await page.evaluate(()=>(window as unknown as {calls:string[]}).calls=[]);
  await page.getByRole('button',{name:'Mod marketplace',exact:true}).click();
}

test('Mac mods show connected capabilities and platform availability in both mod sections',async({page})=>{
  await fixture(page);
  const current=page.getByRole('region',{name:'Current mods'});
  await expect(current).toContainText('Live item spawning');
  await expect(current).toContainText('Live mount registration');
  await expect(current.getByText('Ready',{exact:true})).toHaveCount(2);
  await page.getByLabel('Search Mac mods').fill('stamina');
  const unavailable=page.getByRole('region',{name:'Unavailable on macOS'});
  await expect(unavailable).toContainText('No mount stamina cost');
  await expect(unavailable).toContainText('Unlimited player stamina & spirit');
  await expect(unavailable).not.toContainText('Larger lion');
  await page.getByRole('button',{name:'Mod workshop',exact:true}).click();
  await expect(page.getByRole('heading',{name:'Mac mods.'})).toBeVisible();
  await expect(page.getByLabel('Change shop stock')).toHaveCount(0);
  await page.getByRole('button',{name:'Refresh mod status'}).click();
  await expect(current.getByText('Ready',{exact:true})).toHaveCount(2);
  await page.setViewportSize({width:1024,height:768});
  expect(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth)).toBe(false);
  await page.screenshot({path:'../.local/mac-mods-overview.png'});
  const calls=await page.evaluate(()=>(window as unknown as {calls:string[]}).calls);
  expect(calls).not.toContain('mod_info');
  expect(calls).not.toContain('marketplace_mod_status');
  expect(calls.some(c=>c.includes('grant')||c.includes('launch'))).toBe(false);
});

for(const state of ['offline','error'])test(`Mac module ${state} is not shown as ready or installed`,async({page})=>{
  await fixture(page,state);
  const current=page.getByRole('region',{name:'Current mods'});
  await expect(current.getByText(state==='offline'?'Not connected':'Could not check',{exact:true})).toHaveCount(2);
  await expect(current.getByText('Ready',{exact:true})).toHaveCount(0);
  await expect(page.getByRole('region',{name:'Available on macOS',exact:true})).toContainText('Offline mount registration');
  if(state==='error')await expect(current).toContainText('Synthetic connection failure');
});

test('CDMods stamina is listed as applied while pending and disabled mods remain distinct',async({page})=>{
  await fixture(page);
  const current=page.getByRole('region',{name:'Current mods'});
  const mods=current.getByRole('list',{name:'CDMods registry entries'});
  const stamina=mods.getByRole('listitem').filter({hasText:'Infinite Stamina Steam Mac Byte Patch'});
  await expect(stamina).toContainText('Applied · CDMods');
  await expect(stamina).toContainText('1.0.1');
  await expect(mods.getByRole('listitem').filter({hasText:'Pending mod'})).toContainText('Enabled · not applied');
  await expect(mods.getByRole('listitem').filter({hasText:'Disabled mod'})).toContainText('Disabled');
  await expect(current).toContainText('Registry status does not verify the current in-game effect');
});
for(const registry of ['missing','error'])test(`CDMods ${registry} does not claim there are no installed mods`,async({page})=>{
  await fixture(page,'offline',registry);
  const current=page.getByRole('region',{name:'Current mods'});
  await expect(current).toContainText(registry==='error'?'CDMods registry unreadable':'No CDMods registry found');
  await expect(current.getByRole('list',{name:'CDMods registry entries'})).toHaveCount(0);
});
