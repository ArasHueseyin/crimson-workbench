import { useEffect, useMemo, useRef, useState } from 'react';
import { useVirtualizer } from '@tanstack/react-virtual';
import { Search, X, LoaderCircle, PackagePlus } from 'lucide-react';
import { api, asError } from './api';
import { count, integer, plainText } from './format';
import { emptyQuery, useCatalog } from './store';
import { ItemImage } from './ItemImage';
import type { Catalog, Item } from './types';
import type { AdvancedItem } from './advanced-types';
import { useItemSpawner } from './useItemSpawner';
import { useKnowledge } from './useKnowledge';
import { knowledgeLabel } from './knowledge-types';
import { grantQuantity, maximumNewStacks } from './quantity';
import {SaveSelector,SaveTarget} from './SaveSelector';

/** The picker is independent of file Live-Apply. Grants require a verified runtime. */
export function ItemPicker({ catalog }: { catalog: Catalog }) {
  const requestedItem=useCatalog(s=>s.selected);
  const spawner = useItemSpawner(catalog.session, catalog.game_path);
  const knowledge = useKnowledge(catalog.session);
  const [text, setText] = useState(requestedItem===null?'':String(requestedItem)), [group, setGroup] = useState<number | null>(null);
  const [regex,setRegex]=useState(false);
  const [items, setItems] = useState<Item[]>([]), [total, setTotal] = useState(0), [offset, setOffset] = useState(0);
  const [busy, setBusy] = useState(false), [error, setError] = useState('');
  const [selected, setSelected] = useState<Item | null>(null), [quantity, setQuantity] = useState('1');
  const [stats, setStats] = useState<AdvancedItem | null>(null), [statsError, setStatsError] = useState('');
  const [level, setLevel] = useState(0), [statNames, setStatNames] = useState<Record<number, string>>({});
  const scroll = useRef<HTMLDivElement>(null), search = useRef<HTMLInputElement>(null), generation = useRef(0);
  const groups = catalog.info.groups ?? [], known = new Set(groups.flatMap(g => g.items));
  const documents = useMemo(() => new Set(groups.filter(g => g.internal_key === 'ItemGroup_Category_Document').flatMap(g => g.items)), [groups]);
  const topGroups = groups.filter(g => g.order <= 5), subGroups = groups.filter(g => g.order > 5);
  const selectedGroup = groups.find(g => g.key === group);
  useEffect(() => {
    if(requestedItem===null)return;
    setText(String(requestedItem));setGroup(null);setRegex(false);setOffset(0);setSelected(null);
  }, [requestedItem]);
  useEffect(() => {
    const listener = (e: KeyboardEvent) => { if ((e.ctrlKey || e.metaKey) && e.key === 'k') { e.preventDefault(); search.current?.focus(); } };
    window.addEventListener('keydown', listener); return () => window.removeEventListener('keydown', listener);
  }, []);
  useEffect(() => {
    let active = true;
    api.modInfo(catalog.session).then(info => { if (active) setStatNames(Object.fromEntries((info.advanced?.statuses ?? []).map(s => [s.key, s.name]))); }).catch(() => {});
    return () => { active = false; };
  }, [catalog.session]);
  useEffect(() => {
    const id = ++generation.current; setBusy(true); setError(''); setItems([]); setTotal(0);
    scroll.current?.scrollTo({ top: 0 });
    const timer = setTimeout(() => {
      api.search(catalog.session, { ...emptyQuery, text, regex, group, offset }).then(page => {
        if (generation.current === id) {
          setItems(page.items);setTotal(page.total);
          if(requestedItem!==null&&text===String(requestedItem)) {
            const linked=page.items.find(item=>item.key===requestedItem);
            if(linked){setSelected(linked);setQuantity('1');useCatalog.getState().select(null)}
          }
        }
      }).catch(e => { if (generation.current === id) setError(asError(e).message); })
        .finally(() => { if (generation.current === id) setBusy(false); });
    }, text ? 240 : 0);
    return () => { clearTimeout(timer); generation.current++; };
  }, [catalog.session, text, regex, group, offset]);
  useEffect(() => {
    let active = true; setStats(null); setStatsError(''); setLevel(0);
    if (selected) api.advancedItem(catalog.session, selected.key).then(value => {
      if (active) { setStats(value); setLevel(value.item.enchant_levels[0] ?? 0); }
    }).catch(e => { if (active) setStatsError(asError(e).message); });
    return () => { active = false; };
  }, [catalog.session, selected?.key]);
  const virtual = useVirtualizer({ count: items.length, getScrollElement: () => scroll.current, estimateSize: () => 80, overscan: 5 });
  const memberships = useMemo(() => selected ? subGroups.filter(g => g.items.includes(selected.key)) : [], [selected, groups]);
  const values = stats?.item.stats.filter(s => s.enchant_level === level) ?? [];
  const amount=grantQuantity(quantity),newStacks=maximumNewStacks(amount,selected?.max_stack??'0');
  function chooseGroup(key: number | null) { setGroup(key); setOffset(0); }
  return <section className="item-picker compact-item-picker">
    <div className="page-heading"><div><div className="eyebrow">CHOOSE ITEMS</div><h1>Item picker<span>.</span></h1><p>Icons, descriptions and values from your installation.</p></div><PackagePlus size={28} strokeWidth={1.2} /></div>
    <details className="knowledge-bar knowledge-drawer"><summary>Knowledge from saved game <span>{knowledge.data?.selected_save?'Save selected':'Optional'}</span></summary><div className="knowledge-controls"><SaveSelector label="Knowledge status" saves={knowledge.data?.saves??[]} value={knowledge.data?.selected_save??null} ariaLabel="Save for knowledge display" disabled={knowledge.busy} onChange={value=>void knowledge.refresh(value)}/><button className="secondary" disabled={knowledge.busy} onClick={()=>void knowledge.refresh(knowledge.data?.selected_save??null)}>{knowledge.busy?'Reading …':'Reload knowledge'}</button><small>{knowledge.error?`Knowledge status unavailable: ${knowledge.error}`:knowledge.data?.message}</small></div></details>
    <div className="picker-layout"><div className="picker-main">
      <div className="input-wrap main-search"><Search size={18} /><input ref={search} aria-label="Search item picker" maxLength={512} placeholder={regex?'Pattern, e.g. (lightning|fire).*arrow':'Search name, partial word or item ID …'} value={text} onChange={e => { setText(e.target.value); setOffset(0); }} />{text && <button className="icon-button" aria-label="Clear item search" onClick={() => { setText(''); setOffset(0); }}><X size={16} /></button>}<kbd>Ctrl K</kbd></div>
      <label className="checkbox-label search-mode"><input aria-label="Regex search" type="checkbox" checked={regex} onChange={e=>{setRegex(e.target.checked);setOffset(0);}}/>Regex search<span>{regex?'Use a search pattern':'Partial words also match compounds and plurals.'}</span></label>
      <div className="picker-categories" aria-label="Item categories"><button aria-pressed={group === null} onClick={() => chooseGroup(null)}>All</button>{topGroups.map(g => <button key={g.key} aria-pressed={group === g.key} onClick={() => chooseGroup(g.key)}>{plainText(g.name)}</button>)}</div>
      <label className="picker-subcategory">Subcategory<select aria-label="Item subcategory" value={selectedGroup && selectedGroup.order > 5 ? group! : ''} onChange={e => chooseGroup(e.target.value ? Number(e.target.value) : null)}><option value="">All subcategories</option>{subGroups.map(g => <option key={g.key} value={g.key}>{plainText(g.name)}</option>)}</select></label>
      {catalog.info.group_error && <p className="muted">Named categories are unavailable for this game data. Search remains available.</p>}
      <div className="result-caption" aria-live="polite"><span>{busy ? 'Searching …' : `${count(total)} Items`}{selectedGroup && ` · ${plainText(selectedGroup.name)}`}</span><span>Image & description</span></div>
      {error && <p role="alert">{error}</p>}
      <div className="picker-scroll" ref={scroll} aria-busy={busy}>
        <div role="listbox" aria-label="Item picker" style={{ height: virtual.getTotalSize(), position: 'relative' }}>{virtual.getVirtualItems().map(v => {
          const item = items[v.index]; return <button key={item.key} role="option" aria-selected={selected?.key === item.key} className="picker-row" style={{ position: 'absolute', top: v.start, height: v.size }} onClick={() => { setSelected(item); setQuantity('1'); }}>
            <ItemImage session={catalog.session} itemKey={item.key} /><span className="picker-row-text"><strong>{plainText(item.name)}</strong><span>{plainText(item.description) || 'No description in the game data.'}</span><small>ID {item.key} · Stack {integer(item.max_stack)}{!known.has(item.key) && ' · No group assigned'}{((item.knowledge_keys?.length??0)>0||documents.has(item.key))&&` · ${knowledgeLabel(knowledge.byItem.get(item.key))}`}</small></span>
          </button>;
        })}</div>
        {!items.length && <div className="empty-state">{busy ? <LoaderCircle className="spin" /> : <><h3>No matching items</h3><p>Change the search term or category.</p></>}</div>}
      </div>
      <div className="picker-pages"><button className="secondary" disabled={busy || offset === 0} onClick={() => setOffset(Math.max(0, offset - 200))}>Back</button><span>{items.length ? `${count(offset + 1)}–${count(offset + items.length)} of ${count(total)}` : '0 results'}</span><button className="secondary" disabled={busy || offset + items.length >= total} onClick={() => setOffset(offset + 200)}>Next</button></div>
    </div><aside className="picker-detail" aria-label="Selected item">
      {selected ? <><div className="picker-hero"><ItemImage session={catalog.session} itemKey={selected.key} label={plainText(selected.name)} large /><h2>{plainText(selected.name)}</h2><span className="muted">ID {selected.key}</span>{memberships.length > 0 && <p className="muted">{memberships.map(g => plainText(g.name)).join(' · ')}</p>}</div>
        <details className="picker-more"><summary>Item details &amp; values</summary><h3>Description</h3><p className="picker-description">{plainText(selected.description) || 'No description available.'}</p>
        <h3>Values</h3><dl className="picker-values"><dt>Stack size</dt><dd>{integer(selected.max_stack)}</dd><dt>Tier</dt><dd>{selected.tier}</dd>{stats && <><dt>Durability</dt><dd>{stats.item.max_endurance}</dd></>}</dl>
        {stats && stats.item.enchant_levels.length > 1 && <label className="picker-level">Refinement level<select aria-label="Values for refinement level" value={level} onChange={e => setLevel(Number(e.target.value))}>{stats.item.enchant_levels.map(l => <option key={l} value={l}>{l}</option>)}</select></label>}
        <dl className="picker-values">{values.map((s, i) => <span className="picker-stat" key={`${s.stat}:${s.list}:${i}`}><dt>{statNames[s.stat] ?? `Stat ID ${s.stat}`}<small>{s.list === 'maximum' ? 'Maximum' : s.list === 'regeneration' ? 'Regeneration' : s.list === 'per_level' ? 'Per level' : 'Base'}</small></dt><dd>{integer(s.value)}</dd></span>)}</dl>
        {stats && !values.length && <p className="muted small">No numeric values for this level.</p>}
        {statsError && <p className="muted small">Additional values are unavailable.</p>}
        {!!values.length && <p className="muted small">Table values; equipment, sockets and character bonuses are not included.</p>}
        </details>
        {selected.use_restriction&&<p className="item-use-restriction" role="note">{selected.use_restriction}</p>}
        {((selected.knowledge_keys?.length??0)>0||documents.has(selected.key))&&<div className="selected-knowledge"><SaveTarget saves={knowledge.data?.saves??[]} value={knowledge.data?.selected_save??null} label="Knowledge shown for"/><p className={`knowledge-badge ${knowledge.byItem.get(selected.key)?.state??'unknown'}`}>{knowledgeLabel(knowledge.byItem.get(selected.key))}<small>{selected.knowledge_keys?.length?'In the selected saved game.':'No unique knowledge reward assigned in the item data.'}</small></p></div>}
        <label className="picker-quantity">Quantity<input aria-label="Desired item quantity" type="text" inputMode="numeric" autoComplete="off" spellCheck={false} value={quantity} onFocus={e=>e.currentTarget.select()} onChange={e => setQuantity(e.target.value)} /></label>
        {amount===null?<p className="muted small">Enter a whole number from 1 to 10,000.</p>:newStacks!==null&&<p className="muted small">According to the catalog, at most {integer(newStacks)} new stacks with a stack size of {integer(selected.max_stack)}. Existing stacks are filled where possible; mods may change the stack limit.</p>}
        <button className="primary picker-grant" disabled={!spawner.ready || amount===null || !!selected.use_restriction} onClick={() => {if(amount!==null&&!selected.use_restriction)void spawner.give(selected.key, amount);}}><PackagePlus size={16} />{spawner.sending ? 'Sending request …' : 'Add to inventory'}</button>
      </> : <div className="empty-state"><PackagePlus size={30} strokeWidth={1.2} /><h3>Choose an item</h3><p>Description, values and the desired quantity appear here.</p></div>}
      {catalog.read_only && <div className="picker-pending"><button className="secondary" disabled={spawner.launching || spawner.sending || !!spawner.pending || spawner.connected} onClick={() => void spawner.launch()}>{spawner.launching ? 'Backing up & launching …' : 'Launch game with item spawning'}</button><p className="muted small">Exit the game first and keep Steam open. Saves are backed up before launch. Mac spawning is experimental; ordinary inventory items are supported.</p>{spawner.backup && <p className="muted small" role="status">Verified save backup: {spawner.backup}</p>}</div>}
      <p className="picker-runtime-note" role="status">{spawner.message}</p>
      {spawner.pending && <div className="picker-pending"><p className="muted small">Request: {spawner.pending.quantity} × Item {spawner.pending.key}</p>{(!spawner.result || spawner.result.state === 'queued') && <button className="secondary" disabled={spawner.sending} onClick={() => void spawner.cancel()}>Cancel request</button>}{spawner.result?.state === 'uncertain' && <button className="secondary" disabled={spawner.sending} onClick={spawner.acknowledge}>Result checked in game</button>}</div>}
    </aside></div>
  </section>;
}
