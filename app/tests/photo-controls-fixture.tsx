import React from 'react';
import ReactDOM from 'react-dom/client';
import {MarketplacePhotoControls} from '../src/MarketplacePhotoControls';
import '../src/styles.css';
import '../src/marketplace.css';
import '../src/mods.css';
const enabled=new URL(location.href).searchParams.get('enabled')!=='false';
ReactDOM.createRoot(document.getElementById('root')!).render(<React.StrictMode><div className="market-card" style={{width:570,maxWidth:'100%',margin:'20px auto',boxSizing:'border-box',height:'calc(100vh - 40px)',overflow:'auto'}}><h2>Extended photo mode</h2><MarketplacePhotoControls session={1} enabled={enabled}/></div></React.StrictMode>);
