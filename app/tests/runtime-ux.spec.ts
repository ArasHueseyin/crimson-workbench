import {test,expect,type Page} from '@playwright/test';
async function fixture(page:Page){
 await page.addInitScript(()=>{
  const root=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;ux:{running:boolean;delay:boolean;fail:boolean;readFailure:string;palmSaved:number;calls:{command:string;args:Record<string,unknown>}[];release:()=>void}};
  root.isTauri=true;let resolve:(()=>void)|undefined;
  root.ux={running:false,delay:false,fail:false,readFailure:'',palmSaved:100,calls:[],release:()=>resolve?.()};
  const installed=new Set<string>(['photo_mode']);
  const ids=['glide_speed','teleport','photo_mode','world_controls','auto_loot','mount_dismount','force_palm','force_palm_height','axiom_reach','free_crafting_materials','mount_protection','preserve_consumables','player_speed','player_jump','mount_jump','player_stamina','no_fall','sleep_cooldown','sleep_duration','blackstar','region_return_30min'];
  const settings:Record<string,number>={player_jump:100,player_speed:100,force_palm_height:100,axiom_reach:100,glide_speed:100,player_resources:1};
  let selected:number[]=[];
  const status=()=>({game_running:root.ux.running,mods:ids.map(id=>({id,state:installed.has(id)?'installed':'not_installed',message:installed.has(id)?'Installed files':'Not installed',can_install:!root.ux.running&&!installed.has(id),can_remove:!root.ux.running&&installed.has(id)}))});
  root.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>={})=>{
   root.ux.calls.push({command,args});
   const write=command==='marketplace_mod_change'||command==='marketplace_runtime_setting'&&args.value!=null||command==='marketplace_player_jump'&&args.heightPercent!=null||command==='marketplace_player_speed'&&args.speedPercent!=null||command==='marketplace_consumables'&&args.itemKeys!=null;
   if(write){if(root.ux.delay)await new Promise<void>(done=>{resolve=done});if(root.ux.fail)throw {message:'Owned test: save refused'};if(root.ux.running)throw {message:'Close Crimson Desert first.'}}
   if(command==='marketplace_mod_status')return status();
   if(command==='marketplace_game_running')return root.ux.running;
   if(command==='marketplace_mod_change'){if(args.enabled)installed.add(String(args.id));else installed.delete(String(args.id));return status()}
   if(command==='marketplace_runtime_setting'){const id=String(args.id);if(args.value==null&&root.ux.readFailure===id)throw {message:'Owned test: startup read unavailable'};if(args.value!=null){settings[id]=Number(args.value);if(id==='force_palm_height')root.ux.palmSaved=Number(args.value)}return {id,value:id==='force_palm_height'?root.ux.palmSaved:settings[id],invalid:false,game_running:root.ux.running,can_change:!root.ux.running,message:'Loads at the next game start.'}}
   if(command==='marketplace_player_jump'){if(args.heightPercent!=null)settings.player_jump=Number(args.heightPercent);return {height_percent:settings.player_jump,game_running:root.ux.running,can_change:!root.ux.running,message:'Loads at the next game start.'}}
   if(command==='marketplace_player_speed'){if(args.speedPercent!=null)settings.player_speed=Number(args.speedPercent);return {speed_percent:settings.player_speed,game_running:root.ux.running,can_change:!root.ux.running,message:'Loads at the next game start.'}}
   if(command==='marketplace_consumables'){if(args.itemKeys!=null)selected=[...(args.itemKeys as number[])];return {item_keys:selected,choices:[{key:1,name:'Test Arrow',kind:'Arrow'},{key:2,name:'Test Potion',kind:'Potion'}],game_running:root.ux.running,can_change:!root.ux.running,message:'Loads at the next game start.'}}
   throw {message:`Unexpected owned command: ${command}`};
  }};
 });
 await page.goto('/tests/runtime-ux-fixture.html');
 await expect(page.getByRole('slider',{name:'Gliding speed multiplier'})).toBeEnabled();
}
const card=(page:Page,title:string)=>page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:title,exact:true})});

