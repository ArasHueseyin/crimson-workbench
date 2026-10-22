import {expect,test,type Page} from '@playwright/test';

async function fixture(page:Page,{running=false,version='1.0.0.2976',stale=false,nativeConflict=false,nativeUnmanaged=false,worldInstalled=false,photoInstalled=false,teleportInstalled=false,nativeReadError=false,jumpReadError=false,itemLanguage='eng'}={}) {
 await page.addInitScript(({running,version,stale,nativeConflict,nativeUnmanaged,worldInstalled,photoInstalled,teleportInstalled,nativeReadError,jumpReadError,itemLanguage})=>{
  const root=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;marketCalls:{command:string;args:Record<string,unknown>}[];marketRunning:boolean;marketGameError:boolean};root.isTauri=true;root.marketCalls=[];root.marketRunning=running;root.marketGameError=false;
  let knowledgeUnlocked=false;
  let unlocked=false,jumpHeight=100,movementSpeed=100,mapRevealed=false;
  const runtimeSettings:Record<string,number>={force_palm_height:100,axiom_reach:100,glide_speed:100,player_resources:1};
  let protectedKeys:number[]=[];
  const consumableChoices=[{key:2200,name:'Test Arrow',kind:'Arrow'},{key:10033,name:'Test Honey',kind:'Food'},{key:22009,name:'Test Holy Water',kind:'Healing'},{key:1000034,name:'Test Stat Potion',kind:'Potion'},{key:1000085,name:'Other Stat Potion',kind:'Potion'}];
  const mapPlan=()=>({save_sha256:'a'.repeat(64),lobby_sha256:'b'.repeat(64),plan_id:'e'.repeat(64),before:{revealed_cells:mapRevealed?4194304:12345,total_cells:4194304},after:{revealed_cells:4194304,total_cells:4194304}});
  const contributionBalances=[{key:1000658,name:'Graymane',amount:'10' as string|null},{key:1001753,name:'Demeniss',amount:'20' as string|null},{key:1001754,name:'Hernand',amount:null as string|null}];
  const contributionPlan=(edits:{key:number;target:string}[])=>({save_sha256:'a'.repeat(64),lobby_sha256:'b'.repeat(64),plan_id:'d'.repeat(64),balances:contributionBalances.map(b=>({...b})),changes:edits.map(e=>({key:e.key,name:contributionBalances.find(b=>b.key===e.key)!.name,before:contributionBalances.find(b=>b.key===e.key)!.amount!,after:e.target}))});
  const abilities=Array.from({length:140},(_,i)=>({key:i===0?40003:i===1?40069:50000+i,name:i===0?'Sword Mastery I':i===1?'Focused Strike':`Ability ${i+1}`,owner:i<2?0:i%3,maximum:i===0?6:2}));
  const installed=new Set<string>();
  if(worldInstalled)installed.add('world_controls');
  if(photoInstalled)installed.add('photo_mode');
  if(teleportInstalled)installed.add('teleport');
  const worldFlags=3;
  const worldSnapshot=()=>({pid:123,state:'published',id:'10',session:'11',generation:'12',monotonic_ms:'2000',reason:0,flags:worldFlags,identity:1,world:2,stage:3,clock_reason:0,weather_reason:0,time:{day:7,hour:12,minute:0,second:0,millisecond:0,weekday:0,rate:worldFlags&4?0:12,anchor:'123'},weather:{precipitation:0,cloudiness:0,humidity:0.5,wind_speed:10,height_fog:1,phase:0},time_receipt:'0',weather_receipt:'0',time_result:'ready',time_result_reason:0,weather_result:'ready',weather_result_reason:0});
  const nativeStatus=()=>({game_running:root.marketRunning,mods:['sleep_cooldown','sleep_duration','blackstar','no_fall','player_stamina','mount_jump','player_jump','player_speed','preserve_consumables','mount_protection','free_crafting_materials','force_palm','force_palm_height','axiom_reach','glide_speed','mount_dismount','auto_loot','world_controls','photo_mode','teleport','region_return_30min'].map(id=>({id,state:nativeConflict&&id==='blackstar'?'conflict':installed.has(id)?'installed':nativeUnmanaged&&id==='blackstar'?'unmanaged':'not_installed',message:nativeConflict&&id==='blackstar'?'Existing unknown module will not be overwritten.':installed.has(id)?'Installed · loads at the next game start':'Not installed',can_install:!root.marketRunning&&!installed.has(id)&&!(nativeConflict&&id==='blackstar'),can_remove:!root.marketRunning&&installed.has(id)}))});
  const summary=()=>({total:140,new_abilities:unlocked?0:40,upgraded_abilities:unlocked?0:20,already_maxed:unlocked?140:80,gifted_points:unlocked?[0,0,0]:[20,15,10],available_points:[3,2,1]});
  root.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>={})=>{
   root.marketCalls.push({command,args});
   if(command==='marketplace_runtime_setting'){
    const id=String(args.id);
    if(args.value!=null){if(root.marketRunning||stale)throw{message:'Close Crimson Desert before saving runtime settings.'};runtimeSettings[id]=Number(args.value)}
    return{id,value:runtimeSettings[id],invalid:false,game_running:root.marketRunning,can_change:!root.marketRunning&&version==='1.0.0.2976',message:'Settings load at game startup when the corresponding mod is installed.'};
   }
   if(command==='marketplace_game_running'){if(root.marketGameError)throw{message:'Synthetic game process check failure'};return root.marketRunning}
   if(command==='world_status')return worldSnapshot();
   if(command==='teleport_status'){
    const current={field_definition:1,stage:17,position:{x:10,y:20,z:30},yaw:0.5};
    return{pid:123,state:'ready',id:'10',session:'11',epoch:'12',observed_ms:'2000',reason:0,features:7,identity:123,world:456,native_reason:0,native_error:0,storage_status:1,bookmarks_revision:'0',current,bookmarks:Array.from({length:16},()=>({occupied:false,name:'',position:null})),marker_status:0,markers_observed_ms:'2000',markers:[]};
   }
   if(command==='photo_status')return{pid:123,state:'ready',id:'10',session:'11',epoch:'12',observed_ms:'2000',reason:0,native_photo:false,owned:false,hud_hidden:false,scene_frozen:false};
   if(command==='marketplace_consumables'){
    if(args.itemKeys!=null){if(running||stale)throw{message:'Close Crimson Desert before saving consumable settings.'};protectedKeys=[...(args.itemKeys as number[])].sort((a,b)=>a-b)}
    return{item_keys:[...protectedKeys],choices:consumableChoices,game_running:root.marketRunning,can_change:!running&&version==='1.0.0.2976',message:'Selection loads at game startup when the consumable mod is installed.'};
   }
   const knowledgePlan=()=>({save_sha256:'a'.repeat(64),lobby_sha256:'b'.repeat(64),plan_id:'f'.repeat(64),summary:{total:3939,new_entries:knowledgeUnlocked?0:1966,upgraded_entries:knowledgeUnlocked?0:111,already_complete:knowledgeUnlocked?3939:1862}});
   if(command==='knowledge_unlock_preview')return{save_root:'C:/Synthetic Saves',saves:[{id:'123/slot0',label:'Manual save 0',modified:100}],selected_save:args.save??null,plan:args.save?knowledgePlan():null,game_running:root.marketRunning};
   if(command==='knowledge_unlock_apply'){
    if(root.marketRunning||stale)throw{message:root.marketRunning?'Close Crimson Desert first.':'Save changed since preview. Reload first.'};
    const plan=knowledgePlan();knowledgeUnlocked=true;return{request:args.request,plan,backup:'C:/Synthetic/.local/knowledge-unlocks/backup',message:'Journal knowledge unlocked. Existing progress preserved.'};
   }
   if(command==='map_fog_preview')return{save_root:'C:/Synthetic Saves',saves:[{id:'123/slot0',label:'Manual save 0',modified:100}],selected_save:args.save??null,plan:args.save?mapPlan():null,game_running:root.marketRunning};
   if(command==='map_fog_apply'){
    if(running||stale)throw{message:running?'Close Crimson Desert first.':'Save changed since preview. Reload first.',code:'read_error'};
    const plan=mapPlan();mapRevealed=true;return{request:args.request,plan,backup:'C:/Synthetic/.local/map-reveals/backup',message:'World map fog revealed. Quest, knowledge and discovery records preserved.'};
   }
   if(command==='contribution_preview')return{save_root:'C:/Synthetic Saves',saves:[{id:'123/slot0',label:'Manual save 0',modified:100}],selected_save:args.save??null,balances:args.save?contributionBalances.map(b=>({...b})):[],plan:args.save&&(args.edits as unknown[]).length?contributionPlan(args.edits as {key:number;target:string}[]):null,game_running:root.marketRunning};
   if(command==='contribution_apply'){
    if(running||stale)throw{message:running?'Close Crimson Desert first.':'Save changed since preview. Reload first.',code:'read_error'};
    const request=args.request as {edits:{key:number;target:string}[]};const plan=contributionPlan(request.edits);
    for(const edit of request.edits)contributionBalances.find(b=>b.key===edit.key)!.amount=edit.target;
    return{request,plan,backup:'C:/Synthetic/.local/contribution-edits/backup',message:'Selected contribution balances updated. Faction experience and levels preserved.'};
   }
   if(command==='marketplace_player_jump'){if(jumpReadError)throw{message:'Synthetic jump settings failure'};if(args.heightPercent!=null){if(running||stale)throw{message:'Close Crimson Desert before saving jump settings.'};jumpHeight=Number(args.heightPercent)}return{height_percent:jumpHeight,game_running:root.marketRunning,can_change:!running&&version==='1.0.0.2976',message:'Settings load at game startup when the player jump mod is installed.'}}
   if(command==='marketplace_player_speed'){if(args.speedPercent!=null){if(running||stale)throw{message:'Close Crimson Desert before saving movement settings.'};movementSpeed=Number(args.speedPercent)}return{speed_percent:movementSpeed,game_running:root.marketRunning,can_change:!running&&version==='1.0.0.2976',message:'Settings load at game startup when the player speed mod is installed.'}}
   if(command==='marketplace_mod_status'){if(nativeReadError)throw{message:'Synthetic runtime helper failure'};return nativeStatus()}
   if(command==='marketplace_weapon_effects')return[{source:1001062,source_name:'Electro-Mecha Longsword',effect_name:'Lightning weapon attack',skill:91101,level:3,targets:[{key:123,name:'Test Greatsword',existing:[]},{key:124,name:'Original Ability Sword',existing:[{skill:70147,level:1}]}]},{source:1000451,source_name:'Flame Spear',effect_name:'Fire weapon attack',skill:91105,level:3,targets:[{key:125,name:'Test Spear',existing:[]}]}];
   if(command==='marketplace_mod_change'){if(root.marketRunning)throw{message:'Close Crimson Desert first.'};if(args.enabled)installed.add(String(args.id));else installed.delete(String(args.id));return nativeStatus()}
   if(command==='bootstrap')return{project:'C:/Synthetic',discovery:{installations:[{path:'C:/Synthetic Game',platform:'steam'}],configured_game:'C:/Synthetic Game'},languages:[{language:'eng'},{language:'ger'}]};
   if(command==='open_catalog')return{session:1,game_path:args.game,item_language:{language:itemLanguage,source:'steam'},info:{item_count:0,types:[],categories:[],tiers:[],stats:[],index:{language:itemLanguage,path:'synthetic',fingerprint:'synthetic'},exe_version:version}};
   if(command==='search_items')return (args.query as {text?:string})?.text===(itemLanguage==='ger'?'Trank':'Potion')?{items:[{key:456,name:itemLanguage==='ger'?'Heiltrank':'Test Potion',internal_key:'SyntheticPotion',description:'Synthetic item',item_type:1,category:1,tier:1,max_stack:'10',stat_keys:[],icon_key:456}],total:1,offset:0}:{items:[],total:0,offset:0};
   if(command==='item_icon')return{data_url:'data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///ywAAAAAAQABAAACAUwAOw==',source:'Synthetic icon',reason:null};
   if(command==='mod_preview')return{request:args.request,plan_id:'reviewed-plan',fingerprint:'synthetic',changes:[{module:'Marketplace',table:'appearance',key:1,name:'Shared lion model',field:'Scale',before:'0.90',after:'1.17'}],files:[{path:'0041/0.paz',action:'create',before_sha256:null,after_sha256:'sha',bytes:123}],warnings:[],gates:{game_running:root.marketRunning,can_apply:false,reasons:[]},credits:'Synthetic'};
   if(command==='mod_export')return'C:/Synthetic/exports/market-package';
   if(command==='skill_unlock_preview')return{abilities,save_root:'C:/Synthetic Saves',saves:[{id:'123/slot0',label:'Manual save 0',modified:100}],selected_save:args.save??null,save_sha256:args.save?'a'.repeat(64):null,lobby_sha256:args.save?'b'.repeat(64):null,plan_id:args.save?'c'.repeat(64):null,summary:args.save?(args.abilities?{...summary(),total:(args.abilities as number[]).length,new_abilities:unlocked?0:(args.abilities as number[]).length,upgraded_abilities:0,already_maxed:unlocked?(args.abilities as number[]).length:0}:summary()):null,game_running:root.marketRunning};
   if(command==='skill_unlock_apply'){
    if(stale)throw{message:'Save changed since preview. Reload first.',code:'read_error'};
    unlocked=true;return{request:args.request,summary:summary(),backup:'C:/Synthetic/.local/skill-unlocks/backup',message:'All 140 abilities unlocked. Available points preserved.'};
   }
   throw Error(`Unexpected command: ${command}`);
  }};
 },{running,version,stale,nativeConflict,nativeUnmanaged,worldInstalled,photoInstalled,teleportInstalled,nativeReadError,jumpReadError,itemLanguage});
 await page.goto('/');await expect(page.locator('.workbench')).toHaveAttribute('data-ready','true');await page.getByRole('button',{name:'Mod marketplace',exact:true}).click();
}
test('mod search finds English and German names without losing selections or unsaved settings',async({page})=>{
 await fixture(page);
 const lion=page.locator('.market-card').filter({has:page.getByRole('heading',{name:'Larger lion',exact:true})});
 await lion.getByRole('button',{name:'Select mod',exact:true}).click();
 const jump=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Player jump height',exact:true})});
 await jump.getByRole('slider').fill('175');
 const search=page.getByRole('textbox',{name:'Search mods',exact:true});
 await search.fill('Schwarzstern');
 await expect(page.locator('.market-card:visible')).toHaveCount(1);
 await expect(page.getByRole('heading',{name:'Blackstar anywhere',exact:true})).toBeVisible();
 await search.fill('this mod does not exist');
 await expect(page.locator('.market-card:visible')).toHaveCount(0);
 await expect(page.getByRole('heading',{name:'No mods match your search'})).toBeVisible();
 await page.getByRole('button',{name:'Clear mod search'}).click();
 await expect(page.locator('.market-card:visible')).toHaveCount(36);
 await expect(lion.getByRole('button',{name:'Deselect mod',exact:true})).toHaveAttribute('aria-pressed','true');
 await expect(jump.getByRole('slider')).toHaveValue('175');
 await expect(jump.getByRole('button',{name:'Save jump height'})).toBeEnabled();
 await search.fill('Kraftfaust');
 await expect(page.locator('.market-card:visible')).toHaveCount(2);
 await page.screenshot({path:'../.local/marketplace-search-preview.png',fullPage:false});
 const writes=await page.evaluate(()=>(window as unknown as {marketCalls:{command:string}[]}).marketCalls.filter(c=>['marketplace_mod_change','marketplace_runtime_setting'].includes(c.command)));
 expect(writes.filter(c=>c.command==='marketplace_mod_change')).toHaveLength(0);
});

