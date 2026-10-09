import { create } from 'zustand';
import type { Query } from './types';
import { emptyMod, type ModRequest } from './mod-types';
export const emptyQuery: Query = { text: '', regex: false, item_type: null, category: null, tier: null, stackable: false, stat_key: null, sort: 'name', descending: false, offset: 0 };
interface Store { query: Query; selected: number | null; view: 'items' | 'sources' | 'crafting' | 'mods' | 'spawn' | 'mounts' | 'sockets'; setQuery: (patch: Partial<Query>) => void; select: (key: number | null) => void; setView: (view: Store['view']) => void; reset: () => void }
export const useCatalog = create<Store>((set) => ({ query: emptyQuery, selected: null, view: 'items',
  setQuery: (patch) => set((s) => ({ query: { ...s.query, ...patch, offset: 0 } })),
  select: (selected) => set({ selected }), setView: (view) => set({ view }), reset: () => set({ query: { ...emptyQuery }, selected: null }),
}));
interface CraftState { target: number | null; quantity: string; owned: Record<number,string>; recipes: Record<number,number>; choices: Record<string,number>; acquire: number[]; set: (patch: Partial<Omit<CraftState,'set'|'reset'>>) => void; reset: () => void }
const emptyCraft = { target: null, quantity: '1', owned: {}, recipes: {}, choices: {}, acquire: [] };
export const useCraft = create<CraftState>(set => ({ ...emptyCraft, set: patch => set(patch), reset: () => set({ ...emptyCraft }) }));
export const useMods = create<{ request:ModRequest; set:(patch:Partial<ModRequest>)=>void;reset:()=>void }>(set=>({ request:{...emptyMod},set:patch=>set(s=>({request:{...s.request,...patch}})),reset:()=>set({request:{...emptyMod}}) }));
