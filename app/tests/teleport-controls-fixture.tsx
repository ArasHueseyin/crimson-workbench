import React from 'react';
import ReactDOM from 'react-dom/client';
import {MarketplaceTeleportControls} from '../src/MarketplaceTeleportControls';
import '../src/styles.css';
import '../src/marketplace.css';
import '../src/mods.css';
const enabled=new URL(location.href).searchParams.get('enabled')!=='false';
ReactDOM.createRoot(document.getElementById('root')!).render(<React.StrictMode><div className="market-card" style={{width:620,maxWidth:'100%',margin:'20px auto',boxSizing:'border-box'}}><h2>Teleport</h2><MarketplaceTeleportControls session={1} enabled={enabled}/></div></React.StrictMode>);
