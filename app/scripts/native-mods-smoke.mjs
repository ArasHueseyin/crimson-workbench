// Controls only an explicitly launched, hidden Workbench test instance on port 9225.
import { chromium, expect } from '@playwright/test';
import { mkdir, readFile, writeFile, cp, readdir, unlink } from 'node:fs/promises';
import { resolve, join } from 'node:path';
import { createHash } from 'node:crypto';
const output=resolve('../.local/phase4-v11-native');await mkdir(output,{recursive:true});
let browser;
for(let i=0;i<30;i++){try{browser=await chromium.connectOverCDP('http://127.0.0.1:9225',{timeout:1500});break;}catch{await new Promise(r=>setTimeout(r,500));}}
if(!browser)throw Error('Hidden Workbench WebView2 not reachable');
const page=browser.contexts().flatMap(c=>c.pages()).find(p=>!p.url().startsWith('devtools:'));
if(!page)throw Error('No hidden workbench page');
const errors=[];page.on('pageerror',e=>errors.push(e.message));
await page.locator('.workbench[data-ready="true"]').waitFor({timeout:90000});
await page.getByRole('button',{name:'Modwerkstatt',exact:true}).click();
await expect(page.getByLabel('Shopbestand ändern')).toBeVisible({timeout:90000});
const foreignPanel=page.getByRole('region',{name:'Fremde Zusatzdateien',exact:true});
await foreignPanel.getByRole('button',{name:'Zusatzdateien prüfen',exact:true}).click();
await expect(foreignPanel).toContainText('Keine Bestätigung erforderlich',{timeout:30000});
await expect(foreignPanel.getByRole('button',{name:'Geprüfte Zusatzdateien bestätigen'})).toHaveCount(0);
await expect(foreignPanel.getByRole('button',{name:'Bestätigung zurücknehmen'})).toHaveCount(0);
const foreignCommandGuards=await page.evaluate(async()=>{
 const result=[];for(const command of ['foreign_preview','foreign_confirm','foreign_revoke']){
  try{await window.__TAURI_INTERNALS__.invoke(command,{session:0,reviewId:'invalid',approvalId:'invalid',preserve:true});throw Error(`${command} accepted unopened session`);}
  catch(e){if(e?.code!=='stale_session')throw e;result.push(command);}
 }return result;
});
await page.setViewportSize({width:1024,height:768});await foreignPanel.scrollIntoViewIfNeeded();
if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth))throw Error('Foreign panel overflow');
await page.screenshot({path:resolve(output,'foreign-files-1024.png')});await page.setViewportSize({width:1440,height:920});
const installation=page.getByRole('region',{name:'Installationsprüfung'});
await installation.getByRole('button',{name:'Dateiliste prüfen'}).click();
await expect(installation.locator('.installation-report > [role=status]')).toContainText('285 Dateien gefunden / 285 laut Steam-Cache',{timeout:30000});
await expect(installation).toContainText('Vanilla unbestätigt');await expect(installation).toContainText('Die Live-Prüfung erfolgt separat');
await installation.locator('summary').filter({hasText:'Startprogramme'}).click();
for(const exe of ['CrimsonDesert.exe','crashpad_handler.exe','pers.exe'])await expect(installation).toContainText(exe);
await expect(installation.locator('.installation-issues')).toHaveCount(0);
const gameStoppedAtInventory=(await installation.locator('.installation-report > p').nth(1).innerText()).includes('Spiel beendet');
const auditStart=page.getByRole('button',{name:'Dateiinhalte prüfen',exact:true});
if(gameStoppedAtInventory){await expect(auditStart).toBeEnabled();}else{await expect(auditStart).toBeDisabled();await expect(page.locator('.content-audit')).toContainText('Spiel läuft oder Status unbekannt');}

const installationSummary=await installation.locator('.installation-report > [role=status]').textContent();
const auditCommandGuards=await page.evaluate(async()=>{
 const result=[];for(const command of ['installation_audit_start','installation_audit_cancel','installation_audit_export']){
  try{await window.__TAURI_INTERNALS__.invoke(command,{session:0,id:0});throw Error(`${command} accepted an unopened session`);}
  catch(e){if(e?.code!=='stale_session')throw e;result.push(command);}
 }return result;
});

