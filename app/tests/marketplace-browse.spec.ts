import {expect,test,type Page} from '@playwright/test';

// Same owned Tauri invoke fixture contract as marketplace.spec.ts, with mixed
// installation states so browse filters cannot confuse a draft with an install.
async function fixture(page:Page,{nativeReadError=false}={}) {
 await page.addInitScript(({nativeReadError})=>{
  const root=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;browseCalls:{command:string;args:Record<string,unknown>}[]};
  root.isTauri=true;root.browseCalls=[];
  const ids=['sleep_cooldown','sleep_duration','blackstar','no_fall','player_stamina','mount_jump','player_jump','player_speed','preserve_consumables','mount_protection','free_crafting_materials','force_palm','force_palm_height','axiom_reach','glide_speed','mount_dismount','auto_loot','world_controls','photo_mode','teleport','region_return_30min'];
  const installed=new Set(['world_controls','blackstar']);
  const settings:Record<string,number>={force_palm_height:100,axiom_reach:100,glide_speed:100,player_resources:1};
  let jumpHeight=100,speed=100;
  const status=()=>({game_running:false,mods:ids.map(id=>({id,state:id==='blackstar'&&installed.has(id)?'update_available':installed.has(id)?'installed':'not_installed',message:installed.has(id)?'Installed · start the game to load':'Not installed',can_install:id==='blackstar'||!installed.has(id),can_remove:installed.has(id)}))});
  root.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>={})=>{
   root.browseCalls.push({command,args});
   if(command==='bootstrap')return{project:'C:/Synthetic',discovery:{installations:[{path:'C:/Synthetic Game',platform:'steam'}],configured_game:'C:/Synthetic Game',save_directories:[]},languages:[{language:'eng'},{language:'ger'}]};
   if(command==='open_catalog')return{session:1,game_path:'C:/Synthetic Game',item_language:{language:'eng',source:'steam'},info:{item_count:0,types:[],categories:[],tiers:[],stats:[],groups:[],index:{language:'eng',path:'synthetic',fingerprint:'synthetic'},exe_version:'1.0.0.2976'}};
   if(command==='knowledge_snapshot')return{saves:[],selected_save:null,items:[],message:'No save selected'};
   if(command==='search_items')return{items:[],total:0,offset:0};
   if(command==='mod_info')return{advanced:{statuses:[]}};
   if(command==='spawn_status')throw{message:'Synthetic live module is disconnected'};
   if(command==='marketplace_game_running')return false;
   if(command==='marketplace_mod_status'){if(nativeReadError)throw{message:'Synthetic module status unavailable'};return status()}
   if(command==='marketplace_runtime_setting'){
    const id=String(args.id);if(args.value!=null)settings[id]=Number(args.value);
    return{id,value:settings[id],invalid:false,game_running:false,can_change:true,message:'Settings load at game startup when the corresponding mod is installed.'};
   }
   if(command==='marketplace_player_jump'){if(args.heightPercent!=null)jumpHeight=Number(args.heightPercent);return{height_percent:jumpHeight,game_running:false,can_change:true,message:'Settings load at game startup when the player jump mod is installed.'}}
   if(command==='marketplace_player_speed'){if(args.speedPercent!=null)speed=Number(args.speedPercent);return{speed_percent:speed,game_running:false,can_change:true,message:'Settings load at game startup when the player speed mod is installed.'}}
   if(command==='marketplace_consumables')return{item_keys:[],choices:[],game_running:false,can_change:true,message:'Selection loads at game startup.'};
   if(command==='marketplace_mod_change'){if(args.enabled)installed.add(String(args.id));else installed.delete(String(args.id));return status()}
   throw Error(`Unexpected browse fixture command: ${command}`);
  }};
 },{nativeReadError});
 await page.goto('/');await expect(page.locator('.workbench')).toHaveAttribute('data-ready','true');
 await page.getByRole('button',{name:'Mod marketplace',exact:true}).click();
 await expect(page.locator('.market-card')).toHaveCount(36);
}

const card=(page:Page,name:string)=>page.locator('.market-card').filter({has:page.getByRole('heading',{name,exact:true})});
const filter=(page:Page,name:string)=>page.getByRole('group',{name:'Mod status filter'}).getByRole('button',{name});

