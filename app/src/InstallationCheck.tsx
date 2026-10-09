import { useEffect, useRef, useState } from 'react';
import { LoaderCircle, ScanSearch } from 'lucide-react';
import { api, asError } from './api';
import { count } from './format';
import type { InstallationInventory } from './mod-types';
import { ContentAudit } from './ContentAudit';

const states: Record<string, string> = {
  workbench_registry: 'Registry mit eigenem Mod', workbench_owned: 'Workbench-Datei',
  size_matches: 'Größe passt · Inhalt ungeprüft', missing: 'Fehlt', additional: 'Zusätzlich',
  size_mismatch: 'Größe abweichend', type_mismatch: 'Dateityp abweichend',
  unsupported_entry: 'Verknüpfung / Sondertyp', uncompared: 'Kein Depotvergleich',
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
  return <section className="installation-check" aria-label="Installationsprüfung">
    <div className="mod-result-heading"><div><small>B0 · SPIELBASIS</small><h2>Installation prüfen</h2></div>
      <button type="button" className="secondary" disabled={busy} onClick={() => void check()}>{busy ? <LoaderCircle size={16} className="spin" /> : <ScanSearch size={16} />}{busy ? 'Prüfung läuft …' : 'Dateiliste prüfen'}</button></div>
    <p>Vergleicht Dateien, Größen und Startprogramme mit lokalen Steam-Depotlisten. Auch während des Spielens möglich; liest keine Archivinhalte und verändert keine Spieldateien.</p>
    {busy && <p role="status">Verzeichnisse und Steam-Metadaten werden gelesen …</p>}
    {error && <p role="alert" className="craft-error">{error}</p>}
    {report && <div className="installation-report">
      <p role="status"><strong>{report.directory_scan_complete && report.depot_comparison_available ? 'Dateiliste geprüft' : 'Prüfung unvollständig'}</strong> · {count(report.actual_files)} Dateien gefunden / {report.depot_comparison_available ? `${count(report.expected_files)} laut Steam-Cache` : 'Depotvergleich nicht verfügbar'}.<br />Dateiliste ohne Inhaltsnachweis · Vanilla unbestätigt. Die Live-Prüfung erfolgt separat.</p>
      <p>Stand: {new Date(report.observed_at * 1000).toLocaleString('de-DE')} · Build {report.build_id ?? 'unbekannt'} · {report.game_running === null ? 'Spielstatus unbekannt' : report.game_running ? 'Spiel läuft' : 'Spiel beendet'}.</p>
      {report.issues.length > 0 && <ul className="installation-issues">{report.issues.map((issue, i) => <li key={i}>{issue.path && <code>{issue.path}: </code>}{issue.message}</li>)}</ul>}
      <details><summary>Startprogramme ({report.executables.length})</summary><ul>{report.executables.map(path => <li key={path}><code>{path}</code></li>)}</ul><p>EXE-Funde innerhalb der Installation, einschließlich fehlender Depotdateien. Noch keine Freigabe des Startschutzes am echten Spiel.</p></details>
      <details><summary>Zusätzliche oder abweichende Einträge ({anomalies.length})</summary><p>Zusätzliche Dateien können auch Logs oder andere reguläre Laufzeitdateien sein. Sie sind allein kein Beweis für Fremdmods.</p><ul>{anomalies.map(f => <li key={f.path}><code>{f.path}</code> · {states[f.state] ?? f.state}</li>)}</ul></details>
      {(report.managed_files?.length??0)>0&&<details><summary>Eigene Workbench-Einträge ({report.managed_files!.length})</summary><p>Dateien und Ordner aus der eigenen Historie. Die Dateien sind in der Gesamtzahl enthalten; Inhalte hier nicht erneut gehasht.</p><ul>{report.managed_files!.map(f=><li key={f.path}><code>{f.path}</code></li>)}</ul></details>}
      {(report.foreign_files?.length??0)>0&&<details><summary>Bestätigte fremde Zusätze ({report.foreign_files!.length})</summary><p>Ausdrücklich zum Beibehalten bestätigt. Inhalte hier nicht erneut geprüft; diese Dateien gehören nicht zum Vanilla-Nachweis.</p><ul>{report.foreign_files!.map(f=><li key={f.path}><code>{f.path}</code></li>)}</ul></details>}
      {report.managed_registry&&<p>Die Registry enthält den eigenen Mod. Vor einem neuen Inhaltsbericht Originalzustand wiederherstellen; nach einem Spielupdate den Basiswechsel prüfen.</p>}
      <details><summary>Depotlisten und Archivgruppen</summary><ul>{report.depots.map(d => <li key={d.id}>Depot {d.id} · {d.files} Dateien · Manifest <code>{d.manifest_id}</code> · Herkunft unbestätigt</li>)}</ul><p>{report.groups.filter(g => g.installed).length} registrierte Gruppen installiert, {report.groups.filter(g => g.optional && !g.installed).length} optionale Gruppen fehlen. Registry: {report.registry_matches_observed_build === null ? 'nicht verglichen' : report.registry_matches_observed_build ? 'entspricht dem beobachteten Build' : 'weicht vom beobachteten Build ab'}.</p></details>
      <details><summary>Umfang und nächste Schritte</summary><ul>{report.limitations.map(item => <li key={item}>{item}</li>)}</ul></details>
    </div>}
    <ContentAudit session={session} inventory={report}/>
  </section>;
}