await installation.scrollIntoViewIfNeeded();await page.screenshot({path:resolve(output,'installation-check.png')});
await page.setViewportSize({width:1024,height:768});
if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth))throw Error('Installation report overflows at 1024px');
await page.screenshot({path:resolve(output,'installation-1024.png')});
await page.setViewportSize({width:1440,height:920});
const baselineCommandGuards=await page.evaluate(async()=>{
 const result=[];for(const command of ['baseline_catalog','baseline_preview','baseline_capture','baseline_inspect']){
  try{await window.__TAURI_INTERNALS__.invoke(command,{session:0,reportName:'invalid',reviewId:'invalid',id:'invalid'});throw Error(`${command} accepted unopened session`);}
  catch(e){if(e?.code!=='stale_session')throw e;result.push(command);}
 }return result;
});
const originalAuditName='installation-audit-1789903405-11284-1-0.json';
const originalAudit=await readFile(resolve('../exports',originalAuditName));
const digest=bytes=>createHash('sha256').update(bytes).digest('hex');
if(digest(originalAudit)!=='f32c50a354dbd81e85889cb764b4ecdf6d83958ed09c9b9eef70077cc48a64dc')throw Error('User-provided audit changed');
const baselinePanel=page.getByRole('region',{name:'Ausgangsbasis und Registry-Sicherung'});
await baselinePanel.getByRole('button',{name:'Prüfstände laden'}).click();
await baselinePanel.getByLabel('Prüfbericht auswählen').selectOption(originalAuditName);
await baselinePanel.getByRole('button',{name:'Bericht für Ausgangsbasis prüfen'}).click();
await expect(baselinePanel).toContainText('bereit zum Speichern',{timeout:30000});
await expect(baselinePanel).toContainText('285 Dateien, darunter 195 PAZ-Archive');
await expect(baselinePanel).toContainText('679 Bytes');
await baselinePanel.getByRole('button',{name:'Ausgangsbasis speichern'}).click();
await expect(baselinePanel).toContainText('Registry-Sicherung geprüft',{timeout:30000});
const baselineSavedDirectory=await baselinePanel.locator('.recovery-review > code').textContent();
const baselineManifest=JSON.parse(await readFile(join(baselineSavedDirectory,'snapshot.json'),'utf8'));
const baselineSavedId=baselineManifest.id;
const livePanel=page.getByRole('region',{name:'Live-Anwendung und Restore'});
await livePanel.getByRole('button',{name:'Live-Status laden'}).click();
await expect(livePanel).toContainText('Live-Einrichtung fehlt');
await livePanel.getByLabel('Live-Ausgangsstand').selectOption(baselineSavedId);
await livePanel.getByRole('button',{name:'Live-Einrichtung prüfen'}).click();
await expect(livePanel).toContainText('285 Dateien',{timeout:30000});
await expect(livePanel.getByLabel('Steam-Dateiprüfung bestätigt')).not.toBeChecked();
await expect(livePanel.getByRole('button',{name:'Live-Basis prüfen und einrichten'})).toBeDisabled();
if(!gameStoppedAtInventory)await expect(livePanel.getByLabel('Steam-Dateiprüfung bestätigt')).toBeDisabled();
const liveCommandGuards=await page.evaluate(async()=>{
 const result=[];for(const command of ['live_status','live_update_preview','live_setup_preview','live_preview','live_start','live_job_status','live_cancel']){
  try{await window.__TAURI_INTERNALS__.invoke(command,{session:0,id:0,baselineId:'invalid',request:{action:'restore'},operation:{kind:'setup',baseline_id:'invalid',review_id:'invalid',steam_verified_before_audit:true}});throw Error(`${command} accepted unopened session`);}
  catch(e){if(e?.code!=='stale_session')throw e;result.push(command);}
 }return result;
});
await page.setViewportSize({width:1024,height:768});await livePanel.scrollIntoViewIfNeeded();
if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth))throw Error('Live panel overflow');
await page.screenshot({path:resolve(output,'live-setup-1024.png')});await page.setViewportSize({width:1440,height:920});

