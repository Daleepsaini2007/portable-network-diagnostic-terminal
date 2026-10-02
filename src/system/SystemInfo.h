// Read-only device information. Values are only reported when reliably available.
#pragma once
#include <stdint.h>
#include <stddef.h>

struct SystemSnapshot {
  char device[24];      // from M5Unified board autodetect
  char chip[24];        // ESP.getChipModel() + revision
  uint32_t cpuMHz;
  uint32_t flashBytes;  // 0 = unknown
  uint32_t heapFree;
  uint32_t heapMinFree;
  uint32_t heapTotal;
  int batteryPercent;   // -1 = N/A
  int batteryMilliVolt; // -1 = N/A
  bool charging;
  bool chargingKnown;
  uint64_t uptimeMs;
};

class SystemInfo {
 public:
  void read(SystemSnapshot& out) const;
};