test('category, status and translated search combine with accurate counts',async({page})=>{
 await fixture(page);
 await expect(page.locator('.market-browse [role=status]')).toHaveText('36 mods available');
 await card(page,'Larger lion').getByRole('button',{name:'Select mod',exact:true}).click();
 await filter(page,'Installed').click();
 await expect(page.locator('.market-card:visible')).toHaveCount(2);
 await expect(page.locator('.market-browse [role=status]')).toHaveText('2 of 36 mods');
 await expect(card(page,'Larger lion')).toBeHidden();
 await expect(page.locator('.market-browse-note')).toContainText('verified mod files');
 await page.getByLabel('Mod category',{exact:true}).selectOption('WORLD');
 await expect(page.locator('.market-card:visible')).toHaveCount(1);
 await expect(card(page,'Live time & weather')).toBeVisible();
 await page.getByRole('button',{name:'Reset filters',exact:true}).click();
 await filter(page,'Updates').click();
 await expect(page.locator('.market-card:visible')).toHaveCount(1);
 await expect(card(page,'Blackstar anywhere')).toBeVisible();
 await page.getByRole('textbox',{name:'Search mods',exact:true}).fill('Schwarzstern');
 await expect(page.locator('.market-browse [role=status]')).toHaveText('1 of 36 mods');
 await page.getByRole('button',{name:'Clear mod search'}).click();
 await filter(page,'Live controls').click();
 await expect(page.locator('.market-card:visible')).toHaveCount(3);
 await page.getByLabel('Mod category',{exact:true}).selectOption('CAMERA');
 await expect(page.locator('.market-card:visible')).toHaveCount(1);
 await expect(card(page,'Extended photo mode')).toBeVisible();
});

test('filtering keeps all cards mounted and preserves draft selections and startup values',async({page})=>{
 await fixture(page);
 const lion=card(page,'Larger lion'),jump=card(page,'Player jump height');
 await lion.getByRole('button',{name:'Select mod',exact:true}).click();
 await jump.getByRole('slider').fill('175');
 await page.getByRole('textbox',{name:'Search mods',exact:true}).fill('Kraftfaust');
 await expect(page.locator('.market-card:visible')).toHaveCount(2);
 await filter(page,'Live controls').click();
 await expect(page.getByRole('heading',{name:'No mods match your search'})).toBeVisible();
 await expect(page.locator('.market-card')).toHaveCount(36);
 await page.getByRole('button',{name:'Show all mods',exact:true}).click();
 await expect(page.locator('.market-card:visible')).toHaveCount(36);
 await expect(lion.locator('.market-draft-badge')).toHaveText('Draft selected');
 await expect(lion.getByRole('button',{name:'Deselect mod',exact:true})).toHaveAttribute('aria-pressed','true');
 await expect(jump.getByRole('slider')).toHaveValue('175');
 const writes=await page.evaluate(()=>(window as unknown as {browseCalls:{command:string;args:Record<string,unknown>}[]}).browseCalls.filter(call=>call.command==='marketplace_mod_change'||call.command==='marketplace_runtime_setting'&&call.args.value!=null));
 expect(writes).toEqual([]);
});

test('installed filter follows accepted installation and removal snapshots',async({page})=>{
 await fixture(page);
 await page.getByRole('textbox',{name:'Search mods',exact:true}).fill('Gliding speed');
 const glide=card(page,'Gliding speed');
 await glide.getByRole('button',{name:'Install mod',exact:true}).click();
 await filter(page,'Installed').click();
 await expect(glide).toBeVisible();
 await expect(page.locator('.market-browse [role=status]')).toHaveText('1 of 36 mods');
 await glide.getByRole('button',{name:'Remove mod',exact:true}).click();
 await expect(glide).toBeHidden();
 await expect(page.locator('.market-browse [role=status]')).toHaveText('0 of 36 mods');
});

test('unknown installation status does not claim modules are absent or selected drafts installed',async({page})=>{
 await fixture(page,{nativeReadError:true});
 await card(page,'Larger lion').getByRole('button',{name:'Select mod',exact:true}).click();
 await filter(page,'Installed').click();
 await expect(page.locator('.market-card:visible')).toHaveCount(0);
 await expect(page.locator('.market-browse-note')).toContainText('installation status is unavailable');
 await expect(filter(page,'Installed')).toHaveText('Installed');
});

test('search remains usable after scrolling at desktop and compact sizes',async({page})=>{
 await fixture(page);
 for(const width of [1440,1024]){
  await page.setViewportSize({width,height:768});
  await page.locator('.marketplace').evaluate(element=>{element.scrollTop=1200});
  const topbar=(await page.locator('.topbar').boundingBox())!,search=(await page.getByRole('textbox',{name:'Search mods',exact:true}).boundingBox())!;
  expect(search.y).toBeGreaterThanOrEqual(topbar.y+topbar.height-2);
  expect(search.y+search.height).toBeLessThan(300);
  expect(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth)).toBe(false);
 }
 await page.getByRole('textbox',{name:'Search mods',exact:true}).fill('Kraftfaust');
 await expect(page.locator('.market-card:visible')).toHaveCount(2);
 const results=(await card(page,'Unlimited Force Palm jumps').boundingBox())!;
 expect(results.y).toBeGreaterThan(100);expect(results.y).toBeLessThan(350);
 await page.screenshot({path:'../.local/ui-ux-marketplace-browse.png'});
});
