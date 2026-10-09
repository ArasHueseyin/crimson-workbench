import {test,expect,type Page} from '@playwright/test';
async function fixture(page:Page,repairEntries=0){
 await page.addInitScript(({repairEntries})=>{
  const root=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;advancedCalls:{command:string;args:Record<string,unknown>}[]};root.isTauri=true;root.advancedCalls=[];
  const items=[{key:2200,name:'Testpfeil',entries:0},{key:2201,name:'Testschwert',entries:0}];
  const field=(name:string,value:string,rule='numeric',min='0',max='1000000')=>({name,value,rule,min,max});
  const record=(table:string,key:number,name:string,module:string,fields:ReturnType<typeof field>[])=>({table,key,name,module,fields,display:'',description:'',category:'climbing'});
  root.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>)=>{
   root.advancedCalls.push({command,args});
   if(command==='bootstrap')return{project:'C:/Fixture',discovery:{installations:[{path:'C:/Fixture Game',platform:'steam'}],configured_game:'C:/Fixture Game'},languages:[{language:'ger'}]};
   if(command==='open_catalog')return{session:1,game_path:args.game,info:{item_count:0,types:[],categories:[],tiers:[],stats:[],index:{language:'ger',path:'test',fingerprint:'test'},exe_version:'test'}};
   if(command==='search_items')return{items:[],total:0,offset:0};
   if(command==='mod_info')return{vendors:[],dropsets:[],store_rows:0,stock_rows:0,opaque_stores:0,opaque_dropsets:0,quantity_excluded:0,trust_rows:0,limitations:[],items,append_vendors:[],daily_vendors:[],chance_dropsets:[],guarantee_dropsets:[],advanced:{repair_items:repairEntries,stack_safe_items:1,finite_endurance_items:2,statuses:[{key:1000000,name:'Hp',entries:0}],categories:[{key:1,name:'Kategorie 1',entries:1}],limitations:['Synthetischer Test'],records:[{...record('buffinfo',1000130,'BuffLevel_Passive_Myurdin_Sword','resource',[field('buffs[1].payload.resources[0].varyStatAmount','-5000','spirit','-1000000000','1000000000')]),category:'equipment'},record('characterinfo',1000799,'Riding_Dragon_1','mount',[field('callMercenaryCoolTime','3600','mount_cooldown')]),record('inventory',2,'Character','inventory',[field('defaultSlotCount','50','slots','1','1460'),field('maxSlotCount','240','slots','1','1460')]),...[13,15,16,17,18,19].map(key=>record('inventory',key,key===13?'Kuku':`Housing_${key}`,'inventory',[field('defaultSlotCount','10','slots','1','1460'),field('maxSlotCount','1000','slots','1','1460')])),record('factionreblockadinginfo',16960,'FactionReblockading_Her_Node_Her_PervinFort','reoccupation',[field('delayTime','86400','reoccupation_delay','1','31536000')]),record('skill',77,'Skill_Climb','skill',[field('cooltime','10','skill_cooldown'),field('resources[0].stat[1000026]','-10000','stamina','-1000000000','1000000000')])]}};
   if(command==='advanced_item'){if(args.key===2200)await new Promise(r=>setTimeout(r,200));return{item:{key:args.key,category:1,equip_type:args.key===2200?0:8,stack_size:'1',stack_risk:args.key!==2200,max_endurance:args.key===2200?0:100,repair_entries:repairEntries,enchant_levels:[0,1],can_copy_enchants:true,stats:[],buffs:[]},compatible_buffs:[[12,2]]};}
   if(command==='advanced_skill')return{key:args.key,name:'Skill_Climb',byte_len:2,matrix_start:0,matrix_end:2,level_counts:[1],buffs:[{matrix_level:0,index:0,type_id:17,name:'VoidPassiveBuffData',opaque_layout:false,fields:[{path:'common.mem_24',offset:0,bytes:8,kind:'signed integer',value:'-300',editable:true}]},{matrix_level:0,index:1,type_id:10,name:'SummonBuffData',opaque_layout:false,fields:[{path:'payload.summon.characterKey',offset:10,bytes:4,kind:'unsigned integer',value:'1002494',editable:false},{path:'payload.summon.spawnPercent',offset:14,bytes:8,kind:'unsigned integer',value:'1000000',editable:false}]}],record_fields:[{path:'skillGroupKey',offset:2,bytes:4,kind:'reference',value:'900',editable:false},{path:'useResourceStatList[0].varyStatAmount',offset:10,bytes:8,kind:'signed integer',value:'-10000',editable:false},{path:'useResourceStatList[0].increaseStatusInfo',offset:18,bytes:4,kind:'reference',value:'1000027',editable:false}],raw_hex:'abcd'};
   if(command==='mod_preview'){const q=(args.request as {advanced?:{free_repair?:boolean;items?:Record<string,{free_repair?:boolean}>}}).advanced;if(q?.free_repair||Object.values(q?.items??{}).some(i=>i.free_repair))throw {code:'invalid_input',message:'Kostenfreie Reparatur ist nicht verfügbar.'};}
   if(command==='mod_preview')return{request:JSON.parse(JSON.stringify(args.request,(_k,v)=>v&&typeof v==='object'&&!Array.isArray(v)?Object.fromEntries(Object.entries(v).sort(([a],[b])=>a.localeCompare(b))):v)),plan_id:'advanced-plan',fingerprint:'test',changes:[{module:'dragon',table:'characterinfo',key:1000799,name:'Riding_Dragon_1',field:'cooldown',before:'3600',after:'0'}],files:[],warnings:[],gates:{game_running:true,can_apply:false,reasons:['Spiel läuft']},credits:'fixture'};
   throw {code:'fixture',message:`Unbenutzter Testbefehl ${command}`};
  }};
 },{repairEntries});
 await page.goto('/');await expect(page.locator('.workbench')).toHaveAttribute('data-ready','true');await page.getByRole('button',{name:'Modwerkstatt',exact:true}).click();await expect(page.getByLabel('Spawn-Menge in Prozent')).toBeVisible();
}
async function open(page:Page,title:string){await page.locator('.advanced-mods summary').filter({hasText:title}).click()}
async function request(page:Page){return page.evaluate(()=>(window as unknown as {advancedCalls:{command:string;args:{request:unknown}}[]}).advancedCalls.filter(c=>c.command==='mod_preview').at(-1)?.args.request)}
test('advanced settings flow to one reviewed plan and canonical key order keeps it current',async({page})=>{
 await fixture(page);await page.getByLabel('Spawn-Menge in Prozent').fill('300');await page.getByLabel('Reittiere in Städten freigeben').check();await open(page,'B5 · Blackstar');await page.getByLabel('Blackstar ohne Cooldown').check();await page.getByRole('button',{name:'30 Minuten',exact:true}).click();
 await open(page,'B6 · Inventar');await page.locator('section').filter({has:page.getByRole('heading',{name:'Character · 2',exact:true})}).getByLabel('defaultSlotCount überschreiben').check();await page.getByLabel('2: defaultSlotCount',{exact:true}).fill('300');await page.locator('section').filter({has:page.getByRole('heading',{name:'Character · 2',exact:true})}).getByLabel('maxSlotCount überschreiben').check();await page.getByLabel('2: maxSlotCount',{exact:true}).fill('300');
 await open(page,'B9 · Ausdauer');await page.getByLabel('Klettern: Ausdauer in Prozent').fill('0');await page.getByRole('button',{name:'Vorschau berechnen'}).click();await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toBeEnabled();
 expect(await request(page)).toMatchObject({advanced:{spawn_percent:300,town_running:true,dragon_no_cooldown:true,dragon_duration:1800,cost_percent:{'stamina:climbing':0},fields:{'inventory/2/defaultSlotCount':'300','inventory/2/maxSlotCount':'300'}}});
 await page.getByLabel('Spawn-Menge in Prozent').fill('200');await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toBeDisabled();
});
test('invalid numeric input cannot generate a plan; unsupported repair is disabled',async({page})=>{
 await fixture(page);await page.getByLabel('Spawn-Menge in Prozent').fill('');await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect(await request(page)).toBeUndefined();await page.getByLabel('Spawn-Menge in Prozent').fill('10001');await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect(await request(page)).toBeUndefined();
 await open(page,'B8 · Haltbarkeit');await expect(page.getByLabel('Reparaturkosten auf 0')).toBeDisabled();await expect(page.locator('.advanced-mods')).toContainText('keine Reparaturregeln');
});
test('skill search and explicit overrides preserve signed resource values',async({page})=>{
 await fixture(page);await open(page,'B10 · Skill');await page.getByLabel('Skills suchen').fill('77');await page.getByLabel('Skills auswählen').selectOption('skill/77');await page.getByLabel('Skill 77: Cooldown in Prozent ändern').check();await page.getByLabel('Skill 77: Cooldown in Prozent',{exact:true}).fill('50');await page.getByLabel('resources[0].stat[1000026] überschreiben',{exact:true}).check();await page.getByLabel('77: resources[0].stat[1000026]',{exact:true}).fill('-500');await page.getByRole('button',{name:'Vorschau berechnen'}).click();
 expect(await request(page)).toMatchObject({advanced:{skill_cooldowns:{77:50},fields:{'skill/77/resources[0].stat[1000026]':'-500'}}});
});
test('item templates survive reload, are reusable and remain separate from live apply',async({page})=>{
 await fixture(page);await open(page,'B11 · Item');await page.getByLabel('Editor-Item auswählen').selectOption('2200');await expect(page.getByLabel('Item-Stat',{exact:true})).toBeVisible();await page.getByLabel('Item-Stat',{exact:true}).selectOption('1000000');await page.getByLabel('Item-Stat-Wert').fill('123');await page.getByRole('button',{name:'Stat setzen',exact:true}).click();await page.getByLabel('Item-Buff',{exact:true}).selectOption('12:2');await page.getByRole('button',{name:'Buff setzen',exact:true}).click();await page.getByLabel('Item-Vorlagenname').fill('Mein Item');await page.getByRole('button',{name:'Item-Vorlage speichern',exact:true}).click();
 await page.reload();await expect(page.locator('.workbench')).toHaveAttribute('data-ready','true');await page.getByRole('button',{name:'Modwerkstatt',exact:true}).click();await open(page,'B11 · Item');await page.getByLabel('Editor-Item auswählen').selectOption('2201');await expect(page.getByLabel('Item-Stat',{exact:true})).toBeVisible();await page.getByLabel('Item-Vorlage',{exact:true}).selectOption('Mein Item');await page.getByRole('button',{name:'Vorlage auf Item übernehmen'}).click();await page.getByRole('button',{name:'Vorschau berechnen'}).click();
 expect(await request(page)).toMatchObject({advanced:{items:{2201:{stats:[{enchant_level:0,list:'static',stat:1000000,value:'123'}],buffs:[{enchant_level:0,buff:12,level:2}]}}}});
 const commands=await page.evaluate(()=>(window as unknown as {advancedCalls:{command:string}[]}).advancedCalls.map(c=>c.command));expect(commands).not.toContain('live_start');
});
test('late item responses cannot replace a new selection and narrow view has no overflow',async({page})=>{
 await fixture(page);await open(page,'B11 · Item');await page.getByLabel('Editor-Item auswählen').selectOption('2200');await page.getByLabel('Editor-Item auswählen').selectOption('2201');await expect(page.locator('.advanced-item')).toContainText('Equip-Typ 8');await page.waitForTimeout(250);await expect(page.locator('.advanced-item')).toContainText('Equip-Typ 8');
 await page.setViewportSize({width:1024,height:768});expect(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth)).toBe(true);
});
test('new enchant levels accept stats, persist in templates, and remove dependent edits',async({page})=>{
 await fixture(page);await open(page,'B11 · Item');await page.getByLabel('Editor-Item auswählen').selectOption('2201');
 await page.getByText('Enchant-Stufe hinzufügen',{exact:true}).click();await page.getByLabel('Neue Enchant-Stufe',{exact:true}).fill('1');await expect(page.getByRole('button',{name:'Enchant-Stufe vormerken'})).toBeDisabled();
 await page.getByLabel('Neue Enchant-Stufe',{exact:true}).fill('5');await page.getByRole('button',{name:'Enchant-Stufe vormerken'}).click();await expect(page.getByLabel('Item-Enchant-Stufe')).toHaveValue('5');
 await page.getByLabel('Item-Stat',{exact:true}).selectOption('1000000');await page.getByRole('button',{name:'Stat setzen',exact:true}).click();await page.getByLabel('Item-Vorlagenname').fill('Enchant 5');await page.getByRole('button',{name:'Item-Vorlage speichern'}).click();
 await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect(await request(page)).toMatchObject({advanced:{items:{2201:{enchant_copies:[{source_level:0,target_level:5}],stats:[{enchant_level:5,value:'100'}]}}}});
 await page.getByRole('button',{name:'Enchant-Stufe entfernen'}).click();await expect(page.getByLabel('Item-Enchant-Stufe')).toHaveValue('0');await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect(await request(page)).toMatchObject({advanced:{items:{2201:{stats:[],buffs:[]}}}});
 await page.getByLabel('Item-Vorlage',{exact:true}).selectOption('Enchant 5');await page.getByRole('button',{name:'Vorlage auf Item übernehmen'}).click();await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect(await request(page)).toMatchObject({advanced:{items:{2201:{enchant_copies:[{target_level:5}],stats:[{enchant_level:5}]}}}});
});
test('skill matrices display raw values and all original bytes',async({page})=>{
 await fixture(page);await open(page,'B10 · Skill');await page.getByLabel('Skills auswählen').selectOption('skill/77');
 await page.locator('.skill-details>summary').click();await expect(page.locator('.skill-details')).toContainText('schreibgeschützt');await page.getByLabel('Skill-Buffs filtern').fill('VoidPassive');await page.locator('.skill-buffs summary').filter({hasText:'VoidPassive'}).click();await expect(page.locator('.skill-buffs')).toContainText('-300');
 await page.getByText('Alle Originalbytes anzeigen',{exact:true}).click();await expect(page.locator('.skill-details pre')).toContainText('ab cd');expect(await request(page)).toBeUndefined();
});