test('Force Palm height has an independent 1x to 100x saved multiplier',async({page})=>{
 await fixture(page);
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Force Palm jump height',exact:true})});
 const slider=card.getByRole('slider',{name:'Force Palm height multiplier'});
 await expect(slider).toHaveAttribute('min','100');await expect(slider).toHaveAttribute('max','10000');
 await slider.fill('10000');await expect(card.locator('output')).toHaveText('100×');
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 await card.getByRole('button',{name:'Save force palm height',exact:true}).click();
 await card.getByRole('button',{name:'Install mod',exact:true}).click();
 await expect(card.getByRole('button',{name:'Remove mod',exact:true})).toBeEnabled();
 await card.getByRole('button',{name:'Reset to normal'}).click();
 await expect(slider).toHaveValue('100');
 const calls=await page.evaluate(()=>(window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_runtime_setting'&&c.args.value!=null||c.command==='marketplace_mod_change'));
 expect(calls.map(c=>c.args)).toEqual([{session:1,id:'force_palm_height',value:10000},{session:1,id:'force_palm_height',enabled:true}]);
});

test('Axiom reach saves a chosen multiplier and can restore normal reach',async({page})=>{
 await fixture(page);
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Axiom armlet reach',exact:true})});
 const slider=card.getByRole('slider',{name:'Axiom reach multiplier'});
 await expect(slider).toHaveAttribute('min','200');await expect(slider).toHaveAttribute('max','500');
 await expect(card.locator('output')).toHaveText('1× · Normal');
 await card.getByRole('button',{name:'Use 2× reach',exact:true}).click();
 await expect(card.locator('output')).toHaveText('2×');
 await slider.fill('270');await expect(card.locator('output')).toHaveText('2.7×');
 await card.getByRole('button',{name:'Save axiom reach',exact:true}).click();
 await card.getByRole('button',{name:'Reset to normal'}).click();
 await card.getByRole('button',{name:'Save axiom reach',exact:true}).click();
 const calls=await page.evaluate(()=>(window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_runtime_setting'&&c.args.value!=null));
 expect(calls.map(c=>c.args)).toEqual([{session:1,id:'axiom_reach',value:270},{session:1,id:'axiom_reach',value:100}]);
});

test('spirit and stamina are selected independently through one native module',async({page})=>{
 await fixture(page);
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Unlimited player stamina & spirit',exact:true})});
 await expect(card.getByRole('checkbox',{name:'Unlimited player stamina',exact:true})).toBeChecked();
 await expect(card.getByRole('checkbox',{name:'Unlimited player spirit',exact:true})).not.toBeChecked();
 await card.getByRole('checkbox',{name:'Unlimited player stamina',exact:true}).uncheck();
 await card.getByRole('checkbox',{name:'Unlimited player spirit',exact:true}).check();
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 await card.getByRole('button',{name:'Save player resources',exact:true}).click();
 await card.getByRole('button',{name:'Install mod',exact:true}).click();
 const calls=await page.evaluate(()=>(window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_runtime_setting'&&c.args.value!=null||c.command==='marketplace_mod_change'));
 expect(calls.map(c=>c.args)).toEqual([{session:1,id:'player_resources',value:2},{session:1,id:'player_stamina',enabled:true}]);
});

test('new startup settings stay locked during gameplay and retain failed drafts',async({page})=>{
 await fixture(page,{running:true});
 await expect(page.getByRole('slider',{name:'Force Palm height multiplier'})).toBeDisabled();
 await expect(page.getByRole('slider',{name:'Axiom reach multiplier'})).toBeDisabled();
 await expect(page.getByRole('slider',{name:'Gliding speed multiplier'})).toBeDisabled();
 await expect(page.getByRole('checkbox',{name:'Unlimited player spirit',exact:true})).toBeDisabled();
 await fixture(page,{stale:true});
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Force Palm jump height',exact:true})});
 await card.getByRole('slider').fill('500');await card.getByRole('button',{name:'Save force palm height',exact:true}).click();
 await expect(card).toContainText('Close Crimson Desert before saving runtime settings');
 await expect(card.getByRole('slider')).toHaveValue('500');await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
});

test('gliding speed saves up to 500 percent and resets independently',async({page})=>{
 await fixture(page);
 await page.getByRole('textbox',{name:'Search mods'}).fill('Gleitgeschwindigkeit');
 await expect(page.locator('.market-card:visible')).toHaveCount(1);
 const card=page.locator('.runtime-card:visible');
 const slider=card.getByRole('slider',{name:'Gliding speed multiplier'});
 await expect(slider).toHaveAttribute('min','100');await expect(slider).toHaveAttribute('max','500');
 await slider.fill('500');await expect(card.locator('output')).toHaveText('500% · 5×');
 await page.screenshot({path:'../.local/marketplace-glide-preview.png',fullPage:false});
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 await card.getByRole('button',{name:'Save gliding speed',exact:true}).click();
 await card.getByRole('button',{name:'Install mod',exact:true}).click();
 await card.getByRole('button',{name:'Reset to normal'}).click();
 await card.getByRole('button',{name:'Save gliding speed',exact:true}).click();
 const calls=await page.evaluate(()=>(window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_runtime_setting'&&c.args.value!=null||c.command==='marketplace_mod_change'));
 expect(calls.map(c=>c.args)).toEqual([{session:1,id:'glide_speed',value:500},{session:1,id:'glide_speed',enabled:true},{session:1,id:'glide_speed',value:100}]);
});

test('startup controls unlock automatically after a running game closes without losing drafts',async({page})=>{
 await fixture(page,{running:true});
 const palm=page.getByRole('slider',{name:'Force Palm height multiplier'});
 const glide=page.getByRole('slider',{name:'Gliding speed multiplier'});
 const axiom=page.getByRole('slider',{name:'Axiom reach multiplier'});
 const spirit=page.getByRole('checkbox',{name:'Unlimited player spirit',exact:true});
 for(const control of [palm,glide,axiom,spirit])await expect(control).toBeDisabled();
 await page.evaluate(()=>{(window as unknown as {marketRunning:boolean}).marketRunning=false});
 for(const control of [palm,glide,axiom,spirit])await expect(control).toBeEnabled();
 await palm.fill('500');
 await page.evaluate(()=>{(window as unknown as {marketRunning:boolean}).marketRunning=true});
 await expect(palm).toBeDisabled();
 await page.evaluate(()=>{(window as unknown as {marketRunning:boolean}).marketRunning=false});
 await expect(palm).toBeEnabled();await expect(palm).toHaveValue('500');
 const writes=await page.evaluate(()=>(window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_runtime_setting'&&c.args.value!=null));
 expect(writes).toHaveLength(0);
});

test('English cards, combined selection and reviewed export without automatic writes',async({page})=>{
 await fixture(page);await expect(page.locator('.market-card')).toHaveCount(36);await expect(page.locator('.market-free')).toHaveCount(0);
 await expect(page.locator('.market-card').getByLabel('Game availability',{exact:true})).toHaveCount(36);
 await expect(page.locator('.market-card .market-availability.closed')).toHaveCount(16);
 await expect(page.locator('.runtime-card').getByText('Works while playing',{exact:true})).toHaveCount(17);
 await expect(page.locator('.runtime-card').getByText('Close game for setup',{exact:true})).toHaveCount(20);
 await expect(page.locator('.runtime-card').getByText('Adjust while playing',{exact:true})).toHaveCount(3);
 await page.getByRole('button',{name:'Select mod',exact:true}).first().click();await page.getByRole('button',{name:'Select mod',exact:true}).first().click();
 await page.getByRole('button',{name:'Review combined plan'}).click();await expect(page.locator('.market-changes')).toContainText('0.90 → 1.17');
 await page.getByRole('button',{name:'Export local package'}).click();await expect(page.locator('.market-plan').getByRole('status')).toContainText('exports/market-package');
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 const request=calls.find(c=>c.command==='mod_preview')!.args.request as {marketplace:unknown};
 expect(request.marketplace).toEqual({larger_lion:true,larger_rokade:false,faster_mounts:true,no_mount_stamina:false,inventory_999:false,no_durability_loss:false,region_return_30min:false,automatic_fishing:false,free_crafting_currency:false,free_upgrade_materials:false,item_stacks:{},weapon_effects:{}});
 expect(calls.find(c=>c.command==='mod_export')!.args.planId).toBe('reviewed-plan');
 expect(calls.some(c=>['live_start','skill_unlock_apply','contribution_apply','marketplace_mod_change'].includes(c.command))).toBe(false);
 await page.getByRole('button',{name:'Deselect mod'}).first().click();await expect(page.getByRole('button',{name:'Export local package'})).toHaveCount(0);
 await page.screenshot({path:'../.local/marketplace-preview.png',fullPage:true});
});

test('map reveal binds the selected save and keeps completed requests idempotent',async({page})=>{
 await fixture(page);await page.getByRole('button',{name:'Choose save & reveal map',exact:true}).click();
 const dialog=page.getByRole('dialog',{name:'Reveal world map',exact:true});
 const apply=dialog.getByRole('button',{name:'Back up & reveal map',exact:true});
 await expect(apply).toBeDisabled();
 await dialog.getByLabel('Map reveal save',{exact:true}).selectOption('123/slot0');
 await expect(dialog).toContainText('Revealed map: 0.3% → 100%');
 await expect(dialog).toContainText('Map markers still follow their normal discovery rules.');
 await expect(apply).toBeDisabled();
 await dialog.getByLabel('Confirm map reveal save edit',{exact:true}).check();
 await page.screenshot({path:'../.local/marketplace-map-reveal-preview.png'});
 await apply.click();await expect(dialog.locator('.recovery-success')).toContainText('map-reveals/backup');
 await expect(dialog).toContainText('This map is already fully revealed.');await expect(apply).toBeDisabled();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 const writes=calls.filter(c=>c.command==='map_fog_apply');expect(writes).toHaveLength(1);
 expect(writes[0].args.request).toMatchObject({save_root:'C:/Synthetic Saves',save:'123/slot0',save_sha256:'a'.repeat(64),lobby_sha256:'b'.repeat(64),plan_id:'e'.repeat(64)});
 expect(calls.some(c=>['contribution_apply','live_start','skill_unlock_apply'].includes(c.command))).toBe(false);
});
test('map reveal preview is readable during gameplay and cannot be applied',async({page})=>{
 await fixture(page,{running:true});await page.getByRole('button',{name:'Choose save & reveal map',exact:true}).click();
 const dialog=page.getByRole('dialog',{name:'Reveal world map',exact:true});
 await dialog.getByLabel('Map reveal save',{exact:true}).selectOption('123/slot0');
 await expect(dialog).toContainText('Revealed map: 0.3% → 100%');
 await expect(dialog.getByLabel('Confirm map reveal save edit',{exact:true})).toBeDisabled();
 await expect(dialog.getByRole('button',{name:'Back up & reveal map',exact:true})).toBeDisabled();
});
test('stale map reveal retains its exact request for recovery and discards stale confirmation',async({page})=>{
 await fixture(page,{stale:true});await page.getByRole('button',{name:'Choose save & reveal map',exact:true}).click();
 const dialog=page.getByRole('dialog',{name:'Reveal world map',exact:true});
 await dialog.getByLabel('Map reveal save',{exact:true}).selectOption('123/slot0');
 await dialog.getByLabel('Confirm map reveal save edit',{exact:true}).check();
 await dialog.getByRole('button',{name:'Back up & reveal map',exact:true}).click();
 await expect(dialog).toContainText('Save changed since preview');
 await expect(dialog.getByLabel('Map reveal save',{exact:true})).toBeDisabled();
 await dialog.getByRole('button',{name:'Check / recover pending map reveal',exact:true}).click();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='map_fog_apply'));
 expect(calls).toHaveLength(2);expect(calls[0].args.request).toEqual(calls[1].args.request);
 await dialog.getByRole('button',{name:'Discard saved request & reload map',exact:true}).click();
 await expect(dialog.getByRole('button',{name:'Back up & reveal map',exact:true})).toBeDisabled();
 await dialog.getByLabel('Map reveal save',{exact:true}).selectOption('');
 await expect(dialog.locator('.recovery-review')).toHaveCount(0);
});

test('contribution targets preserve exact large integers and require a reviewed save action',async({page})=>{
 await fixture(page);
 await page.getByRole('button',{name:'Choose save & contribution targets',exact:true}).click();
 const dialog=page.getByRole('dialog',{name:'Contribution points',exact:true});
 const apply=dialog.getByRole('button',{name:'Back up & update contribution points',exact:true});
 await expect(apply).toBeDisabled();
 await expect(dialog.getByLabel('Contribution save',{exact:true}).locator('option')).toHaveCount(2);
 await dialog.getByLabel('Contribution save',{exact:true}).selectOption('123/slot0');
 await expect(dialog.getByLabel('Hernand contribution target',{exact:true})).toBeDisabled();
 await dialog.getByLabel('Graymane contribution target',{exact:true}).fill('9007199254740993');
 await dialog.getByRole('button',{name:'Review contribution changes',exact:true}).click();
 await expect(dialog.locator('.recovery-review')).toContainText('10 → 9007199254740993');
 await expect(apply).toBeDisabled();
 await dialog.getByLabel('Confirm contribution save edit',{exact:true}).check();
 await page.screenshot({path:'../.local/marketplace-contributions-preview.png'});
 await apply.click();
 await expect(dialog.locator('.recovery-success')).toContainText('contribution-edits/backup');
 await expect(dialog.getByLabel('Graymane contribution target',{exact:true})).toHaveValue('');
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 const writes=calls.filter(c=>c.command==='contribution_apply');expect(writes).toHaveLength(1);
 expect(writes[0].args.request).toMatchObject({save:'123/slot0',save_sha256:'a'.repeat(64),lobby_sha256:'b'.repeat(64),plan_id:'d'.repeat(64),edits:[{key:1000658,target:'9007199254740993'}]});
 expect(calls.some(c=>['live_start','skill_unlock_apply','marketplace_mod_change'].includes(c.command))).toBe(false);
});

test('contribution target changes revoke review and invalid values cannot be applied',async({page})=>{
 await fixture(page);await page.getByRole('button',{name:'Choose save & contribution targets',exact:true}).click();
 const dialog=page.getByRole('dialog',{name:'Contribution points',exact:true});
 await dialog.getByLabel('Contribution save',{exact:true}).selectOption('123/slot0');
 const field=dialog.getByLabel('Graymane contribution target',{exact:true}),review=dialog.getByRole('button',{name:'Review contribution changes',exact:true});
 for(const value of ['9','0','1e3','9000000000000000001']){await field.fill(value);await expect(review).toBeDisabled()}
 await field.fill('100');await review.click();await dialog.getByLabel('Confirm contribution save edit',{exact:true}).check();
 await field.fill('101');await expect(dialog.locator('.recovery-review')).toHaveCount(0);
 await expect(dialog.getByRole('button',{name:'Back up & update contribution points',exact:true})).toBeDisabled();
 await dialog.getByLabel('Contribution save',{exact:true}).selectOption('');
 await expect(dialog.locator('.contribution-row')).toHaveCount(0);
});

test('running game permits contribution previews but no save changes',async({page})=>{
 await fixture(page,{running:true});await page.getByRole('button',{name:'Choose save & contribution targets',exact:true}).click();
 const dialog=page.getByRole('dialog',{name:'Contribution points',exact:true});
 await dialog.getByLabel('Contribution save',{exact:true}).selectOption('123/slot0');
 await dialog.getByLabel('Graymane contribution target',{exact:true}).fill('100');
 await dialog.getByRole('button',{name:'Review contribution changes',exact:true}).click();
 await expect(dialog).toContainText('fully close Crimson Desert');
 await expect(dialog.getByLabel('Confirm contribution save edit',{exact:true})).toBeDisabled();
 await expect(dialog.getByRole('button',{name:'Back up & update contribution points',exact:true})).toBeDisabled();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string}[]}).marketCalls);
 expect(calls.some(c=>c.command==='contribution_apply')).toBe(false);
});

test('stale contribution request retains its original targets for recovery',async({page})=>{
 await fixture(page,{stale:true});await page.getByRole('button',{name:'Choose save & contribution targets',exact:true}).click();
 const dialog=page.getByRole('dialog',{name:'Contribution points',exact:true});
 await dialog.getByLabel('Contribution save',{exact:true}).selectOption('123/slot0');
 await dialog.getByLabel('Graymane contribution target',{exact:true}).fill('100');
 await dialog.getByRole('button',{name:'Review contribution changes',exact:true}).click();
 await dialog.getByLabel('Confirm contribution save edit',{exact:true}).check();
 await dialog.getByRole('button',{name:'Back up & update contribution points',exact:true}).click();
 await expect(dialog).toContainText('Save changed since preview');
 await expect(dialog.getByLabel('Graymane contribution target',{exact:true})).toBeDisabled();
 await dialog.getByRole('button',{name:'Check / recover pending contribution edit',exact:true}).click();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='contribution_apply'));
 expect(calls).toHaveLength(2);expect(calls[0].args.request).toEqual(calls[1].args.request);
 await dialog.getByRole('button',{name:'Discard saved request & reload balances',exact:true}).click();
 await expect(dialog.getByLabel('Graymane contribution target',{exact:true})).toBeEnabled();
 await expect(dialog.getByLabel('Graymane contribution target',{exact:true})).toHaveValue('');
});

test('restricted-region timer has independent setup buttons and refuses changes while the game runs',async({page})=>{
 await fixture(page,{running:true});
 const card=page.locator('.market-card').filter({has:page.getByRole('heading',{name:'More time beyond restricted regions',exact:true})});
 await expect(card).toContainText('30 seconds → 30 minutes');
 await expect(card.getByRole('button',{name:'Select mod',exact:true})).toHaveCount(0);
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect(calls.some(c=>['mod_preview','live_start','marketplace_mod_change'].includes(c.command))).toBe(false);
});

test('automatic fishing is explicit about casting and changes only its reviewed draft',async({page})=>{
 await fixture(page,{running:true});
 const card=page.locator('.market-card').filter({has:page.getByRole('heading',{name:'Automatic fishing',exact:true})});
 await expect(card).toContainText('Cast again yourself');
 await card.getByRole('button',{name:'Select mod',exact:true}).click();
 await page.getByRole('button',{name:'Review combined plan',exact:true}).click();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect((calls.find(c=>c.command==='mod_preview')!.args.request as {marketplace:unknown}).marketplace).toMatchObject({automatic_fishing:true});
 expect(calls.some(c=>['live_start','skill_unlock_apply','marketplace_mod_change'].includes(c.command))).toBe(false);
});

test('Rokade size is a separate reviewed choice and keeps the shared lion choice',async({page})=>{
 await fixture(page,{running:true});
 const card=page.locator('.market-card').filter({has:page.getByRole('heading',{name:'Larger Rokade',exact:true})});
 await expect(card).toContainText('Other horses keep their original size');
 await card.getByRole('button',{name:'Select mod',exact:true}).click();
 const lion=page.locator('.market-card').filter({has:page.getByRole('heading',{name:'Larger lion',exact:true})});
 await lion.getByRole('button',{name:'Select mod',exact:true}).click();
 await page.getByRole('button',{name:'Review combined plan',exact:true}).click();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect((calls.find(c=>c.command==='mod_preview')!.args.request as {marketplace:unknown}).marketplace).toMatchObject({larger_rokade:true,larger_lion:true});
 expect(calls.some(c=>['live_start','skill_unlock_apply','marketplace_mod_change'].includes(c.command))).toBe(false);
 await card.getByRole('button',{name:'Deselect mod',exact:true}).click();
 await expect(page.getByRole('button',{name:'Export local package'})).toHaveCount(0);
 await expect(lion.getByRole('button',{name:'Deselect mod',exact:true})).toBeVisible();
});

test('native weapon transfers require compatible selections and stay in the reviewed draft',async({page})=>{
 await fixture(page,{running:true});
 await page.getByRole('button',{name:'Choose weapons & effects',exact:true}).click();
 const dialog=page.getByRole('dialog',{name:'Native weapon effects',exact:true});
 const add=dialog.getByRole('button',{name:'Add effect transfer to plan',exact:true});
 await expect(add).toBeDisabled();
 await dialog.getByLabel('Find weapon effect target',{exact:true}).fill('Greatsword');
 await dialog.getByRole('button',{name:'Test Greatsword · 123',exact:true}).click();
 await expect(add).toBeEnabled();await add.click();
 await dialog.getByRole('button',{name:'Close',exact:true}).click();
 await page.getByRole('button',{name:'Review combined plan',exact:true}).click();
  const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect((calls.find(c=>c.command==='mod_preview')!.args.request as {marketplace:unknown}).marketplace).toMatchObject({weapon_effects:{123:{source:1001062,replace_existing:false}}});
 expect(calls.some(c=>['live_start','skill_unlock_apply','marketplace_mod_change'].includes(c.command))).toBe(false);
 await page.getByRole('button',{name:'Choose weapons & effects',exact:true}).click();
 await dialog.getByRole('button',{name:'Remove weapon effect transfer 123',exact:true}).click();
 await dialog.getByRole('button',{name:'Close',exact:true}).click();
 await expect(page.getByRole('button',{name:'Export local package'})).toHaveCount(0);
});

test('replacing native weapon abilities is explicit and changing the source revokes consent',async({page})=>{
 await fixture(page);
 await page.getByRole('button',{name:'Choose weapons & effects',exact:true}).click();
 const dialog=page.getByRole('dialog',{name:'Native weapon effects',exact:true});
 const add=dialog.getByRole('button',{name:'Add effect transfer to plan',exact:true});
 await dialog.getByLabel('Find weapon effect target',{exact:true}).fill('Original Ability');
 await dialog.getByRole('button',{name:'Original Ability Sword · 124',exact:true}).click();
 await expect(add).toBeDisabled();
 await expect(dialog).toContainText('skill 70147 (level 1)');
 await dialog.getByLabel('Replace existing native weapon abilities',{exact:true}).check();
 await page.screenshot({path:'../.local/marketplace-effects-preview.png'});
 await expect(add).toBeEnabled();await add.click();
 await dialog.getByLabel('Weapon effect source',{exact:true}).selectOption('1000451');
 await expect(dialog.getByLabel('Find weapon effect target',{exact:true})).toHaveValue('');
 await expect(add).toBeDisabled();
 await expect(dialog.getByLabel('Replace existing native weapon abilities',{exact:true})).toHaveCount(0);
 await expect(dialog.getByRole('button',{name:'Original Ability Sword · 124',exact:true})).toHaveCount(0);
 await dialog.getByRole('button',{name:'Test Spear · 125',exact:true}).click();
 await add.click();await dialog.getByRole('button',{name:'Close',exact:true}).click();
 await page.getByRole('button',{name:'Review combined plan',exact:true}).click();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect((calls.find(c=>c.command==='mod_preview')!.args.request as {marketplace:unknown}).marketplace).toMatchObject({weapon_effects:{124:{source:1001062,replace_existing:true},125:{source:1000451,replace_existing:false}}});
 expect(calls.some(c=>['live_start','skill_unlock_apply','marketplace_mod_change'].includes(c.command))).toBe(false);
});

test('a failed settings read does not block the other runtime cards',async({page})=>{
 await fixture(page,{jumpReadError:true});
 await expect(page.locator('.market-runtime-status').getByRole('alert')).toContainText('Jump settings: Synthetic jump settings failure');
 const jump=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Player jump height',exact:true})});
 await expect(jump.getByRole('slider')).toBeDisabled();
 await expect(jump).toContainText('Jump settings unavailable');
 const fall=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'No fall damage',exact:true})});
 await expect(fall.getByRole('button',{name:'Install mod',exact:true})).toBeEnabled();
 const speed=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Player movement speed',exact:true})});
 await expect(speed.getByRole('slider')).toBeEnabled();
 await expect(page.getByLabel('Find consumable to preserve',{exact:true})).toBeEnabled();
 await expect(page.getByText('Checking installed modules…',{exact:true})).toHaveCount(0);
 await fixture(page,{nativeReadError:true});
 await expect(page.locator('.market-runtime-status').getByRole('alert')).toContainText('Runtime status: Synthetic runtime helper failure');
 await expect(jump.getByRole('slider')).toBeEnabled();
 await expect(fall.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 await expect(fall).toContainText('Runtime status unavailable');
});

