import {useEffect,useState} from 'react';
import {api,asError} from './api';
import type {AdvancedSkill} from './advanced-types';

export function SkillDetails({session,skill,edits,change}:{session:number;skill:number;edits:Record<string,string>;change:(edits:Record<string,string>)=>void}) {
 const [detail,setDetail]=useState<AdvancedSkill|null>(null),[error,setError]=useState(''),[search,setSearch]=useState(''),[fieldSearch,setFieldSearch]=useState('');
 useEffect(()=>{let active=true;setDetail(null);setError('');setSearch('');setFieldSearch('');api.advancedSkill(session,skill).then(d=>{if(active)setDetail(d)}).catch(e=>{if(active)setError(asError(e).message)});return()=>{active=false}},[session,skill]);
 if(error)return <p role="alert">Skill-Details: {error}</p>;
 if(!detail)return <p role="status">Buffmatrix wird gelesen …</p>;
 const filtered=detail.buffs.filter(b=>`${b.name} ${b.type_id??'null'} ${b.matrix_level}/${b.index} ${b.fields.map(f=>f.path).join(' ')}`.toLowerCase().includes(search.toLowerCase()));
 const fields=detail.record_fields.filter(f=>`${f.path} ${f.kind} ${f.value}`.toLowerCase().includes(fieldSearch.toLowerCase()));
 return <details className="skill-details"><summary>Buffmatrix & vollständiger Datensatz · {detail.buffs.length} Einträge</summary>
  <p>{detail.level_counts.length} Matrixzeilen · {detail.byte_len.toLocaleString('de-DE')} Bytes. Die Zeilen-/Spaltennummern beginnen bei 0 und sind keine bestätigten Gameplay-Level.</p>
  <p>Bestätigte 64-Bit-Zahlen lassen sich einzeln überschreiben. Referenzen, Typen, Listenlängen und unbekannte Rohblöcke sind schreibgeschützt. „mem“ bezeichnet interne Strukturpositionen; Bedeutung und Einheit bleiben unbekannt. Benannte Beschwörungsfelder und die Referenz des AddSubLevel-Effekts sind lesbar, bleiben aber schreibgeschützt. Ein Feldname bestätigt noch keine Einheit oder Spielwirkung. Änderungen an Rohwerten benötigen einen Spieltest. Eingabegrenze: ±1.000.000.000.</p>
  <details className="skill-record-fields"><summary>Basisdaten, Voraussetzungen & Ressourcen · {detail.record_fields.length} Felder</summary>
   <p>Originalwerte außerhalb der Buffmatrix, einschließlich Item- und Statusreferenzen, Ressourcen und Flags. Diese Ansicht ist schreibgeschützt. Bereits freigegebene Zahlen bearbeitest du in den Skill-Feldern oberhalb. Referenzen sind Kennungen, keine Mengen; unbekannte Zahlenformate bleiben Hexwerte.</p>
   <input aria-label="Skill-Basisfelder filtern" placeholder="Feldname, Kennung oder Wert" value={fieldSearch} onChange={e=>setFieldSearch(e.target.value)}/>
   <small>{fields.length} von {detail.record_fields.length} Feldern</small>
   <dl>{fields.map(f=><div key={f.offset}><dt>{f.path}<small>Byte {f.offset} · {f.bytes} Bytes · {f.kind}</small></dt><dd>{f.value||'∅'}</dd></div>)}</dl>
   {fields.length===0&&<p>Keine passenden Basisfelder.</p>}
  </details>
  <input aria-label="Skill-Buffs filtern" placeholder="Buff-Typ, ID, Feldname oder Zeile/Spalte" value={search} onChange={e=>setSearch(e.target.value)}/>
  <small>{filtered.length} von {detail.buffs.length} Einträgen · Anzahl pro Matrixzeile: {detail.level_counts.join(', ')||'leer'}</small>
  <div className="skill-buffs">{filtered.map(b=><details key={`${b.matrix_level}/${b.index}`}><summary>Zeile {b.matrix_level} / Spalte {b.index} · {b.name}{b.type_id!==null?` · Typ ${b.type_id}`:''}</summary>
   {b.opaque_layout&&<p>Dieser typabhängige Abschnitt ist nur als Rohblock des geprüften Builds abgegrenzt.</p>}
   <dl>{b.fields.map(f=>{const id=`${b.matrix_level}/${b.index}/${f.path}`,active=edits[id]!==undefined,label=`Buff ${b.matrix_level}/${b.index}: ${f.path}`;return <div key={`${f.offset}/${f.path}`}><dt>{f.path}<small>Byte {f.offset} · {f.bytes} Bytes · {f.kind}</small></dt><dd>{f.value||'∅'}</dd>{f.editable&&<><label><input type="checkbox" aria-label={`${label} ändern`} checked={active} onChange={e=>{const next={...edits};if(e.target.checked)next[id]=f.value;else delete next[id];change(next)}}/> Rohwert überschreiben</label><input aria-label={label} type="number" step={1} min={-1000000000} max={1000000000} required disabled={!active} value={edits[id]??f.value} onChange={e=>change({...edits,[id]:e.target.value})}/></>}</div>})}</dl>
  </details>)}</div>
  <details><summary>Alle Originalbytes anzeigen</summary><p>Vollständiger Datensatz einschließlich Referenzen, unbekannter Felder und der Werte außerhalb der Buffmatrix.</p><pre>{(detail.raw_hex.match(/.{1,32}/g)??[]).map((line,index)=>`${(index*16).toString(16).padStart(6,'0')}  ${line.match(/.{1,2}/g)?.join(' ')}`).join('\n')}</pre></details>
 </details>
}
