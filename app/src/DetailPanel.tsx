import { useEffect, useMemo, useRef, useState } from 'react';
import { useVirtualizer } from '@tanstack/react-virtual';
import { Box, Download, ArrowUpRight, LoaderCircle, Search, Info, X, Check, Layers3 } from 'lucide-react';
import { api, asError } from './api';
import { ItemImage } from './ItemImage';
import { RecipeLinks } from './Crafting';
import { count, fieldValue, filterFields, hex, integer, plainText } from './format';
import { useCatalog } from './store';
import type { Catalog, Detail, RawField } from './types';

export function DetailPanel({ catalog }: { catalog: Catalog }) {
  const key = useCatalog(s => s.selected), select = useCatalog(s => s.select);
  const [detail, setDetail] = useState<Detail | null>(null), [error, setError] = useState('');
  const [tab, setTab] = useState<'overview' | 'fields' | 'links'>('overview');
  const [exporting, setExporting] = useState(false), [exported, setExported] = useState('');
  const current = useRef('');
  useEffect(() => {
    let active = true; current.current = `${catalog.session}:${key}`;
    setDetail(null); setError(''); setExported(''); setExporting(false);
    if (key !== null) api.detail(catalog.session, key).then(d => { if (active) setDetail(d); }).catch(e => { if (active) setError(asError(e).message); });
    return () => { active = false; };
  }, [catalog.session, key]);
  async function exportJson() {
    if (key === null) return;
    const token = current.current; setExporting(true); setExported('');
    try { const path = await api.export(catalog.session, key); if (current.current === token) setExported(path); }
    catch (e) { if (current.current === token) setError(asError(e).message); }
    finally { if (current.current === token) setExporting(false); }
  }
  return <aside className="detail-panel" aria-label="Item details">
    <div className="detail-label"><span>ITEM IN DETAIL</span>{key !== null && <button className="icon-button" aria-label="Close details" onClick={() => select(null)}><X size={16} /></button>}</div>
    {key === null ? <div className="detail-empty"><div className="empty-emblem"><Box size={40} strokeWidth={1} /></div><h2>A closer look.</h2><p>Select an item to see its values, references and raw fields.</p><span className="eyebrow">ALL VALUES · READ ONLY</span></div> : !detail ? <div className="empty-state">{error ? <><Info /><h3>Details unavailable</h3><p className="error-text">{error}</p></> : <><LoaderCircle className="spin" /><p>Reading item …</p></>}</div> : <>
      <div className="detail-hero"><ItemImage session={catalog.session} itemKey={key} label={plainText(detail.name)} large /><span className="detail-id mono">ITEM #{key}</span><h2>{plainText(detail.name)}</h2><div className="internal-key mono">{detail.record.string_key}</div><div className="detail-tags"><span>Type {detail.record.item_type}</span><span>Category {detail.record.category_info}</span><span>Tier {detail.record.item_tier}</span></div></div>
      <div className="detail-tabs" role="tablist" aria-label="Detail sections">{([['overview', 'Overview'], ['fields', 'All fields'], ['links', 'References']] as const).map(([id, label]) => <button role="tab" key={id} aria-selected={tab === id} onClick={() => setTab(id)}>{label}{id === 'fields' && <small>{detail.fields.length}</small>}</button>)}</div>
      <div className={`detail-content ${tab === 'fields' ? 'fields-content' : ''}`} role="tabpanel">
        {tab === 'overview' && <>
          <div className="detail-section"><span className="eyebrow">DESCRIPTION</span><p className="description">{plainText(detail.description) || 'No description is available for this item.'}</p></div>
          <div className="values-grid"><div><span>Max. stack size</span><strong>{integer(detail.record.max_stack_count)}</strong></div><div><span>Tier</span><strong>{detail.record.item_tier}</strong></div><div><span>Category ID</span><strong>{detail.record.category_info}</strong></div><div><span>Type ID</span><strong>{detail.record.item_type}</strong></div></div>
          <div className="detail-section"><span className="eyebrow">STAT REFERENCES</span>{detail.record.stat_keys.length ? <><div className="stat-chips">{detail.record.stat_keys.map(stat => <button key={stat} onClick={() => useCatalog.getState().setQuery({ stat_key: stat })} title="Filter items with the same stat ID"><span className="mono">{hex(stat)}</span><ArrowUpRight size={13} /></button>)}</div><p className="fine-print">IDs from enchantment and sharpening values. Names and units are not yet verified.</p></> : <p className="muted small">No stat reference in the loaded stat lists.</p>}</div>
          <div className="provenance"><Layers3 size={17} /><div><strong>Read from your installation</strong><span>{count(detail.fields.length)} fields · {integer(detail.record.length)} Bytes<br />Name: {detail.name_resolved ? 'Text reference resolved' : 'internal key as fallback'}</span></div></div>
        </>}
        {tab === 'fields' && <Fields fields={detail.fields} />}
        {tab === 'links' && <><RecipeLinks session={catalog.session} itemKey={key} /><References detail={detail} /></>}
      </div>
      <div className="detail-actions">{error && <p role="alert" className="error-text">{error}</p>}{exported && <div role="status" className="export-status"><Check size={14} /><span>JSON saved<br /><code>{exported}</code></span></div>}<button className="secondary export-button" onClick={exportJson} disabled={exporting}><Download size={15} />{exporting ? 'Saving …' : 'Save item as JSON'}<span>↗</span></button></div>
    </>}
  </aside>;
}
function Fields({ fields }: { fields: RawField[] }) {
  const [text, setText] = useState(''), [onlyUnknown, setOnlyUnknown] = useState(false);
  const filtered = useMemo(() => filterFields(fields, text, onlyUnknown), [fields, text, onlyUnknown]);
  const scroll = useRef<HTMLDivElement>(null);
  const virtual = useVirtualizer({ count: filtered.length, getScrollElement: () => scroll.current, estimateSize: () => 108, overscan: 4 });
  return <><div className="field-controls"><label className="input-wrap compact"><Search size={14} /><input aria-label="Search raw fields" placeholder="Field, value or offset …" value={text} onChange={e => { setText(e.target.value); scroll.current?.scrollTo({ top: 0 }); }} /></label><div><label className="checkbox-label"><input type="checkbox" checked={onlyUnknown} onChange={e => setOnlyUnknown(e.target.checked)} />Unknown fields only</label><span className="muted small">{filtered.length} / {fields.length}</span></div></div>
    <div className="field-scroll" ref={scroll} aria-label="Raw fields">{!filtered.length ? <p className="empty-state small">No matching fields.</p> : <div style={{ height: virtual.getTotalSize(), position: 'relative' }}>{virtual.getVirtualItems().map(v => { const field = filtered[v.index]; return <div className="raw-field" key={field.path + field.start} style={{ position: 'absolute', top: 0, width: '100%', height: v.size, transform: `translateY(${v.start}px)` }}>
      <div className="field-title"><code title={field.path}>{field.path}</code><span className={field.interpretation === 'unknown' ? 'unknown-tag' : 'muted'}>{field.interpretation === 'unknown' ? 'roh' : field.type_name}</span></div>
      <div className="field-value mono" title={fieldValue(field.value)}>{fieldValue(field.value)}</div><div className="field-meta mono">{hex(field.start)}–{hex(field.end)} · {field.type_name}</div><div className="field-hex mono" title={field.raw_hex}>{field.raw_hex || '∅'}</div>
    </div>; })}</div>}</div></>;
}
function References({ detail }: { detail: Detail }) {
  // Explicit ItemKey references only. Arbitrary u32 values never become links.
  const references = detail.item_references;
  return <><div className="detail-section"><span className="eyebrow">DIRECT ITEM REFERENCES</span>{references.length ? references.map(f => <button className="reference-link" key={f.path} disabled={!f.available} onClick={() => useCatalog.getState().select(f.key)}><span><code>{f.path}</code><strong>{f.name ? plainText(f.name) : `Unresolved ID ${f.key}`}</strong></span>{f.available && <ArrowUpRight size={16} />}</button>) : <p className="muted small">No direct item references in this record.</p>}</div>
    <div className="relation-note"><Info size={17} /><div><strong>Locations not mapped yet</strong><p>Vendors, prices, stock and drop sources are not reliably linked for this build. Missing information does not mean no source exists.</p></div></div>
  </>;
}
