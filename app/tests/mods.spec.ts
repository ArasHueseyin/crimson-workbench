import { test,expect,type Page } from '@playwright/test';
async function fixture(page:Page,failed=false,owned=false) {
 await page.addInitScript(({failed,owned})=>{
  const root=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;modCalls:{command:string;args:Record<string,unknown>}[]};root.isTauri=true;root.modCalls=[];
  const choices=[{key:7,name:'Synthetic_vendor',entries:2},{key:8,name:'Synthetic_other',entries:3}];
  root.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>={})=>{
   root.modCalls.push({command,args});
   if(command==='installation_audit_status')return null;
   if(command==='installation_check') {
    if(failed)throw{message:'Installationsprüfung fehlgeschlagen',code:'read_error'};
    return {version:1,mode:'metadata_inventory',game_path:'C:/Synthetic Game',observed_at:1789850000,build_id:'synthetic',game_running:true,directory_scan_complete:true,depot_comparison_available:true,content_verified:false,certified_vanilla:false,can_apply:false,expected_files:2,actual_files:3,expected_bytes:'100',registry_sha256:'synthetic',registry_matches_observed_build:true,depots:[{id:1,manifest_id:'1190168329432671189',files:2,authenticated:false}],executables:['bin64/Game.exe','bin64/Extra.exe'],groups:[{name:'0000',optional:false,installed:true,in_depots:true}],files:[{path:'0000/0.paz',state:'size_matches'},{path:'bin64/Extra.exe',state:'additional'}],managed_files:owned?[{path:'0041/0.paz',state:'workbench_owned'}]:[],managed_registry:owned,issues:[],limitations:['Dateiinhalte sind ungeprüft.']};
   }
   if(command==='bootstrap')return{project:'C:/Synthetic',discovery:{installations:[{path:'C:/Synthetic Game',platform:'steam'}],configured_game:'C:/Synthetic Game'},languages:[{language:'ger'}]};
   if(command==='open_catalog')return{session:1,game_path:args.game,info:{item_count:0,types:[],categories:[],tiers:[],stats:[],index:{language:args.language,path:'synthetic',fingerprint:'synthetic'},exe_version:'synthetic'}};
   if(command==='search_items')return{items:[],total:0,offset:0};
   if(command==='mod_info')return{vendors:choices,dropsets:[{key:1,name:'Synthetic_drop',entries:1}],store_rows:3,stock_rows:5,opaque_stores:1,opaque_dropsets:1,quantity_excluded:1,trust_rows:3,limitations:['Synthetic only'],items:[{key:2200,name:'Synthetic_arrow',entries:0}],append_vendors:[7],daily_vendors:[7,8],chance_dropsets:[{key:1,name:'Synthetic_drop',entries:1}],guarantee_dropsets:[{key:1,name:'Synthetic_drop',entries:1}]};
   if(command==='mod_preview'){
    if(failed)throw{message:'Unrecognized schema',code:'unsupported_build'};
    return {request:args.request,plan_id:'synthetic-plan',fingerprint:'synthetic',changes:[{module:'shops',table:'storeinfo',key:7,name:'Synthetic_vendor',field:'stocks[0].stock_count',before:'5',after:'999'}],files:[{path:'0041/0.paz',action:'create',before_sha256:null,after_sha256:'synthetic-sha',bytes:100}],warnings:['No live test'],gates:{game_running:true,can_apply:false,reasons:['Crimson Desert läuft.','Vanilla-Nachweis fehlt.']},credits:'Synthetic fixture'};
   }
   if(command==='mod_export')return'C:/Synthetic/exports/preview.json';
   if(command==='mod_rehearse')return{directory:'C:/Synthetic/.local/rehearsals/test',plan_id:args.planId,cycles:2,registry_restored:true,archive_files_untouched:true,scope:'Isolierte Projektkopie',reapply_passed:true,recovery_passed:true,launch_guard_held:true,protected_source_files:2,update_refusal_passed:true,transitions:[{name:'Reapply 1',files:[{path:'0042/0.paz',action:'create',before_sha256:null,after_sha256:'new-generation'},{path:'0041/0.paz',action:'remove',before_sha256:'previous-generation',after_sha256:null}]}]};
   throw Error(`Unexpected ${command}`);
  }};
 },{failed,owned});
 await page.goto('/');await expect(page.locator('.workbench')).toHaveAttribute('data-ready','true');await page.getByRole('button',{name:'Modwerkstatt',exact:true}).click();await expect(page.getByLabel('Shopbestand ändern')).toBeVisible();
}
test('preview is explicit, live apply stays disabled, rehearsal and export use reviewed plan',async({page})=>{
 await fixture(page);await page.getByLabel('Shopbestand ändern').check();await page.getByRole('button',{name:'Vorschau berechnen'}).click();
 await expect(page.getByRole('button',{name:'Geprüften Mod auf Spiel anwenden'})).toHaveCount(0);await expect(page.locator('.mod-diff')).toContainText('999');
 await page.getByRole('button',{name:'Probe an Projektkopie'}).click();await expect(page.locator('.mod-success')).toContainText('2 Apply-/Restore-Zyklen');
 await expect(page.locator('.mod-success')).toContainText('Wiederherstellung nach Abbruch geprüft');await expect(page.locator('.mod-success')).toContainText('Startschutz, Backupschutz und Update-Sperre an Testdateien geprüft.');await page.locator('.mod-rehearsal summary').click();await expect(page.locator('.mod-rehearsal')).toContainText('Reapply 1');await expect(page.locator('.mod-rehearsal')).toContainText('Entfernen');await expect(page.locator('.mod-rehearsal')).toContainText('previous-generation');
 await page.getByRole('button',{name:'Vorschau exportieren'}).click();await expect(page.locator('.mod-success').last()).toContainText('nichts angewendet');
 const calls=await page.evaluate(()=>(window as unknown as {modCalls:{command:string;args:Record<string,unknown>}[]}).modCalls);
 expect(calls.filter(c=>c.command==='mod_export')[0].args.planId).toBe('synthetic-plan');expect(calls.some(c=>c.command==='apply'||c.command==='restore')).toBe(false);
 await page.getByLabel('Trustmultiplikator',{exact:true}).fill('3');await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toBeDisabled();await expect(page.getByRole('button',{name:'Probe an Projektkopie'})).toBeDisabled();
});

