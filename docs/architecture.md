# Architecture

## Layers
- **core/** - pure C++17, no Arduino headers: `Validation`, `PingStats`, `Connectivity` (layer evaluation + heuristic health), `LogFormat` (CSV, UTC, uptime), `TextInput`, `MenuModel`, `Parsers`, `InputMap`. Split out *only* so it can be unit-tested on a PC (`env:native`).
- **network/** - hardware-facing services. Each exposes a non-blocking API (`start()/update()/snapshot()`).
- **storage/Logger** - microSD CSV.
- **ui/** - `DisplayManager` (row cache) and `IScreen` implementations.
- **app/App** - owns all services, routes keyboard events, detects completed operations and logs them.

Deviation from the suggested tree: `core/` was added, `ui/Menu.*` became `MenuScreen` + `core/MenuModel`, and `network/Connectivity.cpp` became `ConnectivityEngine` (+ `core/Connectivity` for the pure part).

## Concurrency model
The Arduino `loop()` task never blocks on the network.
- **Ping**: ESP-IDF `esp_ping_*` (ICMP echo) runs in its own IDF task and calls back on success/timeout/end. Callbacks write a small struct under a spinlock; the UI reads `snapshot()`. `esp_ping_delete_session` only clears a flag (verified in `ping_sock.c`, IDF 4.4.7), so the session is freed from `update()` after `on_ping_end`.
- **DNS**: `WiFi.hostByName()` can block ~15 s, so it runs in a short-lived worker task. A sequence number discards results from abandoned requests; one worker at a time (`Busy` otherwise).
- **Connectivity**: one worker task runs Wi-Fi -> gateway ICMP -> DNS -> internet (ICMP, TCP/443 fallback) and publishes after each step. It owns a private `PingDiagnostics` so it cannot collide with the Ping screen.
- **Scan**: `WiFi.scanNetworks(async=true)` polled via `scanComplete()`, 3 s cooldown, 12 s timeout.

## Rendering
Each screen's `render()` is idempotent and called at most every 150 ms (or after input). `DisplayManager` caches the text/colour of each of the 11 rows and only repaints rows that changed. No framebuffer is allocated (saves ~64 KB RAM).

## Logging
One record per completed test (scan = one batched write). Files: `/network_logs/<category>/<category>.csv` with a header row; `sessions/next_id.txt` is the persistent session counter. Timestamps are UTC from SNTP when available, otherwise empty; `uptime_ms` is always present. The device has no battery-backed clock that this firmware uses.

## Memory
No `String` in hot paths (SSID list uses one temporary `String` per scanned AP). Fixed-size buffers everywhere. Worker stacks: 6 KB (connectivity), 4 KB (DNS), 4 KB (IDF ping).
