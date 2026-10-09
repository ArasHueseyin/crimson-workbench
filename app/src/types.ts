export interface AppError { code: string; message: string }
export interface Installation { path: string; platform: string; build_id: string | null }
export interface Bootstrap { project: string; discovery: { installations: Installation[]; configured_game: string | null; save_directories: string[] }; languages: { language: string }[] }
export interface Facet { value: number; count: number }
export interface ItemGroup { key: number; internal_key: string; name: string; order: number; items: number[] }
export interface Catalog { session: number; game_path: string; info: {
  item_count: number; types: Facet[]; categories: Facet[]; tiers: Facet[]; stats: Facet[];
  groups?: ItemGroup[]; group_error?: string | null;
  exe_version: string | null;
  index: { path: string; rebuilt: boolean; items: number; language: string; fingerprint: string };
} }
export interface Item { key: number; name: string; internal_key: string; description: string; item_type: number; category: number; tier: number; max_stack: string; stat_keys: number[]; knowledge_keys?:number[]; icon_key: number | null; use_restriction?:string|null }
export interface Query { text: string; regex?: boolean; item_type: number | null; category: number | null; group?: number | null; tier: number | null; stackable: boolean; stat_key: number | null; sort: 'name' | 'key' | 'tier' | 'stack'; descending: boolean; offset: number }
export interface Page { items: Item[]; total: number; offset: number }
export interface RawField { path: string; start: number; end: number; type_name: string; raw_hex: string; value: unknown; interpretation: 'unknown' | 'upstream_named' }
export interface Detail { name: string; description: string; language: string; name_resolved: boolean; fields: RawField[]; item_references: { path: string; key: number; name: string | null; available: boolean }[]; record: { key: number; string_key: string; item_type: number; item_tier: number; category_info: number; max_stack_count: number | string; icon_path: number | null; stat_keys: number[]; inventory_info_list: number[]; offset: number; length: number } }
export interface ItemIcon { data_url: string | null; source: string | null; reason: string | null; caption?: string | null }
