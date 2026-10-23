import {expect,test,type Page} from '@playwright/test';
async function fixture(page:Page,running=false,lost=false,wrongSave=false,portrait=false,example=false,exotic=false,readOnly=false,liveMount=false) {
  await page.addInitScript(({running,lost,wrongSave,portrait,example,exotic,readOnly,liveMount})=>{
    const root=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;mountCalls:{command:string;args:Record<string,unknown>}[]};
    root.isTauri=true;root.mountCalls=[];let owned=false,first=true;
    root.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>={})=>{
      if(command==='bootstrap')return {project:'C:/Test',discovery:{installations:[{path:'C:/Synthetic'}],configured_game:'C:/Synthetic'},languages:[{language:'ger'}]};
      if(command==='open_catalog')return {session:1,game_path:'C:/Synthetic',read_only:liveMount,info:{item_count:0,types:[],categories:[],tiers:[],stats:[],groups:[],index:{}}};
      if(command==='search_items')return {items:[],total:0,offset:0};
      if(command==='mount_icon')return {data_url:portrait&&args.key===30108?'data:image/svg+xml;base64,'+btoa('<svg xmlns="http://www.w3.org/2000/svg" width="256" height="256"><circle cx="128" cy="128" r="90" fill="#987654"/></svg>'):null,source:null,reason:portrait?null:'Kein Animal portrait.',caption:example?'Beispielbild derselben Tierart. Farbe dieser Variante kann abweichen.':null};
      if(command==='mount_search'){
        let pattern:RegExp;try{pattern=new RegExp(String(args.text),'i');}catch{throw {message:'Invalid regex pattern.'};}
        return [{key:30108,name:'Schwarzbär'},{key:31378,name:'Rokade'}].filter(m=>pattern.test(m.name)).map(m=>m.key);
      }
      if(command==='mount_catalog'){
        const selected=String(args.save??(wrongSave?'22202/slot1':'123/slot2')),old=selected==='22202/slot1';
        return {registration_available:!readOnly,mounts:[{key:exotic?1000265:30108,name:exotic?'Rotfederraptor':'Schwarzbär',internal:exotic?'Riding_CarmaBirdsaurus_1':'Animal_Black_Bear',family:exotic?'Dinosaurs':'Bears',vehicle:exotic?16986:16979,owned:old?0:owned?1:0,supported:!old&&!owned,registration:exotic?'base':'same_family',description:'Reittier für den Stall.',reason:owned?'Bereits registriert.':exotic?'Basiseintrag ohne Tierart-Vorlage möglich.':'Passende Stallvorlage vorhanden.'},{key:31378,name:'Rokade',internal:'Riding_Rokade',family:'Horses',vehicle:16960,owned:old?0:1,supported:false,description:'Ein Pferd.',reason:'Bereits registriert.'}],saves:[{id:'123/slot2',label:'Slot 2',modified:1700000000},{id:'123/slot1',label:'Slot 1',modified:1600000000},...(wrongSave?[{id:'22202/slot1',label:'Anderes Konto · Slot 1',modified:1500000000}]:[])],selected_save:selected,save_sha256:'a'.repeat(64),lobby_sha256:'b'.repeat(64),game_running:running,message:running?'Save and fully close the game before adding a mount.':'Register in stable.'};
      }
      if(command==='mount_live_status'||command==='mount_live_grant'){
        const request=args.request as Record<string,unknown>|null;
        if(command==='mount_live_grant'){
          root.mountCalls.push({command,args}); owned=true;
          if(lost&&first){first=false;throw {message:'Live reply interrupted.'};}
        }
        return {state:request?'applied':'ready',pid:14,epoch:7,id:request?.id??'',key:request?.key??0,quantity:request?.quantity??0,message:request?'One additional mount confirmed in the loaded stable.':'Live mount module connected.'};
      }
      if(command==='mount_register'){
        root.mountCalls.push({command,args});owned=true;
        if(lost&&first){first=false;throw {message:'Antwort unterbrochen.'};}
        await new Promise(r=>setTimeout(r,100));return {request:args.request,name:exotic?'Rotfederraptor':'Schwarzbär',mercenary_no:'1001',donor_key:exotic?0:30048,backup:'C:/Private/backup',message:exotic?'Rotfederraptor im Stall registriert. Base entry: test summoning and riding in game.':'Schwarzbär im Stall registriert. Diesen Save laden.'};
      }
      throw {message:`Unexpected command ${command}`};
    }};
  },{running,lost,wrongSave,portrait,example,exotic,readOnly,liveMount});
  await page.goto('/');await expect(page.locator('.workbench')).toHaveAttribute('data-ready','true');
  await page.getByRole('button',{name:'Mounts',exact:true}).click();await expect(page.locator('.mount-row')).toHaveCount(2);
}
test('mount search, categories, description and correct save registration',async({page})=>{
  await fixture(page);await page.getByLabel('Search mounts').fill('30108');await expect(page.locator('.mount-row')).toHaveCount(1);
  await page.locator('.mount-row').click();await expect(page.locator('.picker-detail')).toContainText('Reittier für den Stall.');
  await page.getByLabel('Save for mounts').selectOption('123/slot1');await page.getByRole('button',{name:'Register in stable',exact:true}).dblclick();
  await expect(page.locator('.mount-success')).toContainText('Schwarzbär im Stall registriert');
  const calls=await page.evaluate(()=>(window as unknown as {mountCalls:{args:{request:{key:number;save:string}}}[]}).mountCalls);
  expect(calls).toHaveLength(1);expect(calls[0].args.request).toMatchObject({key:30108,save:'123/slot1'});
  await expect(page.getByRole('button',{name:'Register in stable',exact:true})).toBeDisabled();
  await page.screenshot({path:'../.local/mount-family-investigation-20261004/mount-picker.png'});
});

