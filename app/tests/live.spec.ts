import {test,expect,type Page} from '@playwright/test';
type Harness={pending:boolean;active:string;calls:{command:string;args:Record<string,unknown>}[];running:boolean;configured:boolean;complete:boolean;job:Record<string,unknown>|null;failure:boolean;defer:boolean;release:(()=>void)|null};
async function fixture(page:Page){
 await page.addInitScript(()=>{
  const w=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;live:Harness};
  w.isTauri=true;w.live={pending:false,active:'fixture',calls:[],running:false,configured:false,complete:true,job:null,failure:false,defer:false,release:null};
  const state=()=>({configured:w.live.configured,initialized:w.live.configured&&!w.live.pending,game_running:w.live.running,baseline_id:w.live.configured?w.live.active:null,pending_basis:w.live.pending?'new':null,active_overlay:false,recovery_required:false,issues:[]});
  w.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>={})=>{
   const h=w.live;h.calls.push({command,args});
   if(command==='bootstrap')return{project:'C:/Synthetic',discovery:{installations:[{path:'C:/Synthetic Game',platform:'steam'}],configured_game:'C:/Synthetic Game'},languages:[{language:'ger'}]};
   if(command==='open_catalog')return{session:1,game_path:args.game,info:{item_count:0,types:[],categories:[],tiers:[],stats:[],index:{language:'ger',path:'synthetic',fingerprint:'synthetic'},exe_version:'synthetic'}};
   if(command==='search_items')return{items:[],total:0,offset:0};
   if(command==='installation_audit_status')return null;
   if(command==='mod_info')return{vendors:[],dropsets:[],store_rows:0,stock_rows:0,opaque_stores:0,opaque_dropsets:0,quantity_excluded:0,trust_rows:0,limitations:[],items:[],append_vendors:[],daily_vendors:[],chance_dropsets:[],guarantee_dropsets:[]};
   if(command==='mod_preview')return{request:args.request,fingerprint:'fixture',plan_id:'plan',changes:[{module:'trust',table:'dropsetinfo',key:1,name:'fixture',field:'min',before:'5',after:'10'}],files:[],warnings:[],gates:{game_running:false,can_apply:false,reasons:[]},credits:'fixture'};
   if(command==='baseline_catalog')return{reports:[],snapshots:['fixture','new'],truncated:false};
   if(command==='live_status')return state();
   if(command==='live_setup_preview')return{review_id:'setup-token',baseline_id:'fixture',game_path:'C:/Synthetic Game',files:4,total_bytes:'1024',audited_at:1789903545,provenance:'fixture'};
   if(command==='live_update_preview')return{review_id:'update-token',previous_baseline:'fixture',next_baseline:args.baselineId,resume:h.pending,game_path:'C:/Synthetic Game',archive_path:'C:/Synthetic Game/.workbench-history/1',files:[{path:'0041/0.paz',action:'archive',before_sha256:'a'.repeat(64),after_sha256:'a'.repeat(64)}],source_files:3,total_bytes:'1000'};
   if(command==='live_preview'){
    if(h.defer)await new Promise<void>(resolve=>{h.release=resolve});
    return{review_id:'review-token',action:(args.request as {action:string}).action,game_path:'C:/Synthetic Game',baseline_id:'fixture',files:[{path:'meta/0.papgt',action:'replace',before_sha256:'a'.repeat(64),after_sha256:'b'.repeat(64)}],recovery_required:false,protected_source_files:3,total_bytes:'1000'};
   }
   if(command==='live_start'){
    if(h.failure)throw{message:'Live-Vorschau veraltet. Keine Änderung; erneut berechnen.'};
    const op=args.operation as {kind:string;request?:{action:string}};
    h.job={id:7,session:1,phase:'running',error:null,result:op.kind==='setup'?state():{action:op.kind==='update'?'basis_update':op.request?.action,review_id:'review-token',registry_sha256:'a'.repeat(64),source_files_verified:3,pending_recovered:false}};
    if(op.kind==='setup')h.configured=true;
    if(op.kind==='update'){h.active='new';h.pending=false;}
    return structuredClone(h.job);
   }
   if(command==='live_job_status'){if(h.job&&h.complete)h.job.phase='complete';return structuredClone(h.job)}
   if(command==='live_cancel'){if(h.job){h.job.phase='cancelled';h.job.error='synthetic cancellation'}return;}
   throw Error(`Unexpected ${command}`);
  }};
 });
 await page.goto('/');await expect(page.locator('.workbench')).toHaveAttribute('data-ready','true');await page.getByRole('button',{name:'Modwerkstatt',exact:true}).click();
 return page.getByRole('region',{name:'Live-Anwendung und Restore'});
}
async function load(page:Page){await page.getByRole('button',{name:'Live-Status laden'}).click()}
test('setup needs explicit provenance and never starts on status or preview alone',async({page})=>{
 const panel=await fixture(page);
 expect(await page.evaluate(()=>(window as unknown as {live:Harness}).live.calls.some(c=>c.command.startsWith('live_')))).toBe(false);
 await load(page);await panel.getByLabel('Live-Ausgangsstand').selectOption('fixture');await panel.getByRole('button',{name:'Live-Einrichtung prüfen'}).click();
 const start=panel.getByRole('button',{name:'Live-Basis prüfen und einrichten'});await expect(start).toBeDisabled();
 await panel.getByLabel('Steam-Dateiprüfung bestätigt').check();await start.click();await expect(panel).toContainText('Live-Vorgang abgeschlossen.');
 await expect(panel).toContainText('Registry im Originalzustand');
 const writes=await page.evaluate(()=>(window as unknown as {live:Harness}).live.calls.filter(c=>c.command==='live_start'));
 expect(writes).toHaveLength(1);expect(writes[0].args.operation).toEqual({kind:'setup',baseline_id:'fixture',review_id:'setup-token',steam_verified_before_audit:true});
 await page.setViewportSize({width:1024,height:768});expect(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth)).toBe(false);
});
test('running game blocks setup and execution without hiding read-only previews',async({page})=>{
 const panel=await fixture(page);await page.evaluate(()=>{(window as unknown as {live:Harness}).live.running=true});await load(page);
 await panel.getByLabel('Live-Ausgangsstand').selectOption('fixture');await panel.getByRole('button',{name:'Live-Einrichtung prüfen'}).click();
 await expect(panel.getByLabel('Steam-Dateiprüfung bestätigt')).toBeDisabled();await expect(panel.getByRole('button',{name:'Live-Basis prüfen und einrichten'})).toBeDisabled();
 await page.evaluate(()=>{(window as unknown as {live:Harness}).live.configured=true});await load(page);await panel.getByRole('button',{name:'Live-Restore prüfen'}).click();
 await expect(panel.getByRole('button',{name:'Geprüften Live-Restore ausführen'})).toBeDisabled();
 expect(await page.evaluate(()=>(window as unknown as {live:Harness}).live.calls.filter(c=>c.command==='live_start').length)).toBe(0);
});
test('apply uses current field preview and stale server rejection clears approval without retry',async({page})=>{
 const panel=await fixture(page);await page.evaluate(()=>{const h=(window as unknown as {live:Harness}).live;h.configured=true;h.failure=true});await load(page);
 await expect(panel.getByRole('button',{name:'Live-Dateivorschau berechnen'})).toBeDisabled();
 await page.getByLabel('Trustmultiplikator').fill('2');await page.getByRole('button',{name:'Vorschau berechnen',exact:true}).click();
 await panel.getByRole('button',{name:'Live-Dateivorschau berechnen'}).click();await expect(panel).toContainText('meta/0.papgt');
 await panel.getByRole('button',{name:'Geprüften Mod auf Spiel anwenden'}).click();await expect(panel.getByRole('alert')).toContainText('veraltet');
 await expect(panel.getByRole('button',{name:'Geprüften Mod auf Spiel anwenden'})).toHaveCount(0);
 const calls=await page.evaluate(()=>(window as unknown as {live:Harness}).live.calls.filter(c=>c.command==='live_start'));
 expect(calls).toHaveLength(1);expect(calls[0].args.operation).toMatchObject({kind:'execute',review_id:'review-token',request:{action:'apply',settings:{trust_multiplier:2}}});
});
test('restore is independently available and active worker can be cancelled',async({page})=>{
 const panel=await fixture(page);await page.evaluate(()=>{const h=(window as unknown as {live:Harness}).live;h.configured=true;h.complete=false});await load(page);
 await panel.getByRole('button',{name:'Live-Restore prüfen'}).click();await panel.getByRole('button',{name:'Geprüften Live-Restore ausführen'}).click();
 await expect(panel).toContainText('Live-Vorgang läuft');await expect(panel.getByRole('button',{name:'Live-Status laden'})).toBeDisabled();
 await panel.getByRole('button',{name:'Live-Vorgang abbrechen'}).click();await expect(panel).toContainText('Live-Vorgang abgebrochen');
 expect(await page.evaluate(()=>(window as unknown as {live:Harness}).live.calls.filter(c=>c.command==='live_cancel').map(c=>c.args))).toEqual([{session:1,id:7}]);
});
test('late live preview after edits cannot enable an obsolete write',async({page})=>{
 const panel=await fixture(page);await page.evaluate(()=>{const h=(window as unknown as {live:Harness}).live;h.configured=true;h.defer=true});await load(page);
 await page.getByLabel('Trustmultiplikator').fill('2');await page.getByRole('button',{name:'Vorschau berechnen',exact:true}).click();await panel.getByRole('button',{name:'Live-Dateivorschau berechnen'}).click();
 await page.getByLabel('Trustmultiplikator').fill('3');await page.evaluate(()=>{(window as unknown as {live:Harness}).live.release?.()});
 await expect(panel.getByRole('button',{name:'Geprüften Mod auf Spiel anwenden'})).toHaveCount(0);
 await expect(panel.getByRole('button',{name:'Live-Dateivorschau berechnen'})).toBeDisabled();
});

