import { test,expect,type Page } from '@playwright/test';
import type { AuditSnapshot } from '../src/mod-types';
type Harness={running:boolean|null;job:AuditSnapshot|null;calls:string[]};
async function fixture(page:Page,running:boolean|null=false){
 await page.addInitScript(({running})=>{
  const root=window as unknown as {isTauri:boolean;__TAURI_INTERNALS__:unknown;audit:Harness};root.isTauri=true;root.audit={running,job:null,calls:[]};
  root.__TAURI_INTERNALS__={invoke:async(command:string,args:Record<string,unknown>={})=>{
   root.audit.calls.push(command);
   if(command==='bootstrap')return{project:'C:/Synthetic',discovery:{installations:[{path:'C:/Synthetic Game',platform:'steam'}],configured_game:'C:/Synthetic Game'},languages:[{language:'ger'}]};
   if(command==='open_catalog')return{session:1,game_path:args.game,info:{item_count:0,types:[],categories:[],tiers:[],stats:[],index:{language:'ger',path:'synthetic',fingerprint:'synthetic'},exe_version:'synthetic'}};
   if(command==='search_items')return{items:[],total:0,offset:0};
   if(command==='mod_info')return{vendors:[],dropsets:[],store_rows:0,stock_rows:0,opaque_stores:0,opaque_dropsets:0,quantity_excluded:0,trust_rows:0,limitations:[],items:[],append_vendors:[],daily_vendors:[],chance_dropsets:[],guarantee_dropsets:[]};
   if(command==='installation_check')return{version:1,mode:'metadata_inventory',game_path:'C:/Synthetic',observed_at:1789850000,build_id:'synthetic',game_running:root.audit.running,directory_scan_complete:true,depot_comparison_available:true,content_verified:false,certified_vanilla:false,can_apply:false,expected_files:1,actual_files:1,expected_bytes:'1073741824',registry_sha256:'synthetic',registry_matches_observed_build:true,depots:[],executables:[],groups:[],files:[{path:'0000/0.paz',state:'size_matches'}],issues:[],limitations:[]};
   if(command==='installation_audit_status')return structuredClone(root.audit.job);
   if(command==='installation_audit_start'){
    root.audit.job={id:1,session:1,phase:'hashing',progress:{phase:'hashing',files_done:0,total_files:1,bytes_done:'536870912',total_bytes:'1073741824',current_path:'0000/0.paz'},error:null,report:null};return structuredClone(root.audit.job);
   }
   if(command==='installation_audit_cancel'){if(root.audit.job){root.audit.job.phase='cancelled';root.audit.job.error='Content audit cancelled';}return null;}
   if(command==='installation_audit_export')return'C:/Synthetic/exports/installation-audit.json';
   throw Error(`Unexpected ${command}`);
  }};
 },{running});
 await page.goto('/');await expect(page.locator('.workbench')).toHaveAttribute('data-ready','true');await page.getByRole('button',{name:'Mod workshop',exact:true}).click();
}
test('content audit requires inventory and a stopped game; running and unknown stay disabled',async({page})=>{
 await fixture(page,true);const button=page.getByRole('button',{name:'Check file contents',exact:true});await expect(button).toBeDisabled();
 await page.getByRole('button',{name:'Check file list',exact:true}).click();await expect(page.locator('.content-audit')).toContainText('Game running or status unknown');await expect(button).toBeDisabled();
 await page.evaluate(()=>(window as unknown as {audit:Harness}).audit.running=null);await page.getByRole('button',{name:'Check file list',exact:true}).click();await expect(button).toBeDisabled();
 expect(await page.evaluate(()=>(window as unknown as {audit:Harness}).audit.calls.filter(c=>c==='installation_audit_start'))).toHaveLength(0);
});
test('progress survives navigation and cancellation creates no report',async({page})=>{
 await fixture(page);await page.getByRole('button',{name:'Check file list',exact:true}).click();await page.getByRole('button',{name:'Check file contents',exact:true}).click();
 await expect(page.getByRole('progressbar',{name:'Content audit progress'})).toHaveAttribute('value','50');await expect(page.getByRole('button',{name:'Check file contents',exact:true})).toBeDisabled();
 await page.getByRole('button',{name:'Item database',exact:false}).click();await page.getByRole('button',{name:'Mod workshop',exact:true}).click();await expect(page.getByRole('progressbar')).toBeVisible();
 await page.getByRole('button',{name:'Cancel content audit',exact:true}).click();await expect(page.locator('.content-audit [role=alert]')).toContainText('cancelled');await expect(page.getByRole('button',{name:'Export audit report'})).toHaveCount(0);
});
test('completed mismatches are visible and exportable without enabling live apply',async({page})=>{
 await fixture(page);await page.getByRole('button',{name:'Check file list',exact:true}).click();await page.getByRole('button',{name:'Check file contents',exact:true}).click();
 await page.evaluate(()=>{const job=(window as unknown as {audit:Harness}).audit.job!;job.phase='complete';job.report={version:1,kind:'cache-content-audit',game_path:'C:/Synthetic',build_id:'synthetic',started_at:1,finished_at:2,all_files_match_cache:false,metadata_stable:true,certified_vanilla:false,can_apply:false,manifest_sha256:[],files:[{path:'0000/0.paz',bytes:'1073741824',expected_sha1:'expected',sha1:'changed',sha256:'observed',matches_cache:false}],limitations:['Lokaler Cache bestätigt kein Vanilla.']};});
 await expect(page.locator('.audit-result')).toContainText('Content differences found');await expect(page.locator('.audit-result')).toContainText('0000/0.paz');await expect(page.locator('.audit-result')).toContainText('Live apply remains blocked');
 await page.getByRole('button',{name:'Export audit report'}).click();await expect(page.locator('.audit-export')).toContainText('installation-audit.json');
 await page.setViewportSize({width:1024,height:768});expect(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth)).toBe(true);
});
test('game starting or audit failure never displays a completed report',async({page})=>{
 await fixture(page);await page.getByRole('button',{name:'Check file list',exact:true}).click();await page.getByRole('button',{name:'Check file contents',exact:true}).click();
 await page.evaluate(()=>{const job=(window as unknown as {audit:Harness}).audit.job!;job.phase='failed';job.error='Crimson Desert is running. Content audit cancelled.';});
 await expect(page.locator('.content-audit [role=alert]')).toContainText('Crimson Desert is running');await expect(page.locator('.audit-result')).toHaveCount(0);await expect(page.getByRole('button',{name:'Export audit report'})).toHaveCount(0);
});