test('starting and stopping the game updates every install button without resetting draft settings',async({page})=>{
 await fixture(page);
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Stay mounted on longer falls',exact:true})});
 const jump=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Player jump height',exact:true})});
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeEnabled();
 await jump.getByRole('slider').fill('175');
 await page.evaluate(()=>{(window as unknown as {marketRunning:boolean}).marketRunning=true});
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 await expect(page.locator('.runtime-card .mod-actions button.primary:enabled')).toHaveCount(0);
 await expect(jump.getByRole('slider')).toHaveValue('175');
 await expect(jump.getByRole('slider')).toBeDisabled();
 await page.evaluate(()=>{(window as unknown as {marketRunning:boolean}).marketRunning=false});
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeEnabled();
 await expect(jump.getByRole('slider')).toHaveValue('175');
 await expect(jump.getByRole('button',{name:'Save jump height',exact:true})).toBeEnabled();
 const writes=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string}[]}).marketCalls.filter(c=>c.command==='marketplace_mod_change'));
 expect(writes).toHaveLength(0);
});

test('an installation rejected after a game start shows its error on the clicked card',async({page})=>{
 await fixture(page);
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Stay mounted on longer falls',exact:true})});
 const install=card.getByRole('button',{name:'Install mod',exact:true});
 await expect(install).toBeEnabled();
 await page.evaluate(()=>{(window as unknown as {marketRunning:boolean}).marketRunning=true});
 await install.click();
 await expect(card.getByRole('alert')).toContainText('Close Crimson Desert first.');
 await expect(card.getByRole('button',{name:'Remove mod',exact:true})).toHaveCount(0);
});

