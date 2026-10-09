import { useMemo, useRef } from 'react';
import { createColumnHelper, flexRender, getCoreRowModel, useReactTable } from '@tanstack/react-table';
import { useVirtualizer } from '@tanstack/react-virtual';
import { ArrowDown, ArrowUp, ChevronRight, SearchX, LoaderCircle } from 'lucide-react';
import { ItemImage } from './ItemImage';
import { count, integer, plainText } from './format';
import { useCatalog } from './store';
import type { Item, Query } from './types';
const helper = createColumnHelper<Item>();
export function ItemTable({ items, total, session, loading, more, loadMore, reset }: { items: Item[]; total: number; session: number; loading: boolean; more: boolean; loadMore: () => void; reset: () => void }) {
  const selected = useCatalog(s => s.selected), select = useCatalog(s => s.select);
  const query = useCatalog(s => s.query), setQuery = useCatalog(s => s.setQuery);
  const parent = useRef<HTMLDivElement>(null);
  const columns = useMemo(() => [
    helper.accessor('name', { header: 'Gegenstand', cell: ({ row }) => <div className="item-cell"><ItemImage session={session} itemKey={row.original.key} /><div><strong>{plainText(row.original.name)}</strong><span>{row.original.internal_key}</span></div></div> }),
    helper.accessor('key', { header: 'ID', cell: ctx => <span className="mono muted">{ctx.getValue()}</span> }),
    helper.accessor('item_type', { header: 'Typ', cell: ctx => <span className="quiet-badge">{ctx.getValue()}</span> }),
    helper.accessor('tier', { header: 'Tier', cell: ctx => <span className="tier"><i />{ctx.getValue()}</span> }),
    helper.accessor('max_stack', { header: 'Stapel', cell: ctx => <span className="mono stack-count">{integer(ctx.getValue())}</span> }),
    helper.display({ id: 'open', header: '', cell: () => <ChevronRight size={15} className="row-arrow" /> }),
  ], [session]);
  const table = useReactTable({ data: items, columns, getCoreRowModel: getCoreRowModel(), getRowId: row => String(row.key) });
  const rows = table.getRowModel().rows;
  const virtual = useVirtualizer({ count: rows.length, getScrollElement: () => parent.current, estimateSize: () => 68, overscan: 7 });
  function sort(column: string) {
    const name: Query['sort'] = column === 'max_stack' ? 'stack' : column as Query['sort'];
    setQuery({ sort: name, descending: query.sort === name ? !query.descending : false });
    parent.current?.scrollTo({ top: 0 });
  }
  return <section className="results" aria-label="Suchergebnisse">
    <div className="result-caption"><span>{loading && !items.length ? 'Gegenstände werden gesucht …' : <><strong>{count(total)}</strong> Gegenstände</>}</span><span className="caption-tip">Klicken für Details <ChevronRight size={12} /></span></div>
    <div className="table-head" role="row">{table.getFlatHeaders().map(header => {
      const sortable = ['name', 'key', 'tier', 'max_stack'].includes(header.id);
      const active = query.sort === (header.id === 'max_stack' ? 'stack' : header.id);
      return <div key={header.id} role="columnheader">{sortable ? <button onClick={() => sort(header.id)} aria-label={`Nach ${header.column.columnDef.header} sortieren`}>
        {flexRender(header.column.columnDef.header, header.getContext())}{active && (query.descending ? <ArrowDown size={12} /> : <ArrowUp size={12} />)}
      </button> : flexRender(header.column.columnDef.header, header.getContext())}</div>;
    })}</div>
    <div className="table-scroll" ref={parent} onScroll={e => { const el = e.currentTarget; if (el.scrollHeight - el.scrollTop - el.clientHeight < 250 && more && !loading) loadMore(); }} role="listbox" aria-label="Gegenstände" aria-busy={loading}>
      {!items.length ? <div className="empty-state">{loading ? <><LoaderCircle className="spin" /><h3>Daten werden geladen</h3><p>Einen Moment bitte.</p></> : <><SearchX size={32} strokeWidth={1.3} /><h3>Keine passenden Gegenstände</h3><p>Versuche einen anderen Begriff oder entferne die Filter.</p><button className="secondary" onClick={reset}>Suche zurücksetzen</button></>}</div> :
        <div style={{ height: virtual.getTotalSize(), position: 'relative', width: '100%' }}>
          {virtual.getVirtualItems().map(v => { const row = rows[v.index]; return <div role="option" aria-selected={selected === row.original.key} tabIndex={0} key={row.id} data-item-key={row.id}
            className={`item-row ${selected === row.original.key ? 'selected' : ''}`} style={{ position: 'absolute', top: 0, left: 0, width: '100%', height: v.size, transform: `translateY(${v.start}px)` }}
            onClick={() => select(row.original.key)} onKeyDown={e => {
              if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); select(row.original.key); }
              if (e.key === 'ArrowDown' || e.key === 'ArrowUp') {
                e.preventDefault(); const index = Math.min(rows.length - 1, Math.max(0, v.index + (e.key === 'ArrowDown' ? 1 : -1)));
                select(rows[index].original.key); virtual.scrollToIndex(index); requestAnimationFrame(() => parent.current?.querySelector<HTMLElement>(`[data-item-key="${rows[index].id}"]`)?.focus());
              }
            }}>{row.getVisibleCells().map(cell => <div key={cell.id}>{flexRender(cell.column.columnDef.cell, cell.getContext())}</div>)}</div>; })}
        </div>}
      {items.length > 0 && more && <button className="load-more" onClick={loadMore} disabled={loading}>{loading ? 'Wird geladen …' : 'Weitere Gegenstände laden'}</button>}
    </div>
    <footer className="table-footer"><span>{count(items.length)} von {count(total)} geladen</span><span><span className="status-dot" /> Lokale Spieldaten</span></footer>
  </section>;
}
