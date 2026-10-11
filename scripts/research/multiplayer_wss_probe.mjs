// Synthetic outbound WebSocket echo check. No game, relay rooms or gameplay integration.
import { randomUUID } from 'node:crypto';
import { writeFile } from 'node:fs/promises';
import { performance } from 'node:perf_hooks';

const endpoint = 'wss://ws.postman-echo.com/raw';
const output = process.argv[2];

async function probe(client) {
  return new Promise((resolve, reject) => {
    const socket = new WebSocket(endpoint);
    const id = randomUUID();
    let sequence = 0;
    let sent;
    let started;
    let finished = false;
    const timings = [];
    const timeout = setTimeout(() => finish(new Error('WebSocket echo timed out after 15 seconds')), 15_000);

    function finish(error) {
      if (finished) return;
      finished = true;
      clearTimeout(timeout);
      socket.close();
      if (error) reject(error);
      else resolve({ client, messages: timings.length,
        min_rtt_ms: Math.round(Math.min(...timings)),
        max_rtt_ms: Math.round(Math.max(...timings)),
        mean_rtt_ms: Math.round(timings.reduce((a, b) => a + b, 0) / timings.length) });
    }

    function send() {
      sent = JSON.stringify({ kind: 'workbench-feasibility-synthetic-echo', id, sequence });
      started = performance.now();
      socket.send(sent);
    }

    socket.addEventListener('open', send);
    socket.addEventListener('message', event => {
      if (finished) return;
      if (event.data !== sent) return finish(new Error('Echo payload mismatch'));
      timings.push(performance.now() - started);
      if (++sequence === 10) finish();
      else send();
    });
    socket.addEventListener('error', () => finish(new Error('WebSocket connection failed')));
    socket.addEventListener('close', () => {
      if (!finished) finish(new Error('Connection closed before all echoes arrived'));
    });
  });
}

const results = await Promise.allSettled([probe('A'), probe('B')]);
const passed = results.every(result => result.status === 'fulfilled');
const report = { schema: 1, checked_at: new Date().toISOString(), endpoint,
  passed, connections: results.map(result => result.status === 'fulfilled'
    ? result.value : { error: result.reason.message }),
  real_wan_echo_only: true, game_integration: false, peer_to_peer_tested: false,
  cross_network_players_tested: false, regional_latency_tested: false,
  limitations: 'Two clients on one PC reached a public TLS WebSocket echo endpoint; this is not a multiplayer relay or a two-PC game test.' };
const json = JSON.stringify(report, null, 2) + '\n';
if (output) await writeFile(output, json, { flag: 'wx' });
process.stdout.write(json);
if (!passed) process.exitCode = 1;
