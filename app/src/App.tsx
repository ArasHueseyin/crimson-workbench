import { useCallback, useEffect, useRef, useState } from 'react';
import { Box, Database, FolderOpen, Globe2, Layers3, LoaderCircle, RefreshCw, Search, ShieldCheck, SlidersHorizontal, X, AlertTriangle, ChevronRight, BookOpen } from 'lucide-react';
import { api, asError } from './api';
import { count, hex, languageNames } from './format';
import { useCatalog, useCraft, useMods } from './store';
import { Crafting } from './Crafting';
import { Mods } from './Mods';
import { ItemPicker } from './ItemPicker';
import { MountPicker } from './MountPicker';
import { ExtraSockets } from './ExtraSockets';
import { ItemTable } from './ItemTable';
import { DetailPanel } from './DetailPanel';
import type { AppError, Bootstrap, Catalog, Facet, Item, Query } from './types';

export default function App() {
  const [boot, setBoot] = useState<Bootstrap | null>(null), [catalog, setCatalog] = useState<Catalog | null>(null);
  const [error, setError] = useState<AppError | null>(null), [loading, setLoading] = useState(true);
  const [game, setGame] = useState(''), [language, setLanguage] = useState('ger');
  const [savePath, setSavePath] = useState(''), [settingsStatus, setSettingsStatus] = useState('');
  const view = useCatalog(s => s.view), setView = useCatalog(s => s.setView);
  const request = useRef(0);
  const connect = useCallback(async (path: string, lang: string) => {
    const id = ++request.current; setLoading(true); setError(null); setCatalog(null); useCatalog.getState().reset(); useCraft.getState().reset(); useMods.getState().reset();
    setGame(path); setLanguage(lang);
    try { const value = await api.open(path || null, lang); if (request.current === id) { setCatalog(value); setGame(value.game_path); } }
    catch (e) { if (request.current === id) setError(asError(e)); }
    finally { if (request.current === id) setLoading(false); }
  }, []);
  async function saveSettings(lang = language) {
    const id = ++request.current; setLoading(true); setError(null); setSettingsStatus('');
    try {
      const value = await api.saveSettings({ version: 1, game_dir: game.trim() || null, save_dir: savePath.trim() || null, language: lang });
      if (request.current !== id) return;
      setBoot(value); setCatalog(null); setLanguage(value.settings?.language ?? lang);
      setSavePath(value.settings?.save_dir ?? '');
      setSettingsStatus('Einstellungen gespeichert. Sie werden beim nächsten Start geladen.');
      const choices = value.discovery.installations;
      const selected = value.discovery.configured_game ? choices[0] : choices.length === 1 ? choices[0] : null;
      if (selected) await connect(selected.path, value.settings?.language ?? lang);
      else { setGame(value.settings?.game_dir ?? ''); setLoading(false); setView('sources'); }
    } catch (e) { if (request.current === id) { setError(asError(e)); setLoading(false); } }
  }
  function changeLanguage(lang: string) {
    if (boot?.settings) void saveSettings(lang);
    else void connect(game, lang);
  }
  useEffect(() => {
    let active = true;
    api.bootstrap().then(value => {
      if (!active) return; setBoot(value);
      const lang = value.settings?.language ?? 'ger';
      setLanguage(lang); setGame(value.settings?.game_dir ?? value.discovery.configured_game ?? '');
      setSavePath(value.settings?.save_dir ?? '');
      const choices = value.discovery.installations;
      const selected = value.discovery.configured_game ? choices[0] : choices.length === 1 ? choices[0] : null;
      if (selected && !value.settings_warning) void connect(selected.path, lang);
      else { setLoading(false); setView('sources'); }
    }).catch(e => { if (active) { setError(asError(e)); setLoading(false); } });
    return () => { active = false; };
  }, [connect, setView]);
  const languages = boot?.languages ?? [{ language: 'ger' }, { language: 'eng' }];
  return <div className="workbench" data-ready={catalog ? 'true' : 'false'}>
    <aside className="sidebar"><div className="brand"><div className="brand-mark">C</div><div>CRIMSON<span>WORKBENCH</span></div></div>
      <div className="workspace-label">DEINE SPIELDATEN</div>
      <nav aria-label="Hauptnavigation"><button className={view === 'items' ? 'nav-active' : ''} onClick={() => setView('items')}><Box size={18} /><span>Itemdatenbank</span>{catalog && <small>{count(catalog.info.item_count)}</small>}</button><button className={view === 'spawn' ? 'nav-active' : ''} onClick={() => setView('spawn')}><Box size={18} /><span>Item-Auswahl</span></button><button className={view === 'mounts' ? 'nav-active' : ''} onClick={() => setView('mounts')}><Box size={18} /><span>Reittiere</span></button><button className={view === 'sockets' ? 'nav-active' : ''} onClick={() => setView('sockets')}><SlidersHorizontal size={18} /><span>Zusatzsockel</span></button><button className={view === 'crafting' ? 'nav-active' : ''} onClick={() => setView('crafting')}><Layers3 size={18} /><span>Herstellungsplan</span></button><button className={view === 'mods' ? 'nav-active' : ''} onClick={() => setView('mods')}><SlidersHorizontal size={18} /><span>Modwerkstatt</span></button><button className={view === 'sources' ? 'nav-active' : ''} onClick={() => setView('sources')}><Database size={18} /><span>Datenquellen</span></button></nav>
      <div className="sidebar-note"><BookOpen size={20} strokeWidth={1.3} /><strong>Wissen für dein Abenteuer.</strong><p>Durchsuche die Gegenstände von Pywel und entdecke, was in ihnen steckt.</p></div>
      <div className="sidebar-bottom"><div className="read-only"><ShieldCheck size={17} /><div>Kontrollierte Zugriffe<span>Live-Apply nur bei beendetem Spiel.</span></div></div><div className="version-line"><span>CRIMSON DESERT</span><span>v0.6.2 preview</span></div></div>
    </aside>
    <main className="main-area"><header className="topbar"><div className="breadcrumb">Workbench <ChevronRight size={12} /><span>{view === 'items' ? 'Itemdatenbank' : view === 'spawn' ? 'Item-Auswahl' : view === 'mounts' ? 'Reittiere' : view === 'sockets' ? 'Zusatzsockel' : view === 'crafting' ? 'Herstellungsplan' : view === 'mods' ? 'Modwerkstatt' : 'Datenquellen'}</span></div><div className="topbar-actions"><label className="language-picker"><Globe2 size={14} /><select aria-label="Itemsprache" value={language} disabled={loading} onChange={e => changeLanguage(e.target.value)}>{languages.map(l => <option key={l.language} value={l.language}>{languageNames[l.language] ?? l.language}</option>)}</select></label><span className={`connection-pill ${catalog ? 'connected' : ''}`}><span className="status-dot" />{catalog ? 'Verbunden' : loading ? 'Wird geprüft' : 'Nicht verbunden'}</span></div></header>
      {view === 'sockets' && catalog ? <ExtraSockets key={catalog.session} catalog={catalog} /> : view === 'mounts' && catalog ? <MountPicker key={catalog.session} catalog={catalog} /> : view === 'spawn' && catalog ? <ItemPicker key={catalog.session} catalog={catalog} /> : view === 'mods' && catalog ? <Mods key={catalog.session} session={catalog.session} /> : view === 'crafting' && catalog ? <Crafting key={catalog.session} session={catalog.session} /> : view === 'sources' ? <Sources boot={boot} catalog={catalog} game={game} setGame={setGame} savePath={savePath} setSavePath={setSavePath} settingsStatus={settingsStatus} saveSettings={() => void saveSettings()} loading={loading} connect={() => void connect(game, language)} /> : <>
        <div className="page-heading"><div><div className="eyebrow">ENTDECKEN & NACHLESEN</div><h1>Itemdatenbank<span>.</span></h1><p>Jeder Gegenstand. Alle Werte. Direkt aus deiner Installation.</p></div><button className="secondary reload" disabled={loading} onClick={() => void connect(game, language)} title="Installation und Daten neu prüfen"><RefreshCw size={15} className={loading ? 'spin' : ''} />Neu einlesen</button></div>
        {error ? <ErrorCard error={error} retry={() => void connect(game, language)} settings={() => setView('sources')} /> : loading ? <div className="loading-workspace"><div className="loading-symbol"><Layers3 size={32} strokeWidth={1.1} /><LoaderCircle size={66} className="spin loading-ring" /></div><h2>Deine Spielwelt wird eingelesen.</h2><p>Build prüfen, Gegenstände lesen und Suchindex vorbereiten.</p><span>Spiel und Spielstände werden ausschließlich gelesen.</span></div> : catalog ? <Items catalog={catalog} /> : <div className="loading-workspace"><FolderOpen size={40} strokeWidth={1.2} /><h2>Eine Installation auswählen</h2><p>Verbinde deinen lokalen Crimson-Desert-Ordner.</p><button className="primary" onClick={() => setView('sources')}>Datenquellen öffnen</button></div>}
      </>}
      {view === 'sources' && error && <div className="source-error"><ErrorCard error={error} retry={() => void connect(game, language)} settings={() => setView('items')} /></div>}
    </main>
  </div>;
}
function Items({ catalog }: { catalog: Catalog }) {
  const query = useCatalog(s => s.query), setQuery = useCatalog(s => s.setQuery), reset = useCatalog(s => s.reset);
  const [rows, setRows] = useState<Item[]>([]), [total, setTotal] = useState(0), [busy, setBusy] = useState(false), [error, setError] = useState('');
  const [advanced, setAdvanced] = useState(false);
  const generation = useRef(0), pending = useRef(false), currentQuery = useRef<Query>(query);
  const searchInput = useRef<HTMLInputElement>(null);
  useEffect(() => {
    const handler = (e: KeyboardEvent) => { if ((e.ctrlKey || e.metaKey) && e.key === 'k') { e.preventDefault(); searchInput.current?.focus(); } };
    window.addEventListener('keydown', handler); return () => window.removeEventListener('keydown', handler);
  }, []);
  useEffect(() => {
    const id = ++generation.current; pending.current = true; setBusy(true); setError(''); setRows([]); setTotal(0);
    currentQuery.current = query;
    const timer = setTimeout(() => { api.search(catalog.session, { ...query, offset: 0 }).then(page => {
      if (id !== generation.current) return; setRows(page.items); setTotal(page.total);
    }).catch(e => { if (id === generation.current) setError(asError(e).message); }).finally(() => { if (id === generation.current) { setBusy(false); pending.current = false; } }); }, query.text ? 240 : 0);
    return () => { clearTimeout(timer); generation.current++; };
  }, [catalog.session, query]);
  async function more() {
    if (pending.current || rows.length >= total) return;
    const id = generation.current; pending.current = true; setBusy(true);
    try { const page = await api.search(catalog.session, { ...currentQuery.current, offset: rows.length }); if (id === generation.current) setRows(prev => [...prev, ...page.items]); }
    catch (e) { if (id === generation.current) setError(asError(e).message); }
    finally { if (id === generation.current) { pending.current = false; setBusy(false); } }
  }
  const filterCount = [query.item_type, query.category, query.tier, query.stat_key].filter(v => v !== null).length + Number(query.stackable);
  return <div className="catalog-layout"><div className="catalog-main"><div className="search-area"><label className="input-wrap main-search"><Search size={19} /><input ref={searchInput} aria-label="Items suchen" placeholder={query.regex?'Suchmuster, z. B. (Blitz|Feuer).*pfeil':'Name, Teilwort oder Item-ID …'} value={query.text} maxLength={512} onChange={e => setQuery({ text: e.target.value })} /><kbd>Ctrl K</kbd>{query.text && <button className="icon-button" aria-label="Suchtext löschen" onClick={() => setQuery({ text: '' })}><X size={15} /></button>}</label>
      <label className="checkbox-label search-mode"><input aria-label="Regex-Suche" type="checkbox" checked={!!query.regex} onChange={e=>setQuery({regex:e.target.checked})}/>Regex-Suche<span>{query.regex?'Suchmuster verwenden':'Teilwörter finden auch Zusammensetzungen und Mehrzahl.'}</span></label>
      <div className="filter-row"><FacetSelect label="Typ" values={catalog.info.types} selected={query.item_type} onChange={item_type => setQuery({ item_type })} /><FacetSelect label="Kategorie" values={catalog.info.categories} selected={query.category} onChange={category => setQuery({ category })} /><FacetSelect label="Tier" values={catalog.info.tiers} selected={query.tier} onChange={tier => setQuery({ tier })} /><button className={`filter-toggle ${advanced ? 'active' : ''}`} aria-expanded={advanced} onClick={() => setAdvanced(!advanced)}><SlidersHorizontal size={15} />Mehr Filter{filterCount > 0 && <small>{filterCount}</small>}</button>{filterCount > 0 && <button className="icon-button clear-filters" onClick={reset} aria-label="Alle Filter zurücksetzen"><X size={16} /></button>}</div>
      {advanced && <div className="advanced-filters"><label className="checkbox-label"><input type="checkbox" checked={query.stackable} onChange={e => setQuery({ stackable: e.target.checked })} />Stapelbar <span className="muted">(max. Stapel &gt; 1)</span></label><label className="stat-select">Stat-ID<select aria-label="Stat-ID" value={query.stat_key ?? ''} onChange={e => setQuery({ stat_key: e.target.value === '' ? null : Number(e.target.value) })}><option value="">Alle Stats</option>{catalog.info.stats.map(f => <option key={f.value} value={f.value}>{hex(f.value)} · {f.count}</option>)}</select></label><p>Typen und Kategorien werden mit ihren unveränderten Spiel-IDs angezeigt.</p></div>}
    </div>{error ? <div className="search-error" role="alert"><AlertTriangle size={20} /><p>{error}</p><button className="secondary" onClick={reset}>Suche zurücksetzen</button></div> : <ItemTable items={rows} total={total} session={catalog.session} loading={busy} more={rows.length < total} loadMore={() => void more()} reset={reset} />}</div><DetailPanel catalog={catalog} /></div>;
}
function FacetSelect({ label, values, selected, onChange }: { label: string; values: Facet[]; selected: number | null; onChange: (n: number | null) => void }) {
  return <label className={`facet-select ${selected !== null ? 'active' : ''}`}><select aria-label={label} value={selected ?? ''} onChange={e => onChange(e.target.value === '' ? null : Number(e.target.value))}><option value="">{label}: Alle</option>{values.map(f => <option key={f.value} value={f.value}>{label} {f.value} · {count(f.count)}</option>)}</select></label>;
}
function ErrorCard({ error, retry, settings }: { error: AppError; retry: () => void; settings: () => void }) {
  return <div className="error-card" role="alert"><AlertTriangle size={28} /><h2>{error.code === 'unsupported_build' ? 'Dieser Build ist noch nicht unterstützt.' : error.code === 'desktop_required' ? 'Die Desktop-App wird benötigt.' : 'Die Daten konnten nicht gelesen werden.'}</h2><p>{error.code === 'unsupported_build' ? 'Die Dateien passen nicht zum verifizierten Leseschema. Unterstützt sind EXE 1.0.0.2944, 1.0.0.2949 und 1.0.0.2976 mit passenden Originaldateien. Nach einem Update ist eine passende Workbench-Version nötig.' : 'Prüfe den Installationspfad und versuche es erneut.'}</p><details><summary>Technische Details</summary><pre>{error.message}</pre></details><div><button className="primary" onClick={retry}>Erneut versuchen</button><button className="secondary" onClick={settings}>Datenquelle prüfen</button></div></div>;
}
function Sources({ boot, catalog, game, setGame, savePath, setSavePath, settingsStatus, saveSettings, loading, connect }: { boot: Bootstrap | null; catalog: Catalog | null; game: string; setGame: (p: string) => void; savePath: string; setSavePath: (p: string) => void; settingsStatus: string; saveSettings: () => void; loading: boolean; connect: () => void }) {
  return <div className="sources"><div className="page-heading"><div><div className="eyebrow">LOKAL & NACHVOLLZIEHBAR</div><h1>Datenquellen<span>.</span></h1><p>Deine Installation ist die Quelle. Workbench liest ihre Dateien.</p></div></div><section className="source-card"><div className="source-card-heading"><FolderOpen size={23} /><div><h2>Crimson Desert</h2><p>Steam, Epic Games oder Game Pass</p></div>{catalog && <span className="verified-tag"><ShieldCheck size={14} />Leseschema geprüft</span>}</div>
    {boot && boot.discovery.installations.length > 0 && <label className="source-field">Gefundene Installationen<select aria-label="Gefundene Installationen" value={boot.discovery.installations.some(i => i.path === game) ? game : ''} onChange={e => setGame(e.target.value)}><option value="">Manuellen Pfad verwenden</option>{boot.discovery.installations.map(i => <option value={i.path} key={i.path}>{i.platform.toUpperCase()} · {i.path}</option>)}</select></label>}
    <form onSubmit={e => { e.preventDefault(); connect(); }}><label className="source-field">Installationsordner<input aria-label="Installationsordner" value={game} placeholder="C:\…\Crimson Desert" onChange={e => setGame(e.target.value)} disabled={loading} /></label><button className="primary" type="submit" disabled={loading}>{loading ? <LoaderCircle size={16} className="spin" /> : <RefreshCw size={16} />}{loading ? 'Wird geprüft …' : 'Installation einlesen'}</button></form>
    <p className="fine-print">Eigene Crimson-Desert-Installation erforderlich. Unterstützte EXE-Versionen: 1.0.0.2944, 1.0.0.2949 und 1.0.0.2976; die Dateien werden zusätzlich geprüft.</p>
    <label className="source-field">Spielstandordner<input aria-label="Spielstandordner" value={savePath} placeholder="Automatisch bei einem eindeutigen Saveordner" onChange={e => setSavePath(e.target.value)} disabled={loading} /></label>
    {boot && boot.discovery.save_directories.length > 0 && <label className="source-field">Gefundene Spielstandordner<select aria-label="Gefundene Spielstandordner" value={boot.discovery.save_directories.includes(savePath) ? savePath : ''} onChange={e => setSavePath(e.target.value)} disabled={loading}><option value="">Automatisch / manuellen Pfad verwenden</option>{boot.discovery.save_directories.map(path => <option value={path} key={path}>{path}</option>)}</select></label>}
    <p className="fine-print">Wähle den Save-Hauptordner oberhalb der Konto- und Slotordner. Diese Auswahl gilt für Reittiere, Wissensanzeige und Zusatzsockel. Unterstützt werden vollständige save.save-/lobby.save-Paare; andere Speicherformate bleiben gesperrt. Leere Pfade verwenden die automatische Erkennung.</p>
    <button className="secondary" type="button" disabled={loading} onClick={saveSettings}>Einstellungen speichern</button>
    {settingsStatus && <p role="status">{settingsStatus}</p>}
    {boot?.settings_warning && <p role="alert">{boot.settings_warning}</p>}
    {catalog && <div className="source-metrics"><div><span>EXE-Version</span><strong>{catalog.info.exe_version}</strong></div><div><span>Gegenstände</span><strong>{count(catalog.info.item_count)}</strong></div><div><span>Itemsprache</span><strong>{languageNames[catalog.info.index.language]}</strong></div></div>}
  </section><section className="source-card"><div className="source-card-heading"><Database size={21} /><div><h2>Lokaler Suchindex</h2><p>Wird aus den geprüften Spieldaten aufgebaut.</p></div></div><dl className="source-paths"><dt>Arbeitsordner</dt><dd>{boot?.project ?? 'Noch nicht ermittelt'}</dd><dt>Index</dt><dd>{catalog?.info.index.path ?? 'Noch nicht geladen'}</dd>{catalog && <><dt>Fingerprint</dt><dd className="mono">{catalog.info.index.fingerprint}</dd></>}</dl><p className="fine-print">Einstellungen, Indexe, Sicherungen und Exporte bleiben im angezeigten Arbeitsordner. Ein passender Lesebuild ist keine Bestätigung einer unveränderten Vanilla-Installation.</p></section><div className="source-assurance"><ShieldCheck size={20} /><p><strong>Vorschau und Probeläufe bleiben im Projekt.</strong> Live-Apply und Restore benötigen eine eingerichtete Basis, aktuelle Dateivorschau und ein beendetes Spiel. Savefunktionen öffnen ausschließlich den ausgewählten Spielstand; eine Stallregistrierung erfordert eine ausdrückliche Aktion und ein beendetes Spiel.</p></div></div>;
}
