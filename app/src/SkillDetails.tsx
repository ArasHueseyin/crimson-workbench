import {useEffect,useState} from 'react';
import {api,asError} from './api';
import type {AdvancedSkill} from './advanced-types';

export function SkillDetails({session,skill,edits,change}:{session:number;skill:number;edits:Record<string,string>;change:(edits:Record<string,string>)=>void}) {
 const [detail,setDetail]=useState<AdvancedSkill|null>(null),[error,setError]=useState(''),[search,setSearch]=useState(''),[fieldSearch,setFieldSearch]=useState('');
 useEffect(()=>{let active=true;setDetail(null);setError('');setSearch('');setFieldSearch('');api.advancedSkill(session,skill).then(d=>{if(active)setDetail(d)}).catch(e=>{if(active)setError(asError(e).message)});return()=>{active=false}},[session,skill]);
 if(error)return <p role="alert">Skill details: {error}</p>;
 if(!detail)return <p role="status">Reading buff matrix …</p>;
 const filtered=detail.buffs.filter(b=>`${b.name} ${b.type_id??'null'} ${b.matrix_level}/${b.index} ${b.fields.map(f=>f.path).join(' ')}`.toLowerCase().includes(search.toLowerCase()));
 const fields=detail.record_fields.filter(f=>`${f.path} ${f.kind} ${f.value}`.toLowerCase().includes(fieldSearch.toLowerCase()));
 return <details className="skill-details"><summary>Buff matrix & full record · {detail.buffs.length} entries</summary>
  <p>{detail.level_counts.length} matrix rows · {detail.byte_len.toLocaleString('en-US')} bytes. Row/column numbers start at 0 and are not verified gameplay levels.</p>
  <p>Verified 64-bit numbers can be overridden individually. References, types, list lengths and unknown raw blocks are read only. “mem” identifies internal structure positions; meaning and units remain unknown. Named summon fields and the AddSubLevel effect reference are readable but read only. A field name alone does not confirm a unit or game effect. Raw value changes require in-game testing. Input limit: ±1,000,000,000.</p>
  <details className="skill-record-fields"><summary>Base data, requirements & resources · {detail.record_fields.length} fields</summary>
   <p>Original values outside the buff matrix, including item and status references, resources and flags. This view is read only. Edit supported numbers in the skill fields above. References are identifiers, not quantities; unknown number formats remain hexadecimal.</p>
   <input aria-label="Filter skill base fields" placeholder="Field name, identifier or value" value={fieldSearch} onChange={e=>setFieldSearch(e.target.value)}/>
   <small>{fields.length} of {detail.record_fields.length} fields</small>
   <dl>{fields.map(f=><div key={f.offset}><dt>{f.path}<small>Byte {f.offset} · {f.bytes} Bytes · {f.kind}</small></dt><dd>{f.value||'∅'}</dd></div>)}</dl>
   {fields.length===0&&<p>No matching base fields.</p>}
  </details>
  <input aria-label="Filter skill buffs" placeholder="Buff type, ID, field name or row/column" value={search} onChange={e=>setSearch(e.target.value)}/>
  <small>{filtered.length} of {detail.buffs.length} entries · Count per matrix row: {detail.level_counts.join(', ')||'empty'}</small>
  <div className="skill-buffs">{filtered.map(b=><details key={`${b.matrix_level}/${b.index}`}><summary>Row {b.matrix_level} / Column {b.index} · {b.name}{b.type_id!==null?` · Type ${b.type_id}`:''}</summary>
   {b.opaque_layout&&<p>This type-dependent section is bounded only as a raw block of the verified build.</p>}
   <dl>{b.fields.map(f=>{const id=`${b.matrix_level}/${b.index}/${f.path}`,active=edits[id]!==undefined,label=`Buff ${b.matrix_level}/${b.index}: ${f.path}`;return <div key={`${f.offset}/${f.path}`}><dt>{f.path}<small>Byte {f.offset} · {f.bytes} Bytes · {f.kind}</small></dt><dd>{f.value||'∅'}</dd>{f.editable&&<><label><input type="checkbox" aria-label={`${label} change`} checked={active} onChange={e=>{const next={...edits};if(e.target.checked)next[id]=f.value;else delete next[id];change(next)}}/> Override raw value</label><input aria-label={label} type="number" step={1} min={-1000000000} max={1000000000} required disabled={!active} value={edits[id]??f.value} onChange={e=>change({...edits,[id]:e.target.value})}/></>}</div>})}</dl>
  </details>)}</div>
  <details><summary>Show all original bytes</summary><p>Full record including references, unknown fields and values outside the buff matrix.</p><pre>{(detail.raw_hex.match(/.{1,32}/g)??[]).map((line,index)=>`${(index*16).toString(16).padStart(6,'0')}  ${line.match(/.{1,2}/g)?.join(' ')}`).join('\n')}</pre></details>
 </details>
}
