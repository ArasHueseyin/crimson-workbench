export interface CraftItem { key: number; name: string; internal_key: string }
export interface Ingredient { quantity: string; choices: CraftItem[]; group: number | null; group_name: string | null; slot: string }
export interface Recipe { key: number; internal_key: string; output: CraftItem; output_quantity: string; tool_key: number; knowledge_key: number; condition_keys: number[]; ingredients: Ingredient[]; result_dropset: number }
export interface CraftInfo { targets: CraftItem[]; coverage: { recipe_rows: number; group_rows: number; dropset_rows: number; interpreted_dropsets: number; usable_recipes: number; excluded: Record<string, number>; source_note: string } }
export interface CraftRequest { target: number; quantity: string; owned: Record<number, string>; recipes: Record<number, number>; choices: Record<string, number>; acquire: number[] }
export interface CraftNode { item: CraftItem; requested: string; from_owned: string; from_surplus: string; batches: string; produced: string; recipe: Recipe | null; alternatives: Recipe[]; reason: string; children: CraftNode[] }
export interface DropSource { key: number; internal_key: string; minimum: string; maximum: string; rate: string; total_rate: string; roll_type: number; roll_count: number; chance_percent: number | null; world_source: string | null }
export interface CraftPlan { root: CraftNode; materials: { item: CraftItem; needed: string; craftable: boolean; drops: DropSource[]; vendor_status: string }[]; inventory: { item: CraftItem; owned: string; used: string; surplus: string }[]; warnings: string[] }