test('an unknown game process status blocks installation until the check recovers',async({page})=>{
 await fixture(page);
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Stay mounted on longer falls',exact:true})});
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeEnabled();
 await page.evaluate(()=>{(window as unknown as {marketGameError:boolean}).marketGameError=true});
 await expect(page.locator('.market-runtime-status').getByRole('alert')).toContainText('Game status unavailable');
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 await page.evaluate(()=>{(window as unknown as {marketGameError:boolean}).marketGameError=false});
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeEnabled();
});

test('three separate runtime cards install and remove only the clicked mod',async({page})=>{
 await fixture(page);
 for(const name of ['No sleep cooldown','30-hour sleep','Blackstar anywhere'])await expect(page.getByRole('heading',{name,exact:true})).toBeVisible();
 const sleep=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'30-hour sleep',exact:true})});
 await expect(sleep).toContainText('in-game label still says 12 hours');
 await sleep.getByRole('button',{name:'Install mod',exact:true}).click();
 await expect(sleep.getByRole('button',{name:'Remove mod',exact:true})).toBeEnabled();
 await expect(sleep.locator('.runtime-feedback')).toContainText('Mod installed. Start the game');
 await sleep.getByRole('button',{name:'Remove mod',exact:true}).click();
 await expect(sleep.getByRole('button',{name:'Install mod',exact:true})).toBeEnabled();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_mod_change'));
 expect(calls.map(c=>c.args)).toEqual([{session:1,id:'sleep_duration',enabled:true},{session:1,id:'sleep_duration',enabled:false}]);
 await page.locator('.runtime-card').first().scrollIntoViewIfNeeded();
 await page.screenshot({path:'../.local/marketplace-runtime-preview.png'});
});

