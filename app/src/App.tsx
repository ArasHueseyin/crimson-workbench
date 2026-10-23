import { useCallback, useEffect, useRef, useState } from 'react';
import { Box, Database, FolderOpen, Globe2, Layers3, LoaderCircle, RefreshCw, ShieldCheck, SlidersHorizontal, AlertTriangle, ChevronRight } from 'lucide-react';
import { api, asError } from './api';
import { count, languageNames } from './format';
import { useCatalog, useCraft, useMods } from './store';
import { Crafting } from './Crafting';
import { Mods } from './Mods';
import {Marketplace} from './Marketplace';
import { ItemPicker } from './ItemPicker';
import { MountPicker } from './MountPicker';
import { ExtraSockets } from './ExtraSockets';
import type { AppError, Bootstrap, Catalog } from './types';
import { version as appVersion } from '../package.json';
import './app-ux.css';
const sourceKey=(value:string)=>value.trim().replace(/^\\\\\?\\/,'').replaceAll('\\','/').replace(/\/+$/,'').toLocaleLowerCase();

export default function App() {
  const [boot, setBoot] = useState<Bootstrap | null>(null), [catalog, setCatalog] = useState<Catalog | null>(null);
  const [error, setError] = useState<AppError | null>(null), [loading, setLoading] = useState(true);
  const [game, setGame] = useState(''), [language, setLanguage] = useState('auto');
  const [savePath, setSavePath] = useState(''), [settingsStatus, setSettingsStatus] = useState('');
  const [savedSources,setSavedSources]=useState({game:'',save:''});
  const [gameRunning,setGameRunning]=useState<boolean|null>(null);
  const view = useCatalog(s => s.view), setView = useCatalog(s => s.setView);
  const sourceDraftDirty=sourceKey(game)!==sourceKey(savedSources.game)||sourceKey(savePath)!==sourceKey(savedSources.save);
  const request = useRef(0);
  const connect = useCallback(async (path: string, lang: string) => {
    const id = ++request.current; setLoading(true); setError(null); setCatalog(null); useCatalog.getState().reset(); useCraft.getState().reset(); useMods.getState().reset();
    setGame(path); setLanguage(lang);
    try { const value = await api.open(path || null, lang); if (request.current === id) { setCatalog(value); setGame(value.game_path); return true; } }
    catch (e) { if (request.current === id) setError(asError(e)); }
    finally { if (request.current === id) setLoading(false); }
    return false;
  }, []);
  async function saveSettings(lang = language) {
    const id = ++request.current; setLoading(true); setError(null); setSettingsStatus('');
    try {
      const value = await api.saveSettings({ version: 1, game_dir: game.trim() || null, save_dir: savePath.trim() || null, language: lang });
      if (request.current !== id) return;
      setBoot(value); setCatalog(null); setLanguage(value.settings?.language ?? lang);
      setSavePath(value.settings?.save_dir ?? '');
      const choices = value.discovery.installations;
      const selected = value.discovery.configured_game ? choices[0] : choices.length === 1 ? choices[0] : null;
      setSavedSources({game:value.settings?.game_dir??selected?.path??'',save:value.settings?.save_dir??''});
      if (selected) {
        const connected=await connect(selected.path, value.settings?.language ?? lang);
        setSettingsStatus(connected?'Settings saved and catalog connected.':'Settings saved. The catalog could not be connected; check the error below.');
      } else { setGame(value.settings?.game_dir ?? ''); setLoading(false); setView('sources'); setSettingsStatus('Settings saved. Choose an installation to connect.'); }
    } catch (e) { if (request.current === id) { setError(asError(e)); setLoading(false); } }
  }
  function changeLanguage(lang: string) {
    if(sourceDraftDirty){setSettingsStatus('Save & connect the edited folders before changing item-name language.');return;}
    if (boot?.settings) void saveSettings(lang);
    else void connect(game, lang);
  }
  useEffect(() => {
    let active = true;
    api.bootstrap().then(value => {
      if (!active) return; setBoot(value);
      const lang = value.settings?.language ?? 'auto';
      setLanguage(lang); setGame(value.settings?.game_dir ?? value.discovery.configured_game ?? '');
      setSavePath(value.settings?.save_dir ?? '');
      const choices = value.discovery.installations;
      const selected = value.discovery.configured_game ? choices[0] : choices.length === 1 ? choices[0] : null;
      setSavedSources({game:value.settings?.game_dir??selected?.path??'',save:value.settings?.save_dir??''});
      if (selected && !value.settings_warning) void connect(selected.path, lang);
      else { setLoading(false); setView('sources'); }
    }).catch(e => { if (active) { setError(asError(e)); setLoading(false); } });
    return () => { active = false; };
  }, [connect, setView]);
  useEffect(()=>{
    setGameRunning(null);
    if(!catalog || catalog.read_only)return;
    let active=true,inFlight=false;
    async function check(){if(inFlight)return;inFlight=true;try{const running=await api.marketplaceGameRunning(catalog!.session);if(active)setGameRunning(running)}catch{if(active)setGameRunning(null)}finally{inFlight=false}}
    void check();const timer=setInterval(()=>void check(),5000);
    return()=>{active=false;clearInterval(timer)};
  },[catalog?.session]);
  const languages = boot?.languages ?? [{ language: 'ger' }, { language: 'eng' }];
  const detectedLanguage = language === 'auto' && catalog && (languageNames[catalog.info.index.language] ?? catalog.info.index.language);
  return <div className="workbench" data-ready={catalog ? 'true' : 'false'}>
    <aside className="sidebar"><div className="brand"><div className="brand-mark">C</div><div>CRIMSON<span>WORKBENCH</span></div></div>
      <div className="workspace-label">YOUR GAME DATA</div>
      <nav aria-label="Main navigation"><button className={view === 'spawn' ? 'nav-active' : ''} onClick={() => setView('spawn')}><Box size={18} /><span>Item picker</span></button><button className={view === 'mounts' ? 'nav-active' : ''} onClick={() => setView('mounts')}><Box size={18} /><span>Mounts</span></button><button className={view === 'sockets' ? 'nav-active' : ''} onClick={() => setView('sockets')}><SlidersHorizontal size={18} /><span>Extra sockets</span></button><button className={view === 'crafting' ? 'nav-active' : ''} onClick={() => setView('crafting')}><Layers3 size={18} /><span>Crafting planner</span></button><button className={view === 'marketplace' ? 'nav-active' : ''} onClick={() => setView('marketplace')}><Layers3 size={18}/><span>Mod marketplace</span></button><button className={view === 'mods' ? 'nav-active' : ''} onClick={() => setView('mods')}><SlidersHorizontal size={18} /><span>Mod workshop</span></button><button className={view === 'sources' ? 'nav-active' : ''} onClick={() => setView('sources')}><Database size={18} /><span>Data sources</span></button></nav>
      <div className="sidebar-bottom"><div className="read-only"><ShieldCheck size={17} /><div>Controlled access<span>Each tool shows when it can be used.</span></div></div><div className="version-line"><span>CRIMSON DESERT</span><span>v{appVersion}</span></div></div>
    </aside>
    <main className="main-area"><header className="topbar"><div className="breadcrumb">Workbench <ChevronRight size={12} /><span>{view === 'spawn' ? 'Item picker' : view === 'mounts' ? 'Mounts' : view === 'sockets' ? 'Extra sockets' : view === 'crafting' ? 'Crafting planner' : view === 'marketplace' ? 'Mod marketplace' : view === 'mods' ? 'Mod workshop' : 'Data sources'}</span></div><div className="topbar-actions"><label className="language-picker"><Globe2 size={14} /><span className="item-language-label">Item names</span><select aria-label="Item language" title="Item names follow the installed Steam game language. English is used if detection is unavailable. The interface stays English." value={language} disabled={loading||sourceDraftDirty} aria-description={sourceDraftDirty?'Save & connect edited folders before changing item-name language.':'The interface remains English.'} onChange={e => changeLanguage(e.target.value)}><option value="auto">Automatic{detectedLanguage ? ` · ${detectedLanguage}` : " (game language)"}</option>{languages.map(l => <option key={l.language} value={l.language}>{languageNames[l.language] ?? l.language}</option>)}</select></label><span className={`connection-pill ${catalog ? 'connected' : ''}`}><span className="status-dot" />{catalog ? 'Catalog loaded' : loading ? 'Reading catalog' : 'Catalog unavailable'}</span><span className={`game-state-pill ${gameRunning===true?'running':''}`} title="Live module connection is checked separately inside each tool.">{!catalog?'Game not checked':gameRunning===null?'Game status unknown':gameRunning?'Game running':'Game closed'}</span></div></header>
      {catalog?.read_only && <div className="source-assurance" role="status"><ShieldCheck size={20} /><p><strong>Mac preview · Read only.</strong> Browse items and plan crafting. Game and save edits, runtime mods, and live inventory changes are unavailable.</p></div>}
      {view === 'marketplace' && catalog ? <Marketplace key={catalog.session} catalog={catalog}/> : view === 'sockets' && catalog ? <ExtraSockets key={catalog.session} catalog={catalog} /> : view === 'mounts' && catalog ? <MountPicker key={catalog.session} catalog={catalog} /> : view === 'spawn' && catalog ? <ItemPicker key={catalog.session} catalog={catalog} /> : view === 'mods' && catalog ? <Mods key={catalog.session} session={catalog.session} /> : view === 'crafting' && catalog ? <Crafting key={catalog.session} session={catalog.session} /> : view === 'sources' ? <Sources boot={boot} catalog={catalog} game={game} setGame={setGame} savePath={savePath} setSavePath={setSavePath} settingsStatus={settingsStatus} savedSources={savedSources} saveSettings={() => void saveSettings()} loading={loading} connect={() => void connect(game, language)} /> : <>
        <div className="page-heading"><div><div className="eyebrow">CHOOSE ITEMS</div><h1>Item picker<span>.</span></h1><p>Icons, descriptions and values from your installation.</p></div><button className="secondary reload" disabled={loading} onClick={() => void connect(game, language)} title="Recheck installation and data"><RefreshCw size={15} className={loading ? 'spin' : ''} />Reload</button></div>
        {error ? <ErrorCard error={error} retry={() => void connect(game, language)} settings={() => setView('sources')} /> : loading ? <div className="loading-workspace"><div className="loading-symbol"><Layers3 size={32} strokeWidth={1.1} /><LoaderCircle size={66} className="spin loading-ring" /></div><h2>Reading your game world.</h2><p>Checking the build, reading items and preparing the search index.</p><span>Game files and saves are read only.</span></div> : catalog ? <ItemPicker key={catalog.session} catalog={catalog} /> : <div className="loading-workspace"><FolderOpen size={40} strokeWidth={1.2} /><h2>Choose an installation</h2><p>Connect your local Crimson Desert folder.</p><button className="primary" onClick={() => setView('sources')}>Open data sources</button></div>}
      </>}
      {view === 'sources' && error && <div className="source-error"><ErrorCard error={error} retry={() => void connect(game, language)} settings={() => setView('sources')} /></div>}
    </main>
  </div>;
}
function ErrorCard({ error, retry, settings }: { error: AppError; retry: () => void; settings: () => void }) {
  return <div className="error-card" role="alert"><AlertTriangle size={28} /><h2>{error.code === 'unsupported_build' ? 'This build is not supported yet.' : error.code === 'desktop_required' ? 'The desktop app is required.' : 'The data could not be read.'}</h2><p>{error.code === 'unsupported_build' ? 'The files do not match the verified reading schema. Supported EXE versions are 1.0.0.2944, 1.0.0.2949 and 1.0.0.2976 with matching original files. After a game update, a compatible Workbench version is required.' : 'Check the installation path and try again.'}</p><details><summary>Technical details</summary><pre>{error.message}</pre></details><div><button className="primary" onClick={retry}>Try again</button><button className="secondary" onClick={settings}>Check data source</button></div></div>;
}
function Sources({ boot, catalog, game, setGame, savePath, setSavePath, settingsStatus, savedSources, saveSettings, loading, connect }: { boot: Bootstrap | null; catalog: Catalog | null; game: string; setGame: (p: string) => void; savePath: string; setSavePath: (p: string) => void; settingsStatus: string; savedSources: {game:string;save:string}; saveSettings: () => void; loading: boolean; connect: () => void }) {
  const dirty=sourceKey(game)!==sourceKey(savedSources.game)||sourceKey(savePath)!==sourceKey(savedSources.save);
  return <div className="sources"><div className="page-heading"><div><div className="eyebrow">LOCAL & TRANSPARENT</div><h1>Data sources<span>.</span></h1><p>Your installation is the source. Workbench reads its files.</p></div></div><section className="source-card"><div className="source-card-heading"><FolderOpen size={23} /><div><h2>Crimson Desert</h2><p>Steam, Epic Games or Game Pass</p></div>{catalog && <span className="verified-tag"><ShieldCheck size={14} />Reading schema verified</span>}</div>
    {boot && boot.discovery.installations.length > 0 && <label className="source-field">Detected installations<select aria-label="Detected installations" value={boot.discovery.installations.some(i => i.path === game) ? game : ''} onChange={e => setGame(e.target.value)}><option value="">Use a manual path</option>{boot.discovery.installations.map(i => <option value={i.path} key={i.path}>{i.platform.toUpperCase()} · {i.path}</option>)}</select></label>}
    <form id="source-path-form" onSubmit={e => { e.preventDefault(); saveSettings(); }}><label className="source-field">Installation folder<input aria-label="Installation folder" value={game} placeholder="Steam game folder or CrimsonDesert_Steam.app" onChange={e => setGame(e.target.value)} disabled={loading} /></label></form>
    <p className="fine-print">Your own Crimson Desert installation is required. Supported EXE versions: 1.0.0.2944, 1.0.0.2949 and 1.0.0.2976; files are also verified.</p>
    <label className="source-field">Save folder<input form="source-path-form" aria-label="Save folder" value={savePath} placeholder="Automatic when one save folder is found" onChange={e => setSavePath(e.target.value)} disabled={loading} /></label>
    {boot && boot.discovery.save_directories.length > 0 && <label className="source-field">Detected save folders<select aria-label="Detected save folders" value={boot.discovery.save_directories.includes(savePath) ? savePath : ''} onChange={e => setSavePath(e.target.value)} disabled={loading}><option value="">Automatic / use a manual path</option>{boot.discovery.save_directories.map(path => <option value={path} key={path}>{path}</option>)}</select></label>}
    <p className="fine-print">Choose the main save folder above the account and slot folders. This applies to mounts, knowledge and extra sockets. Complete save.save/lobby.save pairs are supported; other save formats remain blocked. Empty paths use automatic detection.</p>
    <p className={`source-draft-status ${dirty?'dirty':''}`} aria-live="polite">{dirty?'Unsaved path changes. Save & connect to use these folders or change item-name language.':'No unsaved path changes.'}</p><button className="primary" type="submit" form="source-path-form" disabled={loading}>{loading ? <LoaderCircle size={16} className="spin" /> : <RefreshCw size={16} />}{loading ? 'Saving & connecting …' : 'Save & connect'}</button><button className="secondary" type="button" disabled={loading||dirty||!catalog} onClick={connect}>Reload connected catalog</button>
    {settingsStatus && <p role="status">{settingsStatus}</p>}
    {boot?.settings_warning && <p role="alert">{boot.settings_warning}</p>}
    {catalog && <div className="source-metrics"><div><span>EXE version</span><strong>{catalog.info.exe_version ?? 'Native Mac · read only'}</strong></div><div><span>Items</span><strong>{count(catalog.info.item_count)}</strong></div><div><span>Item language</span><strong>{languageNames[catalog.info.index.language]}</strong><small>{catalog.item_language?.source === "steam" ? "Detected from Steam" : catalog.item_language?.source === "fallback" ? "English fallback — game language unavailable" : "Manual selection"}</small></div></div>}
  </section><section className="source-card"><div className="source-card-heading"><Database size={21} /><div><h2>Local search index</h2><p>Built from verified game data.</p></div></div><dl className="source-paths"><dt>Workspace folder</dt><dd>{boot?.project ?? 'Not detected yet'}</dd><dt>Index</dt><dd>{catalog?.info.index.path ?? 'Not loaded yet'}</dd>{catalog && <><dt>Fingerprint</dt><dd className="mono">{catalog.info.index.fingerprint}</dd></>}</dl><p className="fine-print">Settings, indexes, backups and exports stay in the workspace shown here. A matching reading schema does not confirm an unmodified vanilla installation.</p></section><div className="source-assurance"><ShieldCheck size={20} /><p><strong>Previews and rehearsals stay in the workspace.</strong> Live apply and restore require a configured baseline, a current file preview and a closed game. Save features open only the selected save; stable registration requires an explicit action and a closed game.</p></div></div>;
}
