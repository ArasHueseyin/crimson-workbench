import { useEffect, useRef, useState } from 'react';
import { api, asError } from './api';
import type { SpawnRequest, SpawnSnapshot } from './spawn-types';

function restore(key: string): SpawnRequest | null {
  try {
    const value = JSON.parse(localStorage.getItem(key) || 'null');
    if (value && /^[0-9a-f-]{32,36}$/i.test(value.id) && [value.pid, value.epoch, value.key, value.quantity].every(n => Number.isSafeInteger(n) && n > 0)) return value;
  } catch { /* A malformed local record never creates a request. */ }
  return null;
}
export function useItemSpawner(session: number, game: string) {
  const storageKey = `crimson-live-item-request-v1:${game.toLowerCase()}`;
  const [pending, setPending] = useState<SpawnRequest | null>(() => restore(storageKey));
  const [runtime, setRuntime] = useState<SpawnSnapshot | null>(null);
  const [result, setResult] = useState<SpawnSnapshot | null>(null);
  const [message, setMessage] = useState('Live-Modul wird geprüft …');
  const [sending, setSending] = useState(false);
  const active = useRef(true), latch = useRef(false), current = useRef(pending), lastResult = useRef<SpawnSnapshot | null>(null), generation = useRef(0);
  const terminal = (value: SpawnSnapshot) => {
    if (!['queued', 'executing', 'uncertain'].includes(value.state)) {
      localStorage.removeItem(storageKey); current.current = null; setPending(null);
    }
    lastResult.current = value; setRuntime(value); setResult(value); setMessage(value.message);
  };
  useEffect(() => {
    active.current = true; const token = ++generation.current; let polling = false, alive = true;
    const restored = restore(storageKey); current.current = restored; setPending(restored); setRuntime(null); lastResult.current = null; setResult(null);
    async function poll() {
      if (polling || latch.current) return; polling = true;
      try {
        const value = await api.spawnStatus(session, current.current);
        if (!alive || token !== generation.current) return;
        if (current.current) terminal(value); else { setRuntime(value); if (!lastResult.current) setMessage(value.message); }
      } catch (e) {
        if (alive) { setRuntime(null); setMessage(current.current ? `${asError(e).message} Die Anfrage bleibt gespeichert; keine automatische Wiederholung.` : asError(e).message); }
      } finally { polling = false; }
    }
    void poll(); const timer = window.setInterval(() => void poll(), 2000);
    return () => { alive = false; active.current = false; window.clearInterval(timer); };
  }, [session, storageKey]);
  async function give(key: number, quantity: number) {
    if (latch.current || current.current || runtime?.state !== 'ready' || !Number.isSafeInteger(quantity) || quantity < 1 || quantity > 10000) return;
    latch.current = true; const token = generation.current; setSending(true); lastResult.current = null; setResult(null);
    const request: SpawnRequest = { id: crypto.randomUUID(), pid: runtime.pid, epoch: runtime.epoch, key, quantity };
    try {
      // Record before sending, so a window reload can query the same ID.
      localStorage.setItem(storageKey, JSON.stringify(request)); current.current = request; setPending(request);
      const value = await api.spawnGrant(session, request); if (active.current && token === generation.current) terminal(value);
    } catch (e) {
      if (active.current && token === generation.current) setMessage(`${asError(e).message}${current.current ? ' Ergebnis wird über dieselbe Anfrage-ID abgefragt.' : ''}`);
    } finally { latch.current = false; if (active.current) setSending(false); }
  }
  async function cancel() {
    if (!current.current || latch.current) return;
    latch.current = true; const token = generation.current; setSending(true);
    try { const value = await api.spawnCancel(session, current.current); if (active.current && token === generation.current) terminal(value); }
    catch (e) { if (active.current) setMessage(asError(e).message); }
    finally { latch.current = false; if (active.current) setSending(false); }
  }
  function acknowledge() {
    if (sending) return; localStorage.removeItem(storageKey); current.current = null; setPending(null); lastResult.current = null; setResult(null); setMessage('Ergebnis geprüft. Verbindung wird neu abgefragt.');
  }
  return { ready: runtime?.state === 'ready' && !pending && !sending, pending, result, message, sending, give, cancel, acknowledge };
}