test('runtime conflict remains protected and known old mods use an explicit import button',async({page})=>{
 await fixture(page,{nativeConflict:true});
 const blackstar=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Blackstar anywhere',exact:true})});
 await expect(blackstar).toContainText('unknown module');await expect(blackstar.getByRole('button',{name:'Install mod'})).toBeDisabled();
 await expect(blackstar).toContainText('Quest, indoor and final placement checks');
});

test('known previous module is imported only on explicit request',async({page})=>{
 await fixture(page,{nativeUnmanaged:true});
 await page.getByRole('button',{name:'Import & update mod'}).click();
 const blackstar=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Blackstar anywhere',exact:true})});
 await expect(blackstar.getByRole('button',{name:'Remove mod'})).toBeEnabled();
});
test('skills require a selected reviewed save and explicit confirmation; completed action cannot grant again',async({page})=>{
 await fixture(page);await page.locator('.skills-card').getByRole('button',{name:'Choose save & review'}).click();
 const apply=page.getByRole('button',{name:'Back up & unlock all skills'});await expect(apply).toBeDisabled();
 await page.getByLabel('Skill unlock save').selectOption('123/slot0');await expect(page.locator('.skills-summary')).toContainText('40');await expect(apply).toBeDisabled();
 await page.getByLabel('Confirm skill save edit').check();await expect(apply).toBeEnabled();await apply.click();
 await expect(page.locator('.recovery-success')).toContainText('All 140 abilities unlocked');await expect(page.locator('.skills-summary')).toContainText('140');await expect(apply).toBeDisabled();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 const requests=calls.filter(c=>c.command==='skill_unlock_apply');expect(requests).toHaveLength(1);
 expect(requests[0].args.request).toMatchObject({save_root:'C:/Synthetic Saves',save:'123/slot0',save_sha256:'a'.repeat(64),lobby_sha256:'b'.repeat(64),plan_id:'c'.repeat(64)});
});
test('running game blocks skill editing and unsupported build disables every recipe',async({page})=>{
 await fixture(page,{running:true});await page.locator('.skills-card').getByRole('button',{name:'Choose save & review'}).click();await page.getByLabel('Skill unlock save').selectOption('123/slot0');
 await expect(page.getByLabel('Confirm skill save edit')).toBeDisabled();await expect(page.getByRole('button',{name:'Back up & unlock all skills'})).toBeDisabled();
 await page.getByRole('button',{name:'Close',exact:true}).click();
 for(const button of await page.locator('.runtime-card').getByRole('button',{name:'Install mod'}).all())await expect(button).toBeDisabled();
 await page.reload();await page.getByRole('button',{name:'Mod marketplace',exact:true}).click();
});
test('unknown version gives a clear explanation and no enabled install actions',async({page})=>{
 await fixture(page,{version:'1.0.0.9999'});await expect(page.getByRole('alert')).toContainText('require game version 1.0.0.2976');
 for (const button of await page.getByRole('button',{name:'Select mod',exact:true}).all())await expect(button).toBeDisabled();
 await expect(page.locator('.skills-card').getByRole('button',{name:'Choose save & review'})).toBeDisabled();
 await expect(page.getByRole('button',{name:'Choose weapons & effects'})).toBeDisabled();
 for(const button of await page.locator('.runtime-card').getByRole('button',{name:'Install mod'}).all())await expect(button).toBeDisabled();
});
test('stale save is refused and retry keeps the same request ID',async({page})=>{
 await fixture(page,{stale:true});await page.locator('.skills-card').getByRole('button',{name:'Choose save & review'}).click();await page.getByLabel('Skill unlock save').selectOption('123/slot0');
 await page.getByLabel('Confirm skill save edit').check();await page.getByRole('button',{name:'Back up & unlock all skills'}).click();await expect(page.getByRole('alert')).toContainText('Save changed since preview');
 await page.getByRole('button',{name:'Check / recover pending unlock'}).click();
 const requests=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='skill_unlock_apply'));
 expect(requests).toHaveLength(2);expect(requests[0].args.request).toEqual(requests[1].args.request);
});


test('individual skill selection is searchable and the exact keys survive preview and apply',async({page})=>{
 await fixture(page);await page.locator('.skills-card').getByRole('button',{name:'Choose save & review'}).click();
 await page.getByLabel('Skill unlock save').selectOption('123/slot0');
 await page.getByRole('button',{name:'Clear selection',exact:true}).click();
 await expect(page.getByRole('button',{name:'Back up & unlock selected skills'})).toBeDisabled();
 await page.getByLabel('Search abilities',{exact:true}).fill('Sword Mastery');
 await expect(page.locator('.skill-selection-row')).toHaveCount(1);
 await page.getByRole('checkbox',{name:'Unlock Sword Mastery I (40003)',exact:true}).check();
 await expect(page.getByRole('status').filter({hasText:'1 of 140 abilities selected'})).toBeVisible();
 await page.getByLabel('Confirm skill save edit').check();
 await page.getByRole('button',{name:'Back up & unlock selected skills',exact:true}).click();
 await expect(page.locator('.recovery-success')).toBeVisible();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 const apply=calls.find(c=>c.command==='skill_unlock_apply')!.args.request as {abilities:number[]};
 expect(apply.abilities).toEqual([40003]);
 expect(calls.some(c=>c.command==='skill_unlock_preview'&&JSON.stringify(c.args.abilities)==='[40003]')).toBe(true);
 expect(calls.some(c=>c.command==='skill_unlock_preview'&&Array.isArray(c.args.abilities)&&c.args.abilities.length===0)).toBe(false);
});

test('changing skill selection revokes confirmation and the all-skills button restores full scope',async({page})=>{
 await fixture(page);await page.locator('.skills-card').getByRole('button',{name:'Choose save & review'}).click();
 await page.getByLabel('Skill unlock save').selectOption('123/slot0');
 await page.getByLabel('Confirm skill save edit').check();
 await page.getByRole('checkbox',{name:'Unlock Sword Mastery I (40003)',exact:true}).uncheck();
 await expect(page.getByLabel('Confirm skill save edit')).not.toBeChecked();
 await expect(page.getByRole('button',{name:'Back up & unlock selected skills',exact:true})).toBeDisabled();
 await page.getByRole('button',{name:'Select all skills',exact:true}).click();
 await expect(page.getByRole('checkbox',{name:'Unlock Sword Mastery I (40003)',exact:true})).toBeChecked();
 await expect(page.getByRole('button',{name:'Back up & unlock all skills',exact:true})).toBeDisabled();
});


test('inventory and durability recipes stay in the reviewed combined plan',async({page})=>{
 await fixture(page,{running:true});
 for(const name of ['999 inventory slots','No durability loss']) {
  const card=page.locator('.market-card').filter({has:page.getByRole('heading',{name,exact:true})});
  await card.getByRole('button',{name:'Select mod',exact:true}).click();
 }
 await page.getByRole('button',{name:'Review combined plan',exact:true}).click();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect((calls.find(c=>c.command==='mod_preview')!.args.request as {marketplace:unknown}).marketplace).toMatchObject({inventory_999:true,no_durability_loss:true});
 expect(calls.some(c=>['live_start','skill_unlock_apply','marketplace_mod_change'].includes(c.command))).toBe(false);
});

