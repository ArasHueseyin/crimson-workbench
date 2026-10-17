import {expect,test,type Page} from '@playwright/test';
type Harness={calls:{command:string;args:Record<string,unknown>}[];running:boolean;configured:boolean;failOpen:boolean};
async function fixture(page:Page){
  await page.addInitScript(()=>{
    const w=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;ux:Harness};
    w.isTauri=true;w.ux={calls:[],running:false,configured:true,failOpen:false};
    let settings={version:1,game_dir:'C:/Synthetic Game',save_dir:'C:/Saves',language:'auto'};
    const saves=[{id:'account-a/slot0',label:'Manual save 0',modified:100},{id:'account-b/slot0',label:'Manual save 0',modified:200}];
    const items=Array.from({length:30},(_,i)=>({key:1000+i,name:`Test item ${i}`,description:'A short item description.',internal_key:`Test_${i}`,item_type:1,category:1,tier:1,max_stack:'10',stat_keys:[],icon_key:null}));
    const info={vendors:[],dropsets:[],store_rows:0,stock_rows:0,opaque_stores:0,opaque_dropsets:0,quantity_excluded:0,trust_rows:0,limitations:[],items:[],append_vendors:[],daily_vendors:[],chance_dropsets:[],guarantee_dropsets:[]};
    const boot=()=>({project:'C:/Synthetic',settings:{...settings},discovery:{installations:[{path:settings.game_dir,platform:'explicit'}],configured_game:settings.game_dir,save_directories:['C:/Saves']},languages:[{language:'eng'},{language:'ger'}]});
    w.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>={})=>{
      w.ux.calls.push({command,args});
      if(command==='bootstrap')return boot();
      if(command==='save_settings'){settings={...settings,...args.settings as typeof settings};return boot()}
      if(command==='open_catalog'){if(w.ux.failOpen)throw{code:'read_error',message:'Synthetic catalog failure'};return{session:1,game_path:args.game,item_language:{language:'ger',source:'steam'},info:{item_count:30,types:[],categories:[],tiers:[],stats:[],groups:[],exe_version:'1.0.0.2976',index:{language:'ger',path:'synthetic',fingerprint:'test'}}}}
      if(command==='marketplace_game_running')return w.ux.running;
      if(command==='spawn_status')throw{message:'Live module not connected.'};
      if(command==='search_items')return{items,total:items.length,offset:0};
      if(command==='item_icon'||command==='mount_icon')return{data_url:null,source:null,reason:'No synthetic icon'};
      if(command==='mod_info')return info;
      if(command==='advanced_item')return{item:{key:args.key,max_endurance:1,enchant_levels:[0],stats:[]}};
      if(command==='knowledge_snapshot')return{saves,selected_save:args.save??saves[0].id,modified:100,items:[],message:'Saved-game knowledge'};
      if(command==='mount_catalog')return{saves,selected_save:args.save??saves[0].id,save_sha256:'a',lobby_sha256:'b',game_running:false,message:'Choose a mount.',mounts:[{key:100,name:'Test lion',internal:'Test_Lion',family:'Lions',vehicle:1,owned:0,supported:true,description:'Test mount',reason:'A supported fixture.'}]};
      if(command==='extra_sockets_snapshot')return{revision:'a',game_running:false,records:[],runtime_log:''};
      if(command==='extra_socket_candidates')return{saves,selected_save:args.save??saves[0].id,save_revision:'b',items:[{uid:'test',item_key:1000,name:'Test sword',baseline:1,location:'Inventory',eligible:true,reason:null,extended:false}]};
      if(command==='abyss_stone_details')return[];
      if(command==='installation_audit_status')return null;
      if(command==='live_status')return{configured:w.ux.configured,initialized:w.ux.configured,game_running:false,baseline_id:'fixture',active_overlay:false,recovery_required:false,issues:[]};
      if(command==='baseline_catalog')return{snapshots:['fixture'],reports:[],truncated:false};
      if(command==='live_job_status')return null;
      if(command==='mod_preview')return{request:args.request,plan_id:'plan',fingerprint:'test',changes:[{module:'shops',table:'storeinfo',key:1,name:'Test change',field:'stock',before:'5',after:'999'}],files:[],warnings:[],gates:{game_running:false,can_apply:false,reasons:[]},credits:'Synthetic'};
      throw Error(`Unexpected ${command}`);
    }};
  });
  await page.goto('/');await expect(page.locator('.workbench')).toHaveAttribute('data-ready','true');
}

test('catalog availability and the game process never claim a live connection',async({page})=>{
  await fixture(page);
  await expect(page.locator('.connection-pill')).toHaveText('Catalog loaded');
  await expect(page.locator('.game-state-pill')).toHaveText('Game closed');
  await expect(page.locator('.picker-runtime-note')).toContainText('Live module not connected');
  await page.evaluate(()=>{(window as unknown as {ux:Harness}).ux.running=true});
  await expect(page.locator('.game-state-pill')).toHaveText('Game running',{timeout:7000});
  await expect(page.locator('.connection-pill')).toHaveText('Catalog loaded');
  await expect(page.locator('.language-picker')).toContainText('Item names');
});