test('example portrait is labelled in the detail and zoom without claiming the exact variant',async({page})=>{
  await fixture(page,false,false,false,true,true);
  await page.locator('.mount-row').filter({hasText:'Schwarzbär'}).click();
  await expect(page.locator('.mount-hero')).toContainText('Example image of species');
  await expect(page.locator('.mount-hero')).toContainText('Farbe dieser Variante kann abweichen');
  await page.getByRole('button',{name:'Enlarge mount image'}).click();
  const dialog=page.getByRole('dialog',{name:'Image preview: Schwarzbär'});
  await expect(dialog.locator('img')).toBeVisible();
  await expect(dialog).toContainText('Beispielbild derselben Tierart');
  await expect(dialog).not.toContainText('Original animal portrait');
  expect(await page.evaluate(()=>(window as unknown as {mountCalls:unknown[]}).mountCalls.length)).toBe(0);
  await page.screenshot({path:'../.local/mount-family-investigation-20261004/mount-example-zoom.png'});
});
test('matched mount portrait loads in the row and can be enlarged without registering an animal',async({page})=>{
  await fixture(page,false,false,false,true);await expect(page.locator('.mount-row').filter({hasText:'Schwarzbär'}).locator('img')).toBeVisible();
  await page.locator('.mount-row').filter({hasText:'Schwarzbär'}).click();await expect(page.locator('.mount-portrait img')).toBeVisible();
  await page.getByRole('button',{name:'Enlarge mount image'}).click();const dialog=page.getByRole('dialog',{name:'Image preview: Schwarzbär'});
  await expect(dialog.locator('img')).toBeVisible();await expect(dialog).toContainText('Original animal portrait');
  await page.keyboard.press('Escape');await expect(dialog).toHaveCount(0);
  expect(await page.evaluate(()=>(window as unknown as {mountCalls:unknown[]}).mountCalls.length)).toBe(0);
});
test('mount symbol can be enlarged and regex search can recover from an invalid pattern',async({page})=>{
  await fixture(page);await page.locator('.mount-row').filter({hasText:'Schwarzbär'}).click();
  await page.getByRole('button',{name:'Enlarge mount image'}).click();
  const dialog=page.getByRole('dialog',{name:'Image preview: Schwarzbär'});await expect(dialog).toBeVisible();await expect(dialog).toContainText('Generic animal icon');
  await page.getByRole('button',{name:'Close image preview'}).click();await expect(dialog).toHaveCount(0);
  await page.getByLabel('Regex search',{exact:true}).check();const search=page.getByLabel('Search mounts');await search.fill('^Rokade$');
  await expect(page.locator('.mount-row')).toHaveCount(1);await expect(page.locator('.mount-row')).toContainText('Rokade');
  await search.fill('[');await expect(page.getByRole('alert')).toContainText('Invalid regex pattern');
  await search.fill('bär');await expect(page.locator('.mount-row')).toHaveCount(1);await expect(page.locator('.mount-row')).toContainText('Schwarzbär');
  await page.screenshot({path:'../.local/mount-family-investigation-20261004/mount-picker-updated.png'});
  expect(await page.evaluate(()=>(window as unknown as {mountCalls:unknown[]}).mountCalls.length)).toBe(0);
});
test('running game and already owned mount disable writes',async({page})=>{
  await fixture(page,true);await page.locator('.mount-row').filter({hasText:'Schwarzbär'}).click();
  await expect(page.getByRole('button',{name:'Register in stable',exact:true})).toBeDisabled();
  await page.getByRole('button',{name:'Horses',exact:true}).click();await expect(page.locator('.mount-row')).toHaveCount(1);
  await page.locator('.mount-row').click();await expect(page.locator('.picker-detail')).toContainText('Bereits registriert.');
  expect(await page.evaluate(()=>(window as unknown as {mountCalls:unknown[]}).mountCalls.length)).toBe(0);
});
test('lost reply keeps UUID and explicit retry reuses the exact request',async({page})=>{
  await fixture(page,false,true);await page.locator('.mount-row').filter({hasText:'Schwarzbär'}).click();
  await page.getByRole('button',{name:'Register in stable',exact:true}).click();await expect(page.getByRole('alert')).toContainText('Antwort unterbrochen');
  await page.getByRole('button',{name:'Check / continue request',exact:true}).click();await expect(page.locator('.mount-success')).toContainText('Schwarzbär im Stall registriert');
  const calls=await page.evaluate(()=>(window as unknown as {mountCalls:{args:{request:unknown}}[]}).mountCalls);
  expect(calls).toHaveLength(2);expect(calls[0].args.request).toEqual(calls[1].args.request);
});
test('no donor in an older account explains the cause and switching to latest exposes available mounts',async({page})=>{
  await fixture(page,false,false,true);
  await expect(page.locator('.mount-status')).toContainText('0 available to add');
  await expect(page.locator('.mount-status')).toContainText('Check the slot and account above');
  await page.getByRole('button',{name:'Select newest save'}).click();
  await expect(page.getByLabel('Save for mounts')).toHaveValue('123/slot2');
  await expect(page.locator('.mount-status')).toContainText('1 available to add');
  await expect(page.getByRole('button',{name:'Bears',exact:true})).toContainText('(1)');
  await page.getByLabel('Only mounts available to add (1)',{exact:true}).check();
  await expect(page.locator('.mount-row')).toHaveCount(1);
  await page.getByRole('button',{name:'Horses',exact:true}).click();
  await expect(page.locator('.mount-list')).toContainText('No mounts available to add with this filter');
  expect(await page.evaluate(()=>(window as unknown as {mountCalls:unknown[]}).mountCalls.length)).toBe(0);
});