test('stack limits use an actual item selection and retain no removed override',async({page})=>{
 await fixture(page);const card=page.locator('.stack-card');
 await expect(card.getByRole('button',{name:'Add stack override'})).toBeDisabled();
 await card.getByLabel('Find item for stack limit').fill('Potion');
 await expect(card.locator('.market-stack-results img')).toBeVisible();
 await expect.poll(()=>card.locator('.market-stack-results img').evaluate((image:HTMLImageElement)=>image.naturalWidth)).toBeGreaterThan(0);
 await card.getByRole('button',{name:'Test Potion · 456',exact:true}).click();
 await expect(card.locator('.market-stack-selected img')).toBeVisible();
 await card.getByLabel('Maximum stack size',{exact:true}).fill('321');
 await card.getByRole('button',{name:'Add stack override'}).click();
 await expect(card).toContainText('Test Potion · Maximum 321');
 await expect(card.locator('.market-stack-overrides img')).toBeVisible();
 await card.scrollIntoViewIfNeeded();await page.screenshot({path:'../.local/marketplace-stack-icons-preview.png'});
 await page.getByRole('button',{name:'Review combined plan',exact:true}).click();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect((calls.filter(c=>c.command==='mod_preview').at(-1)!.args.request as {marketplace:unknown}).marketplace).toMatchObject({item_stacks:{456:321}});
 await card.getByRole('button',{name:'Remove stack override 456'}).click();
 await expect(card.locator('.market-stack-overrides>div')).toHaveCount(0);
 await expect(page.getByRole('button',{name:'Export local package'})).toHaveCount(0);
 await card.getByLabel('Maximum stack size',{exact:true}).fill('0');
 await expect(card.getByRole('button',{name:'Add stack override'})).toBeDisabled();
});

test('stack item names and search follow the detected German or English catalog',async({page})=>{
 for(const [itemLanguage,label,query,itemName] of [['ger','Deutsch','Trank','Heiltrank'],['eng','English','Potion','Test Potion']]){
  await fixture(page,{itemLanguage});
  await expect(page.getByLabel('Item language',{exact:true}).locator('option:checked')).toHaveText(`Automatic · ${label}`);
  const card=page.locator('.stack-card');
  await expect(card).toContainText(`Find an item · ${label}`);
  await card.getByLabel('Find item for stack limit',{exact:true}).fill(query);
  await expect(card.getByRole('button',{name:`${itemName} · 456`,exact:true})).toBeVisible();
  const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
  expect(calls.filter(c=>c.command==='search_items').at(-1)?.args.query).toMatchObject({text:query});
 }
});

test('fall protection has its own install and remove action',async({page})=>{
 await fixture(page);const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'No fall damage',exact:true})});
 await card.getByRole('button',{name:'Install mod',exact:true}).click();
 await expect(card.getByRole('button',{name:'Remove mod',exact:true})).toBeEnabled();
 await card.getByRole('button',{name:'Remove mod',exact:true}).click();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_mod_change'));
 expect(calls.map(c=>c.args)).toEqual([{session:1,id:'no_fall',enabled:true},{session:1,id:'no_fall',enabled:false}]);
});

test('player stamina installs separately and remains locked while the game runs',async({page})=>{
 await fixture(page,{running:true});
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Unlimited player stamina & spirit',exact:true})});
 await expect(card).toContainText('Mount resources are separate');
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 await fixture(page);
 await card.getByRole('button',{name:'Install mod',exact:true}).click();
 await expect(card.getByRole('button',{name:'Remove mod',exact:true})).toBeEnabled();
 await card.getByRole('button',{name:'Remove mod',exact:true}).click();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_mod_change'));
 expect(calls.map(c=>c.args)).toEqual([{session:1,id:'player_stamina',enabled:true},{session:1,id:'player_stamina',enabled:false}]);
});

test('mount jump card uses the requested 40 percent and has an independent lifecycle',async({page})=>{
 await fixture(page,{running:true});
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Higher lion & Rokade jumps',exact:true})});
 await expect(card).toContainText('Jump 40% higher');
 await expect(card).toContainText('forced dismount threshold is a separate mod');
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 await fixture(page);
 await card.getByRole('button',{name:'Install mod',exact:true}).click();
 await expect(card.getByRole('button',{name:'Remove mod',exact:true})).toBeEnabled();
 await card.getByRole('button',{name:'Remove mod',exact:true}).click();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_mod_change'));
 expect(calls.map(c=>c.args)).toEqual([{session:1,id:'mount_jump',enabled:true},{session:1,id:'mount_jump',enabled:false}]);
});


test('player jump slider saves explicitly, resets normally and installs independently of mounts',async({page})=>{
 await fixture(page);
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Player jump height',exact:true})});
 const slider=card.locator('input[type=range]');
 await expect(slider).toHaveValue('100');
 await expect(card.getByRole('button',{name:'Save jump height',exact:true})).toBeDisabled();
 await slider.fill('140');
 await expect(card).toContainText('140% · +40%');
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 let writes=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_player_jump'&&c.args.heightPercent!=null));
 expect(writes).toHaveLength(0);
 await card.getByRole('button',{name:'Save jump height',exact:true}).click();
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeEnabled();
 await card.getByRole('button',{name:'Install mod',exact:true}).click();
 await expect(card.getByRole('button',{name:'Remove mod',exact:true})).toBeEnabled();
 await card.getByRole('button',{name:'Reset to normal',exact:true}).click();
 await expect(slider).toHaveValue('100');
 await card.getByRole('button',{name:'Save jump height',exact:true}).click();
 writes=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_player_jump'&&c.args.heightPercent!=null));
 expect(writes.map(c=>c.args)).toEqual([{session:1,heightPercent:140},{session:1,heightPercent:100}]);
 const mods=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_mod_change'));
 expect(mods.map(c=>c.args)).toEqual([{session:1,id:'player_jump',enabled:true}]);
 await card.scrollIntoViewIfNeeded();await page.screenshot({path:'../.local/marketplace-player-jump-preview.png'});
});

test('running game and unsupported builds lock jump settings; a rejected save stays unsaved',async({page})=>{
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Player jump height',exact:true})});
 for(const options of [{running:true},{version:'1.0.0.9999'}]){
  await fixture(page,options);
  await expect(card.locator('input[type=range]')).toBeDisabled();
  await expect(card.getByRole('button',{name:'Save jump height',exact:true})).toBeDisabled();
 }
 await fixture(page,{stale:true});
 await card.locator('input[type=range]').fill('150');
 await card.getByRole('button',{name:'Save jump height',exact:true}).click();
 await expect(card.getByRole('alert')).toContainText('Close Crimson Desert');
 await expect(card).toContainText('Unsaved setting');
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
});


test('player movement speed saves explicitly and stays independent of jump and mount settings',async({page})=>{
 await fixture(page);
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Player movement speed',exact:true})});
 const jump=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Player jump height',exact:true})});
 const slider=card.locator('input[type=range]');
 await expect(slider).toHaveValue('100');
 await expect(card.getByRole('button',{name:'Save movement speed',exact:true})).toBeDisabled();
 await slider.fill('175');
 await expect(card).toContainText('175% · +75%');
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 let writes=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_player_speed'&&c.args.speedPercent!=null));
 expect(writes).toHaveLength(0);
 await jump.locator('input[type=range]').fill('140');
 await card.getByRole('button',{name:'Save movement speed',exact:true}).click();
 await expect(jump.locator('input[type=range]')).toHaveValue('140');
 await expect(jump).toContainText('Unsaved setting');
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeEnabled();
 await card.getByRole('button',{name:'Install mod',exact:true}).click();
 await expect(card.getByRole('button',{name:'Remove mod',exact:true})).toBeEnabled();
 await card.scrollIntoViewIfNeeded();await page.screenshot({path:'../.local/marketplace-player-speed-preview.png'});
 await card.getByRole('button',{name:'Reset to normal',exact:true}).click();
 await expect(slider).toHaveValue('100');
 await card.getByRole('button',{name:'Save movement speed',exact:true}).click();
 await card.getByRole('button',{name:'Remove mod',exact:true}).click();
 writes=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_player_speed'&&c.args.speedPercent!=null));
 expect(writes.map(c=>c.args)).toEqual([{session:1,speedPercent:175},{session:1,speedPercent:100}]);
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect(calls.filter(c=>c.command==='marketplace_mod_change').map(c=>c.args)).toEqual([{session:1,id:'player_speed',enabled:true},{session:1,id:'player_speed',enabled:false}]);
 expect(calls.some(c=>c.command==='marketplace_player_jump'&&c.args.heightPercent!=null)).toBe(false);
});

test('running game and unsupported builds lock movement settings; failed saves remain unsaved',async({page})=>{
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Player movement speed',exact:true})});
 for(const options of [{running:true},{version:'1.0.0.9999'}]){
  await fixture(page,options);
  await expect(card.locator('input[type=range]')).toBeDisabled();
  await expect(card.getByRole('button',{name:'Save movement speed',exact:true})).toBeDisabled();
  await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 }
 await fixture(page,{stale:true});
 await card.locator('input[type=range]').fill('150');
 await card.getByRole('button',{name:'Save movement speed',exact:true}).click();
 await expect(card.getByRole('alert')).toContainText('Close Crimson Desert');
 await expect(card).toContainText('Unsaved setting');
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
});

test('active mount protection has its own lifecycle and refuses changes during play',async({page})=>{
 await fixture(page,{running:true});
 let card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Protect active mount',exact:true})});
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 await fixture(page);
 card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Protect active mount',exact:true})});
 await expect(card).toContainText('including while you are on foot');
 await expect(card).toContainText('Scripted deaths and despawns keep their normal behavior.');
 await card.getByRole('button',{name:'Install mod',exact:true}).click();
 await expect(card.getByRole('button',{name:'Remove mod',exact:true})).toBeEnabled();
 await card.scrollIntoViewIfNeeded();await page.screenshot({path:'../.local/marketplace-mount-protection-preview.png'});
 await card.getByRole('button',{name:'Remove mod',exact:true}).click();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_mod_change'));
 expect(calls.map(c=>({id:c.args.id,enabled:c.args.enabled}))).toEqual([{id:'mount_protection',enabled:true},{id:'mount_protection',enabled:false}]);
});

