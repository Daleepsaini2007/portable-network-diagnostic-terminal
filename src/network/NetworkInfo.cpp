#include "network/NetworkInfo.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <string.h>
#include <stdio.h>

const char* wifiAuthName(int m) {
  switch (m) {
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/3";
    default: return "?";
  }
}

// Formats an IPAddress without allocating a String. Returns false for 0.0.0.0.
static bool fmtIp(char* dst, size_t cap, const IPAddress& a) {
  if ((uint32_t)a == 0) { snprintf(dst, cap, "N/A"); return false; }
  snprintf(dst, cap, "%u.%u.%u.%u", a[0], a[1], a[2], a[3]);
  return true;
}

void readNetworkInfo(NetworkInfo& o) {
  o = NetworkInfo();
  o.connected = (WiFi.status() == WL_CONNECTED);

  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(o.mac, sizeof o.mac, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  if (!o.connected) return;

  fmtIp(o.ip, sizeof o.ip, WiFi.localIP());
  fmtIp(o.mask, sizeof o.mask, WiFi.subnetMask());
  IPAddress gw = WiFi.gatewayIP();
  o.hasGateway = fmtIp(o.gateway, sizeof o.gateway, gw);
  if (o.hasGateway) for (int i = 0; i < 4; ++i) o.gatewayIp[i] = gw[i];
  fmtIp(o.dns, sizeof o.dns, WiFi.dnsIP(0));

  wifi_ap_record_t ap;
  if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
    strlcpy(o.ssid, reinterpret_cast<const char*>(ap.ssid), sizeof o.ssid);
    if (!o.ssid[0]) strlcpy(o.ssid, "<hidden>", sizeof o.ssid);
    o.rssi = ap.rssi;
    o.rssiValid = true;
    o.channel = ap.primary;
    strlcpy(o.auth, wifiAuthName(ap.authmode), sizeof o.auth);
  }
}
