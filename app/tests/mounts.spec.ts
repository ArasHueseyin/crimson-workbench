import {expect,test,type Page} from '@playwright/test';
async function fixture(page:Page,running=false,lost=false,wrongSave=false,portrait=false,example=false,exotic=false) {
  await page.addInitScript(({running,lost,wrongSave,portrait,example,exotic})=>{
    const root=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;mountCalls:{command:string;args:Record<string,unknown>}[]};
    root.isTauri=true;root.mountCalls=[];let owned=false,first=true;
    root.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>={})=>{
      if(command==='bootstrap')return {project:'C:/Test',discovery:{installations:[{path:'C:/Synthetic'}],configured_game:'C:/Synthetic'},languages:[{language:'ger'}]};
      if(command==='open_catalog')return {session:1,game_path:'C:/Synthetic',info:{item_count:0,types:[],categories:[],tiers:[],stats:[],groups:[],index:{}}};
      if(command==='search_items')return {items:[],total:0,offset:0};
      if(command==='mount_icon')return {data_url:portrait&&args.key===30108?'data:image/svg+xml;base64,'+btoa('<svg xmlns="http://www.w3.org/2000/svg" width="256" height="256"><circle cx="128" cy="128" r="90" fill="#987654"/></svg>'):null,source:null,reason:portrait?null:'Kein Tierporträt.',caption:example?'Beispielbild derselben Tierart. Farbe dieser Variante kann abweichen.':null};
      if(command==='mount_search'){
        let pattern:RegExp;try{pattern=new RegExp(String(args.text),'i');}catch{throw {message:'Ungültiges Regexmuster.'};}
        return [{key:30108,name:'Schwarzbär'},{key:31378,name:'Rokade'}].filter(m=>pattern.test(m.name)).map(m=>m.key);
      }
      if(command==='mount_catalog'){
        const selected=String(args.save??(wrongSave?'22202/slot1':'123/slot2')),old=selected==='22202/slot1';
        return {mounts:[{key:exotic?1000265:30108,name:exotic?'Rotfederraptor':'Schwarzbär',internal:exotic?'Riding_CarmaBirdsaurus_1':'Animal_Black_Bear',family:exotic?'Dinosaurier':'Bären',vehicle:exotic?16986:16979,owned:old?0:owned?1:0,supported:!old&&!owned,registration:exotic?'base':'same_family',description:'Reittier für den Stall.',reason:owned?'Bereits registriert.':exotic?'Basiseintrag ohne Tierart-Vorlage möglich.':'Passende Stallvorlage vorhanden.'},{key:31378,name:'Rokade',internal:'Riding_Rokade',family:'Pferde',vehicle:16960,owned:old?0:1,supported:false,description:'Ein Pferd.',reason:'Bereits registriert.'}],saves:[{id:'123/slot2',label:'Slot 2',modified:1700000000},{id:'123/slot1',label:'Slot 1',modified:1600000000},...(wrongSave?[{id:'22202/slot1',label:'Anderes Konto · Slot 1',modified:1500000000}]:[])],selected_save:selected,save_sha256:'a'.repeat(64),lobby_sha256:'b'.repeat(64),game_running:running,message:running?'Zum Hinzufügen speichern und das Spiel vollständig schließen.':'Im Stall registrieren.'};
      }
      if(command==='mount_register'){
        root.mountCalls.push({command,args});owned=true;
        if(lost&&first){first=false;throw {message:'Antwort unterbrochen.'};}
        await new Promise(r=>setTimeout(r,100));return {request:args.request,name:exotic?'Rotfederraptor':'Schwarzbär',mercenary_no:'1001',donor_key:exotic?0:30048,backup:'C:/Private/backup',message:exotic?'Rotfederraptor im Stall registriert. Basiseintrag: Herbeirufen und Reiten im Spiel prüfen.':'Schwarzbär im Stall registriert. Diesen Spielstand laden.'};
      }
      throw {message:`Unexpected command ${command}`};
    }};
  },{running,lost,wrongSave,portrait,example,exotic});
  await page.goto('/');await expect(page.locator('.workbench')).toHaveAttribute('data-ready','true');
  await page.getByRole('button',{name:'Reittiere',exact:true}).click();await expect(page.locator('.mount-row')).toHaveCount(2);
}
test('mount search, categories, description and correct save registration',async({page})=>{
  await fixture(page);await page.getByLabel('Reittiere durchsuchen').fill('30108');await expect(page.locator('.mount-row')).toHaveCount(1);
  await page.locator('.mount-row').click();await expect(page.locator('.picker-detail')).toContainText('Reittier für den Stall.');
  await page.getByLabel('Spielstand für Reittiere').selectOption('123/slot1');await page.getByRole('button',{name:'Im Stall registrieren',exact:true}).dblclick();
  await expect(page.locator('.mount-success')).toContainText('Schwarzbär im Stall registriert');
  const calls=await page.evaluate(()=>(window as unknown as {mountCalls:{args:{request:{key:number;save:string}}}[]}).mountCalls);
  expect(calls).toHaveLength(1);expect(calls[0].args.request).toMatchObject({key:30108,save:'123/slot1'});
  await expect(page.getByRole('button',{name:'Im Stall registrieren',exact:true})).toBeDisabled();
  await page.screenshot({path:'../.local/mount-family-investigation-20261004/mount-picker.png'});
});

