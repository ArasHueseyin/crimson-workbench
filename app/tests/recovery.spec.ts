import {test,expect,type Page} from '@playwright/test';
type Harness={calls:{command:string;args:Record<string,unknown>}[];failure:string;empty:boolean;clean:boolean;pending:boolean;defer:boolean;release:null|(()=>void)};
async function fixture(page:Page) {
 await page.addInitScript(()=>{
  const root=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;recovery:Harness};
  root.isTauri=true;root.recovery={calls:[],failure:'',empty:false,clean:false,pending:true,defer:false,release:null};
  root.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>={})=>{
   const h=root.recovery;h.calls.push({command,args});
   if(command==='bootstrap')return{project:'C:/Synthetic',discovery:{installations:[{path:'C:/Synthetic Game',platform:'steam'}],configured_game:'C:/Synthetic Game'},languages:[{language:'ger'}]};
   if(command==='open_catalog')return{session:1,game_path:args.game,info:{item_count:0,types:[],categories:[],tiers:[],stats:[],index:{language:'ger',path:'synthetic',fingerprint:'synthetic'},exe_version:'synthetic'}};
   if(command==='search_items')return{items:[],total:0,offset:0};
   if(command==='installation_audit_status')return null;
   if(command==='mod_info')return{vendors:[],dropsets:[],store_rows:0,stock_rows:0,opaque_stores:0,opaque_dropsets:0,quantity_excluded:0,trust_rows:0,limitations:[],items:[],append_vendors:[],daily_vendors:[],chance_dropsets:[],guarantee_dropsets:[]};
   if(command==='rehearsal_list')return{truncated:false,entries:h.empty?[]:[{name:'test',directory:'C:/Synthetic/.local/rehearsals/test',format:3,error:null},{name:'legacy',directory:'C:/Synthetic/.local/rehearsals/legacy',format:2,error:'Ältere ungeschützte Probe'}]};
   if(command==='rehearsal_review'){
    if(h.defer)await new Promise<void>(resolve=>{h.release=resolve});
    if(h.failure==='review')throw{message:'backup verification failed',code:'read_error'};
    return{directory:args.directory,review_id:'reviewed-token',plan_id:'plan',protected_source_files:2,can_restore:!h.clean,scope:'Ausschließlich diese Projektkopie. Kein Live-Restore.',restore:{backup_sha256:'a'.repeat(64),backup_bytes:679,current_registry_sha256:'b'.repeat(64),pending_outcome:h.clean||!h.pending?null:'committed',pending_intent_sha256:h.clean?null:'c'.repeat(64),next_transaction_id:'2',files:h.clean?[]:[{path:'meta/0.papgt',action:'replace',before_sha256:'b'.repeat(64),after_sha256:'a'.repeat(64)},{path:'0041/0.paz',action:'remove',before_sha256:'c'.repeat(64),after_sha256:null}],remove_directories:h.clean?[]:['0041']}};
   }
   if(command==='rehearsal_restore'){
    if(h.failure==='restore')throw{message:'Projektprobe seit der Vorschau geändert. Bitte erneut prüfen; nichts wiederhergestellt.',code:'read_error'};
    h.clean=true;return{directory:args.directory,registry_restored:true,pending_recovered:h.pending,plan_id:'plan'};
   }
   throw Error(`Unexpected ${command}`);
  }};
 });
 await page.goto('/');await expect(page.locator('.workbench')).toHaveAttribute('data-ready','true');
 await page.getByRole('button',{name:'Modwerkstatt',exact:true}).click();
 return page.getByRole('region',{name:'Sicherung und Wiederherstellung'});
}
async function load(page:Page) {
 await page.getByRole('button',{name:'Projektproben laden'}).click();
 await page.getByLabel('Projektprobe auswählen').selectOption({label:'test'});
 await page.getByRole('button',{name:'Backup und Rücknahme prüfen'}).click();
}
test('explicit preview shows pending cleanup and submits exact reviewed token once',async({page})=>{
 const panel=await fixture(page);
 expect(await page.evaluate(()=>(window as unknown as {recovery:Harness}).recovery.calls.some(c=>c.command.startsWith('rehearsal_')))).toBe(false);
 await load(page);await expect(panel).toContainText('Sicherung geprüft · Rücknahme vorbereitet');
 await expect(panel).toContainText('Registry wurde bereits umgeschaltet');await expect(panel).toContainText('0041/0.paz');
 await expect(panel).toContainText('Aus Sicherung ersetzen');await expect(panel).toContainText('SHA-256 der Sicherung');
 await page.setViewportSize({width:1024,height:768});expect(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth)).toBe(false);
 await panel.getByRole('button',{name:'Projektkopie wiederherstellen'}).click();
 await expect(panel).toContainText('Unterbrochene Projektprobe abgeschlossen');await expect(panel).toContainText('bereits im Ausgangszustand');
 await expect(panel.getByRole('button',{name:'Projektkopie wiederherstellen'})).toBeDisabled();
 const writes=await page.evaluate(()=>(window as unknown as {recovery:Harness}).recovery.calls.filter(c=>c.command==='rehearsal_restore'));
 expect(writes).toHaveLength(1);expect(writes[0].args).toEqual({session:1,directory:'C:/Synthetic/.local/rehearsals/test',reviewId:'reviewed-token'});
});
test('corrupt backup and stale review disable recovery without automatic retries',async({page})=>{
 const panel=await fixture(page);await page.evaluate(()=>{(window as unknown as {recovery:Harness}).recovery.failure='review'});
 await load(page);await expect(panel.getByRole('alert')).toContainText('backup verification failed');
 await expect(panel.getByRole('button',{name:'Projektkopie wiederherstellen'})).toHaveCount(0);
 await page.evaluate(()=>{(window as unknown as {recovery:Harness}).recovery.failure='restore'});
 await panel.getByRole('button',{name:'Backup und Rücknahme prüfen'}).click();await panel.getByRole('button',{name:'Projektkopie wiederherstellen'}).click();
 await expect(panel.getByRole('alert')).toContainText('seit der Vorschau geändert');await expect(panel.getByRole('button',{name:'Projektkopie wiederherstellen'})).toHaveCount(0);
 expect(await page.evaluate(()=>(window as unknown as {recovery:Harness}).recovery.calls.filter(c=>c.command==='rehearsal_restore').length)).toBe(1);
});
test('empty and legacy entries are explained; changing selection discards approval',async({page})=>{
 const panel=await fixture(page);await page.evaluate(()=>{(window as unknown as {recovery:Harness}).recovery.empty=true});
 await panel.getByRole('button',{name:'Projektproben laden'}).click();await expect(panel).toContainText('Keine Projektproben vorhanden');
 await page.evaluate(()=>{(window as unknown as {recovery:Harness}).recovery.empty=false});
 await load(page);await expect(panel.locator('option').filter({hasText:'legacy'})).toHaveJSProperty('disabled',true);
 await panel.getByLabel('Projektprobe auswählen').selectOption('');await expect(panel.getByRole('button',{name:'Projektkopie wiederherstellen'})).toHaveCount(0);
});
test('late review after navigation cannot restore a discarded selection',async({page})=>{
 await fixture(page);await page.evaluate(()=>{(window as unknown as {recovery:Harness}).recovery.defer=true});
 await load(page);await expect(page.getByRole('region',{name:'Sicherung und Wiederherstellung'})).toContainText('Projektprobe wird geprüft');
 await page.getByRole('button',{name:/Itemdatenbank/}).click();
 await page.evaluate(()=>{const h=(window as unknown as {recovery:Harness}).recovery;h.defer=false;h.release?.()});
 await page.getByRole('button',{name:'Modwerkstatt',exact:true}).click();
 await expect(page.getByRole('button',{name:'Projektkopie wiederherstellen'})).toHaveCount(0);
 expect(await page.evaluate(()=>(window as unknown as {recovery:Harness}).recovery.calls.some(c=>c.command==='rehearsal_restore'))).toBe(false);
});
