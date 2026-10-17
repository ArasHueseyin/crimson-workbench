import type {NativeModSnapshot} from './marketplace-types';

export type MarketplaceBrowseStatus='all'|'installed'|'updates'|'live';
export interface MarketplaceBrowseFilter {category:string;status:MarketplaceBrowseStatus}
export type MarketplaceBrowseSnapshot=NativeModSnapshot|null;
export const emptyMarketplaceBrowseFilter:MarketplaceBrowseFilter={category:'all',status:'all'};

export const marketplaceBrowseCategories=[
 ['ABILITIES','Abilities'],['APPEARANCE','Appearance'],['CAMERA','Camera'],['CRAFTING','Crafting'],
 ['DRAGON','Dragon'],['EQUIPMENT','Equipment'],['EXPLORATION','Exploration'],['FISHING','Fishing'],
 ['INVENTORY','Inventory'],['LOOT','Loot'],['MOUNTS','Mounts'],['MOVEMENT','Movement'],
 ['PROGRESSION','Progression'],['PROTECTION','Protection'],['RESOURCES','Resources'],
 ['SLEEP','Sleep'],['STAMINA','Stamina'],['TIME','Time'],['WORLD','World'],
] as const;

/** Install filters use verified module status, never a draft selection or overlay flag. */
export function matchesMarketplaceBrowse(
 filter:MarketplaceBrowseFilter,
 card:{id:string;tag:string;live?:boolean},
 snapshot:MarketplaceBrowseSnapshot=null,
):boolean {
 if(filter.category!=='all'&&filter.category!==card.tag)return false;
 if(filter.status==='all')return true;
 if(filter.status==='live')return card.live===true;
 const state=snapshot?.mods.find(mod=>mod.id===card.id)?.state;
 return filter.status==='updates'?state==='update_available':state==='installed'||state==='update_available';
}