test('consumable selection saves exact item types and installs independently',async({page})=>{
 await fixture(page);
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Preserve selected consumables',exact:true})});
 const install=card.getByRole('button',{name:'Install mod',exact:true});await expect(install).toBeDisabled();
 await card.getByLabel('Find consumable to preserve',{exact:true}).fill('1000034');
 await expect(card.locator('.market-consumable-row')).toHaveCount(1);
 await card.getByRole('checkbox',{name:'Preserve Test Stat Potion (1000034)',exact:true}).check();
 await expect(card).toContainText('Unsaved selection');await expect(install).toBeDisabled();
 let writes=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_consumables'&&c.args.itemKeys!=null));expect(writes).toHaveLength(0);
 await card.getByRole('button',{name:'Save protected items',exact:true}).click();await expect(install).toBeEnabled();
 await install.click();await expect(card.getByRole('button',{name:'Remove mod',exact:true})).toBeEnabled();
 await card.getByLabel('Find consumable to preserve',{exact:true}).fill('');
 await expect(card.getByRole('checkbox',{name:'Preserve Test Arrow (2200)',exact:true})).not.toBeChecked();
 await expect(card).toContainText('normal eating, healing, potion or arrow use');
 await card.scrollIntoViewIfNeeded();await page.screenshot({path:'../.local/marketplace-consumables-preview.png'});
 await card.getByRole('button',{name:'Clear protected items',exact:true}).click();
 await card.getByRole('button',{name:'Save protected items',exact:true}).click();
 await card.getByRole('button',{name:'Remove mod',exact:true}).click();await expect(install).toBeDisabled();
 writes=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_consumables'&&c.args.itemKeys!=null));
 expect(writes.map(c=>c.args.itemKeys)).toEqual([[1000034],[]]);
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect(calls.filter(c=>c.command==='marketplace_mod_change').map(c=>c.args.id)).toEqual(['preserve_consumables','preserve_consumables']);
 expect(calls.some(c=>['mod_export','live_start','skill_unlock_apply'].includes(c.command))).toBe(false);
});

test('food and healing selections are independent, searchable and saved explicitly',async({page})=>{
 await fixture(page);const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Preserve selected consumables',exact:true})});
 const search=card.getByLabel('Find consumable to preserve',{exact:true});await search.fill('Food');
 await expect(card.locator('.market-consumable-row')).toHaveCount(1);await card.getByRole('checkbox',{name:'Preserve Test Honey (10033)',exact:true}).check();
 await search.fill('Healing');await card.getByRole('checkbox',{name:'Preserve Test Holy Water (22009)',exact:true}).check();
 await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 let writes=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_consumables'&&c.args.itemKeys!=null));expect(writes).toHaveLength(0);
 await card.getByRole('button',{name:'Save protected items',exact:true}).click();await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeEnabled();
 writes=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='marketplace_consumables'&&c.args.itemKeys!=null));expect(writes.map(c=>c.args.itemKeys)).toEqual([[10033,22009]]);
 await search.fill('');await expect(card.getByRole('checkbox',{name:'Preserve Test Arrow (2200)',exact:true})).not.toBeChecked();await expect(card.getByRole('checkbox',{name:'Preserve Test Stat Potion (1000034)',exact:true})).not.toBeChecked();
});

test('consumable controls refuse running and unsupported changes; failed saves stay unsaved',async({page})=>{
 for(const options of [{running:true},{version:'unknown'}]){
  await fixture(page,options);
  const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Preserve selected consumables',exact:true})});
  await expect(card.getByLabel('Find consumable to preserve',{exact:true})).toBeDisabled();
  for(const checkbox of await card.getByRole('checkbox').all())await expect(checkbox).toBeDisabled();
  await expect(card.getByRole('button',{name:'Save protected items',exact:true})).toBeDisabled();
  await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 }
 await fixture(page,{stale:true});
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Preserve selected consumables',exact:true})});
 await card.getByRole('button',{name:'Select all supported items',exact:true}).click();
 await card.getByRole('button',{name:'Save protected items',exact:true}).click();
 await expect(card.getByRole('alert')).toContainText('Close Crimson Desert before saving consumable settings.');
 await expect(card).toContainText('Unsaved selection');await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
});

