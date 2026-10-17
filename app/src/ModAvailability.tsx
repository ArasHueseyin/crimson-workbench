import {Gamepad2,PowerOff,SlidersHorizontal} from 'lucide-react';

export function ModAvailability({mode}:{mode:'closed'|'runtime'|'live'}) {
 const closed=mode==='closed';
 const Icon=closed?PowerOff:mode==='live'?SlidersHorizontal:Gamepad2;
 return <div className={`market-availability ${mode}`} aria-label="Game availability">
  <span className="market-availability-state"><Icon size={12} aria-hidden="true"/>{closed?'Apply with game closed':mode==='live'?'Adjust while playing':'Works while playing'}</span>
  {!closed&&<span className="market-availability-setup" title="Fully close the game to install or remove this mod. Runtime settings also require the game closed unless labelled as live controls.">Close game for setup</span>}
 </div>;
}