test('installation check is opt-in, distinguishes metadata from trust and lists extra files',async({page})=>{
 await fixture(page);
 expect(await page.evaluate(()=>(window as unknown as {modCalls:{command:string}[]}).modCalls.some(c=>c.command==='installation_check'))).toBe(false);
 const panel=page.getByRole('region',{name:'Installationsprüfung'});
 await panel.getByRole('button',{name:'Dateiliste prüfen'}).click();
 await expect(panel.locator('.installation-report > [role=status]')).toContainText('3 Dateien gefunden / 2 laut Steam-Cache');
 await expect(panel).toContainText('Vanilla unbestätigt');await expect(panel).toContainText('Die Live-Prüfung erfolgt separat');
 await panel.locator('summary').filter({hasText:'Startprogramme'}).click();await expect(panel).toContainText('bin64/Game.exe');
 await panel.locator('summary').filter({hasText:'Zusätzliche'}).click();await expect(panel).toContainText('bin64/Extra.exe');
 await panel.getByRole('button',{name:'Dateiliste prüfen'}).click();await expect(panel.locator('.installation-report > [role=status]')).toContainText('Dateiliste geprüft');
 await page.getByRole('button',{name:'Vorschau berechnen'}).click();await expect(page.getByRole('button',{name:'Geprüften Mod auf Spiel anwenden'})).toHaveCount(0);
});
test('installation failure is visible and can be retried',async({page})=>{
 await fixture(page,true);const panel=page.getByRole('region',{name:'Installationsprüfung'});
 await panel.getByRole('button',{name:'Dateiliste prüfen'}).click();await expect(panel.getByRole('alert')).toContainText('fehlgeschlagen');
 await expect(panel.getByRole('button',{name:'Dateiliste prüfen'})).toBeEnabled();await expect(panel.locator('.installation-report')).toHaveCount(0);
});
test('vendor selection and explicit overrides survive navigation and flow to backend',async({page})=>{
 await fixture(page);const shop=page.locator('.mod-card').first();await shop.locator('summary').first().click();await shop.getByLabel('Synthetic_vendor').check();await shop.getByText('Einzelausnahmen (0)',{exact:true}).click();
 await page.getByLabel('Händler Ausnahme-ID').fill('8');await page.getByLabel('Händler Ausnahmewert').fill('44');await shop.getByRole('button',{name:'Setzen'}).click();await page.getByRole('button',{name:/Itemdatenbank/}).click();await page.getByRole('button',{name:'Modwerkstatt',exact:true}).click();
 await page.getByRole('button',{name:'Vorschau berechnen'}).click();await expect(page.locator('.mod-diff')).toBeVisible();
 const call=await page.evaluate(()=>(window as unknown as {modCalls:{command:string;args:{request:{vendors:number[];vendor_overrides:Record<string,number>}}}[]}).modCalls.find(c=>c.command==='mod_preview'));
 expect(call?.args.request.vendors).toEqual([7]);expect(call?.args.request.vendor_overrides).toEqual({'8':44});
});
test('invalid numeric settings cannot create a misleading preview; 1024px does not overflow',async({page})=>{
 await page.setViewportSize({width:1024,height:768});await fixture(page);await page.getByLabel('Dropmengenmultiplikator').fill('1.5');await page.getByRole('button',{name:'Vorschau berechnen'}).click();await expect(page.locator('.mod-preview')).toHaveCount(0);
 expect(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth)).toBe(false);await page.getByLabel('Dropmengenmultiplikator').fill('2');await page.getByRole('button',{name:'Vorschau berechnen'}).click();await expect(page.locator('.mod-diff')).toBeVisible();
});
test('backend rejection shows reason and produces no export or rehearsal controls',async({page})=>{
 await fixture(page,true);await page.getByRole('button',{name:'Vorschau berechnen'}).click();await expect(page.getByRole('alert')).toContainText('Unrecognized schema');await expect(page.getByRole('button',{name:'Vorschau exportieren'})).toHaveCount(0);
});

