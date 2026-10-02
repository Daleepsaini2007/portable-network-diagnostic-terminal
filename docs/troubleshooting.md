# Troubleshooting

| Symptom | Likely cause / action |
|---|---|
| Upload port not found | Hold G0, plug USB, release; then upload |
| `microSD: NOT AVAILABLE` at boot | No card, not FAT32, or SD pins differ; use Logs -> `M` to remount |
| Connect fails: "Timeout (check password)" | Wrong password, 5 GHz-only SSID, or weak signal. ESP32-S3 is 2.4 GHz only |
| Gateway FAIL but ONLINE | Router ignores ICMP; expected, noted on screen |
| Ping shows 0 ms | RTT is truncated to whole ms |
| DNS "TIMEOUT / NO SERVER" vs "NOT FOUND" | Inferred from response time (>= 3 s = timeout); API gives no error code |
| Resolver busy | A previous lookup is still finishing (up to ~15 s) |
| Timestamps empty in CSV | No SNTP yet (needs internet); `uptime_ms` is always logged |
| Text looks clipped / misaligned | Layout constants in `DisplayManager` (26 cols x 11 rows) were not verified on the panel |
