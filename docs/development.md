# Development notes

## How this was verified (honestly)
- **Unit tests**: `scripts/run_host_tests.sh` ran all 3 suites (60 tests) with g++ 13 + Unity, `-Wall -Wextra -Werror`: all passed. One real bug was found by the tests (`a.com..` accepted) and fixed.
- **Firmware compile**: PlatformIO's servers were unreachable in the build sandbox, so the firmware was compiled with `arduino-cli` using the *same* Arduino-ESP32 **2.0.17** core and xtensa toolchain that `espressif32@6.9.0` uses, board variant `stamp-s3` (= `m5stack_stamp_s3`), library commits as pinned in `platformio.ini`. Result: no errors, no warnings from project code; 1,009,185 bytes flash (76 % of the *Arduino default* 1.3 MB partition; PlatformIO uses an 8 MB table), 64,280 bytes static RAM (19 %).
- **`pio run` itself has not been executed**; the first run on your machine is the real PlatformIO validation (CI does it too).
- **Nothing has run on hardware.**

## APIs checked against source
`Keyboard_Class::KeysState`, `isChange()/isPressed()`, `M5Cardputer.begin(cfg, bool)`, `Power_Class`, `M5.getBoard()`, `esp_ping_*` (IDF 4.4.7 `ping_sock.h/.c`), `WiFi.hostByName/scanNetworks/disconnect/setSleep/persistent` (Arduino-ESP32 2.0.17).

## Adding a screen
Implement `IScreen`, add the id to `AppState.h`, an instance + `screenFor()` case in `App`, and a menu entry.