test('buff overrides combine with cooldowns, invalidate previews and clear without stale optional maps',async({page})=>{
 await fixture(page);await open(page,'B10 · Skill');await page.getByLabel('Skills auswählen').selectOption('skill/77');await page.locator('.skill-details>summary').click();await page.locator('.skill-buffs summary').filter({hasText:'VoidPassive'}).click();
 const label='Buff 0/0: common.mem_24';await expect(page.getByLabel(label,{exact:true})).toBeDisabled();await page.getByLabel(`${label} ändern`,{exact:true}).check();await page.getByLabel(label,{exact:true}).fill('-123');await page.getByLabel('Skill-Cooldowns global in Prozent').fill('50');
 await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect(await request(page)).toMatchObject({advanced:{skill_cooldown_percent:50,skill_buffs:{77:{'0/0/common.mem_24':'-123'}}}});await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toBeEnabled();
 await page.getByLabel(label,{exact:true}).fill('1000000001');await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toBeDisabled();const last=await request(page);await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect(await request(page)).toEqual(last);
 await page.getByLabel(`${label} ändern`,{exact:true}).uncheck();await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect((await request(page) as {advanced:{skill_buffs?:unknown}}).advanced.skill_buffs).toBeUndefined();await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toBeEnabled();
});

test('patrol reset factor is optional and uses a distinct request field',async({page})=>{
 await fixture(page);await page.getByLabel('Patrouillen-Reset in Prozent ändern').check();await page.getByLabel('Patrouillen-Reset in Prozent',{exact:true}).fill('50');await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect(await request(page)).toMatchObject({advanced:{patrol_reset_percent:50,spawn_percent:100}});
 await page.getByLabel('Patrouillen-Reset in Prozent ändern').uncheck();await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect((await request(page) as {advanced:{patrol_reset_percent?:number}}).advanced.patrol_reset_percent).toBeUndefined();
});

