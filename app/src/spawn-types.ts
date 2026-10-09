export type SpawnState = 'offline' | 'ready' | 'starting' | 'queued' | 'executing' | 'applied' | 'rejected' | 'uncertain' | 'expired' | 'cancelled';
export interface SpawnRequest { id: string; pid: number; epoch: number; key: number; quantity: number }
export interface SpawnSnapshot extends SpawnRequest { state: SpawnState; message: string }
