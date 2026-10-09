// Drives only our explicitly launched hidden Workbench instance, never the game.
import {chromium,expect} from '@playwright/test';
import {readFile,writeFile,mkdir,access} from 'node:fs/promises';
import {resolve,join} from 'node:path';
import {createHash} from 'node:crypto';
import {createReadStream} from 'node:fs';
const output=resolve('../.local/phase5-build25455892-native');await mkdir(output,{recursive:true});
const game='C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert';
const digest=b=>createHash('sha256').update(b).digest('hex');
const observed=JSON.parse((await readFile(resolve('../docs/builds/steam-25455892.observed.json'),'utf8')).replace(/^\uFEFF/,''));
const audit=resolve('../exports/installation-audit-1789903405-11284-1-0.json');
const beforeAudit=digest(await readFile(audit));
const metadata=async()=>{const out={};for(const m of observed.files){const hash=createHash('sha256');for await(const chunk of createReadStream(join(game,m.path)))hash.update(chunk);out[m.path]=hash.digest('hex');}return out;};
const before=await metadata();
for(const file of observed.files)if(before[file.path]!==file.sha256)throw Error(`Unexpected source build: ${file.path}`);
let browser;for(let i=0;i<30;i++){try{browser=await chromium.connectOverCDP('http://127.0.0.1:9225',{timeout:1500});break;}catch{await new Promise(r=>setTimeout(r,500));}}
if(!browser)throw Error('Hidden WebView unavailable');
try{
 const page=browser.contexts().flatMap(c=>c.pages()).find(p=>!p.url().startsWith('devtools:'));if(!page)throw Error('No hidden page');const errors=[];page.on('pageerror',e=>errors.push(e.message));
 await page.locator('.workbench[data-ready="true"]').waitFor({timeout:90000});await expect(page.locator('.version-line')).toContainText('v0.5.9 preview');await page.getByRole('button',{name:'Modwerkstatt',exact:true}).click();await expect(page.getByLabel('Spawn-Menge in Prozent')).toBeVisible({timeout:90000});
 const info=await page.evaluate(()=>window.__TAURI_INTERNALS__.invoke('mod_info',{session:1}));
 const advanced=info.advanced;if(advanced.records.filter(r=>r.table==='skill').length!==2069)throw Error('Skill coverage');if(advanced.repair_items!==0)throw Error('Repair coverage changed');
 await page.evaluate(async()=>{try{await window.__TAURI_INTERNALS__.invoke('advanced_item',{session:0,key:2200});throw Error('stale session accepted')}catch(e){if(e.code!=='stale_session')throw e}});
 await page.evaluate(async()=>{try{await window.__TAURI_INTERNALS__.invoke('advanced_skill',{session:0,key:30001});throw Error('stale skill session accepted')}catch(e){if(e.code!=='stale_session')throw e}});
 const open=async(text)=>page.locator('.advanced-mods summary').filter({hasText:text}).click();
 await page.getByLabel('Spawn-Menge in Prozent').fill('300');await page.getByLabel('Reittiere in Städten freigeben').check();
 await page.getByLabel('Patrouillen-Reset in Prozent ändern').check();await page.getByLabel('Patrouillen-Reset in Prozent',{exact:true}).fill('50');
 await page.getByLabel('Wiederbesetzung: Wartezeit in Prozent ändern').check();await page.getByLabel('Wiederbesetzung: Wartezeit in Prozent',{exact:true}).fill('50');
 if(advanced.records.filter(r=>r.table==='factionreblockadinginfo').length!==108)throw Error('Reoccupation coverage');
 if(advanced.records.filter(r=>r.module==='inventory').some(r=>r.fields.some(f=>f.max!=='1460')))throw Error('Inventory ceiling');
 await page.getByLabel('Welt-Datensätze suchen').fill('PervinFort');await page.getByLabel('Welt-Datensätze auswählen').selectOption('factionreblockadinginfo/16960');await page.getByLabel('delayTime überschreiben').check();await page.getByLabel('16960: delayTime',{exact:true}).fill('12345');
 await page.setViewportSize({width:1024,height:768});await page.getByLabel('Wiederbesetzung: Wartezeit in Prozent',{exact:true}).scrollIntoViewIfNeeded();if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth))throw Error('World controls overflow');await page.screenshot({path:resolve(output,'world-reoccupation-1024.png')});await page.setViewportSize({width:1440,height:920});
 await open('B5 · Blackstar');await page.getByLabel('Blackstar ohne Cooldown').check();await page.getByLabel('Flugbeschränkungen an Regionen und Städten aufheben').check();await page.getByRole('button',{name:'30 Minuten',exact:true}).click();
 await open('B6 · Inventar');const slots=page.locator('.advanced-mods .mod-card').filter({has:page.locator('summary').filter({hasText:'B6 · Inventar'})}).locator('section').filter({has:page.getByRole('heading',{name:'Character · 2',exact:true})});
 await slots.getByLabel('defaultSlotCount überschreiben',{exact:true}).check();await slots.getByLabel('2: defaultSlotCount',{exact:true}).fill('300');await slots.getByLabel('maxSlotCount überschreiben',{exact:true}).check();await slots.getByLabel('2: maxSlotCount',{exact:true}).fill('300');
 if(advanced.records.filter(r=>r.module==='inventory').length!==9)throw Error('Storage coverage');
 for(const key of [13,15,16,17,18,19]){
  const section=page.locator('.mod-card section').filter({has:page.getByRole('heading',{name:advanced.records.find(r=>r.table==='inventory'&&r.key===key).name+` · ${key}`,exact:true})});
  await section.getByLabel('defaultSlotCount überschreiben',{exact:true}).check();await section.getByLabel(`${key}: defaultSlotCount`,{exact:true}).fill('900');await section.getByLabel('maxSlotCount überschreiben',{exact:true}).check();await section.getByLabel(`${key}: maxSlotCount`,{exact:true}).fill('1200');
 }
 await page.setViewportSize({width:1024,height:768});await page.getByRole('heading',{name:'Kuku · 13',exact:true}).scrollIntoViewIfNeeded();if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth))throw Error('Storage overflow');await page.screenshot({path:resolve(output,'storage-1024.png')});await page.setViewportSize({width:1440,height:920});
 await open('B7 · Stapel');await page.getByLabel('Globale Stapelgröße ändern').check();await open('B8 · Haltbarkeit');await page.getByLabel('Kein Haltbarkeitsverlust').check();await expect(page.getByLabel('Reparaturkosten auf 0')).toBeDisabled();await expect(page.locator('.advanced-mods')).toContainText('Kostenfreie Reparatur ist nicht verfügbar.');await page.setViewportSize({width:1024,height:768});await page.getByLabel('Reparaturkosten auf 0').scrollIntoViewIfNeeded();await page.screenshot({path:resolve(output,'repair-unavailable-1024.png')});await page.setViewportSize({width:1440,height:920});
 await open('B9 · Ausdauer');await page.getByLabel('Klettern: Ausdauer in Prozent').fill('0');await page.getByLabel('Ausrüstungs-Buffs: Ausdauer in Prozent').fill('0');await page.getByLabel('Ausrüstungs-Buffs: Geist in Prozent').fill('0');await page.getByLabel('Buff-Eigenkosten suchen').fill('Myurdin');await page.getByLabel('Buff-Eigenkosten auswählen').selectOption('buffinfo/1000130');await expect(page.locator('.advanced-mods')).toContainText('Zusätzliche Eigenkosten');await page.setViewportSize({width:1024,height:768});await page.getByLabel('Buff-Eigenkosten auswählen').scrollIntoViewIfNeeded();if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth))throw Error('Buff costs overflow');await page.screenshot({path:resolve(output,'buff-costs-1024.png')});await page.setViewportSize({width:1440,height:920});await open('B10 · Skill');await page.getByLabel('Skill-Cooldowns global in Prozent').fill('50');
 // Inspect all ten newly structured real payloads through the release IPC.
 const structuredPayloads=[];
 for(const key of [75019,65009,65010,65145,41355,41357,41358,91251]){
  const d=await page.evaluate(key=>window.__TAURI_INTERNALS__.invoke('advanced_skill',{session:1,key}),key);
  for(const buff of d.buffs.filter(b=>[10,74].includes(b.type_id))){const fields=buff.fields.filter(f=>f.path.startsWith('payload.'));if(buff.opaque_layout||fields.some(f=>f.editable)||fields.reduce((n,f)=>n+f.bytes,0)!==(buff.type_id===10?181:8))throw Error('Structured payload invariant');structuredPayloads.push({key,type:buff.type_id,fields:fields.length});}
 }
 if(structuredPayloads.filter(b=>b.type===10).length!==9||structuredPayloads.filter(b=>b.type===74).length!==1)throw Error('Structured payload count');
 await page.getByLabel('Skills suchen').fill('75019');await page.getByLabel('Skills auswählen').selectOption('skill/75019');await page.locator('.skill-details>summary').click();await page.getByLabel('Skill-Buffs filtern').fill('characterKey');await expect(page.locator('.skill-buffs summary')).toHaveCount(1);await page.locator('.skill-buffs summary').click();
 const characterField=page.locator('.skill-buffs dl>div').filter({has:page.locator('dt').filter({hasText:'payload.summon.characterKey'})});await expect(characterField).toContainText('1002494');await expect(characterField.locator('input')).toHaveCount(0);
 await page.setViewportSize({width:1024,height:768});await characterField.scrollIntoViewIfNeeded();if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth))throw Error('Structured skill overflow');if(await page.locator('.skill-buffs').evaluate(el=>el.scrollWidth>el.clientWidth))throw Error('Structured field horizontal overflow');await page.screenshot({path:resolve(output,'summon-fields-1024.png')});await page.setViewportSize({width:1440,height:920});
 const resourceSkill=advanced.records.find(r=>r.table==='skill'&&r.category==='climbing'&&r.fields.some(f=>f.rule==='stamina'&&Number(f.value)<0));
 if(!resourceSkill)throw Error('Missing climbing resource skill');
 const resourceDetail=await page.evaluate(key=>window.__TAURI_INTERNALS__.invoke('advanced_skill',{session:1,key}),resourceSkill.key);
 if(resourceDetail.record_fields.some(f=>f.editable)||!resourceDetail.record_fields.some(f=>f.path.endsWith('.varyStatAmount')&&Number(f.value)<0))throw Error('Named resource fields');
 for(const name of ['skillGroupKey','parentSkill','learnKnowledgeInfo','useResourceItemList.count','allowSkillWithLowResource','videoPath'])if(!resourceDetail.record_fields.some(f=>f.path===name))throw Error(`Missing named field ${name}`);
 await page.getByLabel('Skills suchen').fill(String(resourceSkill.key));await page.getByLabel('Skills auswählen').selectOption(`skill/${resourceSkill.key}`);await page.locator('.skill-details>summary').click();await page.locator('.skill-record-fields>summary').click();
 await page.getByLabel('Skill-Basisfelder filtern').fill('useResourceStatList');await expect(page.locator('.skill-record-fields dl')).toContainText('varyStatAmount');await expect(page.locator('.skill-record-fields dl input')).toHaveCount(0);
 await page.setViewportSize({width:1024,height:768});await page.locator('.skill-record-fields').scrollIntoViewIfNeeded();if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth))throw Error('Skill basis page overflow');if(await page.locator('.skill-record-fields dl').evaluate(el=>el.scrollWidth>el.clientWidth))throw Error('Skill basis field overflow');await page.screenshot({path:resolve(output,'skill-basis-1024.png')});await page.setViewportSize({width:1440,height:920});
 await page.getByLabel('Skills suchen').fill('30001');await page.getByLabel('Skills auswählen').selectOption('skill/30001');await expect(page.locator('.advanced-browser').last()).toContainText('Original: 10');
 await page.locator('.skill-details>summary').click();await expect(page.locator('.skill-details')).toContainText('schreibgeschützt');await page.locator('.skill-buffs summary').first().click();await expect(page.locator('.skill-buffs')).toContainText('common.mem_24');
 await page.getByLabel('Buff 0/0: common.mem_24 ändern',{exact:true}).check();await page.getByLabel('Buff 0/0: common.mem_24',{exact:true}).fill('123');
 await page.getByText('Alle Originalbytes anzeigen',{exact:true}).click();await expect(page.locator('.skill-details pre')).not.toBeEmpty();await page.locator('.skill-details').scrollIntoViewIfNeeded();await page.screenshot({path:resolve(output,'skill-matrix.png')});
 await open('B11 · Item');let chosen;
 for(const item of info.items.filter(i=>/schwert|sword/i.test(i.name)).slice(0,100)){const detail=await page.evaluate(key=>window.__TAURI_INTERNALS__.invoke('advanced_item',{session:1,key}),item.key);if(detail.item.equip_type!==0&&detail.item.enchant_levels.length){chosen={...item,detail};break;}}
 if(!chosen)throw Error('No eligible sword found');
 await page.getByLabel('Editor-Item suchen').fill(String(chosen.key));await page.getByLabel('Editor-Item auswählen').selectOption(String(chosen.key));await expect(page.getByLabel('Item-Stat',{exact:true})).toBeVisible();
 const newLevel=Math.max(...chosen.detail.item.enchant_levels)+1;await page.getByText('Enchant-Stufe hinzufügen',{exact:true}).click();await page.getByLabel('Neue Enchant-Stufe',{exact:true}).fill(String(newLevel));await page.getByRole('button',{name:'Enchant-Stufe vormerken'}).click();await expect(page.getByLabel('Item-Enchant-Stufe')).toHaveValue(String(newLevel));
 await page.getByLabel('Item-Stat',{exact:true}).selectOption(String(advanced.statuses[0].key));await page.getByLabel('Item-Stat-Wert').fill('123456');await page.getByRole('button',{name:'Stat setzen',exact:true}).click();
 await page.getByLabel('Item-Vorlagenname').fill('Phase-5-Probe');await page.getByRole('button',{name:'Item-Vorlage speichern',exact:true}).click();await page.getByLabel('Item-Vorlage',{exact:true}).selectOption('Phase-5-Probe');
 await page.setViewportSize({width:1024,height:768});await page.locator('.advanced-item').scrollIntoViewIfNeeded();if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth))throw Error('1024px overflow');await page.screenshot({path:resolve(output,'item-editor-1024.png')});await page.setViewportSize({width:1440,height:920});
 await page.getByRole('button',{name:'Vorschau berechnen'}).click();await expect(page.locator('.mod-preview')).toBeVisible({timeout:90000});await page.getByRole('button',{name:'Vorschau exportieren'}).click();await expect(page.locator('.mod-success').last()).toContainText('nichts angewendet',{timeout:90000});
 const exportPath=await page.locator('.mod-success').last().locator('code').textContent();const plan=JSON.parse(await readFile(exportPath,'utf8'));
 // Direct requests cannot bypass unavailable repair controls or enter the writer.
 for(const scope of ['global','item']){
  const request=structuredClone(plan.request);
  if(scope==='global')request.advanced.free_repair=true;else request.advanced.items[chosen.key].free_repair=true;
  const refusal=await page.evaluate(async request=>{try{await window.__TAURI_INTERNALS__.invoke('mod_preview',{session:1,request});return null}catch(e){return String(e.message??e)}},request);
  if(!refusal?.includes('Kostenfreie Reparatur ist nicht verfügbar'))throw Error(`Repair refusal missing: ${scope}: ${refusal}`);
 }
 const durability=plan.changes.filter(c=>c.table==='iteminfo'&&c.field==='max_endurance');if(durability.length!==advanced.finite_endurance_items||durability.length===0||durability.some(c=>c.before==='0'||c.before==='65535'||c.after!=='65535'))throw Error('Durability item coverage');
 for(const key of [13,15,16,17,18,19])for(const [field,after] of [['defaultSlotCount','900'],['maxSlotCount','1200']])if(!plan.changes.some(c=>c.table==='inventory'&&c.key===key&&c.field===field&&c.after===after))throw Error('Storage diff missing');
 const buffCosts=plan.changes.filter(c=>c.table==='buffinfo');if(buffCosts.length!==36||new Set(buffCosts.map(c=>c.key)).size!==33||buffCosts.some(c=>c.after!=='0'||!c.field.endsWith('.varyStatAmount')))throw Error('Additional buff costs incomplete');
 const ownedRequest=structuredClone(plan.request);ownedRequest.advanced.cost_percent['spirit:other']=0;ownedRequest.advanced.skill_buffs[40013]={'0/0/payload.part_1':'-123'};
 const ownedPreview=await page.evaluate(request=>window.__TAURI_INTERNALS__.invoke('mod_preview',{session:1,request}),ownedRequest);
 const ownedCosts=ownedPreview.changes.filter(c=>[40013,10300].includes(c.key)&&c.field.startsWith('buffs/'));if(ownedCosts.length!==31||ownedCosts.filter(c=>c.after==='-123'&&c.before==='-1000').length!==1||ownedCosts.filter(c=>c.after==='0').length!==30)throw Error('Self-cost preview/exception');
 const tables=[...new Set(plan.changes.map(c=>c.table))];for(const t of ['characterinfo','fieldinfo','regioninfo','inventory','iteminfo','equiptypeinfo','skill','terrainregionautospawninfo','spawningpoolautospawninfo','stageinfo','conditioninfo','factionreblockadinginfo'])if(!tables.includes(t))throw Error(`Missing change ${t}`);
 if(plan.changes.filter(c=>c.table==='conditioninfo'&&c.key===1011130&&c.after==='!CheckNone()').length!==1)throw Error('Town-flight rule missing');
 if(!plan.changes.some(c=>c.key===30001&&c.field==='buffs/0/0/common.mem_24'&&c.after==='123'))throw Error('Buff edit missing');
 if(plan.changes.filter(c=>c.table==='stageinfo'&&c.after==='129600').length!==2)throw Error('Patrol reset missing');
 const reoccupation=plan.changes.filter(c=>c.table==='factionreblockadinginfo');if(reoccupation.length!==108||reoccupation.some(c=>c.field!=='delayTime'||c.after!==(c.key===16960?'12345':String(Number(c.before)/2))))throw Error('Reoccupation changes incorrect');
 if(plan.changes.some(c=>c.table==='characterinfo'&&c.key===60003))throw Error('Boss dragon changed');
 if(!plan.changes.some(c=>c.table==='iteminfo'&&c.key===chosen.key&&c.after==='123456'))throw Error('Item stat missing');
 if(!plan.changes.some(c=>c.table==='iteminfo'&&c.key===chosen.key&&c.field===`enchant[${newLevel}]`))throw Error('New enchant missing');
 await page.getByRole('button',{name:'Probe an Projektkopie'}).click();await expect(page.locator('.mod-success').first()).toContainText('2 Apply-/Restore-Zyklen',{timeout:120000});
 const rehearsalPath=await page.locator('.mod-success').first().locator('code').textContent();const rehearsal=JSON.parse(await readFile(resolve(rehearsalPath,'result.json'),'utf8'));
 for(const k of ['registry_restored','archive_files_untouched','reapply_passed','recovery_passed','launch_guard_held','update_refusal_passed'])if(!rehearsal[k])throw Error(`Rehearsal failed ${k}`);
 await page.locator('.mod-preview .mod-result-heading').scrollIntoViewIfNeeded();await page.screenshot({path:resolve(output,'advanced-plan.png')});
 await page.getByLabel('Spawn-Menge in Prozent').fill('200');await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toBeDisabled();
 if(JSON.stringify(before)!==JSON.stringify(await metadata()))throw Error('Game metadata changed');if(beforeAudit!==digest(await readFile(audit)))throw Error('Original audit changed');
 let liveInitialized=false;try{await access(join(game,'.workbench'));liveInitialized=true}catch{}if(liveInitialized)throw Error('Actual game .workbench unexpectedly present');if(errors.length)throw Error(errors.join('\n'));
 const result={version:'0.5.9',steamBuild:observed.steam_buildid,exeVersion:observed.exe_version,native:true,records:advanced.records.length,skills:2069,items:info.items.length,skillMatrix:true,buffCostChanges:buffCosts.length,buffCostRecords:33,ownedSkillCostChanges:ownedCosts.length,ownedExceptionChecked:true,namedSkillFields:resourceDetail.record_fields.length,resourceSkill:resourceSkill.key,readOnlyBasisSearch:true,structuredPayloads,structuredSearch:true,skillBuffEdit:true,patrolReset:true,reoccupationRules:108,reoccupationOverride:true,inventoryCeiling:1460,inventoryRecords:9,durabilityItems:durability.length,townFlightRule:true,newEnchantLevel:newLevel,stackSafeItems:advanced.stack_safe_items,repairItems:advanced.repair_items,unsafeRepairRequestsRejected:true,changes:plan.changes.length,tables,exportPath,rehearsalPath,rehearsalCycles:rehearsal.cycles,gameRunning:plan.gates.game_running,gameMetadataUnchanged:Object.keys(before).length,originalAuditUnchanged:true,liveApply:false,liveInitialized:false,templates:true,stalePlanBlocked:true,width1024:true,pageErrors:errors};
 await writeFile(resolve(output,'result.json'),JSON.stringify(result,null,2));console.log(JSON.stringify(result));
}finally{await browser.close()}
