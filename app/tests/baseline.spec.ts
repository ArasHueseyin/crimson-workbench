import { test,expect,type Page } from '@playwright/test';
type Harness={calls:{command:string;args:Record<string,unknown>}[];saved:boolean;failure:string;changed:boolean;defer:boolean;release:(()=>void)|null};
async function fixture(page:Page) {
 await page.addInitScript(()=>{
  const root=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;baseline:Harness};
  root.isTauri=true;root.baseline={calls:[],saved:false,failure:'',changed:false,defer:false,release:null};
  const preview={report_name:'installation-audit-fixture.json',review_id:'baseline-token',game_path:'C:/Synthetic Game',build_id:'123',audited_at:1789903545,file_count:285,archive_count:195,total_bytes:'154097618899',registry_bytes:679,registry_sha256:'a'.repeat(64),executables:['bin64/CrimsonDesert.exe'],read_schema_matches:true,certified_vanilla:false,can_apply:false};
  const saved=()=>({id:'saved-fixture',directory:'C:/Synthetic/.local/baselines/saved-fixture',created_at:1789904500,preview,current_state:root.baseline.changed?'changed':'metadata_match',issues:root.baseline.changed?['Registry seit dem Prüfbericht geändert']:[]});
  root.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>={})=>{
   const h=root.baseline;h.calls.push({command,args});
   if(command==='bootstrap')return{project:'C:/Synthetic',discovery:{installations:[{path:'C:/Synthetic Game',platform:'steam'}],configured_game:'C:/Synthetic Game'},languages:[{language:'ger'}]};
   if(command==='open_catalog')return{session:1,game_path:args.game,info:{item_count:0,types:[],categories:[],tiers:[],stats:[],index:{language:'ger',path:'synthetic',fingerprint:'synthetic'},exe_version:'synthetic'}};
   if(command==='search_items')return{items:[],total:0,offset:0};
   if(command==='installation_audit_status')return null;
   if(command==='mod_info')return{vendors:[],dropsets:[],store_rows:0,stock_rows:0,opaque_stores:0,opaque_dropsets:0,quantity_excluded:0,trust_rows:0,limitations:[],items:[],append_vendors:[],daily_vendors:[],chance_dropsets:[],guarantee_dropsets:[]};
   if(command==='baseline_catalog')return{reports:['installation-audit-fixture.json'],snapshots:h.saved?['saved-fixture']:[],truncated:false};
   if(command==='baseline_preview'){
    if(h.defer)await new Promise<void>(resolve=>{h.release=resolve});
    if(h.failure==='preview')throw{message:'Registry seit dem Prüfbericht geändert'};
    return structuredClone(preview);
   }
   if(command==='baseline_capture'){
    if(h.failure==='capture')throw{message:'Prüfbericht oder Installation seit der Vorschau geändert; erneut prüfen'};
    h.saved=true;return structuredClone(saved());
   }
   if(command==='baseline_inspect'){
    if(h.failure==='inspect')throw{message:'Gespeicherter Prüfbericht oder Registry-Sicherung wurde verändert'};
    return structuredClone(saved());
   }
   throw Error(`Unexpected ${command}`);
  }};
 });
 await page.goto('/');await expect(page.locator('.workbench')).toHaveAttribute('data-ready','true');
 await page.getByRole('button',{name:'Modwerkstatt',exact:true}).click();
 return page.getByRole('region',{name:'Ausgangsbasis und Registry-Sicherung'});
}
async function preview(page:Page) {
 await page.getByRole('button',{name:'Prüfstände laden'}).click();
 await page.getByLabel('Prüfbericht auswählen').selectOption('installation-audit-fixture.json');
 await page.getByRole('button',{name:'Bericht für Ausgangsbasis prüfen'}).click();
}
test('explicit audit import saves reviewed baseline and can be reopened after navigation',async({page})=>{
 const panel=await fixture(page);
 expect(await page.evaluate(()=>(window as unknown as {baseline:Harness}).baseline.calls.some(c=>c.command.startsWith('baseline_')))).toBe(false);
 await preview(page);await expect(panel).toContainText('bereit zum Speichern');
 await expect(panel).toContainText('285 Dateien');await expect(panel).toContainText('679 Bytes');
 await expect(panel).toContainText('Gleich große Archivänderungen');
 await panel.getByRole('button',{name:'Ausgangsbasis speichern'}).click();
 await expect(panel).toContainText('Registry-Sicherung geprüft');
 const writes=await page.evaluate(()=>(window as unknown as {baseline:Harness}).baseline.calls.filter(c=>c.command==='baseline_capture'));
 expect(writes).toHaveLength(1);expect(writes[0].args).toEqual({session:1,reportName:'installation-audit-fixture.json',reviewId:'baseline-token'});
 await page.getByRole('button',{name:/Itemdatenbank/}).click();await page.getByRole('button',{name:'Modwerkstatt',exact:true}).click();
 await panel.getByRole('button',{name:'Prüfstände laden'}).click();await page.getByLabel('Ausgangsstand auswählen').selectOption('saved-fixture');
 await panel.getByRole('button',{name:'Gespeicherten Stand prüfen'}).click();await expect(panel).toContainText('Metadaten passen');
 await page.setViewportSize({width:1024,height:768});expect(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth)).toBe(false);
});
test('report mismatch or changed preview does not capture or silently retry',async({page})=>{
 const panel=await fixture(page);await page.evaluate(()=>{(window as unknown as {baseline:Harness}).baseline.failure='preview'});
 await preview(page);await expect(panel.getByRole('alert')).toContainText('Registry seit dem Prüfbericht geändert');
 await expect(panel.getByRole('button',{name:'Ausgangsbasis speichern'})).toHaveCount(0);
 await page.evaluate(()=>{(window as unknown as {baseline:Harness}).baseline.failure='capture'});
 await panel.getByRole('button',{name:'Bericht für Ausgangsbasis prüfen'}).click();await panel.getByRole('button',{name:'Ausgangsbasis speichern'}).click();
 await expect(panel.getByRole('alert')).toContainText('seit der Vorschau geändert');
 expect(await page.evaluate(()=>(window as unknown as {baseline:Harness}).baseline.calls.filter(c=>c.command==='baseline_capture').length)).toBe(1);
 await expect(panel.getByRole('button',{name:'Ausgangsbasis speichern'})).toHaveCount(0);
});
test('stored backup shows update status and damaged copies are refused',async({page})=>{
 const panel=await fixture(page);await page.evaluate(()=>{const h=(window as unknown as {baseline:Harness}).baseline;h.saved=true;h.changed=true});
 await panel.getByRole('button',{name:'Prüfstände laden'}).click();await page.getByLabel('Ausgangsstand auswählen').selectOption('saved-fixture');
 await panel.getByRole('button',{name:'Gespeicherten Stand prüfen'}).click();await expect(panel).toContainText('Registry-Sicherung erhalten · Installation weicht ab');
 await expect(panel).toContainText('Live-Apply gesperrt');
 await page.evaluate(()=>{(window as unknown as {baseline:Harness}).baseline.failure='inspect'});
 await panel.getByRole('button',{name:'Gespeicherten Stand prüfen'}).click();await expect(panel.getByRole('alert')).toContainText('Registry-Sicherung wurde verändert');
 await expect(panel.locator('.recovery-review')).toHaveCount(0);
});
test('discarded selection and late preview cannot leave an enabled capture button',async({page})=>{
 const panel=await fixture(page);await preview(page);await expect(panel.getByRole('button',{name:'Ausgangsbasis speichern'})).toBeEnabled();
 await page.getByLabel('Prüfbericht auswählen').selectOption('');await expect(panel.getByRole('button',{name:'Ausgangsbasis speichern'})).toHaveCount(0);
 await page.evaluate(()=>{(window as unknown as {baseline:Harness}).baseline.defer=true});
 await page.getByLabel('Prüfbericht auswählen').selectOption('installation-audit-fixture.json');await panel.getByRole('button',{name:'Bericht für Ausgangsbasis prüfen'}).click();
 await expect(panel).toContainText('Ausgangsstand wird geprüft');await page.getByRole('button',{name:/Itemdatenbank/}).click();
 await page.evaluate(()=>{const h=(window as unknown as {baseline:Harness}).baseline;h.defer=false;h.release?.()});
 await page.getByRole('button',{name:'Modwerkstatt',exact:true}).click();await expect(panel.getByRole('button',{name:'Ausgangsbasis speichern'})).toHaveCount(0);
});
