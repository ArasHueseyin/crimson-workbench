export interface Mount { key:number; internal:string; name:string; description:string; family:string; vehicle:number; owned:number; supported:boolean; reason:string; registration?:'owned'|'same_family'|'base'|'unavailable' }
export interface SaveChoice { id:string; label:string; modified:number }
export interface MountSnapshot { mounts:Mount[]; saves:SaveChoice[]; selected_save:string|null; save_sha256:string; lobby_sha256:string; game_running:boolean; message:string }
export interface MountRequest { id:string; save:string; save_sha256:string; lobby_sha256:string; key:number }
export interface MountReceipt { request:MountRequest; name:string; mercenary_no:string; donor_key:number; backup:string; message:string }
export function filterMounts(mounts:Mount[],text:string,family:string,availableOnly:boolean):Mount[] {
  const query=text.trim().toLocaleLowerCase('de');
  return mounts.filter(m=>(!family||m.family===family)&&(!availableOnly||m.supported)&&(!query||`${m.name} ${m.internal} ${m.key} ${m.family}`.toLocaleLowerCase('de').includes(query)));
}