test('reoccupation changes are optional, separately scoped and support per-area overrides',async({page})=>{
 await fixture(page);const label='Wiederbesetzung: Wartezeit in Prozent';await expect(page.getByLabel(label,{exact:true})).toBeDisabled();await page.getByLabel(`${label} ändern`).check();await page.getByLabel(label,{exact:true}).fill('50');
 await page.getByLabel('Welt-Datensätze suchen').fill('Pervin');await page.getByLabel('Welt-Datensätze auswählen').selectOption('factionreblockadinginfo/16960');await page.getByLabel('delayTime überschreiben').check();await page.getByLabel('16960: delayTime',{exact:true}).fill('12345');
 await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect(await request(page)).toMatchObject({advanced:{reoccupation_delay_percent:50,spawn_percent:100,fields:{'factionreblockadinginfo/16960/delayTime':'12345'}}});expect((await request(page) as {advanced:{patrol_reset_percent?:number}}).advanced.patrol_reset_percent).toBeUndefined();
 await page.getByLabel(label,{exact:true}).fill('0');await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toBeDisabled();const last=await request(page);await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect(await request(page)).toEqual(last);
 await page.getByLabel(`${label} ändern`).uncheck();await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect((await request(page) as {advanced:{reoccupation_delay_percent?:number}}).advanced.reoccupation_delay_percent).toBeUndefined();expect(await request(page)).toMatchObject({advanced:{fields:{'factionreblockadinginfo/16960/delayTime':'12345'}}});
 await page.getByRole('button',{name:'B4–B11 zurücksetzen'}).click();await expect(page.getByLabel('delayTime überschreiben')).not.toBeChecked();await expect(page.getByLabel(label,{exact:true})).toBeDisabled();
});