test('invalid draft revokes a prepared live review even when the stored number stays unchanged',async({page})=>{
 const panel=await fixture(page);await page.evaluate(()=>{(window as unknown as {live:Harness}).live.configured=true});await load(page);
 const input=page.getByLabel('Trustmultiplikator');await input.fill('2');await page.getByRole('button',{name:'Vorschau berechnen',exact:true}).click();await panel.getByRole('button',{name:'Live-Dateivorschau berechnen'}).click();await expect(panel.getByRole('button',{name:'Geprüften Mod auf Spiel anwenden'})).toBeEnabled();
 await input.fill('1001');await expect(input).toHaveValue('1001');await expect(panel.getByRole('button',{name:'Geprüften Mod auf Spiel anwenden'})).toHaveCount(0);await expect(panel.getByRole('button',{name:'Live-Dateivorschau berechnen'})).toBeDisabled();await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toBeDisabled();
 await input.fill('2');await expect(panel.getByRole('button',{name:'Live-Dateivorschau berechnen'})).toBeEnabled();await expect(panel.getByRole('button',{name:'Geprüften Mod auf Spiel anwenden'})).toHaveCount(0);expect(await page.evaluate(()=>(window as unknown as {live:Harness}).live.calls.filter(c=>c.command==='live_start'))).toHaveLength(0);
});

