#include "app/App.h"
#include "AppConfig.h"
#include "core/LogFormat.h"

void SystemScreen::onInput(App& app, const core::InputEvent& ev) {
  if (ev.type == core::EvType::Back) app.back();
}

void SystemScreen::render(App& app, DisplayManager& d) {
  d.title("SYSTEM");
  static SystemSnapshot s;
  static uint32_t last = 0;
  if (last == 0 || millis() - last > 500) { app.sys.read(s); last = millis(); }
  char b[40];
  d.rowKV(1, "Device: ", s.device);
  d.rowKV(2, "MCU:    ", s.chip);
  snprintf(b, sizeof b, "%lu MHz", (unsigned long)s.cpuMHz);
  d.rowKV(3, "CPU:    ", b);
  if (s.flashBytes) snprintf(b, sizeof b, "%lu MB", (unsigned long)(s.flashBytes >> 20));
  else snprintf(b, sizeof b, "N/A");
  d.rowKV(4, "Flash:  ", b);
  snprintf(b, sizeof b, "%lu/%lu KB free", (unsigned long)(s.heapFree >> 10), (unsigned long)(s.heapTotal >> 10));
  d.rowKV(5, "Heap:   ", b);
  snprintf(b, sizeof b, "%lu KB", (unsigned long)(s.heapMinFree >> 10));
  d.rowKV(6, "MinHeap:", b);
  if (s.batteryPercent >= 0) {
    snprintf(b, sizeof b, "%d%% %dmV%s", s.batteryPercent, s.batteryMilliVolt >= 0 ? s.batteryMilliVolt : 0,
             s.chargingKnown ? (s.charging ? " chg" : "") : "");
  } else snprintf(b, sizeof b, "N/A");
  d.rowKV(7, "Battery:", b);
  char up[24];
  core::formatUptime(up, sizeof up, s.uptimeMs);
  d.rowKV(8, "Uptime: ", up);
  d.rowKV(9, "FW:     ", FW_VERSION);
  d.footer("Esc:menu");
}