if(digest(await readFile(join(baselineSavedDirectory,'audit.json')))!==digest(originalAudit))throw Error('Persisted audit mismatch');
if(digest(await readFile(join(baselineSavedDirectory,'registry.papgt')))!==baselineManifest.registry_sha256)throw Error('Persisted registry mismatch');
await page.getByRole('button',{name:/Itemdatenbank/}).click();
await page.getByRole('button',{name:'Modwerkstatt',exact:true}).click();
await baselinePanel.getByRole('button',{name:'Prüfstände laden'}).click();
await baselinePanel.getByLabel('Ausgangsstand auswählen').selectOption(baselineSavedId);
await baselinePanel.getByRole('button',{name:'Gespeicherten Stand prüfen'}).click();
await expect(baselinePanel).toContainText('Metadaten passen zum gespeicherten Stand',{timeout:30000});
await baselinePanel.locator('summary').click();await expect(baselinePanel).toContainText('Manuelle Spieltests sind bis zum Ende aller Entwicklungsphasen zurückgestellt');
await baselinePanel.scrollIntoViewIfNeeded();await page.screenshot({path:resolve(output,'baseline-saved.png')});
await page.setViewportSize({width:1024,height:768});
if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth))throw Error('Baseline panel overflow');
await baselinePanel.scrollIntoViewIfNeeded();await page.screenshot({path:resolve(output,'baseline-saved-1024.png')});
await page.setViewportSize({width:1440,height:920});
if(digest(await readFile(resolve('../exports',originalAuditName)))!==digest(originalAudit))throw Error('Original audit modified');
await page.getByLabel('Shopbestand ändern').check();
await page.getByLabel('Dropmengenmultiplikator').fill('2');
await page.getByLabel('Trustmultiplikator',{exact:true}).fill('3');
await page.screenshot({path:resolve(output,'mods-settings.png')});
await page.getByRole('button',{name:'Vorschau berechnen'}).click();
await expect(page.locator('.mod-preview .mod-result-heading h2')).toContainText('34.488',{timeout:90000});
await expect(page.getByRole('button',{name:'Geprüften Mod auf Spiel anwenden'})).toHaveCount(0);
await expect(page.locator('.mod-files')).toContainText('meta/0.papgt');
await page.getByRole('button',{name:'Probe an Projektkopie'}).click();
await expect(page.locator('.mod-success')).toContainText('2 Apply-/Restore-Zyklen',{timeout:90000});
await expect(page.locator('.mod-success')).toContainText('Wiederherstellung nach Abbruch geprüft');
const rehearsalPath=await page.locator('.mod-success code').textContent();
const rehearsal=JSON.parse(await readFile(resolve(rehearsalPath,'result.json'),'utf8'));
if(!rehearsal.launch_guard_held||!rehearsal.update_refusal_passed||rehearsal.protected_source_files!==2||!rehearsal.reapply_passed||!rehearsal.recovery_passed||!rehearsal.registry_restored||rehearsal.transitions.length!==6)throw Error('Reapply/recovery rehearsal failed');
for(const step of rehearsal.transitions.filter(s=>s.name.startsWith('Reapply'))){
 if(step.files.length!==5||step.files.filter(f=>f.action==='remove').length!==2||step.files[2].path!=='meta/0.papgt')throw Error('Reapply must publish new files, switch registry, then remove old files');
}
// Verify the new commands are session gated through the native ACL.
const recoveryCommandGuards=await page.evaluate(async()=>{
 const result=[];for(const command of ['rehearsal_list','rehearsal_review','rehearsal_restore']){
  try{await window.__TAURI_INTERNALS__.invoke(command,{session:0,directory:'invalid',reviewId:'invalid'});throw Error(`${command} accepted an unopened session`);}
  catch(e){if(e?.code!=='stale_session')throw e;result.push(command);}
 }return result;
});
const recoveryPanel=page.getByRole('region',{name:'Sicherung und Wiederherstellung'});
await recoveryPanel.getByRole('button',{name:'Letzte Projektprobe prüfen'}).click();
await expect(recoveryPanel).toContainText('bereits im Ausgangszustand',{timeout:30000});
await expect(recoveryPanel.getByRole('button',{name:'Projektkopie wiederherstellen'})).toBeDisabled();
// Build an interrupted transaction only in a fresh copy of OUR rehearsal.
const recoveryBase=resolve('../.local/rehearsals');
const recoveryCopy=resolve(recoveryBase,`native-recovery-v9-${Date.now()}`);
if(!recoveryCopy.startsWith(recoveryBase+'\\'))throw Error('Recovery fixture outside project');
await cp(rehearsalPath,recoveryCopy,{recursive:true,errorOnExist:true,force:false});
const historyDirectory=join(recoveryCopy,'.workbench','transactions');
const historyNames=(await readdir(historyDirectory)).sort();
const lastHistory=join(historyDirectory,historyNames.at(-1));
const originalCompletion=JSON.parse(await readFile(join(lastHistory,'complete.json'),'utf8'));
if(originalCompletion.outcome!=='rolled_back')throw Error('Expected last rehearsal to be rolled back');
await unlink(join(lastHistory,'complete.json'));
await recoveryPanel.getByRole('button',{name:'Projektproben laden'}).click();
await recoveryPanel.getByLabel('Projektprobe auswählen').selectOption({label:recoveryCopy.split('\\').at(-1)});
await recoveryPanel.getByRole('button',{name:'Backup und Rücknahme prüfen'}).click();
await expect(recoveryPanel).toContainText('Registry wurde noch nicht umgeschaltet',{timeout:30000});
await expect(recoveryPanel.getByRole('button',{name:'Projektkopie wiederherstellen'})).toBeEnabled();
await recoveryPanel.scrollIntoViewIfNeeded();await page.screenshot({path:resolve(output,'recovery-pending.png')});
await recoveryPanel.getByRole('button',{name:'Projektkopie wiederherstellen'}).click();
await expect(recoveryPanel).toContainText('Unterbrochene Projektprobe abgeschlossen',{timeout:30000});
await expect(recoveryPanel).toContainText('bereits im Ausgangszustand');
await expect(recoveryPanel.getByRole('button',{name:'Projektkopie wiederherstellen'})).toBeDisabled();
const completedAgain=JSON.parse(await readFile(join(lastHistory,'complete.json'),'utf8'));
if(JSON.stringify(completedAgain)!==JSON.stringify(originalCompletion))throw Error('Recovery completion mismatch');
await page.setViewportSize({width:1024,height:768});
if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth))throw Error('Recovery panel overflows');
await recoveryPanel.scrollIntoViewIfNeeded();await page.screenshot({path:resolve(output,'recovery-restored-1024.png')});
await page.setViewportSize({width:1440,height:920});
await page.locator('.mod-rehearsal summary').click();
await expect(page.locator('.mod-rehearsal')).toContainText('Reapply 2');
await expect(page.locator('.mod-rehearsal')).toContainText('Entfernen');
await page.locator('.mod-rehearsal').scrollIntoViewIfNeeded();
await page.screenshot({path:resolve(output,'rehearsal-transitions.png')});
await page.locator('.mod-rehearsal summary').click();
await page.getByRole('button',{name:'Vorschau exportieren'}).click();
await expect(page.locator('.mod-success').last()).toContainText('nichts angewendet',{timeout:90000});
const file=await page.locator('.mod-success').last().locator('code').textContent();
if(!file||!file.includes('exports\\mod-preview-'))throw Error('Unexpected export destination');
const exported=JSON.parse(await readFile(file,'utf8'));
if(exported.changes.length!==34488||exported.files.length!==3||exported.gates.can_apply!==false)throw Error('Native plan/export mismatch');
if(exported.changes.some(c=>c.key===9500003)||!exported.credits.includes('GildyBoye'))throw Error('Trust penalty changed or credits missing');
await page.getByLabel('Änderungen durchsuchen').fill('trust');
await expect(page.locator('.mod-diff')).toContainText('150');
await expect(page.locator('.mod-diff tbody tr')).toHaveCount(4);
await page.locator('.mod-preview .mod-result-heading').scrollIntoViewIfNeeded();
await page.screenshot({path:resolve(output,'mods-plan.png')});
await page.setViewportSize({width:1024,height:768});
if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth))throw Error('Mod page horizontal overflow');
await page.screenshot({path:resolve(output,'mods-1024.png')});
await page.setViewportSize({width:1440,height:920});
await page.getByLabel('Trustmultiplikator',{exact:true}).fill('4');
await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toBeDisabled();
// Exercise the newly inferred fields through the final native UI as well.
await page.getByRole('button',{name:'Zurücksetzen',exact:true}).click();
await page.getByLabel('Täglicher Shop-Refresh').check();
await page.getByRole('button',{name:'Vorschau berechnen'}).click();
await expect(page.locator('.mod-preview .mod-result-heading h2')).toContainText('246',{timeout:90000});
await page.getByLabel('Änderungen durchsuchen').fill('');
await page.getByRole('button',{name:'Vorschau exportieren'}).click();
await expect(page.locator('.mod-success').last()).toContainText('nichts angewendet',{timeout:90000});
const dailyPath=await page.locator('.mod-success').last().locator('code').textContent();
const daily=JSON.parse(await readFile(dailyPath,'utf8'));
if(daily.changes.length!==246||daily.changes.some(c=>c.field!=='reset_days'||c.after!=='1'))throw Error('Daily interval plan differs');
await page.getByRole('button',{name:'Zurücksetzen',exact:true}).click();
async function choose(label,key,checked=true){
 const scope=page.locator(`input[aria-label="${label} filtern"]`).locator('..');
 if(!(await scope.evaluate(el=>el.open)))await scope.locator('summary').click();
 await scope.getByLabel(`${label} filtern`,{exact:true}).fill(String(key));
 const option=scope.locator('.mod-options label').filter({has:page.locator('small').filter({hasText:new RegExp(`^${key}(?: ·|$)`)})});
 await expect(option).toHaveCount(1);await option.locator('input').setChecked(checked);
}
await choose('Händler',3101);await choose('Zusatzartikel',2200);await choose('Zusatzartikel',50001);
await page.getByLabel('Täglicher Shop-Refresh').check();await page.getByLabel('Dropmengenmultiplikator').fill('3');
await page.getByLabel('Dropchancenmultiplikator',{exact:true}).fill('2');await choose('Dropsets',175521);
await page.getByRole('button',{name:'Vorschau berechnen'}).click();
await expect(page.locator('.mod-preview')).toBeVisible({timeout:90000});
await page.getByRole('button',{name:'Vorschau exportieren'}).click();
await expect(page.locator('.mod-success').last()).toContainText('nichts angewendet',{timeout:90000});
const expandedPath=await page.locator('.mod-success').last().locator('code').textContent();
const expanded=JSON.parse(await readFile(expandedPath,'utf8'));
if(!expanded.changes.some(c=>c.field.endsWith('.item')&&c.key===3101)||!expanded.changes.some(c=>c.field.endsWith('chance_per_million')&&c.key===175521&&c.before==='35000'&&c.after==='70000'))throw Error('Expanded shop/chance plan missing');
await page.getByRole('button',{name:'Probe an Projektkopie'}).click();
await expect(page.locator('.mod-success').first()).toContainText('2 Apply-/Restore-Zyklen',{timeout:90000});
const expandedRehearsalPath=await page.locator('.mod-success').first().locator('code').textContent();
const expandedRehearsal=JSON.parse(await readFile(resolve(expandedRehearsalPath,'result.json'),'utf8'));
if(!expandedRehearsal.launch_guard_held||!expandedRehearsal.update_refusal_passed||!expandedRehearsal.registry_restored||!expandedRehearsal.reapply_passed||!expandedRehearsal.recovery_passed)throw Error('Grown table rehearsal failed');
await page.locator('.mod-preview .mod-result-heading').scrollIntoViewIfNeeded();await page.screenshot({path:resolve(output,'expanded-plan.png')});
await choose('Garantierte Dropsets',175521);await page.getByLabel('Dropchancenmultiplikator',{exact:true}).fill('0');
await page.getByRole('button',{name:'Vorschau berechnen'}).click();await expect(page.locator('.mod-preview')).toBeVisible({timeout:90000});
await page.getByRole('button',{name:'Vorschau exportieren'}).click();await expect(page.locator('.mod-success').last()).toContainText('nichts angewendet',{timeout:90000});
const guaranteePath=await page.locator('.mod-success').last().locator('code').textContent();
const guarantee=JSON.parse(await readFile(guaranteePath,'utf8'));
if(!guarantee.changes.some(c=>c.field.endsWith('chance_per_million')&&c.key===175521&&c.after==='1000000'))throw Error('Manual guarantee precedence failed');
await expect(page.getByRole('button',{name:'Geprüften Mod auf Spiel anwenden'})).toHaveCount(0);
await page.getByRole('button',{name:'Zurücksetzen',exact:true}).click();
await choose('Händler',3101);await choose('Zusatzartikel',50001);
await page.getByText('Händlerdetails (0)',{exact:true}).click();await page.getByLabel('Händlerdetails ID',{exact:true}).fill('3101');
await page.getByRole('button',{name:'Händler hinzufügen',exact:true}).click();await page.getByLabel('Eigene Artikel für Händler 3101',{exact:true}).check();
await choose('Artikel für Händler 3101',50001,false);await choose('Artikel für Händler 3101',2200);
await page.getByLabel('Refresh für Händler 3101',{exact:true}).selectOption('original');
await page.getByRole('button',{name:'Vorschau berechnen'}).click();await expect(page.locator('.mod-preview')).toBeVisible({timeout:90000});
await page.getByRole('button',{name:'Vorschau exportieren'}).click();await expect(page.locator('.mod-success').last()).toContainText('nichts angewendet',{timeout:90000});
const customPath=await page.locator('.mod-success').last().locator('code').textContent();const custom=JSON.parse(await readFile(customPath,'utf8'));
const additions=custom.changes.filter(c=>c.field.endsWith('.item'));
if(additions.length!==1||!additions[0].after.startsWith('2200 ')||custom.request.vendor_options['3101'].daily_refresh!==false)throw Error('Per-vendor item selection did not replace global selection');
await page.locator('.mod-preview .mod-result-heading').scrollIntoViewIfNeeded();await page.screenshot({path:resolve(output,'vendor-options-plan.png')});
if(errors.length)throw Error(errors.join('\n'));
const result={foreignCommandGuards,foreignReadOnly:true,foreignConfirmed:false,liveCommandGuards,liveSetupPreview:true,liveConfigured:false,baselineSavedDirectory,baselineSavedId,baselineCommandGuards,baselinePersisted:true,recoveryCopy,recoveryCommandGuards,reviewedRestore:true,auditCommandGuards,contentAuditGateChecked:true,gameStoppedAtInventory,installationSummary,installationReadOnly:true,native:true,version:'0.4.10',changes:exported.changes.length,files:exported.files.length,gameRunning:exported.gates.game_running,liveApply:false,rehearsalCycles:2,reapply:true,recovery:true,rehearsalTransitions:rehearsal.transitions.length,rehearsalPath,trustPenaltyPreserved:true,exportPath:file,credits:true,stalePlanBlocked:true,width1024:true,dailyChanges:daily.changes.length,dailyPath,expandedChanges:expanded.changes.length,expandedPath,expandedRehearsalPath,guaranteePath,manualGuarantee:true,heldProtection:true,updateRefusal:true,customPath,vendorOptions:true,pageErrors:errors};
await writeFile(resolve(output,'result.json'),JSON.stringify(result,null,2));console.log(JSON.stringify(result));await browser.close();
