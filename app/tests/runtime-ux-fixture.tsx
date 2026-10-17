import React,{useState} from 'react';
import ReactDOM from 'react-dom/client';
import {MarketplaceRuntime} from '../src/MarketplaceRuntime';
import {Dialog} from '../src/Dialog';
import '../src/styles.css';
import '../src/marketplace.css';
import '../src/mods.css';
function Fixture(){
 const [visible,setVisible]=useState(true),[session,setSession]=useState(1),[dialog,setDialog]=useState(false),[blocked,setBlocked]=useState(false);
 return <main style={{padding:20}}><nav style={{display:'flex',gap:12,marginBottom:20}}><button onClick={()=>setVisible(v=>!v)}>{visible?'Leave runtime':'Return to runtime'}</button><button onClick={()=>setSession(s=>s+1)}>Reload catalog</button><button onClick={()=>setDialog(true)}>Open fixture dialog</button><button>Outside action</button></nav>
 {visible&&<div className="market-grid"><MarketplaceRuntime session={session} supported/></div>}
 {dialog&&<Dialog title="Owned accessible dialog" closeDisabled={blocked} onClose={()=>setDialog(false)}><label>First field<input aria-label="First field"/></label><label><input type="checkbox" checked={blocked} onChange={e=>setBlocked(e.target.checked)}/>Block dialog close</label><button>Last dialog action</button></Dialog>}
 </main>;
}
ReactDOM.createRoot(document.getElementById('root')!).render(<React.StrictMode><Fixture/></React.StrictMode>);
