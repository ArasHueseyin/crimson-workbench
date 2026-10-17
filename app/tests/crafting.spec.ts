import { expect, test, type Page } from '@playwright/test';
async function fixture(page: Page) {
  await page.addInitScript(() => {
    const root = window as unknown as { isTauri: boolean; __TAURI_INTERNALS__: unknown; craftRequests: Record<string, unknown>[] };
    root.isTauri = true; root.craftRequests = [];
    const item = (key: number, name: string) => ({ key, name, internal_key: `synthetic_${key}` });
    const target = item(1,'Testwerkzeug'), wood = item(2,'Testholz'), other = item(3,'Testast');
    const recipe = { key: 10, internal_key:'Synthetic_recipe', output:target, output_quantity:'3', tool_key:1, knowledge_key:22, condition_keys:[], ingredients:[{ quantity:'2',choices:[wood,other],group:7,group_name:'Synthetic_group',slot:'10:group:0' }], result_dropset:33 };
    let session=0;
    root.__TAURI_INTERNALS__ = { invoke: async (command:string,args:Record<string,unknown> = {}) => {
      if(command==='bootstrap') return { project:'C:/Synthetic',discovery:{installations:[{path:'C:/Synthetic Game',platform:'steam'}],configured_game:'C:/Synthetic Game'},languages:[{language:'ger'},{language:'eng'}] };
      if(command==='open_catalog')return {session:++session,game_path:args.game,info:{item_count:0,types:[],categories:[],tiers:[],stats:[],index:{language:args.language,path:'synthetic',fingerprint:'synthetic'},exe_version:'synthetic'}};
      if(command==='search_items'){
        const query=args.query as {text:string};
        const items=[target,wood,other].filter(item=>String(item.key)===query.text).map(item=>({...item,description:'Synthetic material.',max_stack:'99',item_type:1,category:1,tier:1,stat_keys:[],icon_key:null}));
        return {items,total:items.length,offset:0};
      }
      if(command==='advanced_item')return {item:{key:args.key,max_endurance:100,enchant_levels:[0],stats:[]}};
      if(command==='item_icon')return {data_url:null,reason:'Synthetic test'};
      if(command==='craft_info')return {targets:[target],coverage:{recipe_rows:2,group_rows:1,dropset_rows:1,interpreted_dropsets:1,usable_recipes:1,excluded:{special:1},source_note:'Weltquellen nicht verifiziert.'}};
      if(command==='craft_plan'){
        const r=args.request as { quantity:string;target:number;owned:Record<string,string>;choices:Record<string,number>;acquire:number[] };
        root.craftRequests.push(r);
        if(r.quantity==='4')await new Promise(resolve=>setTimeout(resolve,650));
        const material=r.choices['10:group:0']===3?other:wood;
        const satisfied=r.owned[String(material.key)]==='99';
        const leaf={item:material,requested:'2',from_owned:satisfied?'2':'0',from_surplus:'0',batches:'0',produced:'0',recipe:null,alternatives:[],reason:satisfied?'owned':'no_supported_recipe',children:[]};
        return {root:{item:target,requested:r.quantity,from_owned:'0',from_surplus:'0',batches:'1',produced:'3',recipe,alternatives:[recipe],reason:'craft',children:[leaf]},materials:satisfied?[]:[{item:material,needed:'2',craftable:false,drops:[],vendor_status:'not_verified'}],inventory:[{item:target,owned:'0',used:'0',surplus:'2'},{item:material,owned:r.owned[String(material.key)]??'0',used:satisfied?'2':'0',surplus:'0'}],warnings:[]};
      }
      throw Error(`Unexpected ${command}`);
    } };
  });
  await page.goto('/');
  await expect(page.locator('.workbench')).toHaveAttribute('data-ready','true');
  await page.getByRole('button',{name:'Crafting planner',exact:true}).click();
  await expect(page.getByRole('status')).toContainText('Material requirements calculated');
}
test('target, alternatives, owned stock and source gaps remain connected',async({page})=>{
  await fixture(page);
  await expect(page.locator('[data-material-key="2"]')).toContainText('Testholz');
  await page.getByLabel('Ingredient 10:group:0').selectOption('3');
  await expect(page.locator('[data-material-key="3"]')).toContainText('Testast');
  await page.locator('[data-material-key="3"] summary').click();
  await expect(page.locator('.material-sources')).toContainText('not yet verified');
  await page.getByRole('button',{name:'Open in item picker',exact:true}).click();
  await expect(page.getByRole('heading',{name:'Item picker.'})).toBeVisible();
  await expect(page.getByRole('textbox',{name:'Search item picker'})).toHaveValue('3');
  await expect(page.locator('.picker-hero')).toContainText('Testast');
  await page.getByRole('button',{name:'Crafting planner',exact:true}).click();
  await page.getByLabel('Stock Testast 3').fill('99');
  await expect(page.locator('.craft-satisfied')).toContainText('Everything available');
  await page.getByRole('button',{name:'Item picker',exact:true}).click();
  await page.getByRole('button',{name:'Crafting planner',exact:true}).click();
  await expect(page.getByLabel('Stock Testast 3')).toHaveValue('99');
  await page.getByRole('button',{name:'Clear',exact:true}).click();
  await expect(page.locator('[data-material-key="3"]')).toBeVisible();
});
test('late plan cannot overwrite a newer quantity and invalid quantities hide results',async({page})=>{
  await fixture(page);
  await page.getByLabel('Target quantity').fill('4');await page.waitForTimeout(240);
  await page.getByLabel('Target quantity').fill('5');
  await expect(page.locator('.craft-tree-card>.craft-node>.craft-node-head>b')).toHaveText('5');
  await page.waitForTimeout(800);
  await expect(page.locator('.craft-tree-card>.craft-node>.craft-node-head>b')).toHaveText('5');
  await page.getByLabel('Target quantity').fill('0');
  await expect(page.getByRole('alert')).toContainText('Target quantity');
  await expect(page.locator('.craft-workspace')).toHaveCount(0);
});
test('1024px crafting layout, empty target search and session reset',async({page})=>{
  await page.setViewportSize({width:1024,height:768});await fixture(page);
  expect(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth)).toBe(false);
  await page.getByLabel('Search craftable items').fill('unbekannt');
  await expect(page.getByLabel('Crafting target')).toContainText('No matching recipes');
  await page.getByLabel('Search craftable items').fill('');
  await page.getByLabel('Stock Testholz 2').fill('99');
  await expect(page.locator('.craft-satisfied')).toBeVisible();
  await page.getByLabel('Item language',{exact:true}).selectOption('eng');
  await expect(page.getByLabel('Stock Testholz 2')).toHaveValue('0');
});