test('inventory inputs enforce the observed 1460 ceiling before generating a plan',async({page})=>{
 await fixture(page);await open(page,'B6 · Inventar');await page.locator('section').filter({has:page.getByRole('heading',{name:'Character · 2',exact:true})}).getByLabel('maxSlotCount überschreiben').check();const max=page.getByLabel('2: maxSlotCount',{exact:true});await expect(max).toHaveAttribute('max','1460');await max.fill('1461');await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect(await request(page)).toBeUndefined();
 await max.fill('1460');await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect(await request(page)).toMatchObject({advanced:{fields:{'inventory/2/maxSlotCount':'1460'}}});await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toBeEnabled();
});


test('named summon fields are searchable and stay read-only beside editable common values',async({page})=>{
 await fixture(page);await open(page,'B10 · Skill');await page.getByLabel('Skills auswählen').selectOption('skill/77');await page.locator('.skill-details>summary').click();
 await page.getByLabel('Skill-Buffs filtern').fill('characterKey');await expect(page.locator('.skill-buffs summary')).toHaveCount(1);await expect(page.locator('.skill-buffs summary')).toContainText('SummonBuffData');await page.locator('.skill-buffs summary').click();
 await expect(page.locator('.skill-buffs')).toContainText('1002494');await expect(page.locator('.skill-buffs input')).toHaveCount(0);await expect(page.locator('.skill-buffs')).not.toContainText('nur als Rohblock');
 await page.getByLabel('Skill-Buffs filtern').fill('SPAWNPERCENT');await expect(page.locator('.skill-buffs summary')).toHaveCount(1);await page.getByLabel('Skill-Buffs filtern').fill('missingField');await expect(page.locator('.skill-buffs summary')).toHaveCount(0);
 await page.getByLabel('Skill-Buffs filtern').fill('');await expect(page.locator('.skill-buffs summary')).toHaveCount(2);await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect((await request(page) as {advanced:{skill_buffs?:unknown}}).advanced?.skill_buffs).toBeUndefined();
});