test('example portrait is labelled in the detail and zoom without claiming the exact variant',async({page})=>{
  await fixture(page,false,false,false,true,true);
  await page.locator('.mount-row').filter({hasText:'Schwarzbär'}).click();
  await expect(page.locator('.mount-hero')).toContainText('Beispielbild der Tierart');
  await expect(page.locator('.mount-hero')).toContainText('Farbe dieser Variante kann abweichen');
  await page.getByRole('button',{name:'Reittierbild vergrößern'}).click();
  const dialog=page.getByRole('dialog',{name:'Bildvorschau: Schwarzbär'});
  await expect(dialog.locator('img')).toBeVisible();
  await expect(dialog).toContainText('Beispielbild derselben Tierart');
  await expect(dialog).not.toContainText('Originales Tierporträt');
  expect(await page.evaluate(()=>(window as unknown as {mountCalls:unknown[]}).mountCalls.length)).toBe(0);
  await page.screenshot({path:'../.local/mount-family-investigation-20261004/mount-example-zoom.png'});
});
test('matched mount portrait loads in the row and can be enlarged without registering an animal',async({page})=>{
  await fixture(page,false,false,false,true);await expect(page.locator('.mount-row').filter({hasText:'Schwarzbär'}).locator('img')).toBeVisible();
  await page.locator('.mount-row').filter({hasText:'Schwarzbär'}).click();await expect(page.locator('.mount-portrait img')).toBeVisible();
  await page.getByRole('button',{name:'Reittierbild vergrößern'}).click();const dialog=page.getByRole('dialog',{name:'Bildvorschau: Schwarzbär'});
  await expect(dialog.locator('img')).toBeVisible();await expect(dialog).toContainText('Originales Tierporträt');
  await page.keyboard.press('Escape');await expect(dialog).toHaveCount(0);
  expect(await page.evaluate(()=>(window as unknown as {mountCalls:unknown[]}).mountCalls.length)).toBe(0);
});
test('mount symbol can be enlarged and regex search can recover from an invalid pattern',async({page})=>{
  await fixture(page);await page.locator('.mount-row').filter({hasText:'Schwarzbär'}).click();
  await page.getByRole('button',{name:'Reittierbild vergrößern'}).click();
  const dialog=page.getByRole('dialog',{name:'Bildvorschau: Schwarzbär'});await expect(dialog).toBeVisible();await expect(dialog).toContainText('Allgemeines Tiersymbol');
  await page.getByRole('button',{name:'Bildvorschau schließen'}).click();await expect(dialog).toHaveCount(0);
  await page.getByLabel('Regex-Suche',{exact:true}).check();const search=page.getByLabel('Reittiere durchsuchen');await search.fill('^Rokade$');
  await expect(page.locator('.mount-row')).toHaveCount(1);await expect(page.locator('.mount-row')).toContainText('Rokade');
  await search.fill('[');await expect(page.getByRole('alert')).toContainText('Ungültiges Regexmuster');
  await search.fill('bär');await expect(page.locator('.mount-row')).toHaveCount(1);await expect(page.locator('.mount-row')).toContainText('Schwarzbär');
  await page.screenshot({path:'../.local/mount-family-investigation-20261004/mount-picker-updated.png'});
  expect(await page.evaluate(()=>(window as unknown as {mountCalls:unknown[]}).mountCalls.length)).toBe(0);
});
test('running game and already owned mount disable writes',async({page})=>{
  await fixture(page,true);await page.locator('.mount-row').filter({hasText:'Schwarzbär'}).click();
  await expect(page.getByRole('button',{name:'Im Stall registrieren',exact:true})).toBeDisabled();
  await page.getByRole('button',{name:'Pferde',exact:true}).click();await expect(page.locator('.mount-row')).toHaveCount(1);
  await page.locator('.mount-row').click();await expect(page.locator('.picker-detail')).toContainText('Bereits registriert.');
  expect(await page.evaluate(()=>(window as unknown as {mountCalls:unknown[]}).mountCalls.length)).toBe(0);
});
test('lost reply keeps UUID and explicit retry reuses the exact request',async({page})=>{
  await fixture(page,false,true);await page.locator('.mount-row').filter({hasText:'Schwarzbär'}).click();
  await page.getByRole('button',{name:'Im Stall registrieren',exact:true}).click();await expect(page.getByRole('alert')).toContainText('Antwort unterbrochen');
  await page.getByRole('button',{name:'Anfrage prüfen / fortsetzen',exact:true}).click();await expect(page.locator('.mount-success')).toContainText('Schwarzbär im Stall registriert');
  const calls=await page.evaluate(()=>(window as unknown as {mountCalls:{args:{request:unknown}}[]}).mountCalls);
  expect(calls).toHaveLength(2);expect(calls[0].args.request).toEqual(calls[1].args.request);
});
test('no donor in an older account explains the cause and switching to latest exposes available mounts',async({page})=>{
  await fixture(page,false,false,true);
  await expect(page.locator('.mount-status')).toContainText('0 hinzufügbar');
  await expect(page.locator('.mount-status')).toContainText('Prüfe oben Slot und Konto');
  await page.getByRole('button',{name:'Neuesten Spielstand auswählen'}).click();
  await expect(page.getByLabel('Spielstand für Reittiere')).toHaveValue('123/slot2');
  await expect(page.locator('.mount-status')).toContainText('1 hinzufügbar');
  await expect(page.getByRole('button',{name:'Bären',exact:true})).toContainText('(1)');
  await page.getByLabel('Nur hinzufügbare Reittiere (1)',{exact:true}).check();
  await expect(page.locator('.mount-row')).toHaveCount(1);
  await page.getByRole('button',{name:'Pferde',exact:true}).click();
  await expect(page.locator('.mount-list')).toContainText('Keine hinzufügbaren Reittiere für diesen Filter');
  expect(await page.evaluate(()=>(window as unknown as {mountCalls:unknown[]}).mountCalls.length)).toBe(0);
});

