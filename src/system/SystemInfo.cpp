#include "system/SystemInfo.h"
#include <Arduino.h>
#include <M5Cardputer.h>
#include <stdio.h>

static const char* boardName(m5gfx::board_t b) {
  switch (b) {
    case m5gfx::board_t::board_M5CardputerADV: return "Cardputer-Adv";
    case m5gfx::board_t::board_M5Cardputer: return "Cardputer (v1.x)";
    default: return "Unknown board";
  }
}

void SystemInfo::read(SystemSnapshot& o) const {
  snprintf(o.device, sizeof o.device, "%s", boardName(M5.getBoard()));
  snprintf(o.chip, sizeof o.chip, "%s rev %u", ESP.getChipModel(), (unsigned)ESP.getChipRevision());
  o.cpuMHz = ESP.getCpuFreqMHz();
  o.flashBytes = ESP.getFlashChipSize();
  o.heapFree = ESP.getFreeHeap();
  o.heapMinFree = ESP.getMinFreeHeap();
  o.heapTotal = ESP.getHeapSize();
  o.uptimeMs = (uint64_t)millis();

  // M5Unified returns -1 / out-of-range when the PMIC/ADC path is unavailable.
  int lvl = M5Cardputer.Power.getBatteryLevel();
  int mv = M5Cardputer.Power.getBatteryVoltage();
  o.batteryPercent = (lvl >= 0 && lvl <= 100) ? lvl : -1;
  o.batteryMilliVolt = (mv > 0) ? mv : -1;
  auto ch = M5Cardputer.Power.isCharging();
  o.chargingKnown = (ch != m5::Power_Class::charge_unknown);
  o.charging = (ch == m5::Power_Class::is_charging);
}
