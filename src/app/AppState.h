#pragma once
#include <stdint.h>

enum class ScreenId : uint8_t {
  Menu,
  Dashboard,
  WifiScan,
  WifiConnect,
  NetInfo,
  Ping,
  Dns,
  Connectivity,
  Health,
  Logs,
  System,
  kCount
};
