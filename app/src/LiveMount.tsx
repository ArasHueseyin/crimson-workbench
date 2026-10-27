import {PawPrint, LoaderCircle} from 'lucide-react';
import {useItemSpawner} from './useItemSpawner';
import type {Catalog} from './types';

export function LiveMount({catalog,mountKey}:{catalog:Catalog;mountKey:number}) {
  const live=useItemSpawner(catalog.session,catalog.game_path,'mount');
  return <section className="mount-status" aria-label="Live mount registration">
    <h3>Register while playing</h3>
    <p>Adds one mount to the currently loaded character’s stable. Ownership shown above comes from the selected saved slot.</p>
    <p role="status">{live.message}</p>
    {!live.connected&&<button className="secondary" disabled={live.launching||!!live.pending} onClick={()=>void live.launch()}>{live.launching?<LoaderCircle className="spin" size={16}/>:null}Launch game with live mounts</button>}
    {live.backup&&<small>Verified save backup: {live.backup}</small>}
    <button className="primary" disabled={!live.ready} onClick={()=>void live.give(mountKey,1)}>{live.sending?<LoaderCircle className="spin" size={16}/>:<PawPrint size={16}/>}Register selected mount live</button>
    {live.pending&&live.result?.state!=='uncertain'&&<button className="secondary" disabled={live.sending} onClick={()=>void live.cancel()}>Cancel pending live request</button>}
    {live.result?.state==='uncertain'&&<button className="secondary" disabled={live.sending} onClick={()=>live.acknowledge()}>I checked the stable</button>}
    <p>Save normally after registration. A queued request is never automatically resubmitted.</p>
  </section>;
}
