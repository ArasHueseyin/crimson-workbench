import { useEffect, useRef, useState } from 'react';
import { LoaderCircle, ScanSearch } from 'lucide-react';
import { api, asError } from './api';
import { count } from './format';
import type { InstallationInventory } from './mod-types';
import { ContentAudit } from './ContentAudit';

const states: Record<string, string> = {
  workbench_registry: 'Registry with own mod', workbench_owned: 'Workbench file',
  size_matches: 'Size matches · Content unchecked', missing: 'Missing', additional: 'Additional',
  size_mismatch: 'Size mismatch', type_mismatch: 'File type mismatch',
  unsupported_entry: 'Link / special type', uncompared: 'No depot comparison',
};
export function InstallationCheck({ session }: { session: number }) {
  const [report, setReport] = useState<InstallationInventory | null>(null);
  const [busy, setBusy] = useState(false), [error, setError] = useState('');
  const generation = useRef(0);
  useEffect(() => { generation.current++; setReport(null); setError(''); setBusy(false); return () => { generation.current++; }; }, [session]);
  async function check() {
    const id = ++generation.current; setBusy(true); setError(''); setReport(null);
    try { const result = await api.installationCheck(session); if (id === generation.current) setReport(result); }
    catch (e) { if (id === generation.current) setError(asError(e).message); }
    finally { if (id === generation.current) setBusy(false); }
  }
  const anomalies = report?.files.filter(f => f.state !== 'size_matches' && f.state !== 'uncompared' && f.state !== 'workbench_registry') ?? [];
  return <section className="installation-check" aria-label="Installation check">
    <div className="mod-result-heading"><div><small>B0 · GAME BASELINE</small><h2>Check installation</h2></div>
      <button type="button" className="secondary" disabled={busy} onClick={() => void check()}>{busy ? <LoaderCircle size={16} className="spin" /> : <ScanSearch size={16} />}{busy ? 'Checking …' : 'Check file list'}</button></div>
    <p>Compares files, sizes and launchers with local Steam depot lists. Available while playing; does not read archive contents or change game files.</p>
    {busy && <p role="status">Reading directories and Steam metadata …</p>}
    {error && <p role="alert" className="craft-error">{error}</p>}
    {report && <div className="installation-report">
      <p role="status"><strong>{report.directory_scan_complete && report.depot_comparison_available ? 'File list checked' : 'Check incomplete'}</strong> · {count(report.actual_files)} files found / {report.depot_comparison_available ? `${count(report.expected_files)} according to Steam cache` : 'Depot comparison unavailable'}.<br />File list without content verification · Vanilla unconfirmed. Live validation is separate.</p>
      <p>As of: {new Date(report.observed_at * 1000).toLocaleString('en-US')} · Build {report.build_id ?? 'unknown'} · {report.game_running === null ? 'Game status unknown' : report.game_running ? 'Game running' : 'Game closed'}.</p>
      {report.issues.length > 0 && <ul className="installation-issues">{report.issues.map((issue, i) => <li key={i}>{issue.path && <code>{issue.path}: </code>}{issue.message}</li>)}</ul>}
      <details><summary>Launchers ({report.executables.length})</summary><ul>{report.executables.map(path => <li key={path}><code>{path}</code></li>)}</ul><p>EXE files found in the installation, including missing depot files. Launch protection has not yet been validated against the real game.</p></details>
      <details><summary>Additional or differing entries ({anomalies.length})</summary><p>Additional files may also be logs or other normal runtime files. Their presence alone does not prove third-party mods are installed.</p><ul>{anomalies.map(f => <li key={f.path}><code>{f.path}</code> · {states[f.state] ?? f.state}</li>)}</ul></details>
      {(report.managed_files?.length??0)>0&&<details><summary>Workbench entries ({report.managed_files!.length})</summary><p>Files and folders from Workbench history. Files are included in the total count; their contents are not hashed again here.</p><ul>{report.managed_files!.map(f=><li key={f.path}><code>{f.path}</code></li>)}</ul></details>}
      {(report.foreign_files?.length??0)>0&&<details><summary>Approved third-party additions ({report.foreign_files!.length})</summary><p>Explicitly approved for keeping. Contents are not checked again here; these files are not part of vanilla verification.</p><ul>{report.foreign_files!.map(f=><li key={f.path}><code>{f.path}</code></li>)}</ul></details>}
      {report.managed_registry&&<p>The registry contains a Workbench mod. Restore the original state before creating a new content report; after a game update, review the baseline change.</p>}
      <details><summary>Depot lists and archive groups</summary><ul>{report.depots.map(d => <li key={d.id}>Depot {d.id} · {d.files} files · Manifest <code>{d.manifest_id}</code> · Origin unconfirmed</li>)}</ul><p>{report.groups.filter(g => g.installed).length} registered groups installed, {report.groups.filter(g => g.optional && !g.installed).length} optional groups missing. Registry: {report.registry_matches_observed_build === null ? 'not compared' : report.registry_matches_observed_build ? 'matches the observed build' : 'differs from the observed build'}.</p></details>
      <details><summary>Scope and next steps</summary><ul>{report.limitations.map(item => <li key={item}>{item}</li>)}</ul></details>
    </div>}
    <ContentAudit session={session} inventory={report}/>
  </section>;
}
