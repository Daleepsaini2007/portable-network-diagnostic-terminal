// Snapshot of the current station connection. Unavailable values are "N/A".
#pragma once
#include <stdint.h>

struct NetworkInfo {
  bool connected = false;
  char ssid[33] = "N/A";
  char ip[16] = "N/A";
  char mask[16] = "N/A";
  char gateway[16] = "N/A";
  char dns[16] = "N/A";
  char mac[18] = "N/A";
  char auth[10] = "N/A";
  bool rssiValid = false;
  int rssi = 0;
  int channel = 0;  // 0 = N/A
  bool hasGateway = false;
  uint8_t gatewayIp[4] = {0, 0, 0, 0};
};

void readNetworkInfo(NetworkInfo& out);
// Short label for wifi_auth_mode_t values ("OPEN", "WPA2", ...).
const char* wifiAuthName(int authMode);