test('repair controls stay unavailable with nonempty metadata; legacy templates can be corrected',async({page})=>{
 await page.addInitScript(()=>localStorage.setItem('crimson-item-templates-v1',JSON.stringify([{name:'Alte Reparaturvorlage',edit:{stack_size:1,free_repair:true,stats:[{enchant_level:0,list:'static',stat:1000000,value:'123'}],buffs:[]}}])));
 await fixture(page,1);await open(page,'B8 · Haltbarkeit');await expect(page.getByLabel('Reparaturkosten auf 0')).toBeDisabled();
 await open(page,'B11 · Item');await page.getByLabel('Editor-Item auswählen').selectOption('2201');await expect(page.getByLabel('Dieses Item kostenlos reparieren')).toBeDisabled();
 await page.getByLabel('Item-Vorlage',{exact:true}).selectOption('Alte Reparaturvorlage');await page.getByRole('button',{name:'Vorlage auf Item übernehmen'}).click();await expect(page.getByLabel('Dieses Item kostenlos reparieren')).toBeChecked();await expect(page.getByLabel('Dieses Item kostenlos reparieren')).toBeEnabled();await expect(page.locator('.advanced-item')).toContainText('Entferne das Häkchen');
 await page.getByRole('button',{name:'Vorschau berechnen'}).click();await expect(page.getByRole('alert').filter({hasText:'Kostenfreie Reparatur ist nicht verfügbar'})).toBeVisible();await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toHaveCount(0);
 await page.getByLabel('Dieses Item kostenlos reparieren').uncheck();await expect(page.getByLabel('Dieses Item kostenlos reparieren')).toBeDisabled();await page.getByRole('button',{name:'Vorschau berechnen'}).click();await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toBeEnabled();expect(await request(page)).toMatchObject({advanced:{items:{2201:{free_repair:false,stack_size:1,stats:[{enchant_level:0,value:'123'}]}}}});
});