test('basis update previews exact archive and requires fresh explicit provenance',async({page})=>{
 const panel=await fixture(page);await page.evaluate(()=>{(window as unknown as {live:Harness}).live.configured=true});await load(page);
 await panel.getByText('Basis nach Spielupdate wechseln',{exact:true}).click();await panel.getByLabel('Neue Live-Basis').selectOption('new');
 await panel.getByRole('button',{name:'Basiswechsel prüfen',exact:true}).click();
 const start=panel.getByRole('button',{name:'Geprüften Basiswechsel ausführen'});await expect(start).toBeDisabled();
 await expect(panel).toContainText('0041/0.paz');await panel.getByLabel('Steam-Prüfung für neue Basis bestätigt').check();await start.click();
 await expect(panel).toContainText('Neue Basis aktiv; bisherige Workbench-Dateien archiviert.');
 const calls=await page.evaluate(()=>(window as unknown as {live:Harness}).live.calls.filter(c=>c.command==='live_start'));
 expect(calls).toHaveLength(1);expect(calls[0].args.operation).toEqual({kind:'update',baseline_id:'new',review_id:'update-token',steam_verified_before_audit:true});
});
test('running game blocks basis update while review remains readable',async({page})=>{
 const panel=await fixture(page);await page.evaluate(()=>{const h=(window as unknown as {live:Harness}).live;h.configured=true;h.running=true});await load(page);
 await panel.getByText('Basis nach Spielupdate wechseln',{exact:true}).click();await panel.getByLabel('Neue Live-Basis').selectOption('new');await panel.getByRole('button',{name:'Basiswechsel prüfen',exact:true}).click();
 await expect(panel.getByLabel('Steam-Prüfung für neue Basis bestätigt')).toBeDisabled();await expect(panel.getByRole('button',{name:'Geprüften Basiswechsel ausführen'})).toBeDisabled();
 expect(await page.evaluate(()=>(window as unknown as {live:Harness}).live.calls.filter(c=>c.command==='live_start'))).toHaveLength(0);
});
test('pending basis migration fixes target and offers explicit resume instead of old restore',async({page})=>{
 const panel=await fixture(page);await page.evaluate(()=>{const h=(window as unknown as {live:Harness}).live;h.configured=true;h.pending=true});await load(page);
 await expect(panel.getByLabel('Neue Live-Basis')).toHaveValue('new');await expect(panel.getByLabel('Neue Live-Basis')).toBeDisabled();
 await expect(panel.getByRole('button',{name:'Live-Restore prüfen'})).toHaveCount(0);await expect(panel.getByRole('button',{name:'Live-Einrichtung prüfen'})).toHaveCount(0);
 await panel.getByRole('button',{name:'Basiswechsel prüfen',exact:true}).click();await expect(panel.getByRole('button',{name:'Geprüften Basiswechsel fortsetzen'})).toBeDisabled();
 await panel.getByLabel('Steam-Prüfung für neue Basis bestätigt').check();await panel.getByRole('button',{name:'Geprüften Basiswechsel fortsetzen'}).click();await expect(panel).toContainText('Neue Basis aktiv');
});
test('changing basis selection discards review and provenance',async({page})=>{
 const panel=await fixture(page);await page.evaluate(()=>{(window as unknown as {live:Harness}).live.configured=true});await load(page);
 await panel.getByText('Basis nach Spielupdate wechseln',{exact:true}).click();await panel.getByLabel('Neue Live-Basis').selectOption('new');await panel.getByRole('button',{name:'Basiswechsel prüfen',exact:true}).click();await panel.getByLabel('Steam-Prüfung für neue Basis bestätigt').check();
 await panel.getByLabel('Neue Live-Basis').selectOption('fixture');await expect(panel.getByRole('button',{name:'Geprüften Basiswechsel ausführen'})).toHaveCount(0);await expect(panel.getByRole('button',{name:'Basiswechsel prüfen',exact:true})).toBeDisabled();
});
