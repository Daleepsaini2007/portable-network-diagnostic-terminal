# Hardware test checklist

Nothing below has been performed yet. Tick each box only after testing on a real Cardputer-Adv, and record firmware version + date.

- [ ] Device boot (boot screen shows Display/WiFi/microSD/Battery/Device = `Cardputer-Adv`)
- [ ] Display readable; 26 columns fit, no clipped rows, no flicker
- [ ] Keyboard: `;`/`.`, Fn arrows, Enter, Esc, Backspace, Tab, `r`, text entry incl. `. - _ :`
- [ ] Wi-Fi initialization (no "hardware init failed")
- [ ] Wi-Fi scanning (list, sorting, rescan cooldown, zero-network case, hidden SSID)
- [ ] Wi-Fi connection: config.h path, manual entry, from scan list, wrong password -> error + Retry, AP switched off -> "Connection lost"
- [ ] Network information (values match router; `N/A` when disconnected)
- [ ] Ping: IP, hostname, 1 and 20 packets, unreachable IP, Wi-Fi dropped mid-test, Esc to stop; compare with a PC ping
- [ ] DNS: valid, invalid name, DNS server unreachable
- [ ] Gateway (Connectivity row) incl. router that blocks ICMP
- [ ] Internet connectivity: all OK; DNS blocked; internet cable unplugged
- [ ] microSD: boot with card / without card / FAT32 / remove card while running
- [ ] Logging: CSV files + headers exist, columns correct, session id increments, log toggle, viewer, card-full behaviour
- [ ] System information values plausible (flash 8 MB, CPU 240 MHz)
- [ ] Battery information (percent, mV, charging indicator) if reported
- [ ] Verify SD pins 40/39/14/12 on the Adv
- [ ] Soak: 30 min on Dashboard + repeated tests, no reboot, heap stable (System screen)