test('ordinary free crafting is an independent runtime mod and preserves the separate fee choice',async({page})=>{
 await fixture(page,{running:true});
 const runtime=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Free crafting materials',exact:true})});
 await expect(runtime.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 await fixture(page);
 await expect(runtime).toContainText('1,507 supported recipes');
 await expect(runtime).toContainText('The separate Clockwork Insect fee is still required');
 const fee=page.locator('.market-card').filter({has:page.getByRole('heading',{name:'No Clockwork Insect crafting fee',exact:true})});
 await fee.getByRole('button',{name:'Select mod',exact:true}).click();
 await runtime.getByRole('button',{name:'Install mod',exact:true}).click();
 await expect(runtime.getByRole('button',{name:'Remove mod',exact:true})).toBeEnabled();
 await expect(fee.getByRole('button',{name:'Deselect mod',exact:true})).toBeEnabled();
 await runtime.scrollIntoViewIfNeeded();await page.screenshot({path:'../.local/marketplace-free-crafting-preview.png'});
 await runtime.getByRole('button',{name:'Remove mod',exact:true}).click();
 await expect(fee.getByRole('button',{name:'Deselect mod',exact:true})).toBeEnabled();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect(calls.filter(c=>c.command==='marketplace_mod_change').map(c=>c.args)).toEqual([{session:1,id:'free_crafting_materials',enabled:true},{session:1,id:'free_crafting_materials',enabled:false}]);
 expect(calls.some(c=>['mod_preview','mod_export','live_start','skill_unlock_apply'].includes(c.command))).toBe(false);
});

test('Force Palm removes the aerial use limit through an independent runtime card',async({page})=>{
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Unlimited Force Palm jumps',exact:true})});
 for(const options of [{running:true},{version:'unknown'}]) {
  await fixture(page,options);
  await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 }
 await fixture(page);
 await expect(card).toContainText('normal three-jump limit');
 await expect(card).toContainText('normal spirit costs and movement checks still apply');
 await card.getByRole('button',{name:'Install mod',exact:true}).click();
 await expect(card.getByRole('button',{name:'Remove mod',exact:true})).toBeEnabled();
 const height=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Player jump height',exact:true})});
 await expect(height.getByRole('button',{name:'Install mod',exact:true})).toBeEnabled();
 await card.scrollIntoViewIfNeeded();await page.screenshot({path:'../.local/marketplace-force-palm-preview.png'});
 await card.getByRole('button',{name:'Remove mod',exact:true}).click();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect(calls.filter(c=>c.command==='marketplace_mod_change').map(c=>c.args)).toEqual([{session:1,id:'force_palm',enabled:true},{session:1,id:'force_palm',enabled:false}]);
 expect(calls.some(c=>['mod_preview','mod_export','live_start','skill_unlock_apply'].includes(c.command))).toBe(false);
});

test('five-times fall threshold has an independent lion and Rokade runtime card',async({page})=>{
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Stay mounted on longer falls',exact:true})});
 for(const options of [{running:true},{version:'unknown'}]) {
  await fixture(page,options);
  await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 }
 await fixture(page);
 await expect(card).toContainText('five times the normal automatic fall-height threshold');
 await expect(card).toContainText('Manual dismounts, combat throws');
 await expect(card.getByText('Works while playing',{exact:true})).toBeVisible();
 await card.getByRole('button',{name:'Install mod',exact:true}).click();
 await expect(card.getByRole('button',{name:'Remove mod',exact:true})).toBeEnabled();
 const jump=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Higher lion & Rokade jumps',exact:true})});
 await expect(jump.getByRole('button',{name:'Install mod',exact:true})).toBeEnabled();
 await card.scrollIntoViewIfNeeded();await page.screenshot({path:'../.local/marketplace-mount-dismount-preview.png'});
 await card.getByRole('button',{name:'Remove mod',exact:true}).click();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect(calls.filter(c=>c.command==='marketplace_mod_change').map(c=>c.args)).toEqual([{session:1,id:'mount_dismount',enabled:true},{session:1,id:'mount_dismount',enabled:false}]);
 expect(calls.some(c=>['mod_preview','mod_export','live_start','skill_unlock_apply'].includes(c.command))).toBe(false);
});

test('world card exposes live controls only after setup and during gameplay',async({page})=>{
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Live time & weather',exact:true})});
 for(const options of [{running:true},{worldInstalled:true},{running:true,worldInstalled:true,version:'unknown'}]) {
  await fixture(page,options);
  await expect(card.getByRole('button',{name:'Open live controls',exact:true})).toBeDisabled();
 }
 await fixture(page,{running:true,worldInstalled:true});
 await expect(card.getByText('Adjust while playing',{exact:true})).toBeVisible();
 await expect(card.getByRole('button',{name:'Remove mod',exact:true})).toBeDisabled();
 await card.getByRole('button',{name:'Open live controls',exact:true}).click();
 const dialog=page.getByRole('dialog',{name:'Live time & weather',exact:true});
 await expect(dialog.getByRole('button',{name:'Reset weather',exact:true})).toBeEnabled();
 await expect(dialog.getByRole('button',{name:'Freeze time',exact:true})).toBeEnabled();
 await page.screenshot({path:'../.local/marketplace-live-world-modal-preview.png'});
 await dialog.getByRole('button',{name:'Close',exact:true}).click();
 await expect(dialog).toHaveCount(0);
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect(calls.some(c=>c.command==='world_status')).toBe(true);
 expect(calls.some(c=>['world_change','mod_export','marketplace_mod_change'].includes(c.command))).toBe(false);
});

test('photo card has independent setup and live controls without starting a photo session automatically',async({page})=>{
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Extended photo mode',exact:true})});
 for(const options of [{running:true},{photoInstalled:true},{running:true,photoInstalled:true,version:'unknown'}]){
  await fixture(page,options);await expect(card.getByRole('button',{name:'Open photo controls',exact:true})).toBeDisabled();
 }
 await fixture(page,{running:true,photoInstalled:true});
 await expect(card.getByText('Adjust while playing',{exact:true})).toBeVisible();
 await expect(card.getByRole('button',{name:'Remove mod',exact:true})).toBeDisabled();
 await card.getByRole('button',{name:'Open photo controls',exact:true}).click();
 const dialog=page.getByRole('dialog',{name:'Extended photo mode',exact:true});
 await expect(dialog.getByRole('button',{name:'Enter photo mode',exact:true})).toBeEnabled();
 await dialog.getByRole('button',{name:'Close',exact:true}).click();await expect(dialog).toHaveCount(0);
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect(calls.some(c=>c.command==='photo_status')).toBe(true);
 expect(calls.some(c=>['photo_change','world_change','marketplace_mod_change'].includes(c.command))).toBe(false);
 await fixture(page);await card.getByRole('button',{name:'Install mod',exact:true}).click();
 await expect(card.getByRole('button',{name:'Remove mod',exact:true})).toBeEnabled();
 await card.getByRole('button',{name:'Remove mod',exact:true}).click();
 const setup=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect(setup.filter(c=>c.command==='marketplace_mod_change').map(c=>c.args)).toEqual([{session:1,id:'photo_mode',enabled:true},{session:1,id:'photo_mode',enabled:false}]);
});

test('teleport card requires setup and exposes its live controls without moving the player',async({page})=>{
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Teleport',exact:true})});
 for(const options of [{running:true},{teleportInstalled:true},{running:true,teleportInstalled:true,version:'unknown'}]){
  await fixture(page,options);await expect(card.getByRole('button',{name:'Open teleport controls',exact:true})).toBeDisabled();
 }
 await fixture(page,{running:true,teleportInstalled:true});await expect(card.getByText('Adjust while playing',{exact:true})).toBeVisible();
 await card.getByRole('button',{name:'Open teleport controls',exact:true}).click();
 const dialog=page.getByRole('dialog',{name:'Teleport',exact:true});await expect(dialog.getByLabel('Position name',{exact:true})).toBeEnabled();
 await dialog.getByRole('button',{name:'Close',exact:true}).click();await expect(dialog).toHaveCount(0);
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect(calls.some(c=>c.command==='teleport_status')).toBe(true);expect(calls.some(c=>['teleport_change','marketplace_mod_change'].includes(c.command))).toBe(false);
 await fixture(page);await card.getByRole('button',{name:'Install mod',exact:true}).click();
 await expect(card.getByRole('button',{name:'Remove mod',exact:true})).toBeEnabled();await card.getByRole('button',{name:'Remove mod',exact:true}).click();
 const setup=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect(setup.filter(c=>c.command==='marketplace_mod_change').map(c=>c.args)).toEqual([{session:1,id:'teleport',enabled:true},{session:1,id:'teleport',enabled:false}]);
});

test('auto-loot uses an independent runtime card with normal pickup limits',async({page})=>{
 const card=page.locator('.runtime-card').filter({has:page.getByRole('heading',{name:'Auto-loot nearby items',exact:true})});
 for(const options of [{running:true},{version:'unknown'}]) {
  await fixture(page,options);
  await expect(card.getByRole('button',{name:'Install mod',exact:true})).toBeDisabled();
 }
 await fixture(page);
 await expect(card).toContainText('normal interaction radius');
 await expect(card).toContainText('normal corpse loot');
 await expect(card).toContainText('native ownership, crime, animation and inventory-capacity checks');
 await expect(card).toContainText('Purchases, stealing and scripted interactions are excluded');
 await expect(card.getByText('Works while playing',{exact:true})).toBeVisible();
 await card.getByRole('button',{name:'Install mod',exact:true}).click();
 await expect(card.getByRole('button',{name:'Remove mod',exact:true})).toBeEnabled();
 await card.scrollIntoViewIfNeeded();await page.screenshot({path:'../.local/marketplace-auto-loot-preview.png'});
 await card.getByRole('button',{name:'Remove mod',exact:true}).click();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect(calls.filter(c=>c.command==='marketplace_mod_change').map(c=>c.args)).toEqual([{session:1,id:'auto_loot',enabled:true},{session:1,id:'auto_loot',enabled:false}]);
 expect(calls.some(c=>['mod_preview','mod_export','live_start','skill_unlock_apply'].includes(c.command))).toBe(false);
});

test('crafting currency component changes only the reviewed archive draft',async({page})=>{
 await fixture(page,{running:true});
 const card=page.locator('.market-card').filter({has:page.getByRole('heading',{name:'No Clockwork Insect crafting fee',exact:true})});
 await expect(card).toContainText('300 Clockwork Insects');
 await expect(card).toContainText('Material costs are controlled separately.');
 await card.getByRole('button',{name:'Select mod',exact:true}).click();
 await page.getByRole('button',{name:'Review combined plan',exact:true}).click();
 const calls=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls);
 expect((calls.find(c=>c.command==='mod_preview')!.args.request as {marketplace:unknown}).marketplace).toMatchObject({free_crafting_currency:true});
 expect(calls.some(c=>['live_start','skill_unlock_apply','marketplace_mod_change'].includes(c.command))).toBe(false);
 await card.getByRole('button',{name:'Deselect mod',exact:true}).click();
 await expect(page.getByRole('button',{name:'Export local package',exact:true})).toHaveCount(0);
});


test('free upgrades remain independent of crafting currency and require the reviewed plan',async({page})=>{
 await fixture(page,{running:true});
 const upgrades=page.locator('.market-card').filter({has:page.getByRole('heading',{name:'Free equipment upgrades',exact:true})});
 const currency=page.locator('.market-card').filter({has:page.getByRole('heading',{name:'No Clockwork Insect crafting fee',exact:true})});
 await expect(upgrades).toContainText('17,069 upgrade recipes');
 await expect(upgrades).toContainText('existing equipment still required');
 await upgrades.getByRole('button',{name:'Select mod',exact:true}).click();
 await page.getByRole('button',{name:'Review combined plan',exact:true}).click();
 let requests=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='mod_preview'));
 expect((requests.at(-1)!.args.request as {marketplace:unknown}).marketplace).toMatchObject({free_upgrade_materials:true,free_crafting_currency:false});
 await currency.getByRole('button',{name:'Select mod',exact:true}).click();
 await expect(page.getByRole('button',{name:'Export local package',exact:true})).toHaveCount(0);
 await page.getByRole('button',{name:'Review combined plan',exact:true}).click();
 requests=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='mod_preview'));
 expect((requests.at(-1)!.args.request as {marketplace:unknown}).marketplace).toMatchObject({free_upgrade_materials:true,free_crafting_currency:true});
 const writes=await page.evaluate(()=> (window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>['live_start','skill_unlock_apply','marketplace_mod_change'].includes(c.command)));
 expect(writes).toHaveLength(0);
 await upgrades.scrollIntoViewIfNeeded();await page.screenshot({path:'../.local/marketplace-upgrade-costs-preview.png'});
});


test('journal knowledge previews this save, confirms changes and disables a complete save',async({page})=>{
 await fixture(page);
 await page.getByRole('textbox',{name:'Search mods',exact:true}).fill('Wissenssammlung');
 const card=page.locator('.market-card:visible');await expect(card).toHaveCount(1);
 await card.getByRole('button',{name:'Choose save & review',exact:true}).click();
 const dialog=page.getByRole('dialog',{name:'Unlock journal knowledge',exact:true});
 const apply=dialog.getByRole('button',{name:'Back up & unlock knowledge',exact:true});
 await expect(apply).toBeDisabled();
 await dialog.getByLabel('Knowledge unlock save',{exact:true}).selectOption('123/slot0');
 await expect(dialog.locator('.skills-summary strong')).toHaveText(['1966','111','1862']);
 await expect(apply).toBeDisabled();
 await dialog.getByLabel('Confirm knowledge save edit',{exact:true}).check();await apply.click();
 await expect(dialog).toContainText('Journal knowledge unlocked. Existing progress preserved.');
 await expect(dialog).toContainText('Journal knowledge is already complete in this save.');
 await expect(apply).toBeDisabled();
 await expect(dialog).toContainText('C:/Synthetic/.local/knowledge-unlocks/backup');
 const writes=await page.evaluate(()=>(window as unknown as {marketCalls:{command:string}[]}).marketCalls.filter(c=>c.command==='knowledge_unlock_apply'));
 expect(writes).toHaveLength(1);
});

test('journal unlock blocks gameplay and retries the same stale request without silently switching saves',async({page})=>{
 for(const running of [true,false]){
  await fixture(page,{running,stale:true});
  await page.locator('.knowledge-card').getByRole('button',{name:'Choose save & review',exact:true}).click();
  const dialog=page.getByRole('dialog',{name:'Unlock journal knowledge',exact:true});
  await dialog.getByLabel('Knowledge unlock save',{exact:true}).selectOption('123/slot0');
  if(running){await expect(dialog.getByLabel('Confirm knowledge save edit',{exact:true})).toBeDisabled();continue}
  await dialog.getByLabel('Confirm knowledge save edit',{exact:true}).check();
  await dialog.getByRole('button',{name:'Back up & unlock knowledge',exact:true}).click();
  await expect(dialog).toContainText('Save changed since preview');
  await expect(dialog.getByLabel('Knowledge unlock save',{exact:true})).toBeDisabled();
  await dialog.getByRole('button',{name:'Check / recover pending knowledge unlock',exact:true}).click();
  const writes=await page.evaluate(()=>(window as unknown as {marketCalls:{command:string;args:Record<string,unknown>}[]}).marketCalls.filter(c=>c.command==='knowledge_unlock_apply'));
  expect(writes).toHaveLength(2);expect(writes[0].args.request).toEqual(writes[1].args.request);
 }
});