test('region countdown installs and removes independently and locks setup while the game runs',async({page})=>{
 await fixture(page);const region=card(page,'More time beyond restricted regions');
 await expect(region.getByText('Apply with game closed',{exact:true})).toBeVisible();
 await expect(region.getByRole('button',{name:'Select mod',exact:true})).toHaveCount(0);
 await region.getByRole('button',{name:'Install mod',exact:true}).click();
 await expect(region.getByText('Installed · start game',{exact:true})).toBeVisible();
 await page.evaluate(()=>(window as unknown as {ux:{running:boolean}}).ux.running=true);
 await expect(region.getByRole('button',{name:'Remove mod',exact:true})).toBeDisabled();
 await page.evaluate(()=>(window as unknown as {ux:{running:boolean}}).ux.running=false);
 await expect(region.getByRole('button',{name:'Remove mod',exact:true})).toBeEnabled();
 await region.getByRole('button',{name:'Remove mod',exact:true}).click();
 await expect(region.getByRole('button',{name:'Install mod',exact:true})).toBeEnabled();
 const writes=await page.evaluate(()=>(window as unknown as {ux:{calls:{command:string;args:Record<string,unknown>}[]}}).ux.calls.filter(c=>c.command==='marketplace_mod_change'));
 expect(writes.map(c=>({id:c.args.id,enabled:c.args.enabled}))).toEqual([{id:'region_return_30min',enabled:true},{id:'region_return_30min',enabled:false}]);
});
test('startup drafts survive navigation, catalog reads, refresh and page reload until explicitly discarded',async({page})=>{
 await fixture(page);const glide=card(page,'Gliding speed');
 await glide.getByRole('spinbutton',{name:'Gliding speed exact value'}).fill('275');await expect(glide.getByText('Unsaved settings',{exact:true})).toBeVisible();
 await page.getByRole('button',{name:'Refresh runtime status'}).click();await expect(glide.getByRole('slider')).toHaveValue('275');
 await page.getByRole('button',{name:'Reload catalog'}).click();await expect(glide.getByRole('slider')).toHaveValue('275');
 await page.getByRole('button',{name:'Leave runtime'}).click();await page.getByRole('button',{name:'Return to runtime'}).click();await expect(glide.getByRole('slider')).toHaveValue('275');
 await page.reload();await expect(glide.getByRole('slider')).toHaveValue('275');
 await glide.getByRole('button',{name:'Discard changes'}).click();await expect(glide.getByRole('slider')).toHaveValue('100');
 await expect(glide.getByText('Unsaved settings',{exact:true})).toHaveCount(0);
 expect(await page.evaluate(()=>(window as unknown as {ux:{calls:{command:string;args:Record<string,unknown>}[]}}).ux.calls.filter(c=>c.command==='marketplace_mod_change'||c.command==='marketplace_runtime_setting'&&c.args.value!=null))).toHaveLength(0);
});
test('busy and errors remain on the originating card, failed drafts stay available',async({page})=>{
 await fixture(page);const glide=card(page,'Gliding speed'),palm=card(page,'Force Palm jump height');
 await glide.getByRole('slider').fill('300');await page.evaluate(()=>{const state=(window as unknown as {ux:{delay:boolean;fail:boolean}}).ux;state.delay=true;state.fail=true});
 await glide.getByRole('button',{name:'Save gliding speed'}).click();await expect(glide).toHaveAttribute('aria-busy','true');await expect(glide.locator('.runtime-feedback')).toContainText('Saving settings');
 await expect(palm.locator('.spin')).toHaveCount(0);await expect(palm.locator('.runtime-feedback')).toHaveCount(0);
 await page.evaluate(()=>(window as unknown as {ux:{release:()=>void}}).ux.release());await expect(glide.getByRole('alert')).toContainText('save refused');await expect(glide.getByRole('slider')).toHaveValue('300');await expect(palm.getByRole('alert')).toHaveCount(0);
 await page.evaluate(()=>{const state=(window as unknown as {ux:{delay:boolean;fail:boolean}}).ux;state.delay=false;state.fail=false});
 await glide.getByRole('button',{name:'Save gliding speed'}).click();await expect(glide.locator('.runtime-feedback')).toContainText('Startup settings saved');await expect(glide.getByText('Unsaved settings',{exact:true})).toHaveCount(0);
});
test('installed files are distinguished from loaded runtime state',async({page})=>{
 await fixture(page);const photo=card(page,'Extended photo mode'),glide=card(page,'Gliding speed');
 await expect(photo.getByText('Installed · start game',{exact:true})).toBeVisible();await glide.getByRole('button',{name:'Install mod',exact:true}).click();await expect(glide.getByText('Installed · start game',{exact:true})).toBeVisible();
 await page.evaluate(()=>(window as unknown as {ux:{running:boolean}}).ux.running=true);await expect(glide.getByText('Installed · load unverified',{exact:true})).toBeVisible();
 await expect(glide.getByRole('button',{name:'Remove mod'})).toBeDisabled();await expect(glide.getByRole('slider')).toBeDisabled();
 await expect(page.getByText('“Installed” confirms the files on disk', {exact:false})).toBeVisible();
});
test('precise input and presets make normal Axiom reach explicit and refuse the unsupported gap',async({page})=>{
 await fixture(page);const axiom=card(page,'Axiom armlet reach');
 await expect(axiom.getByRole('radio',{name:'Normal reach · 1×',exact:true})).toBeChecked();await expect(axiom.locator('output')).toHaveText('1× · Normal');
 await axiom.getByRole('radio',{name:'Extended reach · 2×–5×',exact:true}).check();await expect(axiom.locator('output')).toHaveText('2×');
 const exact=axiom.getByRole('spinbutton',{name:'Axiom reach exact value'});await exact.fill('2.73');await expect(axiom.locator('output')).toHaveText('2.73×');
 await exact.fill('1.5');await expect(exact).toHaveAttribute('aria-invalid','true');await expect(axiom.getByRole('button',{name:'Save axiom reach'})).toBeDisabled();
 await axiom.getByRole('group',{name:'Axiom reach presets'}).getByRole('button',{name:'3×',exact:true}).click();await expect(exact).toHaveValue('3');await axiom.getByRole('button',{name:'Save axiom reach'}).click();await expect(axiom.locator('.runtime-feedback')).toContainText('Startup settings saved');
 await page.screenshot({path:'../.local/runtime-ux-presets-preview.png',fullPage:false});
});
test('native dialogs contain keyboard focus, close with Escape and restore their opener',async({page})=>{
 await fixture(page);const open=page.getByRole('button',{name:'Open fixture dialog'});await open.click();
 const dialog=page.getByRole('dialog',{name:'Owned accessible dialog'});await expect(dialog).toBeVisible();await expect(dialog.getByRole('button',{name:'Close',exact:true})).toBeFocused();
 await dialog.getByRole('button',{name:'Last dialog action'}).focus();await page.keyboard.press('Tab');await expect(dialog.getByRole('button',{name:'Close',exact:true})).toBeFocused();
 await page.keyboard.press('Shift+Tab');await expect(dialog.getByRole('button',{name:'Last dialog action'})).toBeFocused();
 await page.screenshot({path:'../.local/runtime-ux-dialog-preview.png',fullPage:false});
 await page.keyboard.press('Escape');await expect(dialog).toHaveCount(0);await expect(open).toBeFocused();
});
test('invalid exact input blocks installation even when its previous value equals the saved value',async({page})=>{
 await fixture(page);const axiom=card(page,'Axiom armlet reach');
 await axiom.getByRole('spinbutton',{name:'Axiom reach exact value'}).fill('1.5');
 await expect(axiom.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 await axiom.getByRole('spinbutton',{name:'Axiom reach exact value'}).fill('1');await expect(axiom.getByRole('button',{name:'Install mod',exact:true})).toBeEnabled();
 const jump=card(page,'Player jump height');await jump.getByRole('spinbutton',{name:'Jump height exact value'}).fill('');
 await expect(jump.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 const speed=card(page,'Player movement speed');await speed.getByRole('spinbutton',{name:'Movement speed exact value'}).fill('999');
 await expect(speed.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
});
test('busy save dialogs retain their Close and Escape guard',async({page})=>{
 await fixture(page);await page.getByRole('button',{name:'Open fixture dialog'}).click();const dialog=page.getByRole('dialog',{name:'Owned accessible dialog'});
 await dialog.getByRole('checkbox',{name:'Block dialog close'}).check();await expect(dialog.getByRole('button',{name:'Close',exact:true})).toBeDisabled();
 await page.keyboard.press('Escape');await expect(dialog).toBeVisible();
 await dialog.getByRole('checkbox',{name:'Block dialog close'}).uncheck();await page.keyboard.press('Escape');await expect(dialog).toHaveCount(0);
});
test('a failed partial refresh revokes saved-state authority and retains the unsaved draft until a fresh retry',async({page})=>{
 await fixture(page);const palm=card(page,'Force Palm jump height'),glide=card(page,'Gliding speed');
 await palm.getByRole('slider').fill('500');
 await page.evaluate(()=>(window as unknown as {ux:{readFailure:string}}).ux.readFailure='force_palm_height');
 await page.getByRole('button',{name:'Refresh runtime status'}).click();
 await expect(palm.getByText('Settings unavailable · draft retained',{exact:true})).toBeVisible();
 await expect(palm.getByRole('slider')).toHaveValue('500');await expect(palm.getByRole('slider')).toBeDisabled();
 await expect(palm.getByRole('button',{name:'Save force palm height'})).toBeDisabled();await expect(palm.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 await expect(glide.getByRole('slider')).toBeEnabled();
 await page.evaluate(()=>{const state=(window as unknown as {ux:{readFailure:string;palmSaved:number}}).ux;state.readFailure='';state.palmSaved=200});
 await page.getByRole('button',{name:'Refresh runtime status'}).click();
 await expect(palm.getByRole('slider')).toBeEnabled();await expect(palm.getByRole('slider')).toHaveValue('500');
 await expect(palm.getByRole('button',{name:'Save force palm height'})).toBeEnabled();await expect(palm.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 await palm.getByRole('button',{name:'Discard changes'}).click();await expect(palm.getByRole('slider')).toHaveValue('200');
});
