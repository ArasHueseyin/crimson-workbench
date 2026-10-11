import { useEffect, useMemo, useRef, useState } from 'react';
import { ArrowUpRight, Check, ChevronRight, Hammer, Layers3, LoaderCircle, PackageCheck, Search, ShoppingBasket } from 'lucide-react';
import { api, asError } from './api';
import { useCatalog, useCraft } from './store';
import { count, integer, plainText } from './format';
import { ItemImage } from './ItemImage';
import type { CraftInfo, CraftItem, CraftNode, CraftPlan, Recipe } from './craft-types';
import './crafting.css';

export function Crafting({ session }: { session: number }) {
  const [info, setInfo] = useState<CraftInfo | null>(null), [plan, setPlan] = useState<CraftPlan | null>(null);
  const [error, setError] = useState(''), [busy, setBusy] = useState(false), [search, setSearch] = useState('');
  const state = useCraft(), generation = useRef(0);
  useEffect(() => {
    let active = true; setInfo(null); setPlan(null); setError('');
    api.craftInfo(session).then(value => { if (!active) return; setInfo(value); const selected = useCraft.getState().target; if (!value.targets.some(t => t.key === selected)) useCraft.getState().set({ target: value.targets.find(t => t.key === 50001)?.key ?? value.targets[0]?.key ?? null }); }).catch(e => { if (active) setError(asError(e).message); });
    return () => { active = false; };
  }, [session]);
  const { target, quantity, owned, recipes, choices, acquire } = state;
  useEffect(() => {
    const id = ++generation.current;
    if (!info || target === null) return;
    if (!quantity || BigInt(quantity) < 1n || BigInt(quantity) > 1_000_000_000_000n) { setError('Target quantity must be a whole number from 1 to 1,000,000,000,000.'); setPlan(null); setBusy(false); return; }
    setBusy(true); setError('');
    const timer = setTimeout(() => api.craftPlan(session, { target, quantity, owned: Object.fromEntries(Object.entries(owned).map(([k,v]) => [k,v || '0'])), recipes, choices, acquire }).then(result => { if (id === generation.current) setPlan(result); }).catch(e => { if (id === generation.current) { setError(asError(e).message); setPlan(null); } }).finally(() => { if (id === generation.current) setBusy(false); }), 180);
    return () => { clearTimeout(timer); generation.current++; };
  }, [session, info, target, quantity, owned, recipes, choices, acquire]);
  const matches = useMemo(() => info?.targets.filter(t => `${t.name} ${t.internal_key} ${t.key}`.toLocaleLowerCase().includes(search.toLocaleLowerCase())) ?? [], [info, search]);
  const item = info?.targets.find(t => t.key === target);
  return <div className="crafting-page">
    <div className="page-heading"><div><div className="eyebrow">FROM MATERIALS TO ITEMS</div><h1>Crafting planner<span>.</span></h1><p>Choose your goal. Plan every step and keep track of your materials.</p></div><span className="craft-count"><Hammer size={17} />{info ? `${count(info.coverage.usable_recipes)} Recipes` : 'Reading recipes …'}</span></div>
    {error && <div className="craft-error" role="alert">{error}</div>}
    {!info ? !error && <div className="loading-workspace"><LoaderCircle className="spin" /><p>Checking recipe tables and material groups …</p></div> : <>
      <div className="craft-target-card"><div className="craft-target-search"><label className="input-wrap"><Search size={17} /><input aria-label="Search craftable items" value={search} onChange={e => setSearch(e.target.value)} placeholder="Search craftable items …" /></label><select aria-label="Crafting target" value={matches.some(t => t.key === target) ? target! : ''} onChange={e => { state.set({ target: Number(e.target.value) }); setSearch(''); }}><option value="" disabled>{matches.length ? 'Select target' : 'No matching recipes'}</option>{matches.map(t => <option key={t.key} value={t.key}>{plainText(t.name)} · #{t.key}</option>)}</select></div>
        <div className="craft-target-picked">{item && <><ItemImage session={session} itemKey={item.key} /><div><small>YOUR TARGET</small><strong>{plainText(item.name)}</strong><span>#{item.key} · {item.internal_key}</span></div></>}</div>
        <label className="craft-quantity">Desired quantity<input aria-label="Target quantity" inputMode="numeric" value={quantity} maxLength={13} onChange={e => { if (/^\d*$/.test(e.target.value)) state.set({ quantity: e.target.value }); }} /></label>
      </div>
      <div className="craft-status" role="status">{busy ? <><LoaderCircle size={14} className="spin" />Calculating plan …</> : plan ? <><Check size={15} />Material requirements calculated <span>· Whole crafting batches · Stock and surplus included</span></> : null}</div>
      {plan && <div className={`craft-workspace ${busy ? 'craft-updating' : ''}`} aria-busy={busy}>
        <section className="craft-tree-card"><div className="craft-section-heading"><Layers3 size={19} /><div><h2>Material tree</h2><p>Choose recipes and interchangeable ingredients.</p></div></div>{plan.warnings.map(w => <p key={w} className="craft-warning">{w}</p>)}<Tree node={plan.root} session={session} depth={0} />
          <p className="craft-footnote">“No calculable recipe” does not rule out other crafting methods in game. Knowledge and unlock requirements are shown but are not checked against a save.</p>
        </section>
        <div className="craft-side"><section className="craft-material-card"><div className="craft-section-heading"><ShoppingBasket size={20} /><div><h2>Still needed</h2><p>Combined requirements after subtracting your stock</p></div><span className="craft-badge">{plan.materials.length}</span></div>
          {!plan.materials.length ? <div className="craft-satisfied"><PackageCheck size={26} /><strong>Everything available</strong><span>No additional materials are needed for this plan.</span></div> : <div className="craft-materials">{plan.materials.map(m => <details key={m.item.key} className="craft-material" data-material-key={m.item.key}><summary><ItemImage session={session} itemKey={m.item.key} /><div><strong>{plainText(m.item.name)}</strong><small>{m.craftable ? 'Calculable recipe available' : 'No calculable recipe'}</small></div><b>{integer(m.needed)}</b><ChevronRight size={14} /></summary><div className="material-sources"><ItemLink item={m.item} /><p>Vendor, price, stock and region: not yet verified.</p><strong>Referenced drop sets</strong><p>World source not mapped yet. Table rates are not verified drop chances.</p>{!m.drops.length ? <p>No interpretable drop set reference. Other sources may exist.</p> : m.drops.map((d,i) => <div className="drop-source" key={`${d.key}:${i}`}><code>{d.internal_key}</code><span>Quantity {integer(d.minimum)}–{integer(d.maximum)} · {d.chance_percent === null ? 'Unknown chance' : `${d.chance_percent} % within this drop set`}</span><small>Rate {integer(d.rate)} / {integer(d.total_rate)} · Roll type {d.roll_type} · #{d.key}</small></div>)}</div></details>)}</div>}
        </section>
        <section className="craft-inventory-card"><div className="craft-section-heading"><PackageCheck size={20} /><div><h2>Already available</h2><p>Shared stock for the entire plan</p></div><button className="text-button" onClick={() => state.set({ owned: {} })}>Clear</button></div><div className="craft-inventory">{[...plan.inventory].sort((a,b) => a.item.name.localeCompare(b.item.name)).map(row => <label className="owned-row" key={row.item.key}><div><strong>{plainText(row.item.name)}</strong><small>Used: {integer(row.used)}{row.surplus !== '0' && ` · Crafting surplus: ${integer(row.surplus)}`}</small></div><input aria-label={`Stock ${plainText(row.item.name)} ${row.item.key}`} inputMode="numeric" maxLength={20} value={owned[row.item.key] ?? '0'} onChange={e => { if (/^\d*$/.test(e.target.value)) state.set({ owned: { ...owned, [row.item.key]: e.target.value } }); }} /></label>)}</div><p className="craft-footnote">Manual quantities are kept for this session. No automatic save import.</p></section></div>
      </div>}
      <details className="craft-coverage"><summary>Which recipes and sources are included?</summary><p>{info.coverage.source_note}</p><p>{count(info.coverage.recipe_rows)} recipe records and {count(info.coverage.group_rows)} material groups checked. {count(info.coverage.usable_recipes)} recipes can be calculated. Upgrades, random outcomes and unresolved special cases are excluded.</p><p>Choose one material type per ingredient group. The plan does not optimize purchase prices or combinations of different alternatives.</p></details>
    </>}
  </div>;
}
function Tree({ node, session, depth }: { node: CraftNode; session: number; depth: number }) {
  const state = useCraft();
  return <div className="craft-node" data-craft-item={node.item.key}><div className="craft-node-head"><ItemImage session={session} itemKey={node.item.key} /><div><strong>{plainText(node.item.name)}</strong><small>{node.reason === 'craft' ? `${integer(node.batches)} batch(es) → ${integer(node.produced)} units` : node.reason === 'owned' ? 'Covered by stock or surplus' : node.reason === 'cycle' ? 'Recipe cycle · obtain manually' : node.reason === 'acquire' ? 'Obtain manually' : 'No calculable recipe'}{node.from_owned !== '0' && ` · Stock: ${integer(node.from_owned)}`}{node.from_surplus !== '0' && ` · Surplus: ${integer(node.from_surplus)}`}</small></div><b>{integer(node.requested)}</b></div>
    {(node.recipe || node.alternatives.length > 0) && <div className="craft-node-controls">{node.alternatives.length > 1 && <label>Recipe<select aria-label={`Recipe for ${node.item.key}`} value={state.recipes[node.item.key] ?? node.recipe?.key ?? node.alternatives[0].key} onChange={e => state.set({ recipes: { ...state.recipes, [node.item.key]: Number(e.target.value) } })}>{node.alternatives.map(r => <option key={r.key} value={r.key}>{r.internal_key} · #{r.key} · {r.output_quantity} units</option>)}</select></label>}<button className="text-button" onClick={() => state.set({ acquire: state.acquire.includes(node.item.key) ? state.acquire.filter(k => k !== node.item.key) : [...state.acquire,node.item.key] })}>{state.acquire.includes(node.item.key) ? 'Craft it' : 'Obtain instead'}</button></div>}
    {node.recipe && <><details className="recipe-requirements"><summary>Tool and requirements</summary><span>Tool ID {node.recipe.tool_key} · Knowledge ID {node.recipe.knowledge_key || 'none'}</span>{node.recipe.condition_keys.length > 0 && <span>Conditions: {node.recipe.condition_keys.join(', ')}</span>}<code>{node.recipe.internal_key} · #{node.recipe.key}</code></details>{node.recipe.ingredients.filter(i => i.choices.length > 1).map(ingredient => <label className="ingredient-choice" key={ingredient.slot}>{ingredient.group_name?.replace('ItemGroup_', '')} · {ingredient.quantity} per batch<select aria-label={`Ingredient ${ingredient.slot}`} value={state.choices[ingredient.slot] ?? ingredient.choices[0].key} onChange={e => state.set({ choices: { ...state.choices, [ingredient.slot]: Number(e.target.value) } })}>{ingredient.choices.map(choice => <option key={choice.key} value={choice.key}>{plainText(choice.name)} · #{choice.key}</option>)}</select></label>)}</>}
    {node.children.length > 0 && <details className="craft-children" open={depth < 2}><summary>{node.children.length} Ingredients <ChevronRight size={13} /></summary><div>{node.children.map((child,i) => <Tree key={`${i}:${child.item.key}`} node={child} session={session} depth={depth + 1} />)}</div></details>}
  </div>;
}
function ItemLink({ item }: { item: CraftItem }) { return <button className="text-button" onClick={() => { useCatalog.getState().select(item.key); useCatalog.getState().setView('items'); }}>Open item details <ArrowUpRight size={13} /></button>; }
export function RecipeLinks({ session, itemKey }: { session: number; itemKey: number }) {
  const [data,setData] = useState<{ recipes: Recipe[]; used_in: Recipe[] } | null>(null), [error,setError] = useState('');
  useEffect(() => { let active=true;setData(null);setError('');api.craftItem(session,itemKey).then(d => { if(active)setData(d); }).catch(e => { if(active)setError(asError(e).message); });return () => {active=false;}; },[session,itemKey]);
  const open = (r: Recipe) => { const s=useCraft.getState();s.set({ target:r.output.key, recipes:{...s.recipes,[r.output.key]:r.key} });useCatalog.getState().setView('crafting'); };
  return <div className="detail-section"><span className="eyebrow">CRAFTING RECIPES</span>{error ? <p role="alert">{error}</p> : !data ? <p>Reading recipe links …</p> : <><p>{data.recipes.length} calculable recipes · Ingredient in {data.used_in.length} recipes</p>{data.recipes.map(r => <button className="reference-link" key={r.key} onClick={() => open(r)}><span><strong>Craft: {plainText(r.output.name)}</strong><code>{r.internal_key}</code></span><ArrowUpRight size={16} /></button>)}<details><summary>Used as an ingredient</summary>{data.used_in.map(r => <button className="reference-link" key={r.key} onClick={() => open(r)}><span><strong>{plainText(r.output.name)}</strong><code>{r.internal_key}</code></span><ArrowUpRight size={16} /></button>)}</details></>}</div>;
}