test('edited data sources stay unsaved until Save & connect, and do not piggyback on language selection',async({page})=>{
  await fixture(page);await page.getByRole('button',{name:'Data sources',exact:true}).click();
  await page.getByLabel('Save folder',{exact:true}).fill('D:/Other saves');
  await expect(page.locator('.source-draft-status')).toContainText('Unsaved path changes');
  await expect(page.getByLabel('Item language')).toBeDisabled();
  await expect(page.getByLabel('Item language')).toHaveAttribute('aria-description',/Save & connect/);
  expect(await page.evaluate(()=>(window as unknown as {ux:Harness}).ux.calls.filter(c=>c.command==='save_settings'))).toHaveLength(0);
  await page.getByRole('button',{name:'Save & connect',exact:true}).click();
  await expect(page.getByText('Settings saved and catalog connected.',{exact:true})).toBeVisible();
  await expect(page.getByLabel('Item language')).toBeEnabled();
  const writes=await page.evaluate(()=>(window as unknown as {ux:Harness}).ux.calls.filter(c=>c.command==='save_settings'));
  expect(writes).toHaveLength(1);expect(writes[0].args.settings).toMatchObject({game_dir:'C:/Synthetic Game',save_dir:'D:/Other saves',language:'auto'});
});

test('a failed connection is not announced as connected after settings save',async({page})=>{
  await fixture(page);await page.getByRole('button',{name:'Data sources',exact:true}).click();
  await page.evaluate(()=>{(window as unknown as {ux:Harness}).ux.failOpen=true});
  await page.getByRole('button',{name:'Save & connect',exact:true}).click();
  await expect(page.getByText('Settings saved. The catalog could not be connected; check the error below.',{exact:true})).toBeVisible();
  await expect(page.locator('.connection-pill')).toHaveText('Catalog unavailable');
});

test('the compact item picker keeps its results and grant action in a 720px viewport',async({page})=>{
  await page.setViewportSize({width:1280,height:720});await fixture(page);
  await expect(page.locator('.knowledge-drawer')).not.toHaveAttribute('open','');
  await page.locator('.picker-row').first().click();
  await expect(page.getByLabel('Desired item quantity')).toBeVisible();
  const action=await page.getByRole('button',{name:'Add to inventory',exact:true}).boundingBox();
  const results=await page.locator('.picker-scroll').boundingBox();
  expect(action!.y+action!.height).toBeLessThanOrEqual(720);expect(results!.height).toBeGreaterThan(200);
  expect(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth)).toBe(true);
  await page.screenshot({path:'../.local/app-ux-compact-picker.png'});
});

test('save selectors distinguish accounts and keep each write target beside its action',async({page})=>{
  await fixture(page);await page.getByRole('button',{name:'Mounts',exact:true}).click();
  const save=page.getByLabel('Save for mounts');
  await expect(save.locator('option[value="account-b/slot0"]')).toContainText('Account account-b');
  await expect(save.locator('option[value="account-b/slot0"]')).toContainText('newest');
  await save.selectOption('account-b/slot0');await page.locator('.mount-row').click();
  await expect(page.locator('.picker-detail .save-target')).toContainText('Account account-b');
  await page.screenshot({path:'../.local/app-ux-save-target.png'});
  await page.getByRole('button',{name:'Extra sockets',exact:true}).click();
  await page.getByLabel('Save for socket expansion').selectOption('account-b/slot0');await page.locator('.extra-owned button').click();
  await expect(page.locator('.extra-add-selection .save-target')).toContainText('Account account-b');
  expect(await page.evaluate(()=>(window as unknown as {ux:Harness}).ux.calls.some(c=>['mount_register','extra_socket_add'].includes(c.command)))).toBe(false);
});

test('workshop setup starts open, collapses only on known completion, and respects an explicit reopen',async({page})=>{
  await fixture(page);await page.getByRole('button',{name:'Mod workshop',exact:true}).click();
  const setup=page.locator('.workshop-setup');await expect(setup).toHaveAttribute('open','');
  await page.getByRole('button',{name:'Load live status',exact:true}).click();await expect(setup).not.toHaveAttribute('open','');
  await page.evaluate(()=>{(window as unknown as {ux:Harness}).ux.configured=false});
  await page.getByRole('button',{name:'Load live status',exact:true}).click();await expect(setup).toHaveAttribute('open','');
  await page.evaluate(()=>{(window as unknown as {ux:Harness}).ux.configured=true});
  await page.getByRole('button',{name:'Load live status',exact:true}).click();await expect(setup).not.toHaveAttribute('open','');
  await page.getByLabel('Trust multiplier',{exact:true}).fill('2');await page.getByRole('button',{name:'Calculate preview',exact:true}).click();
  await expect(page.locator('.workshop-steps a[aria-current="step"]')).toContainText('4. Apply');
  await page.getByRole('link',{name:/3\. Checks/}).click();await expect(setup).toHaveAttribute('open','');
  await page.getByRole('button',{name:'Load live status',exact:true}).click();await expect(setup).toHaveAttribute('open','');
  expect(await page.evaluate(()=>(window as unknown as {ux:Harness}).ux.calls.some(c=>c.command==='live_start'))).toBe(false);
  await page.locator('.mods-page').evaluate(element=>{element.scrollTop=0});
  await page.screenshot({path:'../.local/app-ux-workshop.png'});
});