test('named skill basis fields are searchable and read-only without changing pending edits',async({page})=>{
 await fixture(page);await open(page,'B10 · Skill');await page.getByLabel('Skills auswählen').selectOption('skill/77');
 await page.getByLabel('resources[0].stat[1000026] überschreiben',{exact:true}).check();await page.getByLabel('77: resources[0].stat[1000026]',{exact:true}).fill('-500');
 await page.getByRole('button',{name:'Vorschau berechnen'}).click();const before=await request(page);
 await page.locator('.skill-details>summary').click();await page.locator('.skill-record-fields>summary').click();
 await expect(page.locator('.skill-record-fields')).toContainText('3 von 3 Feldern');await expect(page.locator('.skill-record-fields dl')).toContainText('skillGroupKey');
 await page.getByLabel('Skill-Basisfelder filtern').fill('1000027');await expect(page.locator('.skill-record-fields dl>div')).toHaveCount(1);await expect(page.locator('.skill-record-fields dl')).toContainText('increaseStatusInfo');
 await expect(page.locator('.skill-record-fields dl input')).toHaveCount(0);await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toBeEnabled();expect(await request(page)).toEqual(before);
 await page.getByLabel('Skill-Basisfelder filtern').fill('varyStatAmount');await expect(page.locator('.skill-record-fields dl')).toContainText('-10000');await page.setViewportSize({width:1024,height:768});expect(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth)).toBe(true);
 await page.getByLabel('Skill-Basisfelder filtern').fill('missing-field');await expect(page.locator('.skill-record-fields')).toContainText('Keine passenden Basisfelder.');
});

