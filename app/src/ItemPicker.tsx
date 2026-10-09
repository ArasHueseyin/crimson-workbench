import { useEffect, useMemo, useRef, useState } from 'react';
import { useVirtualizer } from '@tanstack/react-virtual';
import { Search, X, LoaderCircle, PackagePlus } from 'lucide-react';
import { api, asError } from './api';
import { count, integer, plainText } from './format';
import { emptyQuery } from './store';
import { ItemImage } from './ItemImage';
import type { Catalog, Item } from './types';
import type { AdvancedItem } from './advanced-types';
import { useItemSpawner } from './useItemSpawner';
import { useKnowledge } from './useKnowledge';
import { knowledgeLabel } from './knowledge-types';
import { grantQuantity, maximumNewStacks } from './quantity';

/** The picker is independent of file Live-Apply. Grants require a verified runtime. */
export function ItemPicker({ catalog }: { catalog: Catalog }) {
  const spawner = useItemSpawner(catalog.session, catalog.game_path);
  const knowledge = useKnowledge(catalog.session);
  const [text, setText] = useState(''), [group, setGroup] = useState<number | null>(null);
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
        if (generation.current === id) { setItems(page.items); setTotal(page.total); }
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
  const virtual = useVirtualizer({ count: items.length, getScrollElement: () => scroll.current, estimateSize: () => 96, overscan: 5 });
  const memberships = useMemo(() => selected ? subGroups.filter(g => g.items.includes(selected.key)) : [], [selected, groups]);
  const values = stats?.item.stats.filter(s => s.enchant_level === level) ?? [];
  const amount=grantQuantity(quantity),newStacks=maximumNewStacks(amount,selected?.max_stack??'0');
  function chooseGroup(key: number | null) { setGroup(key); setOffset(0); }
  return <section className="item-picker">
    <div className="page-heading"><div><div className="eyebrow">GEGENSTÄNDE AUSWÄHLEN</div><h1>Item-Auswahl<span>.</span></h1><p>Icons, Beschreibungen und Werte aus deiner Installation.</p></div><PackagePlus size={28} strokeWidth={1.2} /></div>
    <div className="knowledge-bar"><label>Wissensstand<select aria-label="Spielstand für Wissensanzeige" disabled={knowledge.busy} value={knowledge.data?.selected_save??''} onChange={e=>void knowledge.refresh(e.target.value)}>{knowledge.data?.saves.map(s=><option key={s.id} value={s.id}>{s.label} · {new Date(s.modified*1000).toLocaleString('de-AT')}</option>)}</select></label><button className="secondary" disabled={knowledge.busy} onClick={()=>void knowledge.refresh(knowledge.data?.selected_save??null)}>{knowledge.busy?'Wird gelesen …':'Wissen neu einlesen'}</button><small>{knowledge.error?`Wissensstand nicht verfügbar: ${knowledge.error}`:knowledge.data?.message}</small></div>
    <div className="picker-layout"><div className="picker-main">
      <div className="input-wrap main-search"><Search size={18} /><input ref={search} aria-label="Item-Auswahl durchsuchen" maxLength={512} placeholder={regex?'Suchmuster, z. B. (Blitz|Feuer).*pfeil':'Name, Teilwort oder Item-ID suchen …'} value={text} onChange={e => { setText(e.target.value); setOffset(0); }} />{text && <button className="icon-button" aria-label="Item-Suche leeren" onClick={() => { setText(''); setOffset(0); }}><X size={16} /></button>}<kbd>Ctrl K</kbd></div>
      <label className="checkbox-label search-mode"><input aria-label="Regex-Suche" type="checkbox" checked={regex} onChange={e=>{setRegex(e.target.checked);setOffset(0);}}/>Regex-Suche<span>{regex?'Suchmuster verwenden':'Teilwörter finden auch Zusammensetzungen und Mehrzahl.'}</span></label>
      <div className="picker-categories" aria-label="Itemkategorien"><button aria-pressed={group === null} onClick={() => chooseGroup(null)}>Alle</button>{topGroups.map(g => <button key={g.key} aria-pressed={group === g.key} onClick={() => chooseGroup(g.key)}>{plainText(g.name)}</button>)}</div>
      <label className="picker-subcategory">Unterkategorie<select aria-label="Item-Unterkategorie" value={selectedGroup && selectedGroup.order > 5 ? group! : ''} onChange={e => chooseGroup(e.target.value ? Number(e.target.value) : null)}><option value="">Alle Unterkategorien</option>{subGroups.map(g => <option key={g.key} value={g.key}>{plainText(g.name)}</option>)}</select></label>
      {catalog.info.group_error && <p className="muted">Benannte Kategorien sind für diese Spieldaten nicht verfügbar. Die Suche bleibt verfügbar.</p>}
      <div className="result-caption" aria-live="polite"><span>{busy ? 'Suche läuft …' : `${count(total)} Gegenstände`}{selectedGroup && ` · ${plainText(selectedGroup.name)}`}</span><span>Bild & Beschreibung</span></div>
      {error && <p role="alert">{error}</p>}
      <div className="picker-scroll" ref={scroll} aria-busy={busy}>
        <div role="listbox" aria-label="Item-Auswahl" style={{ height: virtual.getTotalSize(), position: 'relative' }}>{virtual.getVirtualItems().map(v => {
          const item = items[v.index]; return <button key={item.key} role="option" aria-selected={selected?.key === item.key} className="picker-row" style={{ position: 'absolute', top: v.start, height: v.size }} onClick={() => { setSelected(item); setQuantity('1'); }}>
            <ItemImage session={catalog.session} itemKey={item.key} /><span className="picker-row-text"><strong>{plainText(item.name)}</strong><span>{plainText(item.description) || 'Keine Beschreibung in den Spieldaten.'}</span><small>ID {item.key} · Stapel {integer(item.max_stack)}{!known.has(item.key) && ' · Ohne Gruppenzuordnung'}{((item.knowledge_keys?.length??0)>0||documents.has(item.key))&&` · ${knowledgeLabel(knowledge.byItem.get(item.key))}`}</small></span>
          </button>;
        })}</div>
        {!items.length && <div className="empty-state">{busy ? <LoaderCircle className="spin" /> : <><h3>Keine passenden Gegenstände</h3><p>Ändere den Suchbegriff oder die Kategorie.</p></>}</div>}
      </div>
      <div className="picker-pages"><button className="secondary" disabled={busy || offset === 0} onClick={() => setOffset(Math.max(0, offset - 200))}>Zurück</button><span>{items.length ? `${count(offset + 1)}–${count(offset + items.length)} von ${count(total)}` : '0 Ergebnisse'}</span><button className="secondary" disabled={busy || offset + items.length >= total} onClick={() => setOffset(offset + 200)}>Weiter</button></div>
    </div><aside className="picker-detail" aria-label="Ausgewähltes Item">
      {selected ? <><div className="picker-hero"><ItemImage session={catalog.session} itemKey={selected.key} label={plainText(selected.name)} large /><h2>{plainText(selected.name)}</h2><span className="muted">ID {selected.key}</span>{memberships.length > 0 && <p className="muted">{memberships.map(g => plainText(g.name)).join(' · ')}</p>}</div>
        <h3>Beschreibung</h3><p className="picker-description">{plainText(selected.description) || 'Keine Beschreibung verfügbar.'}</p>
        {selected.use_restriction&&<p className="item-use-restriction" role="note">{selected.use_restriction}</p>}
        {((selected.knowledge_keys?.length??0)>0||documents.has(selected.key))&&<p className={`knowledge-badge ${knowledge.byItem.get(selected.key)?.state??'unknown'}`}>{knowledgeLabel(knowledge.byItem.get(selected.key))}<small>{selected.knowledge_keys?.length?'Im ausgewählten gespeicherten Spielstand.':'Keine eindeutige Wissensbelohnung in den Itemdaten zugeordnet.'}</small></p>}
        <h3>Werte</h3><dl className="picker-values"><dt>Stapelgröße</dt><dd>{integer(selected.max_stack)}</dd><dt>Tier</dt><dd>{selected.tier}</dd>{stats && <><dt>Haltbarkeit</dt><dd>{stats.item.max_endurance}</dd></>}</dl>
        {stats && stats.item.enchant_levels.length > 1 && <label className="picker-level">Verfeinerungsstufe<select aria-label="Werte für Verfeinerungsstufe" value={level} onChange={e => setLevel(Number(e.target.value))}>{stats.item.enchant_levels.map(l => <option key={l} value={l}>{l}</option>)}</select></label>}
        <dl className="picker-values">{values.map((s, i) => <span className="picker-stat" key={`${s.stat}:${s.list}:${i}`}><dt>{statNames[s.stat] ?? `Stat-ID ${s.stat}`}<small>{s.list === 'maximum' ? 'Maximum' : s.list === 'regeneration' ? 'Regeneration' : s.list === 'per_level' ? 'Pro Stufe' : 'Basis'}</small></dt><dd>{integer(s.value)}</dd></span>)}</dl>
        {stats && !values.length && <p className="muted small">Keine numerischen Werte für diese Stufe hinterlegt.</p>}
        {statsError && <p className="muted small">Zusätzliche Werte sind nicht verfügbar.</p>}
        {!!values.length && <p className="muted small">Tabellenwerte; Ausrüstung, Sockel und Charakterboni sind hier nicht eingerechnet.</p>}
        <label className="picker-quantity">Menge<input aria-label="Gewünschte Itemmenge" type="text" inputMode="numeric" autoComplete="off" spellCheck={false} value={quantity} onFocus={e=>e.currentTarget.select()} onChange={e => setQuantity(e.target.value)} /></label>
        {amount===null?<p className="muted small">Gib eine ganze Zahl von 1 bis 10.000 ein.</p>:newStacks!==null&&<p className="muted small">Laut Katalog höchstens {integer(newStacks)} neue Stapel bei Stapelgröße {integer(selected.max_stack)}. Vorhandene Stapel werden nach Möglichkeit aufgefüllt; Mods können das Stapellimit verändern.</p>}
        <button className="primary picker-grant" disabled={!spawner.ready || amount===null || !!selected.use_restriction} onClick={() => {if(amount!==null&&!selected.use_restriction)void spawner.give(selected.key, amount);}}><PackagePlus size={16} />{spawner.sending ? 'Anfrage senden …' : 'Ins Inventar geben'}</button>
      </> : <div className="empty-state"><PackagePlus size={30} strokeWidth={1.2} /><h3>Wähle einen Gegenstand</h3><p>Hier erscheinen Beschreibung, Werte und die gewünschte Menge.</p></div>}
      <p className="picker-runtime-note" role="status">{spawner.message}</p>
      {spawner.pending && <div className="picker-pending"><p className="muted small">Anfrage: {spawner.pending.quantity} × Item {spawner.pending.key}</p>{spawner.result?.state === 'queued' && <button className="secondary" disabled={spawner.sending} onClick={() => void spawner.cancel()}>Anfrage abbrechen</button>}{(!spawner.result || spawner.result.state === 'uncertain') && <button className="secondary" disabled={spawner.sending} onClick={spawner.acknowledge}>Ergebnis im Spiel geprüft</button>}</div>}
    </aside></div>
  </section>;
}
