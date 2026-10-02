# Hardware notes

| Item | Value | Source |
|---|---|---|
| Board detection | `m5gfx::board_t::board_M5CardputerADV` (auto-detected) | M5GFX `M5GFX.cpp` |
| SD card SPI | SCK 40, MISO 39, MOSI 14, CS 12, 25 MHz | M5Cardputer `examples/Basic/sdcard` |
| Arrow keys | Not dedicated: Fn+`;` `,` `.` `/` | M5Cardputer `Keyboard.h` |
| Esc | Fn+`` ` `` | `Keyboard.cpp` |
| Battery | `Power.getBatteryLevel()/getBatteryVoltage()/isCharging()`; values outside range shown as N/A | M5Unified |
| PlatformIO board | `m5stack-stamps3` (8 MB flash, `default_8MB.csv`) | platform-espressif32 v6.9.0 |

Libraries are pinned to the commits whose source was inspected. The SD pin numbers are taken from M5Stack's example for the Cardputer family; **confirming they are identical on the Adv is part of the hardware checklist.**