test('all six additional storage controls compose with no durability loss and reset together',async({page})=>{
 await fixture(page);await open(page,'B6 · Inventar');
 for(const key of [13,15,16,17,18,19]){
  const section=page.locator('.mod-card section').filter({has:page.getByRole('heading',{name:key===13?'Kuku · 13':`Housing_${key} · ${key}`,exact:true})});
  await section.getByLabel('defaultSlotCount überschreiben',{exact:true}).check();await section.getByLabel(`${key}: defaultSlotCount`,{exact:true}).fill('900');
  await section.getByLabel('maxSlotCount überschreiben',{exact:true}).check();await section.getByLabel(`${key}: maxSlotCount`,{exact:true}).fill('1200');
 }
 await open(page,'B8 · Haltbarkeit');await expect(page.locator('.advanced-mods')).toContainText('2 Items mit endlicher Haltbarkeit');await page.getByLabel('Kein Haltbarkeitsverlust').check();
 await page.getByRole('button',{name:'Vorschau berechnen'}).click();const q=(await request(page)) as {advanced:{no_wear:boolean;fields:Record<string,string>}};
 expect(q.advanced.no_wear).toBe(true);for(const key of [13,15,16,17,18,19]){expect(q.advanced.fields[`inventory/${key}/defaultSlotCount`]).toBe('900');expect(q.advanced.fields[`inventory/${key}/maxSlotCount`]).toBe('1200')}
 await page.getByRole('button',{name:'B4–B11 zurücksetzen'}).click();await expect(page.getByLabel('Kein Haltbarkeitsverlust')).not.toBeChecked();await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toBeDisabled();
 await page.getByRole('button',{name:'Vorschau berechnen'}).click();expect((await request(page) as {advanced?:unknown}).advanced).toBeUndefined();
});


test('equipment cost factors and per-buff exceptions are previewed and reset together',async({page})=>{
 await fixture(page);await open(page,'B9 · Ausdauer');
 await page.getByRole('button',{name:'Alle erfassten Geistkosten auf 0',exact:true}).click();
 await expect(page.getByLabel('Ausrüstungs-Buffs: Geist in Prozent')).toHaveValue('0');
 await expect(page.getByLabel('Ausrüstungs-Buffs: Ausdauer in Prozent')).toHaveValue('100');
 await page.getByLabel('Ausrüstungs-Buffs: Ausdauer in Prozent').fill('50');
 await page.getByLabel('Buff-Eigenkosten suchen').fill('Myurdin');await page.getByLabel('Buff-Eigenkosten auswählen').selectOption('buffinfo/1000130');
 const field='buffs[1].payload.resources[0].varyStatAmount';await page.getByLabel(`${field} überschreiben`,{exact:true}).check();await page.getByLabel(`1000130: ${field}`,{exact:true}).fill('-123');
 await page.getByRole('button',{name:'Vorschau berechnen'}).click();
 expect(await request(page)).toMatchObject({advanced:{cost_percent:{'spirit:equipment':0,'stamina:equipment':50,'spirit:combat':0,'spirit:other':0},fields:{'buffinfo/1000130/buffs[1].payload.resources[0].varyStatAmount':'-123'}}});
 await page.setViewportSize({width:1024,height:768});expect(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth)).toBe(true);
 await page.getByLabel('Ausrüstungs-Buffs: Geist in Prozent').fill('200');await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toBeDisabled();
 await page.getByRole('button',{name:'B4–B11 zurücksetzen'}).click();await expect(page.getByLabel('Ausrüstungs-Buffs: Geist in Prozent')).toHaveValue('100');await expect(page.getByLabel(`${field} überschreiben`,{exact:true})).not.toBeChecked();
});