test('exotic mount base entry is selectable and distinguishes copied stats from pending game behavior',async({page})=>{
  await fixture(page,false,false,false,false,false,true);
  await page.getByLabel('Only mounts available to add (1)',{exact:true}).check();
  await expect(page.locator('.mount-row')).toHaveCount(1);
  await expect(page.locator('.mount-row')).toContainText('Base entry · Game test pending');
  await page.locator('.mount-row').click();
  await expect(page.locator('.picker-detail')).toContainText('health and levels from another animal are not copied');
  await expect(page.locator('.picker-detail')).toContainText('in-game behavior is not yet confirmed');
  await expect(page.locator('.picker-detail')).not.toContainText('riding equipment are copied from the existing template');
  await page.getByRole('button',{name:'Register in stable',exact:true}).click();
  await expect(page.locator('.mount-success')).toContainText('test summoning and riding in game');
  const calls=await page.evaluate(()=>(window as unknown as {mountCalls:{args:{request:{key:number}}}[]}).mountCalls);
  expect(calls).toHaveLength(1);expect(calls[0].args.request.key).toBe(1000265);
  await page.screenshot({path:'../.local/mount-family-investigation-20261004/base-picker.png'});
});

test('read-only mount catalog never offers registration or template recovery',async({page})=>{
  await fixture(page,false,false,false,false,false,false,true);
  await page.locator('.mount-row').filter({hasText:'Schwarzbär'}).click();
  await expect(page.getByRole('button',{name:'Register in stable',exact:true})).toBeDisabled();
  await expect(page.locator('.picker-detail')).not.toContainText('Values and riding equipment are copied');
  await expect(page.locator('.mount-status')).not.toContainText('no suitable stable template');
  await expect(page.locator('.page-heading')).toContainText('inspect ownership');
});

test('native live mounts send one request for the selected catalog key while game is running',async({page})=>{
  await fixture(page,true,false,false,false,false,false,false,true);
  await page.locator('.mount-row').filter({hasText:'Schwarzbär'}).click();
  const button=page.getByRole('button',{name:'Register selected mount live',exact:true});
  await expect(button).toBeEnabled();
  await button.dblclick();
  await expect(page.getByRole('region',{name:'Live mount registration'})).toContainText('additional mount confirmed');
  const calls=await page.evaluate(()=>(window as unknown as {mountCalls:{command:string;args:{request:{key:number;quantity:number;pid:number;epoch:number}}}[]}).mountCalls);
  expect(calls).toHaveLength(1);
  expect(calls[0].command).toBe('mount_live_grant');
  expect(calls[0].args.request).toMatchObject({key:30108,quantity:1,pid:14,epoch:7});
});
test('live mount lost reply queries its saved ID without resubmitting the grant',async({page})=>{
  await fixture(page,true,true,false,false,false,false,false,true);
  await page.locator('.mount-row').filter({hasText:'Schwarzbär'}).click();
  await page.getByRole('button',{name:'Register selected mount live',exact:true}).click();
  await expect(page.getByRole('region',{name:'Live mount registration'})).toContainText('additional mount confirmed');
  expect(await page.evaluate(()=>(window as unknown as {mountCalls:unknown[]}).mountCalls.length)).toBe(1);
});