test('daily refresh, additional items and manual guarantees form one reviewed request',async({page})=>{
 await fixture(page);await page.getByLabel('Täglicher Shop-Refresh').check();await page.getByText('Zusatzartikel: keine Auswahl',{exact:true}).click();await page.getByLabel('Synthetic_arrow').check();
 await page.getByLabel('Dropchancenmultiplikator',{exact:true}).fill('2');await page.getByText('Garantierte Dropsets: keine Auswahl',{exact:true}).click();
 const guaranteed=page.locator('details').filter({has:page.locator('input[aria-label="Garantierte Dropsets filtern"]')});await guaranteed.getByLabel('Synthetic_drop').check();
 await page.getByRole('button',{name:'Vorschau berechnen'}).click();await expect(page.locator('.mod-preview')).toBeVisible();
 const call=await page.evaluate(()=>(window as unknown as {modCalls:{command:string;args:{request:Record<string,unknown>}}[]}).modCalls.find(c=>c.command==='mod_preview'));
 expect(call?.args.request.daily_refresh).toBe(true);expect(call?.args.request.shop_items).toEqual([2200]);expect(call?.args.request.chance_multiplier).toBe(2);expect(call?.args.request.guaranteed_dropsets).toEqual([1]);
 await page.getByRole('button',{name:'Zurücksetzen',exact:true}).click();await expect(page.getByLabel('Täglicher Shop-Refresh')).not.toBeChecked();await expect(page.getByLabel('Dropchancenmultiplikator',{exact:true})).toHaveValue('1');await expect(page.getByText('Zusatzartikel: keine Auswahl',{exact:true})).toBeVisible();
});

test('per-vendor assortment and refresh exceptions can override and then inherit globals',async({page})=>{
 await fixture(page);await page.getByText('Händlerdetails (0)',{exact:true}).click();
 await page.getByLabel('Händlerdetails ID',{exact:true}).fill('7');await page.getByRole('button',{name:'Händler hinzufügen',exact:true}).click();
 await page.getByLabel('Eigene Artikel für Händler 7',{exact:true}).check();
 await page.getByText('Artikel für Händler 7: keine Auswahl',{exact:true}).click();
 const scope=page.locator('details.mod-scope').filter({has:page.locator('input[aria-label="Artikel für Händler 7 filtern"]')});await scope.getByLabel('Synthetic_arrow').check();
 await page.getByLabel('Refresh für Händler 7',{exact:true}).selectOption('original');
 await page.getByRole('button',{name:'Vorschau berechnen'}).click();await expect(page.locator('.mod-preview')).toBeVisible();
 const requests=()=>page.evaluate(()=>(window as unknown as {modCalls:{command:string;args:{request:{vendor_options:Record<string,unknown>}}}[]}).modCalls.filter(c=>c.command==='mod_preview').map(c=>c.args.request));
 expect((await requests()).at(-1)?.vendor_options).toEqual({'7':{items:[2200],daily_refresh:false}});
 await page.getByLabel('Eigene Artikel für Händler 7',{exact:true}).uncheck();await page.getByLabel('Refresh für Händler 7',{exact:true}).selectOption('inherit');
 await page.getByRole('button',{name:'Vorschau berechnen'}).click();await expect(page.locator('.mod-preview')).toBeVisible();
 expect((await requests()).at(-1)?.vendor_options).toEqual({'7':{items:null,daily_refresh:null}});
 await page.getByLabel('Händlerdetails 7 entfernen',{exact:true}).click();await expect(page.getByText('Händlerdetails (0)',{exact:true})).toBeVisible();
});

test('own files appear separately from unexplained files and active registry is explicit',async({page})=>{
 await fixture(page,false,true);const panel=page.getByRole('region',{name:'Installationsprüfung'});await panel.getByRole('button',{name:'Dateiliste prüfen'}).click();
 await panel.getByText('Eigene Workbench-Einträge (1)',{exact:true}).click();await expect(panel).toContainText('0041/0.paz');await expect(panel).toContainText('Registry enthält den eigenen Mod');
 const extra=panel.locator('details').filter({has:page.locator('summary').filter({hasText:'Zusätzliche'})});await extra.locator('summary').click();await expect(extra).toContainText('bin64/Extra.exe');await expect(extra).not.toContainText('0041/0.paz');
 await expect(panel.getByRole('button',{name:'Dateiinhalte prüfen',exact:true})).toBeDisabled();
});