test('exotic mount base entry is selectable and distinguishes copied stats from pending game behavior',async({page})=>{
  await fixture(page,false,false,false,false,false,true);
  await page.getByLabel('Nur hinzufügbare Reittiere (1)',{exact:true}).check();
  await expect(page.locator('.mount-row')).toHaveCount(1);
  await expect(page.locator('.mount-row')).toContainText('Basiseintrag · Spieltest offen');
  await page.locator('.mount-row').click();
  await expect(page.locator('.picker-detail')).toContainText('Lebenspunkte und Levelwerte eines anderen Tiers werden nicht kopiert');
  await expect(page.locator('.picker-detail')).toContainText('Funktion im Spiel ist noch unbestätigt');
  await expect(page.locator('.picker-detail')).not.toContainText('Reitausrüstung der vorhandenen Vorlage werden übernommen');
  await page.getByRole('button',{name:'Im Stall registrieren',exact:true}).click();
  await expect(page.locator('.mount-success')).toContainText('Herbeirufen und Reiten im Spiel prüfen');
  const calls=await page.evaluate(()=>(window as unknown as {mountCalls:{args:{request:{key:number}}}[]}).mountCalls);
  expect(calls).toHaveLength(1);expect(calls[0].args.request.key).toBe(1000265);
  await page.screenshot({path:'../.local/mount-family-investigation-20261004/base-picker.png'});
});
